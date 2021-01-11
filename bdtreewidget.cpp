#include "bdtreewidget.h"
#include <QtWidgets>
#include <QVBoxLayout>
#include <QDebug>
#include <set>
#include <algorithm>

using namespace bd;
TreeWidget::TreeWidget(QWidget *parent)
    : QWidget(parent)
    , m_enableShrink(true)
{
    m_scrollbar = new QScrollBar(Qt::Vertical, this);
    m_scrollbar->setPageStep(50);
    m_scrollbar->setSingleStep(50);
    m_scrollbar->setMinimum(0);
    m_scrollbar->setMaximum(0);
    connect(m_scrollbar, &QScrollBar::valueChanged, this, &TreeWidget::slotValueChanged);
}

void TreeWidget::setCreateWidget(const CreateWidgetFunc &createWidget)
{
    m_createWidgetFunc = createWidget;
}

void TreeWidget::setData(const QList<ItemInfo> &data)
{
    m_list = data;
}

const QList<ItemInfo>& TreeWidget::data()
{
    return m_list;
}

std::string TreeWidget::lastSelectId()
{
    return m_lastSelectId;
}

ItemInfo TreeWidget::getItem(const std::string &id)
{
    ItemInfo result;
    foreachTree(false, [&](ItemInfo &info) -> bool {
        if (id == info.id) {
            result = info;
            return true;
        }
        return false;
        });
    return result;
}

void TreeWidget::insertChild(const QList<ItemInfo> &infos)
{
    for (int i = 0; i < infos.size(); i++) {
        bool isFind = false;
        foreachTree(Top, [&](ItemInfo &info) {
            if (info.id == infos[i].parentId) {
                info.childList.append(infos[i]);
                isFind = true;
            }
            return isFind;
            });
        if (!isFind) {
            foreachTree(Parent, [&](ItemInfo &info) {
                if (info.id == infos[i].parentId) {
                    info.childList.append(infos[i]);
                    isFind = true;
                }
                return isFind;
                });
        }
    }
}

QPair<std::string, std::string> TreeWidget::del(const std::string& id)
{
    std::string pre, next; // 前一个和后一个item的id
    auto getPreNextId = [&pre, &next](int index, const QList<ItemInfo> &infos) {
        if (index > 0) {
            pre = infos[index - 1].id;
        }
        if (index < infos.size() - 1) {
            next = infos[index + 1].id;
        }
    };

    for (int i = 0; i < m_list.size(); i++) {
        if (m_list[i].id == id) {
            getPreNextId(i, m_list);
            m_list.removeAt(i);
            return qMakePair(pre, next);
        }
        QList<ItemInfo> &parentList = m_list[i].childList;
        for (int j = 0; j < parentList.size(); j++) {
            if (parentList[j].id == id) {
                getPreNextId(j, parentList);
                parentList.removeAt(j);
                return qMakePair(pre, next);
            }
            QList<ItemInfo> &childList = parentList[j].childList;
            for (int k = 0; k < childList.size(); k++) {
                if (childList[k].id == id) {
                    getPreNextId(k, childList);
                    childList.removeAt(k);
                    return qMakePair(pre, next);
                }
            }
        }
    }
    return qMakePair(pre, next);
}

void TreeWidget::mod(const ItemInfo &newInfo)
{
    foreachTree(false, [&](ItemInfo &info) -> bool {
        if (info.id == newInfo.id) {
            info = newInfo;
            return true;
        }
        return false;
        });
}

void TreeWidget::modShowData(const std::string &id, const QString &key, const QVariant &value)
{
    if (id.empty() || key.isEmpty()) {
        return;
    }

    foreachTree(false, [&](ItemInfo &info) -> bool {
        if (info.id == id) {
            QVariantMap vm = info.showData.toMap();
            vm[key] = value;
            info.showData = vm;
            return true;
        }
        return false;
        });
}

void TreeWidget::append(const ItemInfo &info)
{
    m_list.append(info);
}

