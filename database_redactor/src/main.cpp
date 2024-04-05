#include "app.h"

#include <QApplication>
#include <QIcon>
#include <QMainWindow>

void initApp(App* app) {
    QIcon icon("resources/antivirus.ico");
    app->setWindowIcon(icon);
    app->setWindowTitle("Database Redactor v1.0");
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    App w;
    initApp(&w);
    w.show();
    return a.exec();
}
