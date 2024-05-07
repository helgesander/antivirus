#ifndef APP_H
#define APP_H

#include <QMainWindow>
#include <QAbstractButton>

QT_BEGIN_NAMESPACE
namespace Ui {
class App;
}
QT_END_NAMESPACE

class App : public QMainWindow
{
    Q_OBJECT

public:
    App(QWidget *parent = nullptr);
    ~App();

private slots:
    void on_buttonBox_accepted();

    void on_createDatabaseButton_clicked();

private:
    Ui::App *ui;
};
#endif // APP_H