void TreeWidget::appendAndRefresh(const ItemInfo &info)
{
    auto moveItem = [this](const ItemInfo &item, int &y, int &startPos, bool &isContinue) {
        if (y >= m_scrollbar->value() && isContinue) {
            getWidget(item)->move(0, startPos);
            startPos += item.height;
            isContinue = startPos < this->height();
        }
        y += item.height;
    };

    // 获取先前的widget起始位置和内容高度以及是否继续创建widget
    int startPos, y;
    bool isContinue;
    getCurrentInfo(startPos, y, isContinue);

    moveItem(info, y, startPos, isContinue);
    if (info.childList.count() > 0 && info.isExpand) {
        for (int j = 0; j < info.childList.count(); j++) {
            const ItemInfo &parentItem = info.childList[j];
            moveItem(parentItem, y, startPos, isContinue);
            if (parentItem.childList.count() > 0 && parentItem.isExpand) {
                for (int k = 0; k < parentItem.childList.count(); k++) {
                    const ItemInfo &childItem = parentItem.childList[k];
                    moveItem(childItem, y, startPos, isContinue);
                }
            }
        }
    }

    if (y > startPos || startPos > this->height()) {
        m_scrollbar->setMaximum(y - this->height());
        m_scrollbar->show();
        m_scrollbar->raise();
    }

    m_list.append(info);
}

void TreeWidget::gotoSelected(const std::string& id)
{
    m_selectList.clear();
    m_shiftSelectList.clear();
    std::string oldChildId = m_lastSelectId;
    if (!id.empty()) {
        m_lastSelectId = id;
        m_selectList.push_back(id);
        if (oldChildId == m_lastSelectId) {
            return;
        }
    }

    if (m_lastSelectId.empty() || m_widgets.empty()) {
        if (oldChildId != m_lastSelectId) {
            emit sigSelectChanged();
        }
        return;
    }

    // 存在可见的widget
    bool isFind = false;
    for (int i = 0; i < m_widgets.size(); i++) {
        if (m_widgets[i]->isVisible() && m_widgets[i]->data().id == m_lastSelectId) {
            m_widgets[i]->setSelected(true);
            isFind = true;
        } else {
            m_widgets[i]->setSelected(false);
        }
    }

    if (!isFind) {
        // 保存当前Child的Top和Parent，用来修改isExpand值
        ItemInfo *topItem = nullptr, *parentItem = nullptr;
        foreachTree(false, [this, &isFind, &topItem, &parentItem](ItemInfo &info) -> bool {
            if (info.type == Top) {
                topItem = &info;
            } else if (info.type == Parent) {
                parentItem = &info;
            } else if (info.type == Child) {
                if (info.id == m_lastSelectId) {
                    isFind = true;
                    bool refresh = false;
                    if (topItem && !topItem->isExpand) {
                        topItem->isExpand = true;
                        refresh = true;
                    }
                    if (parentItem && !parentItem->isExpand) {
                        parentItem->isExpand = true;
                        refresh = true;
                    }
                    if (refresh) {
                        refreshWidgets();
                    }
                }
            }
            return isFind;
            });

        if (isFind) {
            int y = 0;
            // 滚动到可见区域
            foreachTree(true, [this, &y](ItemInfo &info) -> bool {
                if (info.type == Child && info.id == m_lastSelectId) {
                    m_scrollbar->setValue(y);
                    return true;
                }
                y += info.height;
                return false;
                });
        }
    }

    if (oldChildId != m_lastSelectId) {
        emit sigSelectChanged();
    }
}

