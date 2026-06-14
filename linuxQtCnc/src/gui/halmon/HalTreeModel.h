/*
 * linuxQtCncGui - LinuxCNC Qt6 GUI 控制器
 * HalTreeModel - HAL pin/signal 树模型
 *
 * 为 QTreeView 提供数据模型，展示 HAL 组件、pin 和 signal 的树形结构。
 * 支持：
 * - 按组件/信号分组显示
 * - pin 类型、方向、值的实时显示
 * - 信号与 pin 的连接关系
 */

#ifndef HALTREEMODEL_H
#define HALTREEMODEL_H

#include <QAbstractItemModel>
#include <QVariant>
#include <QStringList>
#include <QVector>

/**
 * @brief HAL pin 类型
 */
enum class HalPinType {
    Bit,       ///< 位（0/1）
    Float,     ///< 浮点数
    S32,       ///< 32 位有符号整数
    U32        ///< 32 位无符号整数
};

/**
 * @brief HAL pin 方向
 */
enum class HalPinDirection {
    In,        ///< 输入 pin
    Out,       ///< 输出 pin
    IO         ///< 双向 pin
};

/**
 * @brief HAL pin 数据
 */
struct HalPinData {
    QString name;         ///< pin 名称
    HalPinType type = HalPinType::Bit;  ///< pin 类型
    HalPinDirection direction = HalPinDirection::In; ///< pin 方向
    QVariant value;       ///< 当前值
    QString signalName;   ///< 连接的信号名（空=未连接）
    bool connected = false; ///< 是否已连接到信号
};

/**
 * @brief HAL 信号数据
 */
struct HalSignalData {
    QString name;         ///< 信号名称
    HalPinType type = HalPinType::Bit;  ///< 信号类型
    QVariant value;       ///< 当前值
    QStringList writers;  ///< 写入者 pin 列表
    QStringList readers;  ///< 读取者 pin 列表
};

/**
 * @brief HAL 组件数据
 */
struct HalComponentData {
    QString name;         ///< 组件名称
    QString type;         ///< 组件类型
    int pid = 0;          ///< 进程 ID
    QVector<HalPinData> pins;  ///< pin 列表
};

// ============================================================================
// 树节点
// ============================================================================

/**
 * @brief HalTreeNode - 树模型节点基类
 */
struct HalTreeNode
{
    enum class NodeType {
        Root,       ///< 根节点
        Component,  ///< 组件节点
        Pin,        ///< pin 节点
        Signal,     ///< 信号节点
        SignalPin   ///< 信号下的 pin 节点
    };

    explicit HalTreeNode(NodeType type, HalTreeNode *parent = nullptr)
        : nodeType(type), parentItem(parent) {}

    ~HalTreeNode()
    {
        qDeleteAll(childItems);
    }

    NodeType nodeType;
    QString displayName;
    HalTreeNode *parentItem = nullptr;
    QList<HalTreeNode *> childItems;

    // 附加数据（根据节点类型使用）
    HalComponentData componentData;
    HalPinData pinData;
    HalSignalData signalData;

    void appendChild(HalTreeNode *child)
    {
        childItems.append(child);
    }

    HalTreeNode *child(int row) const
    {
        if (row >= 0 && row < childItems.size()) {
            return childItems.at(row);
        }
        return nullptr;
    }

    int childCount() const { return childItems.size(); }
    int row() const
    {
        if (parentItem) {
            return parentItem->childItems.indexOf(const_cast<HalTreeNode *>(this));
        }
        return 0;
    }
};

// ============================================================================
// 树模型
// ============================================================================

/**
 * @brief HalTreeModel - HAL pin/signal 树形数据模型
 *
 * 为 QTreeView 提供数据，展示 HAL 系统的层次结构：
 * - 根节点
 *   - 组件节点
 *     - pin 节点
 *   - 信号节点
 *     - 连接的 pin 节点
 */
class HalTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit HalTreeModel(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~HalTreeModel() override;

    // ---- QAbstractItemModel 接口 ----

    QModelIndex index(int row, int column,
                     const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                         int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    // ---- 自定义方法 ----

    /**
     * @brief 更新 HAL 数据
     * @param components 组件列表
     * @param signals 信号列表
     */
    void updateData(const QVector<HalComponentData> &components,
                    const QVector<HalSignalData> &signals_);

    /**
     * @brief 清除所有数据
     */
    void clear();

    /**
     * @brief 获取指定索引的节点
     * @param index 模型索引
     * @return 节点指针（可能为 nullptr）
     */
    HalTreeNode *nodeFromIndex(const QModelIndex &index) const;

private:
    HalTreeNode *m_rootNode = nullptr;  ///< 根节点
};

#endif // HALTREEMODEL_H
