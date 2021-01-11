#include "bdtreeitem.h"
#include <QDateTime>
#include <QSizePolicy>

using namespace bd;
ItemWidget::ItemWidget(QWidget *parent)
    : QFrame(parent)
    , m_isSelected(false)

{
}

void ItemWidget::setData(const ItemInfo &info)
{
    m_info = info;
    this->setFixedHeight(info.height);
    updateData(info);
}

const ItemInfo& ItemWidget::data()
{
    return m_info;
}

ItemInfo& ItemWidget::mutableData()
{
    return m_info;
}

void ItemWidget::setSelected(bool isSelected)
{
    m_isSelected = isSelected;
    if (m_info.type == Child) {
        emit sigSelected(isSelected);
    }
}

bool ItemWidget::isSelected() const
{
    return m_isSelected;
}

void ItemWidget::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);
    emit sigMousePress(event);
}

void ItemWidget::mouseReleaseEvent(QMouseEvent *event)
{
    QWidget::mouseReleaseEvent(event);
    emit sigMouseRelease(event);
}

void ItemWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    QWidget::mouseDoubleClickEvent(event);
    emit sigMouseDoubleClick();
}

void ItemWidget::enterEvent(QEvent *e)
{
    QFrame::enterEvent(e);
    emit sigMouseEnter();
}

void ItemWidget::leaveEvent(QEvent *e)
{
    QFrame::enterEvent(e);
    emit sigMouseLeave();
}

void ItemWidget::updateData(const ItemInfo &info)
{
}

ParentWidget::ParentWidget(QWidget *parent)
    : ItemWidget(parent)
{
    m_label = new QLabel(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->addWidget(m_label);
    this->setStyleSheet("background:#f00;");
}

void ParentWidget::updateData(const ItemInfo &info)
{
    m_label->setText(info.showData.toMap()["name"].toString());
}

ChildWidget::ChildWidget(QWidget *parent)
    : ItemWidget(parent)
{
    m_image = new QLabel(this);
    m_head = new QLabel(this);
    m_content = new QLabel(this);
    m_image->setFixedSize(40, 40);

    QVBoxLayout *rLayout = new QVBoxLayout();
    rLayout->addWidget(m_head);
    rLayout->addWidget(m_content);

    QHBoxLayout *hLayout = new QHBoxLayout();
    hLayout->addWidget(m_image);
    hLayout->addLayout(rLayout, 1);

    this->setLayout(hLayout);
    this->setStyleSheet("background:grey");
}


bool ChildWidget::eventFilter(QObject *obj, QEvent *e)
{
    return false;
}

void ChildWidget::updateData(const ItemInfo &info)
{
    QVariantMap vm = info.showData.toMap();
    m_image->setText(vm["image"].toString());
    m_head->setText(vm["head"].toString());
    m_content->setText(vm["content"].toString());
}

void ChildWidget::updateStyle()
{
}

