#include <QApplication>
#include <QFile>
#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QTableView>
#include <QComboBox>
#include <QMap>
#include <QItemSelectionModel>

#include <QtUiTools/QUiLoader>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQueryModel>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QSettings>

#include <QIntValidator>
#include <QDebug>

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

QSqlQuery executeQuery(const QSqlDatabase &db, const QString &queryStr) {
    if (!db.isOpen()) {
        qDebug() << "Database is not open!";
        qWarning() << "Database is not open!";
        return QSqlQuery();
    }

    QSqlQuery query(db);
    if (!query.exec(queryStr)) {
        qDebug() << "Query failed: " << query.lastError().text();
        qWarning() << "Database is not open!";
        return QSqlQuery();
    }

    return query;
}

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

    gpa = qualityPoints / totalHours;

    QString formattedGPA = QString::number(gpa, 'f', 2);
    return formattedGPA;
}

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

void onSaveButtonClicked(QString studID, QWidget* addEditWindow, std::shared_ptr<QMap<std::string, QString>> selectedRow) {
    // get combo boxes
    QComboBox* gradeCombo = addEditWindow->findChild<QComboBox*>("gradeCombo");
    QComboBox* crnCombo = addEditWindow->findChild<QComboBox*>("crnCombo");

    // get values
    int crn = QString(crnCombo->currentText()).toInt();
    QString grade = QString(gradeCombo->currentText());

    QSqlQuery gradeQuery;

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

void setComboBoxValues(QComboBox* crnCombo, QComboBox* prefixCombo, QComboBox* numberCombo, QSqlQuery coursesInfo) {
    QList<QList<QString>> uniqueItems(3);

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
}

// Function to create the add/edit window
QWidget* createAddEditWindow(QSqlDatabase db, QString studID, std::shared_ptr<QMap<std::string, QString>> selectedRow) {
    QWidget* addEditWindow = loadUiFile("../src/addEditWindow.ui");
    QList<QList<QString>> uniqueItems(3);

    // get combo boxes
    QComboBox* semesterCombo = addEditWindow->findChild<QComboBox*>("semesterCombo");
    QComboBox* gradeCombo = addEditWindow->findChild<QComboBox*>("gradeCombo");
    QComboBox* hoursCombo = addEditWindow->findChild<QComboBox*>("hoursCombo");
    QComboBox* yearCombo = addEditWindow->findChild<QComboBox*>("yearCombo");
    QComboBox* crnCombo = addEditWindow->findChild<QComboBox*>("crnCombo");
    QComboBox* prefixCombo = addEditWindow->findChild<QComboBox*>("prefixCombo");
    QComboBox* numberCombo = addEditWindow->findChild<QComboBox*>("numberCombo");

    // set static combo box values
    semesterCombo->addItems({"Fall", "Winter", "Spring", "Summer"});
    gradeCombo->addItems({"A", "B", "C", "D", "F", "FN", "W"});
    hoursCombo->addItems({"1.0", "2.0", "3.0", "4.0"});

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
    
    // set dynamic combo box values
    QString coursesQuery = QString("SELECT crn, course_prefix, course_num FROM courses");
    QSqlQuery coursesInfo = executeQuery(db, coursesQuery);
    
    setComboBoxValues(crnCombo, prefixCombo, numberCombo, std::move(coursesInfo));
    /*
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
    */
    // set initial state of all combo boxes to no selection
    if (selectedRow->isEmpty()) {
        semesterCombo->setCurrentIndex(-1);
        yearCombo->setCurrentIndex(-1);
        gradeCombo->setCurrentIndex(-1);
        hoursCombo->setCurrentIndex(-1);
        crnCombo->setCurrentIndex(-1);
        prefixCombo->setCurrentIndex(-1);
        numberCombo->setCurrentIndex(-1);
    } else {
        semesterCombo->setCurrentText((*selectedRow)["semester"]);
        semesterCombo->setDisabled(true);
        yearCombo->setCurrentText((*selectedRow)["year"]);
        yearCombo->setDisabled(true);
        gradeCombo->setCurrentText((*selectedRow)["grade"]);
        hoursCombo->setCurrentText((*selectedRow)["hours"]+".0");
        hoursCombo->setDisabled(true);
        crnCombo->setCurrentText((*selectedRow)["crn"]);
        crnCombo->setDisabled(true);
        prefixCombo->setCurrentText((*selectedRow)["course_prefix"]);
        prefixCombo->setDisabled(true);
        numberCombo->setCurrentText((*selectedRow)["course_num"]);
        numberCombo->setDisabled(true);
    }

    // filter combo box options based on crn selected
    QObject::connect(crnCombo, &QComboBox::currentIndexChanged, addEditWindow, [](){

    });


    // filter combo box options based on course prefix selected

    // filter combo box options based on course number selected

    // filter combo box options based on semester selected

    // filter combo box options based on year selected

    // filter combo box options based on hours selected
    
    // get and connect save button
    QPushButton* saveButton = addEditWindow->findChild<QPushButton*>("saveButton");
    QObject::connect(saveButton, &QPushButton::clicked, addEditWindow, [studID, addEditWindow, selectedRow](){
        onSaveButtonClicked(studID, addEditWindow, selectedRow);
    });

    // get and connect cancel button
    QPushButton* cancelButton = addEditWindow->findChild<QPushButton*>("cancelButton");
    QObject::connect(cancelButton, &QPushButton::clicked, addEditWindow, &QWidget::close);
    
    return addEditWindow;
}

// Function to create the grade window
QWidget* createGradeWindow(QString studID, QSqlDatabase db) {
    QWidget* gradeWindow = loadUiFile("../src/resultswindow.ui");
    std::shared_ptr<QMap<std::string, QString>> selectedRow = std::make_shared<QMap<std::string, QString>>();


    // get student name
    QString studentQuery = QString("SELECT * FROM students WHERE student_id LIKE '%%1%'").arg(studID);
    QSqlQuery nameResults = executeQuery(db, studentQuery);

    while (nameResults.next()) {
        QString fullName = nameResults.value("first_name").toString() + " " + nameResults.value("last_name").toString();

        // get and set displayName label
        QLabel* displayName = gradeWindow->findChild<QLabel*>("displayName");
        displayName->setText(fullName);
    }

    // query grades using student id
    QString gradeQuery = QString("SELECT courses.crn, course_prefix, course_num, semester, year, hours, grade FROM courses INNER JOIN grades on courses.crn = grades.crn WHERE grades.student_id LIKE '%%1%'").arg(studID);
    QSqlQuery gradeData = executeQuery(db, gradeQuery);

    // GPA calculation
    QString cumGpa = calculateGPA(gradeData);
    QLabel* gpaLabel = gradeWindow->findChild<QLabel*>("cumGpa");
    gpaLabel->setText(cumGpa);

    // display in table
    QSqlQueryModel* model = new QSqlQueryModel();
    model->setQuery(std::move(gradeData));

    if (model->lastError().isValid()) {
        qDebug() << "Model error: " << model->lastError().text();
    }
    
    // connect model to table
    QTableView* gradeTable = gradeWindow->findChild<QTableView*>("gradeTable");
    gradeTable->setModel(model);
    gradeTable->resizeColumnsToContents();
    gradeTable->setSelectionBehavior(QTableView::SelectRows);
    
    // Connect the selection changed signal to function
    QObject::connect(gradeTable->selectionModel(), &QItemSelectionModel::selectionChanged,
                gradeWindow, [model, selectedRow](const QItemSelection &selected, const QItemSelection &deselected) {
        *selectedRow = onSelectionChanged(model, selected, deselected);
    });

    // get and connect add/edit button
    QPushButton* addEditButton = gradeWindow->findChild<QPushButton*>("addEditButton");
    QObject::connect(addEditButton, &QPushButton::clicked, gradeWindow, [db, studID, selectedRow](){
        // create addEditWindow
        QWidget* addEditWindow = createAddEditWindow(db, studID, selectedRow);
        
        addEditWindow->show();
    });

    // get and connect close button
    QPushButton* closeButton = gradeWindow->findChild<QPushButton*>("closeButton");
    QObject::connect(closeButton, &QPushButton::clicked, gradeWindow, &QWidget::close);

    return gradeWindow;
}

// Function to connect to database
QSqlDatabase databaseConnection() {
    QString dbName = "gradeDB";
    QString user = "mitch";
    // database configuration
    QSettings settings("config.ini", QSettings::IniFormat);

    QString dbIp = settings.value("Database/IP").toString();
    QString dbPassword = settings.value("Database/Password").toString();

    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
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

// Function to handle student id search
void onSearchButtonClicked(QWidget* mainWindow) {
    QLineEdit* inputBox = mainWindow->findChild<QLineEdit*>("idInput");
    QString studID = inputBox->text();

    // ensure db connection exists
    QSqlDatabase db = databaseConnection();
    if (db.isValid()) {
        // Create grade window
        QWidget* gradeWindow = createGradeWindow(studID, db);
        
        gradeWindow->show();
    }
    inputBox->setText("");
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Load the main window
    QWidget* mainWindow = loadUiFile("../src/mainwindow.ui");

    // get input box and search button
    QPushButton* searchButton = mainWindow->findChild<QPushButton*>("searchButton");

    QObject::connect(searchButton, &QPushButton::clicked, mainWindow, [mainWindow](){
        onSearchButtonClicked(mainWindow);
    });

    // Show the main window
    mainWindow->show();

    // Run the application
    return app.exec();
}
