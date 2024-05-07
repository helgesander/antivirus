#include "app.h"

#include <QApplication>
#include <QIcon>
#include <QMainWindow>
#include <QDataStream>
#include <QMessageBox>
#include <QFile>

void initApp(App* app) {
    QIcon icon("resources/antivirus.ico");
    app->setWindowIcon(icon);
    app->setWindowTitle("Database Redactor v1.0");
    QFile db("database.bin");
    if (!db.exists()) {
        app->setFixedSize(311, 211);
    }
    if (!db.open(QIODevice::ReadWrite)) {
        qDebug() << "Could not open database";
        QMessageBox::warning(nullptr, "Problem", "Cannot open or create database");
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    App w;
    initApp(&w);
    w.show();
    return a.exec();
}
