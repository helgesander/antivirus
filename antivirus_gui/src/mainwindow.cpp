#include "mainwindow.h"
#include "src/ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this -> setTrayIconActions();
    this -> showTrayIcon();
}

MainWindow::~MainWindow()
{
    delete ui;
}
/* Отвечает за переопределение крестика (он не завершает все приложение, иконка в системном трее остается)*/
void MainWindow::closeEvent(QCloseEvent *event) {
    if (this->isVisible()) {
        event->ignore();
        this->hide();
    }
}

void MainWindow::showTrayIcon() {
    trayIcon = new QSystemTrayIcon(this);
    QIcon trayImage(APP_ICON);
    if (trayImage.isNull()) qDebug() << "Не удалось загрузить иконку";
    trayIcon -> setIcon(trayImage);
    trayIcon -> setContextMenu(trayIconMenu);
    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
    trayIcon -> show();
}

void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason)
    {
    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::DoubleClick:
        this->show();
        break;
    default:
        break;
    }
}

void MainWindow::setTrayIconActions()
{
    quitAction = new QAction("Выход", this);
    connect (quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    trayIconMenu = new QMenu(this);
    trayIconMenu -> addAction (quitAction);
}

/*
* Отвечает за поведение приложения после действия "Свернуть" (если приложение сворачивается, то программа прячется)
*/
void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    if (event -> type() == QEvent::WindowStateChange)
    {
        if (isMinimized())
        {
            this -> hide();
        }
    }
}
