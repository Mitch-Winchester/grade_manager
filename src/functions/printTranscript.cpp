#include "functions.h"

#include <QPdfWriter>
#include <QPainter>

void printTranscript(QString studID, QString fullName, QString cumGpa) {
    int lineShift = 5;
    // query grades using student id
    QSqlDatabase transcriptConn = databaseConnection(QString("transcriptConn"));
    QString gradeQuery = QString("SELECT courses.crn, course_prefix, course_num, semester, year, hours, grade FROM courses INNER JOIN grades on courses.crn = grades.crn WHERE grades.student_id LIKE '%%1%'").arg(studID);
    QSqlQuery gradeData = executeQuery(transcriptConn, gradeQuery);

    // setup pdf writer
    QString filePath = QString("../transcripts/%1_transcript.pdf").arg(studID);
    QPdfWriter pdfWriter(filePath);
    pdfWriter.setPageSize(QPageSize(QPageSize::A4));
    pdfWriter.setResolution(75); // Set DPI

    // create painter for drawing
    QPainter painter(&pdfWriter);
    // create a bold pen for drawing lines
    QPen boldPen(Qt::black);
    boldPen.setWidth(2);

    // Set fonts
    QFont titleFont("Helvetica", 18, QFont::Bold);
    QFont normalFont("Helvetica", 12);

    // Write to PDF
    painter.setFont(titleFont);
    painter.drawText(25, 100, "Student Transcript");
    painter.setFont(normalFont);
    painter.drawText(25, 130, QString("Name: %1").arg(fullName));
    painter.drawText(25, 150, QString("Student ID: %1").arg(studID));
    
    // Add space
    painter.drawText(25, 160, "");
    painter.drawText(25, 180, QString("GPA: %1").arg(cumGpa));

    // Table headers
    int tableLeft = 25;
    int tableTop = 230;
    int rowHeight = 10;
    int columnWidth[] = {100, 100, 100, 80, 80, 80};
    QStringList headers = {"Course Prefix", "Course Number", "Semester", "Year", "Hours", "Grade"};
    for (int col = 0; col < headers.size(); col++) {
        int headerShift = 0;
        if (col >= 3) {
            headerShift = 60;
        }
        painter.drawText(tableLeft + headerShift + col * columnWidth[col], tableTop, headers[col]);
    }

    // Draw horizontal lines before & after the header
    painter.setPen(boldPen);
    painter.drawLine(tableLeft-lineShift, tableTop-rowHeight-lineShift, tableLeft + columnWidth[0] + columnWidth[1] + columnWidth[2] + columnWidth[3] + columnWidth[4] + columnWidth[5]-lineShift, tableTop-rowHeight-lineShift);
    painter.drawLine(tableLeft-lineShift, tableTop + rowHeight-lineShift, tableLeft + columnWidth[0] + columnWidth[1] + columnWidth[2] + columnWidth[3] + columnWidth[4] + columnWidth[5]-lineShift, tableTop + rowHeight - lineShift);
    
    // Table data
    int y = tableTop + rowHeight + 10;
    while (gradeData.next()) {
        QString coursePrefix = gradeData.value("course_prefix").toString();
        QString courseNum = gradeData.value("course_num").toString();
        QString semester = gradeData.value("semester").toString();
        int year = gradeData.value("year").toInt();
        int hours = gradeData.value("hours").toInt();
        QString grade = gradeData.value("grade").toString();

        // Write the row data
        painter.drawText(tableLeft, y, coursePrefix);
        painter.drawText(tableLeft + columnWidth[0], y, courseNum);
        painter.drawText(tableLeft + columnWidth[0] + columnWidth[1], y, semester);
        painter.drawText(tableLeft + columnWidth[0] + columnWidth[1] + columnWidth[2], y, QString::number(year));
        painter.drawText(tableLeft + columnWidth[0] + columnWidth[1] + columnWidth[2] + columnWidth[3], y, QString::number(hours));
        painter.drawText(tableLeft + columnWidth[0] + columnWidth[1] + columnWidth[2] + columnWidth[3] + columnWidth[4], y, grade);

        // draw line between row
        painter.setPen(QPen());
        painter.drawLine(tableLeft-lineShift, y+lineShift, tableLeft + columnWidth[0] + columnWidth[1] + columnWidth[2] + columnWidth[3] + columnWidth[4] + columnWidth[5]-lineShift, y+lineShift);
    
        // Move to the next row
        y += rowHeight+10;
    }

    // Draw bottom & vertical lines for the table
    painter.setPen(boldPen);
    painter.drawLine(tableLeft-lineShift, y+lineShift-(rowHeight+10), tableLeft + columnWidth[0] + columnWidth[1] + columnWidth[2] + columnWidth[3] + columnWidth[4] + columnWidth[5]-lineShift, y+lineShift-(rowHeight+10));
    for (int col = 0; col <= 6; col++) {
        int headerShift = 0;
        if (col > 3) {
            headerShift = 60;
        }
        int x = tableLeft + headerShift + col * (col == 0 ? 0 : columnWidth[col - 1]);
        painter.drawLine(x-lineShift, tableTop-rowHeight-lineShift, x-lineShift, y-(rowHeight+5));  // Vertical lines
    }

    // Add footer
    painter.drawText(25, y + 30, QString("Generated on: %1").arg(QDateTime::currentDateTime().toString("MM/dd/yyyy")));

    painter.end();
    qDebug() << "Transcript PDF created at: " << filePath;
}