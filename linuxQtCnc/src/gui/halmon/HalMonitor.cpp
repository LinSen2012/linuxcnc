#include "HalMonitor.h"
#include "LcncCore.h"

#include <QMessageBox>

// ============================================================
// 构造 / 析构
// ============================================================

HalMonitor::HalMonitor(QWidget *parent)
    : QWidget(parent)
    , m_core(nullptr)
    , m_treeModel(nullptr)
    , m_refreshTimer(nullptr)
{
    setupUi();
    setupConnections();

    // 创建刷新定时器
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(REFRESH_INTERVAL_MS);
    // 连接 LcncCore 后再启动定时器
}

HalMonitor::~HalMonitor()
{
    if (m_refreshTimer) {
        m_refreshTimer->stop();
    }
}

// ============================================================
// 公共接口
// ============================================================

void HalMonitor::setCore(LcncCore *core)
{
    m_core = core;
    if (m_core) {
        // 首次刷新数据
        refreshData();
        // 启动定时刷新
        m_refreshTimer->start();
    }
}

void HalMonitor::refreshData()
{
    if (!m_core) return;

    // 通过 LcncCore 获取 HAL 数据
    // 注意：此处调用 LcncCore 提供的 HAL 数据接口
    // 实际实现取决于 LcncCore 的 API
    QVector<QMap<QString, QVector<HalPinInfo>>> components;

    // TODO: 从 LcncCore 获取实际 HAL 数据
    // 当 LcncCore 可用时，取消注释以下代码：
    // components = m_core->halComponents();

    // 更新树形模型
    m_treeModel->updateData(components);

    // 更新 Watch 列表值
    updateWatchValues();
}

void HalMonitor::addToWatch(const QString &pinName)
{
    if (pinName.isEmpty() || m_watchPins.contains(pinName)) {
        return;
    }

    // 初始值为 0
    m_watchPins[pinName] = 0.0;

    // 添加到 Watch 表格
    int row = m_watchTable->rowCount();
    m_watchTable->insertRow(row);
    m_watchTable->setItem(row, 0, new QTableWidgetItem(pinName));
    m_watchTable->setItem(row, 1, new QTableWidgetItem("0.0000"));

    // 从核心获取初始值
    if (m_core) {
        // TODO: m_core->getHalPinValue(pinName, &m_watchPins[pinName]);
    }
}

void HalMonitor::removeFromWatch(const QString &pinName)
{
    m_watchPins.remove(pinName);

    // 从表格中查找并移除
    for (int row = 0; row < m_watchTable->rowCount(); ++row) {
        QTableWidgetItem *item = m_watchTable->item(row, 0);
        if (item && item->text() == pinName) {
            m_watchTable->removeRow(row);
            break;
        }
    }
}

// ============================================================
// 私有槽函数
// ============================================================

void HalMonitor::onSearchTextChanged(const QString &text)
{
    m_treeModel->setFilter(text);
}

void HalMonitor::onTreeSelectionChanged()
{
    QModelIndexList selected = m_treeView->selectionModel()->selectedIndexes();
    if (selected.isEmpty()) {
        m_detailPanel->clear();
        return;
    }

    QModelIndex idx = selected.first();
    HalPinInfo info = m_treeModel->pinInfoAt(idx);

    if (info.name.isEmpty()) {
        // 选中了非叶子节点，显示基本信息
        HalTreeNode *node = static_cast<HalTreeNode*>(idx.internalPointer());
        if (node) {
            QString html = QString("<h3>%1</h3>").arg(node->name());
            if (node->nodeType() == HalTreeNode::Component) {
                html += "<p>HAL 组件</p>";
            } else if (node->nodeType() == HalTreeNode::Category) {
                html += QString("<p>子项数量: %1</p>").arg(node->childCount());
            }
            m_detailPanel->setHtml(html);
        }
    } else {
        updateDetailPanel(info);
    }
}

void HalMonitor::onTreeDoubleClicked(const QModelIndex &index)
{
    HalPinInfo info = m_treeModel->pinInfoAt(index);
    if (!info.writable) {
        return;
    }

    // 弹出编辑对话框
    bool ok = false;
    double newValue = QInputDialog::getDouble(
        this,
        QString("设置引脚值 - %1").arg(info.name),
        QString("当前值: %1\n新值:").arg(info.value),
        info.value,
        -1e9, 1e9, 6, &ok
    );

    if (ok && m_core) {
        // TODO: m_core->setHalPinValue(info.name, newValue);
        // 立即刷新
        refreshData();
    }
}

void HalMonitor::onAddToWatch()
{
    QModelIndexList selected = m_treeView->selectionModel()->selectedIndexes();
    if (selected.isEmpty()) {
        return;
    }

    QModelIndex idx = selected.first();
    HalPinInfo info = m_treeModel->pinInfoAt(idx);
    if (!info.name.isEmpty()) {
        addToWatch(info.name);
    }
}

void HalMonitor::onRemoveFromWatch()
{
    int row = m_watchTable->currentRow();
    if (row < 0) return;

    QTableWidgetItem *item = m_watchTable->item(row, 0);
    if (item) {
        removeFromWatch(item->text());
    }
}

// ============================================================
// 界面初始化
// ============================================================

