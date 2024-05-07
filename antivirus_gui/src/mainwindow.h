#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QApplication>
#include <QProcess>
#include <QMessageBox>
#include <windows.h>
#include <QIcon>
#include <QDebug>
#include <QSystemTrayIcon>
#include <QCloseEvent>

#define APP_ICON ":/images/resources/antivirus.ico"
#define SERVICE_NAME "antivirus" // поменять!

enum ServiceStatus {
    SERVICE_NOT_RUNNING,
    SERVICE_IS_RUNNING,
    SERVICE_ERROR_OPEN,
    SERVICE_ERROR_GET_STATUS,
    SCM_ERROR
}; // to be continued...

int checkWorkingService(LPCSTR name_of_service);
void log(std::string);


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void changeEvent(QEvent*);
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void closeEvent(QCloseEvent *event);
    void setTrayIconActions();
    void showTrayIcon();
    void on_scanFileMenuButton_clicked();

    void on_scanFolderMenuButton_clicked();

    void on_directoryMonitoringMenuButton_clicked();

    void on_scheduleScanMenuButton_clicked();

    void on_deleteMaliciousFilesMenuButton_clicked();

    void on_quarantineMenuButton_clicked();

private:
    Ui::MainWindow *ui;
    QMenu *trayIconMenu;
    QAction *quitAction;
    QAction *showAction;
    QSystemTrayIcon *trayIcon;

    void hideVisibleWidgets(const QString &objectNameToKeepVisible);
};
#endif // MAINWINDOW_H
