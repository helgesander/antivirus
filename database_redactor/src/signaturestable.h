#ifndef SIGNATURESTABLE_H
#define SIGNATURESTABLE_H

#include <QWidget>

class SignaturesTable : public QWidget
{
    Q_OBJECT
public:
    explicit SignaturesTable(QWidget *parent = nullptr);
    // QVBoxLayout *layout = new QVBoxLayout();
    // tableWidget = new QTableWidget();
    // layout->addWidget(tableWidget);
    // this->setLayout(layout);

signals:
};

#endif // SIGNATURESTABLE_H
