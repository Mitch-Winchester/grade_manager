#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <QString>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QItemSelection>

QSqlDatabase databaseConnection();
QWidget* loadUiFile(const QString& filePath);
QString calculateGPA(QSqlQuery &gradeData);
QSqlQuery executeQuery(const QSqlDatabase &db, const QString &queryStr);
QMap<std::string, QString> onSelectionChanged(QSqlQueryModel* model, const QItemSelection &selected, const QItemSelection &deselected);

#endif