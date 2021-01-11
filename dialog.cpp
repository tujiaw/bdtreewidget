#include "dialog.h"
#include "ui_dialog.h"

using namespace  bd;
Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);
    m_treeWidget = new TreeWidget(this);
    m_treeWidget->setCreateWidget([this](const ItemInfo &info) -> ItemWidget* {
        if (info.type == Parent) {
            return new ParentWidget(m_treeWidget);
        } else {
            return new ChildWidget(m_treeWidget);
        }
    });

    int contentIndex = 0;
    QList<ItemInfo> dataList;
    for (int i = 0; i < 100; i++) {
        QVariantMap vm;
        vm["name"] = QString("parent name:%1").arg(i + 1);
        std::string id = "parent_" + std::to_string(i + 1);
        ItemInfo data("", id, Parent, vm, 30);
        data.isExpand = true;

        for (int j = 0; j < 1000; j++) {
            QVariantMap subvm;
            subvm["image"] = "Image";
            subvm["head"] = QString("head%1").arg(j);
            subvm["content"] = QString("content%1").arg(++contentIndex);
            std::string subId = "child_" + std::to_string(j + 1);
            ItemInfo subData(id, subId, Child, subvm, 50);
            data.childList.push_back(subData);
        }

        dataList.push_back(data);
    }
    m_treeWidget->setEnableShrink(true);
    m_treeWidget->setData(dataList);
    m_treeWidget->setScrollBarStyle(QString("QScrollBar:vertical{background:transparent;width:6px;min-width:6px;max-width:6px;margin:0;}\
                                                QScrollBar::handle:vertical{background:%1;min-height:30px;margin:0px 0px 0px 0px;border-radius:2px;border:none;}").arg("#000"));
    ui->mainLayout->addWidget(m_treeWidget);
}

Dialog::~Dialog()
{
    delete ui;
}

