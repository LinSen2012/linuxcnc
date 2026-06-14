/*
 * linuxQtCncGui - LinuxCNC Qt5 GUI 控制器
 * ToolTableWidget - 刀具表编辑器实现
 */

#include "ToolTableWidget.h"

#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QInputDialog>
#include <QDebug>

// ============================================================================
// 构造 / 析构
// ============================================================================

ToolTableWidget::ToolTableWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

ToolTableWidget::~ToolTableWidget() = default;

// ============================================================================
// UI 初始化
// ============================================================================

void ToolTableWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    // ---- 工具栏 ----
    auto *toolbar = new QHBoxLayout();

    m_addBtn = new QPushButton(tr("添加刀具"), this);
    connect(m_addBtn, &QPushButton::clicked, this, &ToolTableWidget::addTool);
    toolbar->addWidget(m_addBtn);

    m_deleteBtn = new QPushButton(tr("删除刀具"), this);
    connect(m_deleteBtn, &QPushButton::clicked, this, &ToolTableWidget::deleteTool);
    toolbar->addWidget(m_deleteBtn);

    toolbar->addStretch();

    mainLayout->addLayout(toolbar);

    // ---- 刀具表格 ----
    createToolTable();
    mainLayout->addWidget(m_toolTable, 1);

    // ---- 刀具详情 ----
    m_detailsGroup = new QGroupBox(tr("刀具参数"), this);
    auto *detailsLayout = new QGridLayout(m_detailsGroup);

    int row = 0;
    detailsLayout->addWidget(new QLabel(tr("刀位号:"), this), row, 0);
    m_pocketSpin = new QSpinBox(this);
    m_pocketSpin->setRange(0, 999);
    detailsLayout->addWidget(m_pocketSpin, row++, 1);

    detailsLayout->addWidget(new QLabel(tr("刀具号:"), this), row, 0);
    m_toolNoSpin = new QSpinBox(this);
    m_toolNoSpin->setRange(0, 9999);
    detailsLayout->addWidget(m_toolNoSpin, row++, 1);

    detailsLayout->addWidget(new QLabel(tr("X 偏置:"), this), row, 0);
    m_xOffsetSpin = new QDoubleSpinBox(this);
    m_xOffsetSpin->setRange(-1000, 1000);
    m_xOffsetSpin->setDecimals(4);
    m_xOffsetSpin->setSuffix(" mm");
    detailsLayout->addWidget(m_xOffsetSpin, row++, 1);

    detailsLayout->addWidget(new QLabel(tr("Y 偏置:"), this), row, 0);
    m_yOffsetSpin = new QDoubleSpinBox(this);
    m_yOffsetSpin->setRange(-1000, 1000);
    m_yOffsetSpin->setDecimals(4);
    m_yOffsetSpin->setSuffix(" mm");
    detailsLayout->addWidget(m_yOffsetSpin, row++, 1);

    detailsLayout->addWidget(new QLabel(tr("Z 偏置:"), this), row, 0);
    m_zOffsetSpin = new QDoubleSpinBox(this);
    m_zOffsetSpin->setRange(-1000, 1000);
    m_zOffsetSpin->setDecimals(4);
    m_zOffsetSpin->setSuffix(" mm");
    detailsLayout->addWidget(m_zOffsetSpin, row++, 1);

    detailsLayout->addWidget(new QLabel(tr("直径:"), this), row, 0);
    m_diameterSpin = new QDoubleSpinBox(this);
    m_diameterSpin->setRange(0, 1000);
    m_diameterSpin->setDecimals(4);
    m_diameterSpin->setSuffix(" mm");
    detailsLayout->addWidget(m_diameterSpin, row++, 1);

    detailsLayout->addWidget(new QLabel(tr("前角:"), this), row, 0);
    m_frontAngleSpin = new QDoubleSpinBox(this);
    m_frontAngleSpin->setRange(-180, 180);
    m_frontAngleSpin->setDecimals(2);
    m_frontAngleSpin->setSuffix("°");
    detailsLayout->addWidget(m_frontAngleSpin, row++, 1);

    detailsLayout->addWidget(new QLabel(tr("后角:"), this), row, 0);
    m_backAngleSpin = new QDoubleSpinBox(this);
    m_backAngleSpin->setRange(-180, 180);
    m_backAngleSpin->setDecimals(2);
    m_backAngleSpin->setSuffix("°");
    detailsLayout->addWidget(m_backAngleSpin, row++, 1);

    detailsLayout->addWidget(new QLabel(tr("方向:"), this), row, 0);
    m_orientationCombo = new QComboBox(this);
    m_orientationCombo->addItems({"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"});
    detailsLayout->addWidget(m_orientationCombo, row++, 1);

    m_applyBtn = new QPushButton(tr("应用修改"), this);
    connect(m_applyBtn, &QPushButton::clicked, this, &ToolTableWidget::applyChanges);
    detailsLayout->addWidget(m_applyBtn, row, 0, 1, 2);

    mainLayout->addWidget(m_detailsGroup);

    // 初始化示例数据
    refresh();
}