void TreeWidget::moreSelected(const std::string &id, const Qt::MouseButton &btn)
{
    this->setFocus();
    auto getParentId = [this](const std::string &id) -> std::string {
        bd::ItemInfo item = this->getItem(id);
        return item.parentId;
    };

    const std::string curParentId = getParentId(id);
    if (curParentId.empty()) {
        return;
    }

    // 移除其他消息中选中的项
    auto removeSelectFromOtherMsg = [getParentId](const std::string &curParentId, QList<std::string> &selectList) {
        for (auto iter = selectList.begin(); iter != selectList.end();) {
            const std::string parentId = getParentId(*iter);
            if (parentId != curParentId) {
                iter = selectList.erase(iter);
            } else {
                ++iter;
            }
        }
    };

    std::vector<std::string> oldSelectList = getAllSelectId();
    removeSelectFromOtherMsg(curParentId, m_selectList);
    removeSelectFromOtherMsg(curParentId, m_shiftSelectList);

    bool isCtrlPressed = QGuiApplication::keyboardModifiers() & Qt::ControlModifier;
    bool isShiftPressed = QGuiApplication::keyboardModifiers() & Qt::ShiftModifier;
    if (btn == Qt::RightButton) {
        if (!m_shiftSelectList.contains(id) && !m_selectList.contains(id)) {
            m_shiftSelectList.clear();
            m_selectList.clear();
            m_selectList.push_back(id);
        }
    } else {
        if (isShiftPressed) {
            m_shiftSelectList.clear();
            std::string firstSelectId = m_selectList.size() > 0 ? m_selectList[m_selectList.size() - 1] : "";
            std::string lastSelectId = id;
            if (firstSelectId.empty() || firstSelectId == lastSelectId) {
                m_shiftSelectList.push_back(firstSelectId);
            } else {
                // shift多选逻辑，按shift之前最后一次的选中和shift之后的最后一次选中
                bd::ItemInfo parentItem = getItem(curParentId);
                int findCount = 0;
                for (int i = 0; i < parentItem.childList.size(); i++) {
                    if (firstSelectId == parentItem.childList[i].id) {
                        ++findCount;
                    } else if (lastSelectId == parentItem.childList[i].id) {
                        ++findCount;
                    }
                    if (findCount == 1 || findCount == 2) {
                        m_shiftSelectList.push_back(parentItem.childList[i].id);
                        if (findCount == 2) {
                            break;
                        }
                    }
                }
            }
        } else if (isCtrlPressed) {
            m_selectList.append(m_shiftSelectList);
            m_shiftSelectList.clear();
            if (m_selectList.contains(id)) {
                m_selectList.removeOne(id);
            } else {
                m_selectList.push_back(id);
            }
        } else {
            m_shiftSelectList.clear();
            m_selectList.clear();
            m_selectList.push_back(id);
        }
    }

    m_lastSelectId = id;
    selectCurrentItems();
    std::vector<std::string> newSelectList = getAllSelectId();
    if (oldSelectList != newSelectList) {
        emit sigSelectChanged();
    }
}

void TreeWidget::selectNext()
{
    if (m_lastSelectId.empty()) {
        return;
    }

    bool isFind = false;
    foreachTree(Parent, [this, &isFind](ItemInfo &info) -> bool {
        if (isFind) {
            gotoSelected(info.id);
            return true;
        }
        isFind = (info.id == m_lastSelectId);
        return false;
        });
}

void TreeWidget::selectPrev()
{
    if (m_lastSelectId.empty()) {
        return;
    }

    std::string prevId;
    foreachTree(Parent, [this, &prevId](ItemInfo &info) -> bool {
        if (info.id == m_lastSelectId) {
            if (!prevId.empty()) {
                gotoSelected(prevId);
            }
            return true;
        }
        prevId = info.id;
        return false;
        });
}

void TreeWidget::pageDown()
{
    ItemWidget *widget = nullptr;
    for (int i = 0; i < m_widgets.size(); i++) {
        if (m_widgets[i]->data().id == m_lastSelectId) {
            widget = m_widgets[i];
            break;
        }
    }

    int nextValue = m_scrollbar->value() + m_scrollbar->pageStep();
    nextValue = qMin(nextValue, m_scrollbar->maximum());
    m_scrollbar->setValue(nextValue);

    if (widget) {
        gotoSelected(widget->data().id);
    }
}

