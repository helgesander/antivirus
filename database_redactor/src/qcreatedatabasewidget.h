#ifndef QCREATEDATABASEWIDGET_H
#define QCREATEDATABASEWIDGET_H

#include <QWidget>
#include <QDialogButtonBox>
#include

class QCreateDatabaseWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QCreateDatabaseWidget(QWidget *parent = nullptr);
private:
    QDialogButtonBox *buttonBox;
    QLabel *label;
signals:
};

#endif // QCREATEDATABASEWIDGET_H