void ToolTableWidget::createToolTable()
{
    m_toolTable = new QTableWidget(this);
    m_toolTable->setColumnCount(10);
    m_toolTable->setHorizontalHeaderLabels({
        tr("刀位"), tr("刀具号"), tr("X 偏置"), tr("Y 偏置"),
        tr("Z 偏置"), tr("直径"), tr("前角"), tr("后角"),
        tr("方向"), tr("备注")
    });

    m_toolTable->setAlternatingRowColors(true);
    m_toolTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_toolTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_toolTable->setSortingEnabled(true);

    // 列宽
    m_toolTable->setColumnWidth(0, 60);
    m_toolTable->setColumnWidth(1, 60);
    m_toolTable->horizontalHeader()->setStretchLastSection(true);

    connect(m_toolTable, &QTableWidget::itemSelectionChanged,
            this, &ToolTableWidget::onSelectionChanged);
    connect(m_toolTable, &QTableWidget::cellChanged,
            this, &ToolTableWidget::onCellChanged);
}

// ============================================================================
// 数据操作
// ============================================================================

bool ToolTableWidget::loadFromFile(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream in(&file);
    int lineNum = 0;
    m_tools.clear();

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        ++lineNum;

        // 跳过空行和注释
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        // 解析行格式: Pocket T  X    Y    Z    A    B    O  Comm
        // 注意: 实际格式可能因配置而异
        QStringList parts = line.split(QRegExp("\\s+"));
        if (parts.size() >= 6) {
            ToolData tool;
            tool.pocketNo = parts[0].toInt();
            tool.toolNo = parts[1].toInt();
            tool.xOffset = parts[2].toDouble();
            tool.yOffset = parts[3].toDouble();
            tool.zOffset = parts[4].toDouble();
            tool.diameter = parts[5].toDouble();
            if (parts.size() >= 7) tool.frontAngle = parts[6].toDouble();
            if (parts.size() >= 8) tool.backAngle = parts[7].toDouble();
            if (parts.size() >= 9) tool.orientation = parts[8].toInt();
            m_tools.append(tool);
        }
    }

    file.close();
    refresh();
    return true;
}

bool ToolTableWidget::saveToFile(const QString &filename) const
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out << "# LinuxCNC Tool Table\n";
    out << "# Pocket  T     X        Y        Z        Diameter  Front  Back  Orient\n";

    for (const auto &tool : m_tools) {
        out << QString::number(tool.pocketNo).rightJustified(6)
            << QString::number(tool.toolNo).rightJustified(6)
            << QString::number(tool.xOffset, 'f', 4).rightJustified(10)
            << QString::number(tool.yOffset, 'f', 4).rightJustified(10)
            << QString::number(tool.zOffset, 'f', 4).rightJustified(10)
            << QString::number(tool.diameter, 'f', 4).rightJustified(10)
            << QString::number(tool.frontAngle, 'f', 2).rightJustified(7)
            << QString::number(tool.backAngle, 'f', 2).rightJustified(7)
            << QString::number(tool.orientation).rightJustified(7)
            << "\n";
    }

    file.close();
    return true;
}

ToolData ToolTableWidget::toolAt(int pocket) const
{
    for (const auto &tool : m_tools) {
        if (tool.pocketNo == pocket) {
            return tool;
        }
    }
    return ToolData();
}

void ToolTableWidget::setToolAt(int pocket, const ToolData &data)
{
    for (auto &tool : m_tools) {
        if (tool.pocketNo == pocket) {
            tool = data;
            m_modified = true;
            return;
        }
    }
    m_tools.append(data);
    m_modified = true;
}

int ToolTableWidget::currentTool() const
{
    int row = m_toolTable->currentRow();
    if (row >= 0 && row < m_tools.size()) {
        return m_tools[row].toolNo;
    }
    return 0;
}

void ToolTableWidget::clear()
{
    m_tools.clear();
    m_toolTable->setRowCount(0);
    m_modified = false;
}

