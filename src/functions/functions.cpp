#include "functions.h"

#include <QFile>
#include <QUiLoader>
#include <QSettings>
#include <QSqlRecord>

// Function to load .ui files
QWidget* loadUiFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qFatal("Cannot open UI file: %s", qPrintable(filePath));
    }

    QUiLoader loader;
    QWidget* widget = loader.load(&file);
    file.close();

    if (!widget) {
        qFatal("Failed to load UI file: %s", qPrintable(filePath));
    }

    return widget;
}

// Function to create alert message window
QWidget* createAlertWindow(QString message) {
    QWidget* alertWindow = loadUiFile("../src/gui/alertwindow.ui");

    // get and set message label
    QLabel* messageLabel = alertWindow->findChild<QLabel*>("messageLabel");
    messageLabel->setText(message);

    // get and connect cancel button
    QPushButton* okButton = alertWindow->findChild<QPushButton*>("okButton");
    QObject::connect(okButton, &QPushButton::clicked, alertWindow, &QWidget::close);
    
    return alertWindow;
}

// Function to calculate GPA
QString calculateGPA(QSqlQuery &gradeData) {
    double totalHours = 0;
    double qualityPoints = 0;
    double gpa = 0.0;
    while (gradeData.next()) {
        totalHours += gradeData.value("hours").toDouble();

        if (gradeData.value("grade").toString() == "A") {
            qualityPoints += 4 * gradeData.value("hours").toDouble();
        } else if (gradeData.value("grade").toString() == "B") {
            qualityPoints += 3 * gradeData.value("hours").toDouble();
        } else if (gradeData.value("grade").toString() == "C") {
            qualityPoints += 2 * gradeData.value("hours").toDouble();
        } else if (gradeData.value("grade").toString() == "D") {
            qualityPoints += 1 * gradeData.value("hours").toDouble();
        }
    }

    QString formattedGPA;
    if (totalHours == 0) {
        formattedGPA = "0.00";
    } else {
        gpa = qualityPoints / totalHours;
        formattedGPA = QString::number(gpa, 'f', 2);
    }

    return formattedGPA;
}

// Function to connect to database
QSqlDatabase databaseConnection(QString connectionName) {
    QString dbName = "gradeDB";
    QString user = "mitch";
    // database configuration
    QSettings settings("config.ini", QSettings::IniFormat);

    QString dbIp = settings.value("Database/IP").toString();
    QString dbPassword = settings.value("Database/Password").toString();

    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", connectionName);
    db.setHostName(dbIp);
    db.setDatabaseName(dbName);
    db.setUserName(user);
    db.setPassword(dbPassword);

    if (!db.open()) {
        qDebug() << "Error: Unable to connect to database!";
        qDebug() << db.lastError().text();
        return QSqlDatabase();
    }

    qDebug() << "Connected to the database.";
    return db;
}

// Function to handle selection of row from grade table
QMap<std::string, QString> onSelectionChanged(QSqlQueryModel* model, const QItemSelection &selected, const QItemSelection &deselected) {
    std::array keys = {"crn", "course_prefix", "course_num", "semester", "year", "hours", "grade"};
    QMap<std::string, QString> selectedRow;

    // Check if there's any selected row
    QModelIndexList selectedIndexes = selected.indexes();

    if (!selectedIndexes.isEmpty()) {
        // Get index of selected row
        QModelIndex index = selectedIndexes.first();

        // Retrieve data from the selected row
        int row = index.row();
        for (int column = 0; column < model->columnCount(); ++column) {
            QModelIndex idx = model->index(row, column);
            selectedRow.insert(keys[column], QString(model->data(idx).toString()));
        }
    }

    return selectedRow;
}

// Function to handle save button from grade entry form
void onSaveButtonClicked(const QSqlDatabase &db, QString studID, QWidget* addEditWindow, std::shared_ptr<QMap<std::string, QString>> selectedRow) {
    // get combo boxes
    QComboBox* gradeCombo = addEditWindow->findChild<QComboBox*>("gradeCombo");
    QComboBox* crnCombo = addEditWindow->findChild<QComboBox*>("crnCombo");

    // get values
    int crn = QString(crnCombo->currentText()).toInt();
    QString grade = QString(gradeCombo->currentText());

    QSqlQuery gradeQuery(db);

    // sql insert string
    if (selectedRow->isEmpty()) {
        gradeQuery.prepare("INSERT INTO grades (grade, student_id, crn) VALUES (:grade, :student_id, :crn)");
        gradeQuery.bindValue(":grade", grade);
        gradeQuery.bindValue(":student_id", studID);
        gradeQuery.bindValue(":crn", crn);

        gradeQuery.exec();
    } else { // sql update string
        gradeQuery.prepare("UPDATE grades SET grade=:grade WHERE student_id=:student_id AND crn=:crn");
        gradeQuery.bindValue(":grade", grade);
        gradeQuery.bindValue(":student_id", studID);
        gradeQuery.bindValue(":crn", crn);

        gradeQuery.exec();
    }

    // print for testing
    qDebug() << " ID: " << studID << "CRN: " << crn << "Grade: " << grade;
}