void HalMonitor::setupUi()
{
    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    // ---- 顶部：搜索框 ----
    QHBoxLayout *searchLayout = new QHBoxLayout();
    QLabel *searchLabel = new QLabel("搜索:");
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("输入引脚名、类型或方向过滤...");
    searchLayout->addWidget(searchLabel);
    searchLayout->addWidget(m_searchEdit);
    mainLayout->addLayout(searchLayout);

    // ---- 中间：树形视图 + 详细信息 ----
    m_mainSplitter = new QSplitter(Qt::Horizontal);

    // 左侧：树形视图
    m_treeModel = new HalTreeModel(this);
    m_treeView = new QTreeView();
    m_treeView->setModel(m_treeModel);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setSortingEnabled(true);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_treeView->setUniformRowHeights(true);
    m_treeView->header()->setStretchLastSection(true);
    m_treeView->setExpandsOnDoubleClick(false);  // 双击用于编辑值

    m_mainSplitter->addWidget(m_treeView);

    // 右侧：详细信息面板
    m_detailPanel = new QTextBrowser();
    m_detailPanel->setOpenExternalLinks(false);
    m_detailPanel->setMinimumWidth(200);
    m_mainSplitter->addWidget(m_detailPanel);

    // 设置分割比例
    m_mainSplitter->setSizes(QList<int>{400, 250});

    mainLayout->addWidget(m_mainSplitter, 1);

    // ---- 底部：Watch 列表 ----
    m_bottomSplitter = new QSplitter(Qt::Vertical);

    // Watch 列表
    m_watchTable = new QTableWidget(0, 2);
    m_watchTable->setHorizontalHeaderLabels({"引脚名称", "当前值"});
    m_watchTable->horizontalHeader()->setStretchLastSection(true);
    m_watchTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_watchTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_watchTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_watchTable->setAlternatingRowColors(true);
    m_watchTable->setMaximumHeight(200);

    // Watch 按钮
    QWidget *watchBtnWidget = new QWidget();
    QHBoxLayout *watchBtnLayout = new QHBoxLayout(watchBtnWidget);
    watchBtnLayout->setContentsMargins(0, 2, 0, 2);
    m_addWatchBtn = new QPushButton("添加到 Watch");
    m_removeWatchBtn = new QPushButton("从 Watch 移除");
    watchBtnLayout->addWidget(m_addWatchBtn);
    watchBtnLayout->addWidget(m_removeWatchBtn);
    watchBtnLayout->addStretch();

    QVBoxLayout *watchLayout = new QVBoxLayout();
    watchLayout->setContentsMargins(0, 0, 0, 0);
    watchLayout->addWidget(new QLabel("Watch 列表:"));
    watchLayout->addWidget(m_watchTable);
    watchLayout->addWidget(watchBtnWidget);

    QWidget *watchWidget = new QWidget();
    watchWidget->setLayout(watchLayout);
    m_bottomSplitter->addWidget(watchWidget);

    m_bottomSplitter->setSizes(QList<int>{500, 200});

    mainLayout->addWidget(m_bottomSplitter, 0);

    // 设置窗口标题
    setWindowTitle("HAL 监视器");
    resize(800, 600);
}

void HalMonitor::setupConnections()
{
    // 搜索框
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &HalMonitor::onSearchTextChanged);

    // 树视图选择
    connect(m_treeView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &HalMonitor::onTreeSelectionChanged);

    // 树视图双击
    connect(m_treeView, &QTreeView::doubleClicked,
            this, &HalMonitor::onTreeDoubleClicked);

    // Watch 按钮
    connect(m_addWatchBtn, &QPushButton::clicked,
            this, &HalMonitor::onAddToWatch);
    connect(m_removeWatchBtn, &QPushButton::clicked,
            this, &HalMonitor::onRemoveFromWatch);

    // 刷新定时器
    if (m_refreshTimer) {
        connect(m_refreshTimer, &QTimer::timeout,
                this, &HalMonitor::refreshData);
    }
}

// ============================================================
// 辅助函数
// ============================================================

void HalMonitor::updateDetailPanel(const HalPinInfo &info)
{
    QString html;
    html += QString("<h3>%1</h3>").arg(info.name);
    html += "<table cellpadding='4' cellspacing='0' style='border:1px solid #555;'>";
    html += QString("<tr><td><b>类型:</b></td><td>%1</td></tr>").arg(info.type);
    html += QString("<tr><td><b>方向:</b></td><td>%1</td></tr>").arg(info.dir);

    // 格式化值显示
    QString valueStr;
    if (info.type == "BIT") {
        valueStr = info.value != 0 ? "TRUE" : "FALSE";
    } else if (info.type == "FLOAT") {
        valueStr = QString::number(info.value, 'f', 6);
    } else {
        valueStr = QString::number(info.value, 'f', 4);
    }
    html += QString("<tr><td><b>值:</b></td><td>%1</td></tr>").arg(valueStr);
    html += QString("<tr><td><b>可写:</b></td><td>%1</td></tr>")
                .arg(info.writable ? "是" : "否");
    html += "</table>";

    if (info.writable) {
        html += "<p><i>双击可编辑此引脚的值</i></p>";
    }

    m_detailPanel->setHtml(html);
}

void HalMonitor::updateWatchValues()
{
    if (!m_core) return;

    for (auto it = m_watchPins.begin(); it != m_watchPins.end(); ++it) {
        // TODO: 从 LcncCore 获取引脚值
        // m_core->getHalPinValue(it.key(), &it.value());

        // 在表格中更新显示
        for (int row = 0; row < m_watchTable->rowCount(); ++row) {
            QTableWidgetItem *nameItem = m_watchTable->item(row, 0);
            if (nameItem && nameItem->text() == it.key()) {
                QTableWidgetItem *valueItem = m_watchTable->item(row, 1);
                if (valueItem) {
                    valueItem->setText(QString::number(it.value(), 'f', 4));
                }
                break;
            }
        }
    }
}