void TreeWidget::pageUp()
{
    ItemWidget *widget = nullptr;
    for (int i = 0; i < m_widgets.size(); i++) {
        if (m_widgets[i]->data().id == m_lastSelectId) {
            widget = m_widgets[i];
            break;
        }
    }

    int prevValue = m_scrollbar->value() - m_scrollbar->pageStep();
    prevValue = qMax(prevValue, 0);
    m_scrollbar->setValue(prevValue);

    if (widget) {
        gotoSelected(widget->data().id);
    }
}

void TreeWidget::refreshWidgets()
{
    for (int i = 0; i < m_widgets.size(); i++) {
        m_widgets[i]->resize(this->width(), m_widgets[i]->height());
        m_widgets[i]->hide();
    }

    auto moveItem = [this](const ItemInfo &item, int &y, int &startPos, bool &isContinue) {
        if (y >= m_scrollbar->value() && isContinue) {
            getWidget(item)->move(0, startPos);
            startPos += item.height;
            isContinue = startPos < this->height();
        }
        y += item.height;
    };

    int y = 0, startPos = 0;
    bool isContinue = true;
    for (int i = 0; i < m_list.count(); i++) {
        const ItemInfo &topItem = m_list[i];
        moveItem(topItem, y, startPos, isContinue);
        if (topItem.childList.count() > 0 && topItem.isExpand) {
            for (int j = 0; j < topItem.childList.count(); j++) {
                const ItemInfo &parentItem = topItem.childList[j];
                moveItem(parentItem, y, startPos, isContinue);
                if (parentItem.childList.count() > 0 && parentItem.isExpand) {
                    for (int k = 0; k < parentItem.childList.count(); k++) {
                        const ItemInfo &childItem = parentItem.childList[k];
                        moveItem(childItem, y, startPos, isContinue);
                    }
                }
            }
        }
    }

    //qDebug() << "y:" << y << ", startPos:" << startPos << ", height:" << this->height();
    m_scrollbar->move(this->width() - m_scrollbar->width(), 0);
    m_scrollbar->resize(m_scrollbar->width(), this->height());
    m_scrollbar->setPageStep(this->height());
    if (y > startPos || startPos > this->height()) {
        m_scrollbar->setMaximum(y - this->height());
        m_scrollbar->show();
        m_scrollbar->raise();
    } else {
        m_scrollbar->setMaximum(0);
        m_scrollbar->setValue(0);
        m_scrollbar->hide();
    }
    //qDebug() << "min:" << m_scrollbar->minimum() << ", max:" << m_scrollbar->maximum() << ", value:" << m_scrollbar->value();
}

void TreeWidget::refreshWidget(const ItemInfo &info)
{
    mod(info);
    for (int i = 0; i < m_widgets.size(); i++) {
        const ItemInfo &item = m_widgets[i]->data();
        if (info.type == item.type && info.parentId == item.parentId && info.id == item.id) {
            m_widgets[i]->setData(info);
            break;
        }
    }
}

void TreeWidget::setScrollBarStyle(const QString &style)
{
    m_scrollbar->setStyleSheet(style);
}

void TreeWidget::setEnableShrink(bool enable)
{
    m_enableShrink = enable;
}

void TreeWidget::setCurrentParentSelect()
{
    auto getParentItem = [this](const QList<std::string> &selectList) -> bd::ItemInfo {
        bd::ItemInfo parentItem;
        for (auto iter = selectList.begin(); iter != selectList.end(); ++iter) {
            bd::ItemInfo item = getItem(*iter);
            if (!item.parentId.empty() && item.type != Top) {
                parentItem = getItem(item.parentId);
                if (!parentItem.id.empty()) {
                    return parentItem;
                }
            }
        }
        return parentItem;
    };

    std::vector<std::string> oldSelectIds = getAllSelectId();
    // 根据当前选中的child来全选parent下的所有child
    bd::ItemInfo parentItem = getParentItem(m_selectList);
    if (parentItem.id.empty()) {
        parentItem = getParentItem(m_shiftSelectList);
    }
    if (!parentItem.id.empty()) {
        m_selectList.clear();
        m_shiftSelectList.clear();
        for (auto iter = parentItem.childList.begin(); iter != parentItem.childList.end(); ++iter) {
            m_selectList.push_back(iter->id);
        }
        selectCurrentItems();
    }
    std::vector<std::string> newSelectIds = getAllSelectId();
    if (oldSelectIds != newSelectIds) {
        emit sigSelectChanged();
    }
}

