#include <QApplication>
#include <QFile>
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QtUiTools/QUiLoader>

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

// Function to create the second window
QWidget* createAddEditWindow() {
    QWidget* addEditWindow = new QWidget;
    addEditWindow->setWindowTitle("Grade Entry Form");
    addEditWindow->resize(300, 150);

    QVBoxLayout* layout = new QVBoxLayout(addEditWindow);

    QPushButton* closeButton = new QPushButton("Close", addEditWindow);
    layout->addWidget(closeButton);

    QObject::connect(closeButton, &QPushButton::clicked, addEditWindow, &QWidget::close);

    return addEditWindow;
}

// Function to create the second window
QWidget* createGradeWindow() {
    QWidget* gradeWindow = loadUiFile("../src/resultswindow.ui");


    // get and connect close button
    QPushButton* closeButton = gradeWindow->findChild<QPushButton*>("closeButton");
    QObject::connect(closeButton, &QPushButton::clicked, gradeWindow, &QWidget::close);

    return gradeWindow;
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Load the main window
    QWidget* mainWindow = loadUiFile("../src/mainwindow.ui");

    //
    QPushButton* searchButton = mainWindow->findChild<QPushButton*>("searchButton");

    // Create the second window
    QWidget* gradeWindow = createGradeWindow();

    // Connect the button to show the second window
    QObject::connect(searchButton, &QPushButton::clicked, gradeWindow, &QWidget::show);

    // Show the main window
    mainWindow->show();

    // Run the application
    return app.exec();
}
