#ifndef BD_TREEITEM_H
#define BD_TREEITEM_H

#include <QFrame>
#include <functional>
#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>

namespace bd
{
    // 节点类型
    enum NodeType {
        None,
        Top,
        Parent,
        Child
    };

    struct ItemInfo {
        // 控件展示内容
        QVariant showData;

        // 必须参数
        std::string parentId;           // 父节点ID
        std::string id;                 // 当前节点ID
        NodeType type;              // 节点类型
        bool isExpand;              // 是否展开（Child节点无效）
        int height;                 // 节点widget高度
        QList<ItemInfo> childList;  // 子节点列表（Child节点无效）

        ItemInfo() { }
        ItemInfo(const std::string& _parentId, const std::string& _id, NodeType type_, const QVariant &_showData, int _height)
            : parentId(_parentId), id(_id), type(type_), showData(_showData), height(_height) {
            isExpand = (type_ == Child);
        }
    };

    class ItemWidget : public QFrame
    {
        Q_OBJECT
    public:
        ItemWidget(QWidget *parent = 0);
        void setData(const ItemInfo &info);
        const ItemInfo& data();
        ItemInfo& mutableData();
        void setSelected(bool isSelected);
        bool isSelected() const;

    signals:
        void sigSelected(bool isSelected);
        void sigMousePress(QMouseEvent *event);
        void sigMouseRelease(QMouseEvent *event);
        void sigMouseDoubleClick(const QString &from = "");
        void sigError(const std::string& id, int errcode = 0);
        void sigMouseEnter();
        void sigMouseLeave();

    protected:
        void mousePressEvent(QMouseEvent *event);
        void mouseReleaseEvent(QMouseEvent *event);
        void mouseDoubleClickEvent(QMouseEvent *event);
        void enterEvent(QEvent *);
        void leaveEvent(QEvent *);

    private:
        virtual void updateData(const ItemInfo &info);

    protected:
        ItemInfo m_info;
        bool m_isSelected;
    };

    class ParentWidget : public ItemWidget
    {
        Q_OBJECT
    public:
        ParentWidget(QWidget *parent = 0);

    private:
        virtual void updateData(const ItemInfo &info);

    private:
        QLabel *m_label;
    };

    class ChildWidget : public ItemWidget
    {
        Q_OBJECT
    public:
        ChildWidget(QWidget *parent = 0);

    protected:
        bool eventFilter(QObject *, QEvent *);

    private:
        virtual void updateData(const ItemInfo &info);
        void updateStyle();

    private:
        QLabel *m_image;
        QLabel *m_head;
        QLabel *m_content;
    };
}
#endif // TREEITEM_H