std::vector<std::string> TreeWidget::getAllSelectId() const
{
    std::set<std::string> result;
    for (auto iter = m_selectList.begin(); iter != m_selectList.end(); ++iter) {
        result.insert(*iter);
    }
    for (auto iter = m_shiftSelectList.begin(); iter != m_shiftSelectList.end(); ++iter) {
        result.insert(*iter);
    }
    std::vector<std::string> vec(result.begin(), result.end());
    std::sort(vec.begin(), vec.end());
    return vec;
}

void TreeWidget::scrollToTop()
{
    m_scrollbar->setValue(0);
}

void TreeWidget::scrollToBottom()
{
    if (!m_scrollbar->isVisible()) {
        return;
    }

    int maxVal = m_scrollbar->maximum();
    if (maxVal > 0) {
        m_scrollbar->setValue(maxVal);
    }
}

bool TreeWidget::isScrollBottom() const
{
    int maxVal = m_scrollbar->maximum();
    if (maxVal == 0) {
        return true;
    }

    int diff = 5;
    if (m_scrollbar->value() <= maxVal && m_scrollbar->value() > maxVal - diff) {
        return true;
    }
    return false;
}

void TreeWidget::showEvent(QShowEvent *event)
{
    refreshWidgets();
    QWidget::showEvent(event);
}

void TreeWidget::resizeEvent(QResizeEvent *event)
{
    refreshWidgets();
    QWidget::resizeEvent(event);
}

void TreeWidget::wheelEvent(QWheelEvent *event)
{
    int step = m_scrollbar->value() - event->delta();
    m_scrollbar->setValue(step);
    QWidget::wheelEvent(event);
}

void TreeWidget::slotValueChanged(int value)
{
    refreshWidgets();
}

ItemWidget *TreeWidget::getWidget(const ItemInfo &info)
{
    // 存在这种类型的widget，并且没有被使用（不可见）直接返回
    ItemWidget *widget = nullptr;
    for (int i = 0; i < m_widgets.size(); i++) {
        if (m_widgets[i]->data().type == info.type && !m_widgets[i]->isVisibleTo(this)) {
            widget = m_widgets[i];
            break;
        }
    }

    if (widget == nullptr) {
        Q_ASSERT(m_createWidgetFunc);
        widget = m_createWidgetFunc(info);
        Q_ASSERT(widget);
        connect(widget, &ItemWidget::sigMousePress, [this, widget](QMouseEvent *event) {
            emit sigWillItemMousePress(widget->mutableData());
            if (widget->data().type != Child) {
                if (m_enableShrink) {
                    ItemInfo newInfo = widget->data();
                    newInfo.isExpand = !newInfo.isExpand;
                    updateItemInfo(newInfo);
                    refreshWidgets();
                }
            } else {
                moreSelected(widget->data().id, event->button());
            }
            emit sigItemMousePress(widget->data());
        });
        connect(widget, &ItemWidget::sigMouseRelease, [this, widget](QMouseEvent *event) {
            if (event->button() == Qt::RightButton && widget->data().type == Child) {
                emit sigPopupRightButtonMenu(widget->data());
            }
        });
        connect(widget, &ItemWidget::sigMouseDoubleClick, [this, widget](const QString &name) {
            emit sigItemMouseDoubleClick(widget->data(), name);
        });
        m_widgets.append(widget);
    }

    widget->setData(info);
    // 如果是当前选中的widget
    if (widget->data().type == Child) {
        bool isSelected = m_selectList.contains(widget->data().id);
        if (!isSelected) {
            isSelected = m_shiftSelectList.contains(widget->data().id);
        }
        widget->setSelected(isSelected);
    }
    widget->resize(this->width(), widget->height());
    widget->show();
    widget->raise();
    return widget;
}

