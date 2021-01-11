#ifndef BD_TREEWIDGET_H
#define BD_TREEWIDGET_H

#include "bdtreeitem.h"

class QLabel;
class QScrollBar;

namespace bd
{
    class TreeWidget : public QWidget
    {
        Q_OBJECT
    public:
        typedef std::function<ItemWidget*(const ItemInfo &info)> CreateWidgetFunc;

        TreeWidget(QWidget *parent = 0);
        void setCreateWidget(const  CreateWidgetFunc &createWidget);

        void setData(const QList<ItemInfo> &data);
        const QList<ItemInfo>& data();
        std::string lastSelectId();
        ItemInfo getItem(const std::string &id);

        void insertChild(const QList<ItemInfo> &infos);
        QPair<std::string, std::string> del(const std::string& id);
        void mod(const ItemInfo &newInfo);
        void modShowData(const std::string &id, const QString &key, const QVariant &value);
        void append(const ItemInfo &info);
        void appendAndRefresh(const ItemInfo &info);

        void gotoSelected(const std::string& id = "");
        void moreSelected(const std::string &id, const Qt::MouseButton &btn = Qt::NoButton);
        void selectNext();
        void selectPrev();
        void pageDown();
        void pageUp();

        void refreshWidgets();
        void refreshWidget(const ItemInfo &info);
        void setScrollBarStyle(const QString &style);
        void setEnableShrink(bool enable);
        void setCurrentParentSelect();
        std::vector<std::string> getAllSelectId() const;

        void scrollToTop();
        void scrollToBottom();
        bool isScrollBottom() const;

    signals:
        void sigSelectChanged();
        void sigWillItemMousePress(ItemInfo &info);
        void sigItemMousePress(const ItemInfo &info);
        void sigItemMouseDoubleClick(const ItemInfo &info, const QString &from);
        void sigPopupRightButtonMenu(const ItemInfo &info);

    protected:
        void showEvent(QShowEvent *event);
        void resizeEvent(QResizeEvent *event);
        void wheelEvent(QWheelEvent *event);

    private slots:
        void slotValueChanged(int value);

    private:
        ItemWidget *getWidget(const ItemInfo &info);
        void updateItemInfo(const ItemInfo &newInfo);
        void foreachTree(bool isExpand, const std::function<bool(ItemInfo &info)> &cb);
        void foreachTree(NodeType type, const std::function<bool(ItemInfo &info)> &cb);
        void getCurrentInfo(int &startPos, int &contentHeight, bool &isContinue);
        void selectCurrentItems();

    private:
        CreateWidgetFunc m_createWidgetFunc;
        QList<ItemInfo> m_list;
        QList<ItemWidget*> m_widgets;
        QScrollBar *m_scrollbar;
        bool m_enableShrink;
        std::string m_lastSelectId;
        QList<std::string> m_selectList;
        QList<std::string> m_shiftSelectList;
    };
}
#endif // TREEWIDGET_H
