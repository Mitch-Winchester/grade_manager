#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <QString>
#include <QComboBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlQueryModel>
#include <QItemSelection>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

QSqlDatabase databaseConnection(QString connectionName);
QWidget* loadUiFile(const QString& filePath);
QString calculateGPA(QSqlQuery &gradeData);
QSqlQuery executeQuery(const QSqlDatabase &db, const QString &queryStr);
QMap<std::string, QString> onSelectionChanged(QSqlQueryModel* model, const QItemSelection &selected, const QItemSelection &deselected);
void onSaveButtonClicked(const QSqlDatabase &db, QString studID, QWidget* addEditWindow, std::shared_ptr<QMap<std::string, QString>> selectedRow);
void onDeleteButtonClicked(const QSqlDatabase &db, QString studID, std::shared_ptr<QMap<std::string, QString>> selectedRow);
void setComboBoxValues(QComboBox* crnCombo, QComboBox* prefixCombo, QComboBox* numberCombo, QSqlQuery coursesInfo);
void importGrades();
void printTranscript(QString studID, QString , QString cumGpa);

#endif