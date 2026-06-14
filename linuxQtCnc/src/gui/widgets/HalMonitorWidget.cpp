/*
 * linuxQtCncGui - LinuxCNC Qt5 GUI 控制器
 * HalMonitorWidget - HAL 信号监视器实现
 */

#include "HalMonitorWidget.h"

#include <QHeaderView>
#include <QTimer>
#include <QSortFilterProxyModel>
#include <QMenu>
#include <QAction>
#include <QTableWidgetItem>

// ============================================================================
// 构造 / 析构
// ============================================================================

HalMonitorWidget::HalMonitorWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();

    // 启动刷新定时器
    QTimer *refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &HalMonitorWidget::onRefreshTimer);
    refreshTimer->start(m_refreshRate);
}

HalMonitorWidget::~HalMonitorWidget() = default;

// ============================================================================
// UI 初始化
// ============================================================================

void HalMonitorWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    // ---- 工具栏 ----
    auto *toolbar = new QHBoxLayout();

    m_filterEdit = new QLineEdit(this);
    m_filterEdit->setPlaceholderText(tr("搜索信号..."));
    connect(m_filterEdit, &QLineEdit::textChanged,
            this, &HalMonitorWidget::onFilterChanged);
    toolbar->addWidget(new QLabel(tr("过滤:"), this));
    toolbar->addWidget(m_filterEdit);

    m_typeFilter = new QComboBox(this);
    m_typeFilter->addItem(tr("全部"), QString());
    m_typeFilter->addItem(tr("输入"), QStringLiteral("in"));
    m_typeFilter->addItem(tr("输出"), QStringLiteral("out"));
    m_typeFilter->addItem(tr("双向"), QStringLiteral("io"));
    m_typeFilter->addItem(tr("参数"), QStringLiteral("param"));
    toolbar->addWidget(new QLabel(tr("类型:"), this));
    toolbar->addWidget(m_typeFilter);

    toolbar->addStretch();

    m_signalCountLabel = new QLabel(tr("信号数: 0"), this);
    toolbar->addWidget(m_signalCountLabel);

    mainLayout->addLayout(toolbar);

    // ---- 信号表格 ----
    createSignalTable();
    mainLayout->addWidget(m_signalTable);
}

void HalMonitorWidget::createSignalTable()
{
    m_signalTable = new QTableWidget(this);
    m_signalTable->setColumnCount(4);
    m_signalTable->setHorizontalHeaderLabels({
        tr("名称"), tr("类型"), tr("值"), tr("说明")
    });

    // 设置表格属性
    m_signalTable->setAlternatingRowColors(true);
    m_signalTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_signalTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_signalTable->setSortingEnabled(true);
    m_signalTable->verticalHeader()->setVisible(false);

    // 设置列宽
    m_signalTable->setColumnWidth(0, 200);  // 名称
    m_signalTable->setColumnWidth(1, 60);   // 类型
    m_signalTable->setColumnWidth(2, 100);  // 值
    m_signalTable->horizontalHeader()->setStretchLastSection(true);

    // 连接选择信号
    connect(m_signalTable, &QTableWidget::itemSelectionChanged,
            this, &HalMonitorWidget::onSelectionChanged);

    // 右键菜单
    m_signalTable->setContextMenuPolicy(Qt::CustomContextMenu);
}

// ============================================================================
// 公共方法
// ============================================================================

void HalMonitorWidget::refresh()
{
    // TODO: 从 HAL 读取真实数据
    // 暂时填充示例数据
    if (m_signalTable->rowCount() == 0) {
        m_signalTable->setRowCount(5);
        QStringList names = {"motion-command-handle", "motion-analog-in-00",
                           "joint-0-pos-fb", "spindle-vel-cmd", " coolant-flood"};
        QStringList types = {"out", "in", "in", "out", "out"};
        QStringList values = {"0.000", "0.123", "10.567", "1500.0", "0"};
        QStringList descs = {"运动命令句柄", "模拟输入0", "关节0位置反馈",
                           "主轴速度命令", "冷却液喷流"};

        for (int i = 0; i < 5; ++i) {
            m_signalTable->setItem(i, 0, new QTableWidgetItem(names[i]));
            m_signalTable->setItem(i, 1, new QTableWidgetItem(types[i]));
            m_signalTable->setItem(i, 2, new QTableWidgetItem(values[i]));
            m_signalTable->setItem(i, 3, new QTableWidgetItem(descs[i]));
        }
        m_signalCountLabel->setText(tr("信号数: %1").arg(5));
    }
}

void HalMonitorWidget::setFilter(const QString &filter)
{
    m_filterEdit->setText(filter);
}

QString HalMonitorWidget::selectedSignal() const
{
    int row = m_signalTable->currentRow();
    if (row >= 0) {
        QTableWidgetItem *item = m_signalTable->item(row, 0);
        if (item) {
            return item->text();
        }
    }
    return QString();
}

// ============================================================================
// 私有槽
// ============================================================================

void HalMonitorWidget::onFilterChanged(const QString &text)
{
    m_currentFilter = text;
    // 过滤逻辑
    for (int i = 0; i < m_signalTable->rowCount(); ++i) {
        bool visible = true;
        if (!text.isEmpty()) {
            QTableWidgetItem *nameItem = m_signalTable->item(i, 0);
            if (nameItem) {
                visible = nameItem->text().contains(text, Qt::CaseInsensitive);
            }
        }
        m_signalTable->setRowHidden(i, !visible);
    }
}

void HalMonitorWidget::onSelectionChanged()
{
    QString signal = selectedSignal();
    if (!signal.isEmpty()) {
        emit signalSelected(signal);
    }
}

void HalMonitorWidget::onRefreshTimer()
{
    // 定期刷新值（模拟数据更新）
    for (int i = 0; i < m_signalTable->rowCount(); ++i) {
        QTableWidgetItem *valueItem = m_signalTable->item(i, 2);
        if (valueItem) {
            bool ok;
            double val = valueItem->text().toDouble(&ok);
            if (ok) {
                // 添加小幅随机变化模拟真实数据
                double noise = (qrand() % 100 - 50) / 1000.0;
                valueItem->setText(QString::number(val + noise, 'f', 3));
            }
        }
    }
}