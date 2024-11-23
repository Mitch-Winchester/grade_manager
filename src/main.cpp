#include <QApplication>
#include <QFile>
#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QTableView>
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

QSqlQuery executeQuery (const QSqlDatabase &db, const QString &queryStr) {
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

// Function to create the second window
QWidget* createAddEditWindow() {
    QWidget* addEditWindow = loadUiFile("../src/addEditWindow.ui");

    // get and connect cancel button
    QPushButton* cancelButton = addEditWindow->findChild<QPushButton*>("cancelButton");
    QObject::connect(cancelButton, &QPushButton::clicked, addEditWindow, &QWidget::close);

    return addEditWindow;
}

// Function to create the second window
QWidget* createGradeWindow(QString &input, QSqlDatabase &db) {
    QWidget* gradeWindow = loadUiFile("../src/resultswindow.ui");

    // get student name
    QString studentQuery = QString("SELECT * FROM students WHERE student_id LIKE '%%1%'").arg(input);
    QSqlQuery nameResults = executeQuery(db, studentQuery);

    while (nameResults.next()) {
        QString fullName = nameResults.value("first_name").toString() + " " + nameResults.value("last_name").toString();

        // get and set displayName label
        QLabel* displayName = gradeWindow->findChild<QLabel*>("displayName");
        displayName->setText(fullName);
    }

    // query grades using student id
    QString gradeQuery = QString("SELECT courses.crn, course_prefix, course_num, semester, year, hours, grade FROM courses INNER JOIN grades on courses.crn = grades.crn WHERE grades.student_id LIKE '%%1%'").arg(input);
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

    // create addEditWindow
    QWidget* addEditWindow = createAddEditWindow();

    // get and connect add/edit button
    QPushButton* addEditButton = gradeWindow->findChild<QPushButton*>("addEditButton");
    QObject::connect(addEditButton, &QPushButton::clicked, addEditWindow, &QWidget::show);

    // get and connect close button
    QPushButton* closeButton = gradeWindow->findChild<QPushButton*>("closeButton");
    QObject::connect(closeButton, &QPushButton::clicked, gradeWindow, &QWidget::close);

    return gradeWindow;
}

// Function to connect to database
QSqlDatabase databaseConnection () {
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
    QString input = inputBox->text();

    // ensure db connection exists
    QSqlDatabase db = databaseConnection();
    if (db.isValid()) {
        // Create grade window
        QWidget* gradeWindow = createGradeWindow(input, db);
        
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