void ToolTableWidget::refresh()
{
    m_toolTable->setRowCount(m_tools.size());

    for (int i = 0; i < m_tools.size(); ++i) {
        const auto &tool = m_tools[i];
        m_toolTable->setItem(i, 0, new QTableWidgetItem(QString::number(tool.pocketNo)));
        m_toolTable->setItem(i, 1, new QTableWidgetItem(QString::number(tool.toolNo)));
        m_toolTable->setItem(i, 2, new QTableWidgetItem(QString::number(tool.xOffset, 'f', 4)));
        m_toolTable->setItem(i, 3, new QTableWidgetItem(QString::number(tool.yOffset, 'f', 4)));
        m_toolTable->setItem(i, 4, new QTableWidgetItem(QString::number(tool.zOffset, 'f', 4)));
        m_toolTable->setItem(i, 5, new QTableWidgetItem(QString::number(tool.diameter, 'f', 4)));
        m_toolTable->setItem(i, 6, new QTableWidgetItem(QString::number(tool.frontAngle, 'f', 2)));
        m_toolTable->setItem(i, 7, new QTableWidgetItem(QString::number(tool.backAngle, 'f', 2)));
        m_toolTable->setItem(i, 8, new QTableWidgetItem(QString::number(tool.orientation)));
        m_toolTable->setItem(i, 9, new QTableWidgetItem());
    }
}

// ============================================================================
// 私有槽
// ============================================================================

void ToolTableWidget::onSelectionChanged()
{
    updateToolDetails();
}

void ToolTableWidget::onCellChanged(int row, int column)
{
    Q_UNUSED(row)
    Q_UNUSED(column)
    m_modified = true;
    emit dataModified();
}

void ToolTableWidget::applyChanges()
{
    int row = selectedRow();
    if (row < 0 || row >= m_tools.size()) return;

    auto &tool = m_tools[row];
    tool.pocketNo = m_pocketSpin->value();
    tool.toolNo = m_toolNoSpin->value();
    tool.xOffset = m_xOffsetSpin->value();
    tool.yOffset = m_yOffsetSpin->value();
    tool.zOffset = m_zOffsetSpin->value();
    tool.diameter = m_diameterSpin->value();
    tool.frontAngle = m_frontAngleSpin->value();
    tool.backAngle = m_backAngleSpin->value();
    tool.orientation = m_orientationCombo->currentText().toInt();

    m_modified = true;
    refresh();
    emit toolChanged(tool.toolNo);
    emit dataModified();
}

int ToolTableWidget::selectedRow() const
{
    QList<QTableWidgetItem *> selected = m_toolTable->selectedItems();
    if (selected.isEmpty()) return -1;
    return selected.first()->row();
}

void ToolTableWidget::updateToolDetails()
{
    int row = selectedRow();
    if (row < 0 || row >= m_tools.size()) {
        m_pocketSpin->setValue(0);
        m_toolNoSpin->setValue(0);
        m_xOffsetSpin->setValue(0);
        m_yOffsetSpin->setValue(0);
        m_zOffsetSpin->setValue(0);
        m_diameterSpin->setValue(0);
        m_frontAngleSpin->setValue(0);
        m_backAngleSpin->setValue(0);
        m_orientationCombo->setCurrentIndex(0);
        return;
    }

    const auto &tool = m_tools[row];
    m_pocketSpin->setValue(tool.pocketNo);
    m_toolNoSpin->setValue(tool.toolNo);
    m_xOffsetSpin->setValue(tool.xOffset);
    m_yOffsetSpin->setValue(tool.yOffset);
    m_zOffsetSpin->setValue(tool.zOffset);
    m_diameterSpin->setValue(tool.diameter);
    m_frontAngleSpin->setValue(tool.frontAngle);
    m_backAngleSpin->setValue(tool.backAngle);
    m_orientationCombo->setCurrentIndex(tool.orientation);
}

void ToolTableWidget::addTool()
{
    bool ok;
    int pocket = QInputDialog::getInt(this, tr("添加刀具"),
        tr("刀位号:"), 1, 1, 999, 1, &ok);
    if (!ok) return;

    ToolData tool;
    tool.pocketNo = pocket;
    tool.toolNo = pocket;  // 默认刀具号等于刀位号

    m_tools.append(tool);
    m_modified = true;
    refresh();

    // 选中新行
    for (int i = 0; i < m_toolTable->rowCount(); ++i) {
        QTableWidgetItem *item = m_toolTable->item(i, 0);
        if (item && item->text().toInt() == pocket) {
            m_toolTable->selectRow(i);
            break;
        }
    }

    emit dataModified();
}

void ToolTableWidget::deleteTool()
{
    int row = selectedRow();
    if (row < 0) return;

    int pocket = m_tools[row].pocketNo;
    auto reply = QMessageBox::question(this, tr("删除刀具"),
        tr("确定要删除刀位 %1 的刀具吗?").arg(pocket));
    if (reply != QMessageBox::Yes) return;

    m_tools.removeAt(row);
    m_modified = true;
    refresh();
    emit dataModified();
}