#include "functions.h"

#include <QFile>
#include <QUiLoader>
#include <QSettings>

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

    gpa = qualityPoints / totalHours;

    QString formattedGPA = QString::number(gpa, 'f', 2);
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

// Function to execute query
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