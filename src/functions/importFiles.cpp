#include "functions.h"
#include "xlsxdocument.h"

#include <filesystem>
#include <vector>

QStringList getSubdirectories(const std::string &parentDir) {
    QStringList subDirs;

    for (const auto &entry : std::filesystem::directory_iterator(parentDir)) {
        if (entry.is_directory()) {
            std::string name = entry.path().filename().string();
            if (name != "." && name != "..") { // Exclude special entries
                subDirs.append(QString::fromStdString(name));
            }
        }
    }

    return subDirs;
}

QStringList getFilesInDirectory(const std::string &dir) {
    QStringList files;

    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_regular_file()) {
            files.append(QString::fromStdString(entry.path().filename().string()));
        }
    }

    return files;
}

int getCRN(QSqlDatabase &db, const QString &coursePrefix, int courseNum, const QString &year, const QString &semester) {
    QSqlQuery crnQuery(db);
    crnQuery.prepare("SELECT crn FROM courses WHERE course_num = :course_num AND course_prefix = :course_prefix AND year = :year AND semester = :semester");
    crnQuery.bindValue(":course_num", courseNum);
    crnQuery.bindValue(":course_prefix", coursePrefix);
    crnQuery.bindValue(":year", year);
    crnQuery.bindValue(":semester", semester);

    if (crnQuery.exec() && crnQuery.next()) {
        return crnQuery.value("crn").toInt();
    }

    qDebug() << "Failed CRN query: " << crnQuery.lastError().text();
    return -1;
}

bool studentExists(int id, QSqlDatabase &db, QString &firstName, QString &lastName) {
    QSqlQuery studQuery(db);
    studQuery.prepare("SELECT * FROM students WHERE student_id = :student_id");
    studQuery.bindValue(":student_id", id);

    if (studQuery.exec() && studQuery.next()) {
        firstName = studQuery.value("first_name").toString();
        lastName = studQuery.value("last_name").toString();
        return true;
    }

    return false;
}

bool insertGrade(int crn, int id, const QString &grade, QSqlDatabase &db) {
    QSqlQuery insertQuery(db);
    insertQuery.prepare("INSERT INTO grades (grade, student_id, crn) VALUES (:grade, :student_id, :crn)");
    insertQuery.bindValue(":grade", grade);
    insertQuery.bindValue(":student_id", id);
    insertQuery.bindValue(":crn", crn);

    if (!insertQuery.exec()) {
        qDebug() << "Insert failed: " << insertQuery.lastError().text();
        return false;
    }

    return true;
}

void processExcelFile(QSqlDatabase &db, const QString &filePath, int crn) {
    QXlsx::Document xlsx(filePath);

    if (!xlsx.load()) {
        qDebug() << "Failed to open Excel file: " << filePath;
        return;
    }

    int rowCount = xlsx.dimension().rowCount();
    for (int row = 2; row <= rowCount; ++row) {
        QString name = xlsx.read(row, 1).toString();
        int id = xlsx.read(row, 2).toInt();
        QString grade = xlsx.read(row, 3).toString();

        QString firstName, lastName;
        qDebug() << "Name: " << name << "ID: " << id << "Grade: " << grade;
        
        if (studentExists(id, db, firstName, lastName) && (name == firstName + " " + lastName)) {
            if (insertGrade(crn, id, grade, db)) {
                qDebug() << "Insert success! Name: " << name << "ID: " << "Grade: " << grade;
            }
        } else {
            qDebug() << "Student mismatch or ID not found for: " << id;
        }
    }
}

void importGrades() {
    std::string parentDir = "../all_grades";

    // get all subdirectories in the parent directory
    QStringList subDirs = getSubdirectories(parentDir);

    for (const QString &subDirName : subDirs) {
        if (!subDirName.startsWith("Grades")) {
            qDebug() << "Skipping unrelated folder: " << subDirName;
            continue;
        }

        QStringList folderParts = subDirName.split(' ');
        if (folderParts.size() != 3) {
            qDebug() << "Skipping folder with unexpected format: " << subDirName;
        }

        QString year = folderParts[1];
        QString semester = folderParts[2];

        qDebug() << "Processing folder: " << subDirName;
        std::string subDirPath = parentDir + "/" + QString(subDirName).toStdString();
        QStringList files = getFilesInDirectory(subDirPath);

        for (const QString &fileName : files) {
            QFileInfo fileInfo(fileName);
            QString baseName = fileInfo.completeBaseName();
            QStringList fileParts = baseName.split(' ');

            if (fileParts.size() != 4) {
                qDebug() << "Skipping file with unexpected format: " << baseName;
                continue;
            }

            QString coursePrefix = fileParts[0];
            int courseNum = fileParts[1].toInt();

            // ensure db connection exists
            QSqlDatabase importConn = databaseConnection(QString("importConn"));
            try {
                int crn = getCRN(importConn, coursePrefix, courseNum, year, semester);
                if (crn == -1) {
                    importConn.close();
                    throw "CRN not found!";
                }

                processExcelFile(importConn, QString::fromStdString(subDirPath) + "/" + fileName, crn);
                importConn.close();
            } catch (std::exception& crnEx) {
                qDebug() << "Failed CRN query: " << crnEx.what();
            }

        }
    }

}