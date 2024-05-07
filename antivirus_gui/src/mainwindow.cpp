#include "mainwindow.h"
#include "src/ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this -> setTrayIconActions();
    this -> showTrayIcon();
    // костыли
    ui->scanFileWidget->setHidden(true);
    ui->scanFolderWidget->setHidden(true);
    ui->meowLabel->setHidden(true);
}

void MainWindow::hideVisibleWidgets(const QString &objectNameToKeepVisible) {
    const QObjectList &children = ui->mainWidget->children();
    for (QObject *child : children)
    {
        QWidget *widget = qobject_cast<QWidget*>(child);
        // qDebug() << widget->objectName();
        if (widget)
        {
            if (widget->isVisible() and widget->objectName() != objectNameToKeepVisible)
            {
                widget->hide();
            }
        }
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

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
    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::trayIconActivated);
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
    showAction = new QAction("Показать Касперский v2.0", this);
    connect (quitAction, &QAction::triggered, qApp, &QApplication::quit);
    connect(showAction, &QAction::triggered, qApp, [this]() { // спустя три жутких костыля...
        this->show();
    });
    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(showAction);
    trayIconMenu->addAction (quitAction);
}

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



void MainWindow::on_scanFileMenuButton_clicked()
{
    hideVisibleWidgets("scanFileWidget");
    ui->scanFileWidget->show();
}


void MainWindow::on_scanFolderMenuButton_clicked()
{
    hideVisibleWidgets("scanFolderWidget");
    ui->scanFolderWidget->show();
}


void MainWindow::on_directoryMonitoringMenuButton_clicked()
{
    hideVisibleWidgets("meowLabel");
    ui->meowLabel->show();
}


void MainWindow::on_scheduleScanMenuButton_clicked()
{
    hideVisibleWidgets("meowLabel");
    ui->meowLabel->show();
}


void MainWindow::on_deleteMaliciousFilesMenuButton_clicked()
{
    hideVisibleWidgets("meowLabel");
    ui->meowLabel->show();
}


void MainWindow::on_quarantineMenuButton_clicked()
{
    hideVisibleWidgets("meowLabel");
    ui->meowLabel->show();
}