void TreeWidget::updateItemInfo(const ItemInfo &newInfo)
{
    foreachTree(false, [&](ItemInfo &info) -> bool {
        if (newInfo.id == info.id) {
            info = newInfo;
            return true;
        }
        return false;
        });
}

// 遍历tree，isExpand为true只遍历展开的，为false所有的节点都遍历
void TreeWidget::foreachTree(bool isExpand, const std::function<bool(ItemInfo &info)> &cb)
{
    for (int i = 0; i < m_list.size(); i++) {
        if (cb(m_list[i])) {
            return;
        }
        if (isExpand && !m_list[i].isExpand) {
            continue;
        }

        QList<ItemInfo> &parentList = m_list[i].childList;
        for (int j = 0; j < parentList.size(); j++) {
            if (cb(parentList[j])) {
                return;
            }
            if (isExpand && !parentList[j].isExpand) {
                continue;
            }

            QList<ItemInfo> &childList = parentList[j].childList;
            for (int k = 0; k < childList.size(); k++) {
                if (cb(childList[k])) {
                    return;
                }
            }
        }
    }
}

// 遍历指定类型的节点，提高效率
void TreeWidget::foreachTree(NodeType type, const std::function<bool(ItemInfo &info)> &cb)
{
    if (type == Top) {
        for (int i = 0; i < m_list.size(); i++) {
            if (cb(m_list[i])) {
                return;
            }
        }
    } else if (type == Parent) {
        for (int i = 0; i < m_list.size(); i++) {
            QList<ItemInfo> &parentList = m_list[i].childList;
            for (int j = 0; j < parentList.size(); j++) {
                if (cb(parentList[j])) {
                    return;
                }
            }
        }
    } else if (type == Child) {
        for (int i = 0; i < m_list.size(); i++) {
            QList<ItemInfo> &parentList = m_list[i].childList;
            for (int j = 0; j < parentList.size(); j++) {
                QList<ItemInfo> &childList = parentList[j].childList;
                for (int k = 0; k < childList.size(); k++) {
                    if (cb(childList[k])) {
                        return;
                    }
                }
            }
        }
    } else {
        foreachTree(false, cb);
    }
}

void TreeWidget::getCurrentInfo(int &startPos, int &contentHeight, bool &isContinue)
{
    auto getItem = [this](const ItemInfo &item, int &y, int &startPos, bool &isContinue) {
        if (y >= m_scrollbar->value() && isContinue) {
            startPos += item.height;
            isContinue = startPos < this->height();
        }
        y += item.height;
    };

    startPos = 0;
    contentHeight = 0;
    isContinue = true;
    for (int i = 0; i < m_list.count(); i++) {
        const ItemInfo &topItem = m_list[i];
        getItem(topItem, contentHeight, startPos, isContinue);
        if (topItem.childList.count() > 0 && topItem.isExpand) {
            for (int j = 0; j < topItem.childList.count(); j++) {
                const ItemInfo &parentItem = topItem.childList[j];
                getItem(parentItem, contentHeight, startPos, isContinue);
                if (parentItem.childList.count() > 0 && parentItem.isExpand) {
                    for (int k = 0; k < parentItem.childList.count(); k++) {
                        const ItemInfo &childItem = parentItem.childList[k];
                        getItem(childItem, contentHeight, startPos, isContinue);
                    }
                }
            }
        }
    }
}

void TreeWidget::selectCurrentItems()
{
    for (int i = 0; i < m_widgets.size(); i++) {
        if (m_widgets[i]->isVisible() && (m_selectList.contains(m_widgets[i]->data().id)
            || m_shiftSelectList.contains(m_widgets[i]->data().id))) {
            m_widgets[i]->setSelected(true);
        } else {
            m_widgets[i]->setSelected(false);
        }
    }
}