#include "mainwindow.h"
#include <QFile>


void loadStyle() {
    QFile styleF(":/qss/resources/style.css");
    styleF.open(QFile::ReadOnly);
    QString qssStr = styleF.readAll();
    qApp->setStyleSheet(qssStr);
}

int checkWorkingService(LPCSTR name_of_service) {
    SC_HANDLE scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (scm == nullptr)
        return SCM_ERROR;
    SC_HANDLE service = OpenServiceA(scm, name_of_service, SERVICE_QUERY_STATUS);
    if (service == nullptr) {
        CloseServiceHandle(scm);
        return SERVICE_ERROR_OPEN;
    }

    SERVICE_STATUS status;
    if (!QueryServiceStatus(service, &status)) {
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        return SERVICE_ERROR_GET_STATUS;
    }

    if (status.dwCurrentState == SERVICE_RUNNING)
    {
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        return SERVICE_IS_RUNNING;
    }
    else {
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        return SERVICE_NOT_RUNNING;
    }

}

void startAntivirusService() {
    QString command = "sc start " + QString::fromStdString(SERVICE_NAME); // не будет работать без прав админа???
    QProcess process;
    process.setProgram("cmd");
    process.setArguments({"/c", command});
    process.start();
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.startDetached();
    int exitCode;
    QString output;
    if (process.waitForFinished()) {
        exitCode = process.exitCode();
        output = QString(process.readAllStandardOutput());
    }
    if (!exitCode)
        QMessageBox::critical(nullptr, "Error", "Can't start service: " + output);
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    loadStyle();
    MainWindow w;
    w.setWindowTitle("Касперский v2.0");
    QIcon icon(APP_ICON);
    w.setWindowIcon(icon);
    return a.exec();
}
