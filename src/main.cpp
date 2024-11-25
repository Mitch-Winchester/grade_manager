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

    // ensure db connection exists
    QSqlDatabase addEditConn = databaseConnection(QString("addEditConn"));
    // set dynamic combo box values
    QString coursesQuery = QString("SELECT crn, course_prefix, course_num FROM courses");
    QSqlQuery coursesInfo = executeQuery(addEditConn, coursesQuery);
    
    setComboBoxValues(crnCombo, prefixCombo, numberCombo, std::move(coursesInfo));

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
        QString studentQuery = QString("SELECT * FROM students WHERE student_id LIKE '%%1%'").arg(studID);
        QSqlQuery studentResults = executeQuery(studIdConn, studentQuery);
        studIdConn.close();

        if (studentResults.next()) {
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