// Function to handle delete button from grade display window
void onDeleteButtonClicked(const QSqlDatabase &db, QString studID, std::shared_ptr<QMap<std::string, QString>> selectedRow) {
    QSqlQuery deleteQuery(db);

    if (selectedRow->isEmpty()) {
        //Error message
    } else {
        deleteQuery.prepare("DELETE FROM grades WHERE student_id=:student_id AND crn=:crn");
        deleteQuery.bindValue(":student_id", studID);
        deleteQuery.bindValue(":crn", (*selectedRow)["crn"]);

        deleteQuery.exec();
    }
}

// Function to handle grade entry combo box values
void setComboBoxValues(QSqlDatabase dbConn, QWidget* addEditWindow, QSqlQuery coursesInfo, std::shared_ptr<QVariantMap> stateTracker) {
    int length;
    if (coursesInfo.next()) {
        coursesInfo.previous();
        length = coursesInfo.record().count();
    } else {
        length = 3;
    }
    QList<QList<QString>> uniqueItems(length);

    // get combo boxes
    QComboBox* crnCombo = addEditWindow->findChild<QComboBox*>("crnCombo");
    QComboBox* prefixCombo = addEditWindow->findChild<QComboBox*>("prefixCombo");
    QComboBox* numberCombo = addEditWindow->findChild<QComboBox*>("numberCombo");
    QComboBox* yearCombo = addEditWindow->findChild<QComboBox*>("yearCombo");
    QComboBox* semesterCombo = addEditWindow->findChild<QComboBox*>("semesterCombo");
    QComboBox* hoursCombo = addEditWindow->findChild<QComboBox*>("hoursCombo");

    // clear combo boxes
    crnCombo->clear();
    prefixCombo->clear();
    numberCombo->clear();
    yearCombo->clear();
    semesterCombo->clear();
    hoursCombo->clear();

    if (length == 3) {
        QSqlQuery coursesInfo(dbConn);
        coursesInfo.prepare("SELECT crn, course_prefix, course_num FROM courses");
        coursesInfo.exec();
        
        QComboBox* gradeCombo = addEditWindow->findChild<QComboBox*>("gradeCombo");
        // set year combo box values using current year and
        // going back 10 years as that is the oldest course work
        // the university will accept
        // Get the current time
        std::time_t now = std::time(nullptr);
        std::tm* local_time = std::localtime(&now);

        // Extract the year
        int current_year = local_time->tm_year + 1900;

        for (int i = current_year; i > current_year-10; i--) {
            yearCombo->addItem(QString::number(i));
        }
        semesterCombo->addItems({"Fall", "Winter", "Spring", "Summer"});
        hoursCombo->addItems({"1.0", "2.0", "3.0", "4.0"});

        while (coursesInfo.next()) {
            if (!uniqueItems[0].contains(coursesInfo.value("crn").toString())) {
                uniqueItems[0].append(coursesInfo.value("crn").toString());
                crnCombo->addItem(coursesInfo.value("crn").toString());
            }
            if (!uniqueItems[1].contains(coursesInfo.value("course_prefix").toString())) {
                uniqueItems[1].append(coursesInfo.value("course_prefix").toString());
                prefixCombo->addItem(coursesInfo.value("course_prefix").toString());
            }
            if (!uniqueItems[2].contains(coursesInfo.value("course_num").toString())) {
                uniqueItems[2].append(coursesInfo.value("course_num").toString());
                numberCombo->addItem(coursesInfo.value("course_num").toString());
            }
        }
        // set initial state of all combo boxes to no selection
        semesterCombo->setCurrentIndex(-1);
        yearCombo->setCurrentIndex(-1);
        gradeCombo->setCurrentIndex(-1);
        hoursCombo->setCurrentIndex(-1);
        crnCombo->setCurrentIndex(-1);
        prefixCombo->setCurrentIndex(-1);
        numberCombo->setCurrentIndex(-1);
    } else {
        while (coursesInfo.next()) {
            if (!uniqueItems[0].contains(coursesInfo.value("crn").toString())) {
                uniqueItems[0].append(coursesInfo.value("crn").toString());
                crnCombo->addItem(coursesInfo.value("crn").toString());
            }
            if (!uniqueItems[1].contains(coursesInfo.value("course_prefix").toString())) {
                uniqueItems[1].append(coursesInfo.value("course_prefix").toString());
                prefixCombo->addItem(coursesInfo.value("course_prefix").toString());
            }
            if (!uniqueItems[2].contains(coursesInfo.value("course_num").toString())) {
                uniqueItems[2].append(coursesInfo.value("course_num").toString());
                numberCombo->addItem(coursesInfo.value("course_num").toString());
            }
            if (!uniqueItems[3].contains(coursesInfo.value("year").toString())) {
                uniqueItems[3].append(coursesInfo.value("year").toString());
                yearCombo->addItem(coursesInfo.value("year").toString());
            }
            if (!uniqueItems[4].contains(coursesInfo.value("semester").toString())) {
                uniqueItems[4].append(coursesInfo.value("semester").toString());
                semesterCombo->addItem(coursesInfo.value("semester").toString());
            }
            if (!uniqueItems[5].contains(coursesInfo.value("hours").toString())) {
                uniqueItems[5].append(coursesInfo.value("hours").toString());
                hoursCombo->addItem(coursesInfo.value("hours").toString()+".0");
            }
        }
    }
    // Check if more than one course option
    bool multipleCourses = std::any_of(uniqueItems.begin(), uniqueItems.end(), [](const QList<QString>& row) {
        return row.size() > 1;
    });
    // set initial selection of comboBoxes based on stateTracker
    if (multipleCourses) {
        for (auto state = (*stateTracker).begin(); state != (*stateTracker).end(); state++) {
            if (state.value().toInt() == -1 || state.value().toString() == "") {
                QString box = state.key() + "Combo";
                QComboBox* combo = addEditWindow->findChild<QComboBox*>(box);
                combo->setCurrentIndex(-1);
            }
        }
    }
}

