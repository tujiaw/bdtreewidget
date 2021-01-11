#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include "bdtreewidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Dialog; }
QT_END_NAMESPACE

class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = nullptr);
    ~Dialog();

private:
    Ui::Dialog *ui;
    bd::TreeWidget *m_treeWidget;
};
#endif // DIALOG_H
