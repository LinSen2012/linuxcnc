#include "HalTreeModel.h"
#include <algorithm>

// ============================================================
// HalTreeNode 实现
// ============================================================

HalTreeNode::HalTreeNode(const QString &name, HalTreeNode *parent)
    : m_parent(parent)
    , m_name(name)
    , m_hasPin(false)
    , m_nodeType(Root)
{
}

HalTreeNode::~HalTreeNode()
{
    qDeleteAll(m_children);
}

void HalTreeNode::appendChild(HalTreeNode *child)
{
    m_children.append(child);
}

void HalTreeNode::removeChild(int row)
{
    if (row >= 0 && row < m_children.size()) {
        delete m_children.takeAt(row);
    }
}

HalTreeNode *HalTreeNode::child(int row) const
{
    if (row >= 0 && row < m_children.size()) {
        return m_children.at(row);
    }
    return nullptr;
}

int HalTreeNode::childCount() const
{
    return m_children.size();
}

int HalTreeNode::row() const
{
    if (m_parent) {
        return m_parent->m_children.indexOf(const_cast<HalTreeNode*>(this));
    }
    return 0;
}

HalTreeNode *HalTreeNode::parent() const
{
    return m_parent;
}

// ============================================================
// HalTreeModel 实现
// ============================================================

HalTreeModel::HalTreeModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_root(new HalTreeNode("HAL Components"))
{
}

HalTreeModel::~HalTreeModel()
{
    delete m_root;
}

// ============================================================
// QAbstractItemModel 接口
// ============================================================

QModelIndex HalTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    HalTreeNode *parentNode = parent.isValid()
        ? static_cast<HalTreeNode*>(parent.internalPointer())
        : m_root;

    HalTreeNode *childNode = parentNode->child(row);
    if (childNode) {
        return createIndex(row, column, childNode);
    }
    return QModelIndex();
}

QModelIndex HalTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }

    HalTreeNode *childNode = static_cast<HalTreeNode*>(index.internalPointer());
    HalTreeNode *parentNode = childNode->parent();

    if (parentNode == m_root || !parentNode) {
        return QModelIndex();
    }

    return createIndex(parentNode->row(), 0, parentNode);
}

int HalTreeModel::rowCount(const QModelIndex &parent) const
{
    HalTreeNode *parentNode = parent.isValid()
        ? static_cast<HalTreeNode*>(parent.internalPointer())
        : m_root;

    return parentNode->childCount();
}

int HalTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 4;  // 名称、类型、方向、值
}

QVariant HalTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    HalTreeNode *node = static_cast<HalTreeNode*>(index.internalPointer());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0: // 名称
            return node->name();
        case 1: // 类型
            if (node->hasPinInfo()) {
                return node->pinInfo.type;
            }
            return QVariant();
        case 2: // 方向
            if (node->hasPinInfo()) {
                return node->pinInfo.dir;
            }
            return QVariant();
        case 3: // 值
            if (node->hasPinInfo()) {
                if (node->pinInfo.type == "BIT") {
                    return node->pinInfo.value != 0 ? "TRUE" : "FALSE";
                } else if (node->pinInfo.type == "STRING") {
                    return QString::number(node->pinInfo.value, 'f', 3);
                }
                return QString::number(node->pinInfo.value, 'f', 4);
            }
            return QVariant();
        default:
            return QVariant();
        }
    }

    if (role == Qt::ForegroundRole) {
        // 根据类型设置颜色
        if (node->hasPinInfo()) {
            const HalPinInfo &info = node->pinInfo;
            if (info.dir == "IN") {
                return QVariant(QColor(100, 180, 255));   // 蓝色 - 输入
            } else if (info.dir == "OUT") {
                return QVariant(QColor(100, 255, 100));   // 绿色 - 输出
            } else if (info.dir == "IO") {
                return QVariant(QColor(255, 200, 100));   // 黄色 - 双向
            }
        }
        // 组件节点用白色
        if (node->nodeType() == HalTreeNode::Component) {
            return QVariant(QColor(220, 220, 255));
        }
        // 分类节点用灰色
        if (node->nodeType() == HalTreeNode::Category) {
            return QVariant(QColor(180, 180, 180));
        }
        return QVariant();
    }

    if (role == Qt::FontRole) {
        // 组件节点加粗
        if (node->nodeType() == HalTreeNode::Component) {
            QFont boldFont;
            boldFont.setBold(true);
            return boldFont;
        }
        return QVariant();
    }

    // 自定义角色
    if (role == PinNameRole && node->hasPinInfo()) {
        return node->pinInfo.name;
    }
    if (role == PinTypeRole && node->hasPinInfo()) {
        return node->pinInfo.type;
    }
    if (role == PinDirRole && node->hasPinInfo()) {
        return node->pinInfo.dir;
    }
    if (role == PinValueRole && node->hasPinInfo()) {
        return node->pinInfo.value;
    }
    if (role == PinWritableRole && node->hasPinInfo()) {
        return node->pinInfo.writable;
    }

    return QVariant();
}