// Function to set initial state of desired combo boxes to no selection
void setComboBoxesToNone(QWidget* addEditWindow, QComboBox* filter) {
    // get combo boxes
    QComboBox* crnCombo = addEditWindow->findChild<QComboBox*>("crnCombo");
    QComboBox* prefixCombo = addEditWindow->findChild<QComboBox*>("prefixCombo");
    QComboBox* numberCombo = addEditWindow->findChild<QComboBox*>("numberCombo");
    QComboBox* yearCombo = addEditWindow->findChild<QComboBox*>("yearCombo");
    QComboBox* semesterCombo = addEditWindow->findChild<QComboBox*>("semesterCombo");
    QComboBox* hoursCombo = addEditWindow->findChild<QComboBox*>("hoursCombo");

    QList<QComboBox*> comboBoxes = {crnCombo, prefixCombo, numberCombo, yearCombo, semesterCombo, hoursCombo};

    for (QComboBox* comboBox : comboBoxes) {
        if (comboBox != filter) {
            comboBox->setCurrentIndex(-1);
        }
    }
}

// Function to block combo box signals during filter to prevent infinite loop
void blockAllComboBoxes(QWidget* addEditWindow, bool boxSet) {
    // get combo boxes
    QComboBox* crnCombo = addEditWindow->findChild<QComboBox*>("crnCombo");
    QComboBox* prefixCombo = addEditWindow->findChild<QComboBox*>("prefixCombo");
    QComboBox* numberCombo = addEditWindow->findChild<QComboBox*>("numberCombo");
    QComboBox* yearCombo = addEditWindow->findChild<QComboBox*>("yearCombo");
    QComboBox* semesterCombo = addEditWindow->findChild<QComboBox*>("semesterCombo");
    QComboBox* hoursCombo = addEditWindow->findChild<QComboBox*>("hoursCombo");
    
    QList<QComboBox*> comboBoxes = {crnCombo, prefixCombo, numberCombo, yearCombo, semesterCombo, hoursCombo};

    for (QComboBox* comboBox : comboBoxes) {
        if (comboBox) {
            comboBox->blockSignals(boxSet);
        }
    }
}
