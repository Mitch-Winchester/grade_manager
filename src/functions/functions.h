#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <QCoreApplication>
#include <QString>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QItemSelection>

#include <QSqlQuery>
#include <QSqlError>
#include <QSqlQueryModel>

#include <QDir>
#include <QFileInfo>

#include <QDebug>

QSqlDatabase databaseConnection(QString connectionName);
QWidget* loadUiFile(const QString& filePath);
QWidget* createAlertWindow(QString message);
QString calculateGPA(QSqlQuery &gradeData);
void blockAllComboBoxes(QWidget* addEditWindow, bool boxSet);
void setComboBoxesToNone(QWidget* addEditWindow, QComboBox* filter);
QMap<std::string, QString> onSelectionChanged(QSqlQueryModel* model, const QItemSelection &selected, const QItemSelection &deselected);
void onSaveButtonClicked(const QSqlDatabase &db, QString studID, QWidget* addEditWindow, std::shared_ptr<QMap<std::string, QString>> selectedRow);
void onDeleteButtonClicked(const QSqlDatabase &db, QString studID, std::shared_ptr<QMap<std::string, QString>> selectedRow);
void setComboBoxValues(QSqlDatabase dbConn, QWidget* addEditWindow, QSqlQuery coursesInfo, std::shared_ptr<QVariantMap> stateTracker);
void importGrades();
void printTranscript(QString studID, QString , QString cumGpa);

#endif