#include "functions/functions.h"

#include <QApplication>
#include <QLineEdit>
#include <QTableView>

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

    // variables to keep track of selected options
    std::shared_ptr<QVariantMap> stateTracker = std::make_shared<QVariantMap>(); 
    *stateTracker = {{"crn", -1}, {"prefix", ""}, {"number", -1}, {"year", -1}, {"semester", ""}, {"hours", -1}};

    // get combo boxes
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
    
    // set initial combo box values
    QSqlQuery coursesInfo;
    setComboBoxValues(addEditConn, addEditWindow, std::move(coursesInfo), stateTracker);
    
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
        setComboBoxValues(addEditConn, addEditWindow, std::move(coursesInfo), stateTracker);

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
        coursesInfo.prepare("SELECT * FROM courses WHERE course_prefix LIKE :prefix AND (:course_num IS NULL OR course_num = :course_num) AND (:semester IS NULL OR semester = :semester) AND (:year IS NULL OR year = :year) AND (:hours IS NULL OR hours = :hours)");
        coursesInfo.bindValue(":prefix", (*stateTracker)["prefix"].toString());

        // check for other filters
        if ((*stateTracker)["number"].toInt() != -1){
            coursesInfo.bindValue(":course_num", (*stateTracker)["number"].toInt());
        }
        if ((*stateTracker)["semester"] != ""){
            coursesInfo.bindValue(":semester", (*stateTracker)["semester"].toString());
        }
        if ((*stateTracker)["year"].toInt() != -1){
            coursesInfo.bindValue(":year", (*stateTracker)["year"].toInt());
        }
        if ((*stateTracker)["hours"].toInt() != -1){
            coursesInfo.bindValue(":hours", (*stateTracker)["hours"].toInt());
        }
        
        if (!coursesInfo.exec()) {
            qDebug() << "Query failed: " << coursesInfo.lastError().text();
        } else {
            qDebug() << "Query executed successfully.";
        }
        setComboBoxValues(addEditConn, addEditWindow, std::move(coursesInfo), stateTracker);

        // Unblock signals after the update
        blockAllComboBoxes(addEditWindow, false);
    });

    // filter combo box options based on course number selected
    QObject::connect(numberCombo, &QComboBox::currentIndexChanged, addEditWindow, [addEditConn, numberCombo, addEditWindow, stateTracker](){
        // Block signals to prevent recursive calls
        blockAllComboBoxes(addEditWindow, true);

        // set tracker
        (*stateTracker)["number"] = QString(numberCombo->currentText()).toInt();
        
        QSqlQuery coursesInfo(addEditConn);
        coursesInfo.prepare("SELECT * FROM courses WHERE course_num LIKE :num AND (:course_prefix IS NULL OR course_prefix = :course_prefix) AND (:semester IS NULL OR semester = :semester) AND (:year IS NULL OR year = :year) AND (:hours IS NULL OR hours = :hours)");
        coursesInfo.bindValue(":num", (*stateTracker)["number"].toInt());

        // check for other filters
        if ((*stateTracker)["prefix"] != "") {
            coursesInfo.bindValue(":course_prefix", (*stateTracker)["prefix"].toString());
        }
        if ((*stateTracker)["semester"] != ""){
            coursesInfo.bindValue(":semester", (*stateTracker)["semester"].toString());
        }
        if ((*stateTracker)["year"].toInt() != -1){
            coursesInfo.bindValue(":year", (*stateTracker)["year"].toInt());
        }
        if ((*stateTracker)["hours"].toInt() != -1){
            coursesInfo.bindValue(":hours", (*stateTracker)["hours"].toInt());
        }

        if (!coursesInfo.exec()) {
            qDebug() << "Query failed: " << coursesInfo.lastError().text();
        } else {
            qDebug() << "Query executed successfully.";
        }
        setComboBoxValues(addEditConn, addEditWindow, std::move(coursesInfo), stateTracker);

        // Unblock signals after the update
        blockAllComboBoxes(addEditWindow, false);
    });

    // filter combo box options based on semester selected
    QObject::connect(semesterCombo, &QComboBox::currentIndexChanged, addEditWindow, [addEditConn, semesterCombo, addEditWindow, stateTracker](){
        // Block signals to prevent recursive calls
        blockAllComboBoxes(addEditWindow, true);

        // set tracker
        (*stateTracker)["semester"] = QString(semesterCombo->currentText());

        QSqlQuery coursesInfo(addEditConn);
        coursesInfo.prepare("SELECT * FROM courses WHERE semester LIKE :sem AND (:course_prefix IS NULL OR course_prefix = :course_prefix) AND (:course_num IS NULL OR course_num = :course_num) AND (:year IS NULL OR year = :year) AND (:hours IS NULL OR hours = :hours)");
        coursesInfo.bindValue(":sem", (*stateTracker)["semester"].toString());

        // check for other filters
        if ((*stateTracker)["prefix"] != "") {
            coursesInfo.bindValue(":course_prefix", (*stateTracker)["prefix"].toString());
        }
        if ((*stateTracker)["number"].toInt() != -1){
            coursesInfo.bindValue(":course_num", (*stateTracker)["number"].toInt());
        }
        if ((*stateTracker)["year"].toInt() != -1){
            coursesInfo.bindValue(":year", (*stateTracker)["year"].toInt());
        }
        if ((*stateTracker)["hours"].toInt() != -1){
            coursesInfo.bindValue(":hours", (*stateTracker)["hours"].toInt());
        }

        if (!coursesInfo.exec()) {
            qDebug() << "Query failed: " << coursesInfo.lastError().text();
        } else {
            qDebug() << "Query executed successfully.";
        }
        // error message if query empty
        if (!coursesInfo.next()) {
            QWidget* errMessage = createAlertWindow("No courses match your selection");
            errMessage->show();
            *stateTracker = {{"crn", -1}, {"prefix", ""}, {"number", -1}, {"year", -1}, {"semester", ""}, {"hours", -1}};
            setComboBoxValues(addEditConn, addEditWindow, std::move(coursesInfo), stateTracker);
        } else {
            setComboBoxValues(addEditConn, addEditWindow, std::move(coursesInfo), stateTracker);
        }
        // Unblock signals after the update
        blockAllComboBoxes(addEditWindow, false);
    });

    // filter combo box options based on year selected
    QObject::connect(yearCombo, &QComboBox::currentIndexChanged, addEditWindow, [addEditConn, yearCombo, addEditWindow, stateTracker](){
        // Block signals to prevent recursive calls
        blockAllComboBoxes(addEditWindow, true);

        // set tracker
        (*stateTracker)["year"] = QString(yearCombo->currentText()).toInt();

        QSqlQuery coursesInfo(addEditConn);
        coursesInfo.prepare("SELECT * FROM courses WHERE year LIKE :year AND (:course_prefix IS NULL OR course_prefix = :course_prefix) AND (:course_num IS NULL OR course_num = :course_num) AND (:semester IS NULL OR semester = :semester) AND (:hours IS NULL OR hours = :hours)");
        coursesInfo.bindValue(":year", (*stateTracker)["year"]);

        // check for other filters
        if ((*stateTracker)["number"].toInt() != -1){
            coursesInfo.bindValue(":course_num", (*stateTracker)["number"].toInt());
        }
        if ((*stateTracker)["prefix"] != "") {
            coursesInfo.bindValue(":course_prefix", (*stateTracker)["prefix"].toString());
        }
        if ((*stateTracker)["semester"] != ""){
            coursesInfo.bindValue(":semester", (*stateTracker)["semester"].toString());
        }
        if ((*stateTracker)["hours"].toInt() != -1){
            coursesInfo.bindValue(":hours", (*stateTracker)["hours"].toInt());
        }

        if (!coursesInfo.exec()) {
            qDebug() << "Query failed: " << coursesInfo.lastError().text();
        } else {
            qDebug() << "Query executed successfully.";
        }
        // error message if query empty
        if (!coursesInfo.next()) {
            QWidget* errMessage = createAlertWindow("No courses match your selection");
            errMessage->show();
            *stateTracker = {{"crn", -1}, {"prefix", ""}, {"number", -1}, {"year", -1}, {"semester", ""}, {"hours", -1}};
            setComboBoxValues(addEditConn, addEditWindow, std::move(coursesInfo), stateTracker);
        } else {
            setComboBoxValues(addEditConn, addEditWindow, std::move(coursesInfo), stateTracker);
        }
        // Unblock signals after the update
        blockAllComboBoxes(addEditWindow, false);
    });

    // filter combo box options based on hours selected
    QObject::connect(hoursCombo, &QComboBox::currentIndexChanged, addEditWindow, [addEditConn, hoursCombo, addEditWindow, stateTracker](){
        // Block signals to prevent recursive calls
        blockAllComboBoxes(addEditWindow, true);

        // set state
        (*stateTracker)["hours"] = (int)(QString(hoursCombo->currentText()).toDouble());

        QSqlQuery coursesInfo(addEditConn);
        coursesInfo.prepare("SELECT * FROM courses WHERE hours LIKE :hours AND (:course_prefix IS NULL OR course_prefix = :course_prefix) AND (:course_num IS NULL OR course_num = :course_num) AND (:semester IS NULL OR semester = :semester) AND (:year IS NULL OR year = :year)");
        coursesInfo.bindValue(":hours", (*stateTracker)["hours"].toInt());

        // check for other filters
        if ((*stateTracker)["number"].toInt() != -1){
            coursesInfo.bindValue(":course_num", (*stateTracker)["number"].toInt());
        }
        if ((*stateTracker)["prefix"] != "") {
            coursesInfo.bindValue(":course_prefix", (*stateTracker)["prefix"].toString());
        }
        if ((*stateTracker)["semester"] != ""){
            coursesInfo.bindValue(":semester", (*stateTracker)["semester"].toString());
        }
        if ((*stateTracker)["year"].toInt() != -1){
            coursesInfo.bindValue(":year", (*stateTracker)["year"].toInt());
        }

        if (!coursesInfo.exec()) {
            qDebug() << "Query failed: " << coursesInfo.lastError().text();
        } else {
            qDebug() << "Query executed successfully.";
        }

        // error message if query empty
        if (!coursesInfo.next()) {
            QWidget* errMessage = createAlertWindow("No courses match your selection");
            errMessage->show();
            *stateTracker = {{"crn", -1}, {"prefix", ""}, {"number", -1}, {"year", -1}, {"semester", ""}, {"hours", -1}};
            setComboBoxValues(addEditConn, addEditWindow, std::move(coursesInfo), stateTracker);
        } else {
            setComboBoxValues(addEditConn, addEditWindow, std::move(coursesInfo), stateTracker);
        }

        // Unblock signals after the update
        blockAllComboBoxes(addEditWindow, false);
    });

    //get and connect reset button
    QPushButton* resetButton = addEditWindow->findChild<QPushButton*>("resetButton");
    QObject::connect(resetButton, &QPushButton::clicked, addEditWindow, [addEditConn, addEditWindow, selectedRow, gradeCombo, stateTracker](){
        // Block signals to prevent recursive calls
        blockAllComboBoxes(addEditWindow, true);

        if (selectedRow->isEmpty()) {
            QSqlQuery coursesInfo;
            setComboBoxValues(addEditConn, addEditWindow, std::move(coursesInfo), stateTracker);
        } else {
            gradeCombo->setCurrentText((*selectedRow)["grade"]);
        }

        // reset stateTracker
        *stateTracker = {{"crn", -1}, {"prefix", ""}, {"number", -1}, {"year", -1}, {"semester", ""}, {"hours", -1}};
     
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

    // confirm user input contains only integers
    bool isInteger = false;
    studID.toInt(&isInteger);

    if (isInteger) {
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
    } else {
        QString message = "Please enter a valid student id containing only numbers!";
        qDebug() << message;
        QWidget* alertWindow = createAlertWindow(message);
        alertWindow->show();
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
    // connect search button with enter key, but 
    // only after the user has put a search in the text box
    QLineEdit* inputField = mainWindow->findChild<QLineEdit*>("idInput");
    if (inputField && searchButton) {
        QObject::connect(inputField, &QLineEdit::returnPressed, searchButton, &QPushButton::click);
    }


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
