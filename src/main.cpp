#include "functions/functions.h"

#include <QApplication>
#include <QWidget>
#include <QLineEdit>
#include <QTableView>
#include <QComboBox>

#include <QIntValidator>

// Function to create confirmation message window
QWidget* createConfirmWindow(QString message, std::function<void()> onConfirm) {
    QWidget* confirmWindow = loadUiFile("../src/gui/confirmationwindow.ui");

    // get and set message label
    QLabel* messageLabel = confirmWindow->findChild<QLabel*>("messageLabel");
    messageLabel->setText(message);

    // get and connect confirm button
    QPushButton* confirmButton = confirmWindow->findChild<QPushButton*>("confirmButton");
    QObject::connect(confirmButton, &QPushButton::clicked, confirmWindow, [confirmWindow, onConfirm](){
        if (onConfirm) {
            onConfirm();
        }
        confirmWindow->close();
    });

    // get and connect cancel button
    QPushButton* cancelButton = confirmWindow->findChild<QPushButton*>("cancelButton");
    QObject::connect(cancelButton, &QPushButton::clicked, confirmWindow, &QWidget::close);
    
    return confirmWindow;
}

// Function to create the add/edit window
QWidget* createAddEditWindow(QString studID, std::shared_ptr<QMap<std::string, QString>> selectedRow, std::function<void()> refreshGradeTable) {
    QWidget* addEditWindow = loadUiFile("../src/gui/addEditWindow.ui");
    QList<QList<QString>> uniqueItems(3);

    // get combo boxes    // get combo boxes
    QComboBox* crnCombo = addEditWindow->findChild<QComboBox*>("crnCombo");
    QComboBox* prefixCombo = addEditWindow->findChild<QComboBox*>("prefixCombo");
    QComboBox* numberCombo = addEditWindow->findChild<QComboBox*>("numberCombo");
    QComboBox* yearCombo = addEditWindow->findChild<QComboBox*>("yearCombo");
    QComboBox* semesterCombo = addEditWindow->findChild<QComboBox*>("semesterCombo");
    QComboBox* hoursCombo = addEditWindow->findChild<QComboBox*>("hoursCombo");
    QComboBox* gradeCombo = addEditWindow->findChild<QComboBox*>("gradeCombo");

    // set grades combo box values
    gradeCombo->addItems({"A", "B", "C", "D", "F", "FN", "W"});

    // ensure db connection exists
    QSqlDatabase addEditConn = databaseConnection(QString("addEditConn"));
    // set dynamic combo box values
    QString coursesQuery = QString("SELECT crn, course_prefix, course_num FROM courses");
    QSqlQuery coursesInfo = executeQuery(addEditConn, coursesQuery);
    
    setComboBoxValues(addEditWindow, std::move(coursesInfo));

    // set initial state of combo boxes for update
    if (!selectedRow->isEmpty()) {
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

    // variables to keep track of selected options
    std::shared_ptr<QVariantMap> stateTracker = std::make_shared<QVariantMap>(); 
    *stateTracker = {{"crn", -1}, {"prefix", ""}, {"num", -1}, {"year", -1}, {"sem", ""}, {"hrs", -1}};
                                
    // filter combo box options based on crn selected
    QObject::connect(crnCombo, &QComboBox::currentIndexChanged, addEditWindow, [addEditConn, crnCombo, addEditWindow, stateTracker](){
        // Block signals to prevent recursive calls
        blockAllComboBoxes(addEditWindow, true);

        // set tracker
        (*stateTracker)["crn"] = QString(crnCombo->currentText()).toInt();

        QSqlQuery coursesInfo(addEditConn);
        coursesInfo.prepare("SELECT * FROM courses WHERE crn LIKE :crn");
        coursesInfo.bindValue(":crn", (*stateTracker)["crn"].toInt());
        if (!coursesInfo.exec()) {
            qDebug() << "Query failed: " << coursesInfo.lastError().text();
        } else {
            qDebug() << "Query executed successfully.";
        }
        setComboBoxValues(addEditWindow, std::move(coursesInfo));

        // Unblock signals after the update
        blockAllComboBoxes(addEditWindow, false);
    });

    // filter combo box options based on course prefix selected
    QObject::connect(prefixCombo, &QComboBox::currentIndexChanged, addEditWindow, [addEditConn, prefixCombo, addEditWindow, stateTracker](){
        // Block signals to prevent recursive calls
        blockAllComboBoxes(addEditWindow, true);

        // set tracker
        (*stateTracker)["prefix"] = QString(prefixCombo->currentText());

        QSqlQuery coursesInfo(addEditConn);
        coursesInfo.prepare("SELECT * FROM courses WHERE course_prefix LIKE :prefix AND (:course_num IS NULL OR course_num = :course_num)");
        coursesInfo.bindValue(":prefix", (*stateTracker)["prefix"].toString());

        if ((*stateTracker)["num"].toInt() != -1){
            coursesInfo.bindValue(":course_num", (*stateTracker)["num"].toInt());
        }

        if (!coursesInfo.exec()) {
            qDebug() << "Query failed: " << coursesInfo.lastError().text();
        } else {
            qDebug() << "Query executed successfully.";
        }
        setComboBoxValues(addEditWindow, std::move(coursesInfo));

        // Unblock signals after the update
        blockAllComboBoxes(addEditWindow, false);
    });

    // filter combo box options based on course number selected
    QObject::connect(numberCombo, &QComboBox::currentIndexChanged, addEditWindow, [addEditConn, numberCombo, addEditWindow, stateTracker](){
        // Block signals to prevent recursive calls
        blockAllComboBoxes(addEditWindow, true);

        // set tracker
        (*stateTracker)["num"] = QString(numberCombo->currentText()).toInt();
        
        QSqlQuery coursesInfo(addEditConn);
        coursesInfo.prepare("SELECT * FROM courses WHERE course_num LIKE :num AND (:course_prefix IS NULL OR course_prefix = :course_prefix)");
        coursesInfo.bindValue(":num", (*stateTracker)["num"].toInt());

        if ((*stateTracker)["prefix"] != "") {
            coursesInfo.bindValue(":course_prefix", (*stateTracker)["prefix"].toString());
        }

        if (!coursesInfo.exec()) {
            qDebug() << "Query failed: " << coursesInfo.lastError().text();
        } else {
            qDebug() << "Query executed successfully.";
        }
        setComboBoxValues(addEditWindow, std::move(coursesInfo));

        // Unblock signals after the update
        blockAllComboBoxes(addEditWindow, false);
    });

    // filter combo box options based on semester selected
    QObject::connect(semesterCombo, &QComboBox::currentIndexChanged, addEditWindow, [addEditConn, semesterCombo, addEditWindow](){
        // Block signals to prevent recursive calls
        blockAllComboBoxes(addEditWindow, true);

        // set dynamic combo box values
        QString sem = QString(semesterCombo->currentText());
        QSqlQuery coursesInfo(addEditConn);
        coursesInfo.prepare("SELECT * FROM courses WHERE semester LIKE :sem");
        coursesInfo.bindValue(":sem", sem);
        if (!coursesInfo.exec()) {
            qDebug() << "Query failed: " << coursesInfo.lastError().text();
        } else {
            qDebug() << "Query executed successfully.";
        }
        setComboBoxValues(addEditWindow, std::move(coursesInfo));

        // Unblock signals after the update
        blockAllComboBoxes(addEditWindow, false);
    });

    // filter combo box options based on year selected
    QObject::connect(yearCombo, &QComboBox::currentIndexChanged, addEditWindow, [addEditConn, yearCombo, addEditWindow](){
        // Block signals to prevent recursive calls
        blockAllComboBoxes(addEditWindow, true);

        // set dynamic combo box values
        int year = QString(yearCombo->currentText()).toInt();
        QSqlQuery coursesInfo(addEditConn);
        coursesInfo.prepare("SELECT * FROM courses WHERE year LIKE :year");
        coursesInfo.bindValue(":year", year);
        if (!coursesInfo.exec()) {
            qDebug() << "Query failed: " << coursesInfo.lastError().text();
        } else {
            qDebug() << "Query executed successfully.";
        }
        setComboBoxValues(addEditWindow, std::move(coursesInfo));

        // Unblock signals after the update
        blockAllComboBoxes(addEditWindow, false);
    });

    // filter combo box options based on hours selected
    QObject::connect(hoursCombo, &QComboBox::currentIndexChanged, addEditWindow, [addEditConn, hoursCombo, addEditWindow](){
        // Block signals to prevent recursive calls
        blockAllComboBoxes(addEditWindow, true);

        // set dynamic combo box values
        int hrs = QString(hoursCombo->currentText()).toInt();
        QSqlQuery coursesInfo(addEditConn);
        coursesInfo.prepare("SELECT * FROM courses WHERE hours LIKE :hours");
        coursesInfo.bindValue(":hours", hrs);
        if (!coursesInfo.exec()) {
            qDebug() << "Query failed: " << coursesInfo.lastError().text();
        } else {
            qDebug() << "Query executed successfully.";
        }
        setComboBoxValues(addEditWindow, std::move(coursesInfo));

        // Unblock signals after the update
        blockAllComboBoxes(addEditWindow, false);
    });

    //get and connect reset button
    QPushButton* resetButton = addEditWindow->findChild<QPushButton*>("resetButton");
    QObject::connect(resetButton, &QPushButton::clicked, addEditWindow, [addEditConn, addEditWindow, selectedRow, gradeCombo, stateTracker](){
        // Block signals to prevent recursive calls
        blockAllComboBoxes(addEditWindow, true);

        if (selectedRow->isEmpty()) {
            // set dynamic combo box values
            QString coursesQuery = QString("SELECT crn, course_prefix, course_num FROM courses");
            QSqlQuery coursesInfo = executeQuery(addEditConn, coursesQuery);
            
            setComboBoxValues(addEditWindow, std::move(coursesInfo));
        } else {
            gradeCombo->setCurrentText((*selectedRow)["grade"]);
        }

        // reset stateTracker
        *stateTracker = {{"crn", -1}, {"prefix", ""}, {"num", -1}, {"year", -1}, {"sem", ""}, {"hrs", -1}};
     
        // Unblock signals after the update
        blockAllComboBoxes(addEditWindow, false);
    });

    // get and connect save button
    QPushButton* saveButton = addEditWindow->findChild<QPushButton*>("saveButton");
    QObject::connect(saveButton, &QPushButton::clicked, addEditWindow, 
                        [addEditConn, studID, addEditWindow, selectedRow, refreshGradeTable](){
        // create confirmation window
        QString message;
        if (selectedRow->isEmpty()) {
            message = "Are you sure you want to add this new record?";
        } else {
            message = "Are you sure you want to update the existing record?";
        }
        QWidget* confirmWindow = createConfirmWindow(message, 
                        [addEditConn, studID, addEditWindow, selectedRow, refreshGradeTable](){
            onSaveButtonClicked(addEditConn, studID, addEditWindow, selectedRow);
            refreshGradeTable();
            addEditWindow->close();
        });

        confirmWindow->show();
    });

    // get and connect cancel button
    QPushButton* cancelButton = addEditWindow->findChild<QPushButton*>("cancelButton");
    QObject::connect(cancelButton, &QPushButton::clicked, addEditWindow, &QWidget::close);
    
    return addEditWindow;
}

// Function to create the grade window
QWidget* createGradeWindow(QString studID) {
    QWidget* gradeWindow = loadUiFile("../src/gui/resultswindow.ui");
    std::shared_ptr<QMap<std::string, QString>> selectedRow = std::make_shared<QMap<std::string, QString>>();

    // ensure db connection exists
    QSqlDatabase gradeTableConn = databaseConnection(QString("gradeTableConn"));
    // get student name
    QString studentQuery = QString("SELECT * FROM students WHERE student_id LIKE '%%1%'").arg(studID);
    QSqlQuery nameResults = executeQuery(gradeTableConn, studentQuery);
    QString fullName;

    while (nameResults.next()) {
        fullName = nameResults.value("first_name").toString() + " " + nameResults.value("last_name").toString();

        // get and set displayName label
        QLabel* displayName = gradeWindow->findChild<QLabel*>("displayName");
        displayName->setText(fullName);
    }

    // Display grade data
    QSqlQueryModel* model = new QSqlQueryModel();
    QTableView* gradeTable = gradeWindow->findChild<QTableView*>("gradeTable");
    // lambda expression to refresh table
    auto refreshGradeTable = [model, gradeTable, studID, gradeTableConn, gradeWindow]() {
        // query grades using student id
        QString gradeQuery = QString("SELECT courses.crn, course_prefix, course_num, semester, year, hours, grade FROM courses INNER JOIN grades on courses.crn = grades.crn WHERE grades.student_id LIKE '%%1%'").arg(studID);
        QSqlQuery gradeData = executeQuery(gradeTableConn, gradeQuery);

        // GPA calculation
        QString calculatedGpa = calculateGPA(gradeData);
        QLabel* gpaLabel = gradeWindow->findChild<QLabel*>("cumGpa");
        gpaLabel->setText(calculatedGpa);

        model->setQuery(std::move(gradeData));

        if (model->lastError().isValid()) {
            qDebug() << "Model error: " << model->lastError().text();
        }

        QStringList headers = {"CRN", "Prefix", "Number", "Semester", "Year", "Hours", "Grade"};
        for (int i = 0; i < headers.size(); ++i) {
            model->setHeaderData(i, Qt::Horizontal, headers.at(i));
        }

        // connect model to table
        gradeTable->setModel(model);
        gradeTable->resizeColumnsToContents();
        gradeTable->setSelectionBehavior(QTableView::SelectRows);
    };
    
    // Initial table load
    refreshGradeTable();

    // Connect the selection changed signal to function
    QObject::connect(gradeTable->selectionModel(), &QItemSelectionModel::selectionChanged,
                gradeWindow, [model, selectedRow](const QItemSelection &selected, const QItemSelection &deselected) {
        *selectedRow = onSelectionChanged(model, selected, deselected);
    });

    // get and connect add/edit button
    QPushButton* addEditButton = gradeWindow->findChild<QPushButton*>("addEditButton");
    QObject::connect(addEditButton, &QPushButton::clicked, gradeWindow, [studID, selectedRow, refreshGradeTable](){
        // create addEditWindow
        QWidget* addEditWindow = createAddEditWindow(studID, selectedRow, refreshGradeTable);

        addEditWindow->show();
    });

    // get and connect print button
    QPushButton* printButton = gradeWindow->findChild<QPushButton*>("printButton");
    QObject::connect(printButton, &QPushButton::clicked, gradeWindow, [gradeWindow, studID, fullName](){
        QLabel* gpaLabel = gradeWindow->findChild<QLabel*>("cumGpa");
        QString cumGpa = QString(gpaLabel->text());
        printTranscript(studID, fullName, cumGpa);
        QString message = QString("Transcript for %1 has been successfully printed!").arg(fullName);
        QWidget* alertWindow = createAlertWindow(message);
        alertWindow->show();
    });

    // get and connect delete button
    QPushButton* deleteButton = gradeWindow->findChild<QPushButton*>("deleteButton");
    QObject::connect(deleteButton, &QPushButton::clicked, gradeWindow, 
                                            [gradeWindow, gradeTableConn, studID, selectedRow, refreshGradeTable](){
        // create confirmation window
        if (selectedRow->isEmpty()) {
            QWidget* alertWindow = createAlertWindow("Please select a valid record from the table!");
            alertWindow->show();
        } else {
            QWidget* confirmWindow = createConfirmWindow("Are you sure you want to delete the selected grade record?", 
                                                                [gradeTableConn, studID, selectedRow, refreshGradeTable](){
                onDeleteButtonClicked(gradeTableConn, studID, selectedRow);
                refreshGradeTable();
            });

            confirmWindow->show();
        }
    });

    // get and connect close button
    QPushButton* closeButton = gradeWindow->findChild<QPushButton*>("closeButton");
    QObject::connect(closeButton, &QPushButton::clicked, gradeWindow, &QWidget::close);

    return gradeWindow;
}

// Function to handle student id search
void onSearchButtonClicked(QWidget* mainWindow) {
    QLineEdit* inputBox = mainWindow->findChild<QLineEdit*>("idInput");
    QString studID = inputBox->text();

    // ensure db connection exists
    QSqlDatabase studIdConn = databaseConnection(QString("studIdConn"));
    if (studIdConn.isValid() && studID != "") {
        // validate student id
        QSqlQuery studentQuery(studIdConn);
        studentQuery.prepare("SELECT * FROM students WHERE student_id LIKE :studID");
        studentQuery.bindValue(":studID", studID);
        if (!studentQuery.exec()) {
            qDebug() << "Query failed: " << studentQuery.lastError().text();
        } else {
            qDebug() << "Query executed successfully.";
        }
        studIdConn.close();

        if (studentQuery.next()) {
            // Create grade window
            QWidget* gradeWindow = createGradeWindow(studID);
            gradeWindow->show();
        } else {
            QString message = "Invalid student id!";
            qDebug() << message;
            QWidget* alertWindow = createAlertWindow(message);
            alertWindow->show();
        }

    }
    inputBox->setText("");
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Load the main window
    QWidget* mainWindow = loadUiFile("../src/gui/mainwindow.ui");

    // get and connect search button
    QPushButton* searchButton = mainWindow->findChild<QPushButton*>("searchButton");
    QObject::connect(searchButton, &QPushButton::clicked, mainWindow, [mainWindow](){
        onSearchButtonClicked(mainWindow);
    });

    // get and connect import button
    QPushButton* importButton = mainWindow->findChild<QPushButton*>("importButton");
    QObject::connect(importButton, &QPushButton::clicked, mainWindow, [mainWindow](){
        importGrades();
    });

    // Show the main window
    mainWindow->show();

    // Run the application
    return app.exec();
}