QVariant HalTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case 0: return QString("名称");
        case 1: return QString("类型");
        case 2: return QString("方向");
        case 3: return QString("值");
        default: return QVariant();
        }
    }
    return QVariant();
}

Qt::ItemFlags HalTreeModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);

    if (index.isValid()) {
        HalTreeNode *node = static_cast<HalTreeNode*>(index.internalPointer());
        // 可写的引脚允许编辑
        if (node->hasPinInfo() && node->pinInfo.writable && index.column() == 3) {
            return defaultFlags | Qt::ItemIsEditable;
        }
    }

    return defaultFlags;
}

// ============================================================
// 数据操作
// ============================================================

void HalTreeModel::setFilter(const QString &filter)
{
    m_filter = filter;
    // 简单实现：发出 layoutChanged 信号触发视图刷新
    // 完整实现应使用 QSortFilterProxyModel
    emit layoutChanged();
}

void HalTreeModel::updateData(const QVector<QMap<QString, QVector<HalPinInfo>>> &components)
{
    beginResetModel();

    // 清除旧数据
    clearChildren(m_root);

    // 构建新的树形结构
    for (const auto &compData : components) {
        // 获取组件名
        if (compData.isEmpty()) continue;

        QString compName;
        for (auto it = compData.constBegin(); it != compData.constEnd(); ++it) {
            if (!it.value().isEmpty()) {
                // 从引脚名提取组件名
                QString fullName = it.value().first().name;
                int dotIdx = fullName.indexOf('.');
                compName = (dotIdx > 0) ? fullName.left(dotIdx) : fullName;
                break;
            }
        }

        if (compName.isEmpty()) continue;

        // 创建组件节点
        HalTreeNode *compNode = new HalTreeNode(compName, m_root);
        compNode->setNodeType(HalTreeNode::Component);
        m_root->appendChild(compNode);

        // 创建分类节点
        HalTreeNode *pinsNode = nullptr;
        HalTreeNode *signalsNode = nullptr;
        HalTreeNode *paramsNode = nullptr;

        if (compData.contains("pins")) {
            pinsNode = new HalTreeNode("pins", compNode);
            pinsNode->setNodeType(HalTreeNode::Category);
            compNode->appendChild(pinsNode);

            for (const auto &pin : compData.value("pins")) {
                HalTreeNode *pinNode = new HalTreeNode(pin.name, pinsNode);
                pinNode->setNodeType(HalTreeNode::Pin);
                pinNode->setPinInfo(pin);
                pinsNode->appendChild(pinNode);
            }
        }

        if (compData.contains("signals")) {
            signalsNode = new HalTreeNode("signals", compNode);
            signalsNode->setNodeType(HalTreeNode::Category);
            compNode->appendChild(signalsNode);

            for (const auto &sig : compData.value("signals")) {
                HalTreeNode *sigNode = new HalTreeNode(sig.name, signalsNode);
                sigNode->setNodeType(HalTreeNode::Pin);
                sigNode->setPinInfo(sig);
                signalsNode->appendChild(sigNode);
            }
        }

        if (compData.contains("params")) {
            paramsNode = new HalTreeNode("params", compNode);
            paramsNode->setNodeType(HalTreeNode::Category);
            compNode->appendChild(paramsNode);

            for (const auto &param : compData.value("params")) {
                HalTreeNode *paramNode = new HalTreeNode(param.name, paramsNode);
                paramNode->setNodeType(HalTreeNode::Pin);
                paramNode->setPinInfo(param);
                paramsNode->appendChild(paramNode);
            }
        }
    }

    endResetModel();
}

HalPinInfo HalTreeModel::pinInfoAt(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return HalPinInfo();
    }

    HalTreeNode *node = static_cast<HalTreeNode*>(index.internalPointer());
    if (node && node->hasPinInfo()) {
        return node->pinInfo;
    }
    return HalPinInfo();
}

// ============================================================
// 辅助函数
// ============================================================

bool HalTreeModel::matchesFilter(HalTreeNode *node, const QString &filter) const
{
    if (filter.isEmpty()) return true;
    if (node->name().contains(filter, Qt::CaseInsensitive)) return true;
    if (node->hasPinInfo()) {
        if (node->pinInfo.type.contains(filter, Qt::CaseInsensitive)) return true;
        if (node->pinInfo.dir.contains(filter, Qt::CaseInsensitive)) return true;
    }
    return false;
}

void HalTreeModel::clearChildren(HalTreeNode *node)
{
    while (node->childCount() > 0) {
        node->removeChild(0);
    }
}
