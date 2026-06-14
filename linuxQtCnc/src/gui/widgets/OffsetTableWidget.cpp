/*
 * linuxQtCncGui - LinuxCNC Qt5 GUI 控制器
 * OffsetTableWidget - 坐标系偏置表实现
 */

#include "OffsetTableWidget.h"

#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QInputDialog>
#include <QDebug>
#include <QHeaderView>

// ============================================================================
// 构造 / 析构
// ============================================================================

OffsetTableWidget::OffsetTableWidget(QWidget *parent)
    : QWidget(parent)
{
    // 初始化 9 个坐标系 (G54 ~ G59.3)
    QStringList names = {"G54", "G55", "G56", "G57", "G58", "G59",
                        "G59.1", "G59.2", "G59.3"};
    QStringList descs = {
        tr("坐标系 1"), tr("坐标系 2"), tr("坐标系 3"),
        tr("坐标系 4"), tr("坐标系 5"), tr("坐标系 6"),
        tr("坐标系 7"), tr("坐标系 8"), tr("坐标系 9")
    };

    for (int i = 0; i < 9; ++i) {
        CoordinateSystem cs;
        cs.name = names[i];
        cs.description = descs[i];
        for (int j = 0; j < 6; ++j) {
            cs.offsets[j] = 0.0;
        }
        m_systems.append(cs);
    }

    setupUi();
}

OffsetTableWidget::~OffsetTableWidget() = default;

// ============================================================================
// UI 初始化
// ============================================================================

void OffsetTableWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    // ---- 工具栏 ----
    auto *toolbar = new QHBoxLayout();

    m_applyBtn = new QPushButton(tr("应用修改"), this);
    connect(m_applyBtn, &QPushButton::clicked, this, &OffsetTableWidget::applyChanges);
    toolbar->addWidget(m_applyBtn);

    m_resetBtn = new QPushButton(tr("复位"), this);
    connect(m_resetBtn, &QPushButton::clicked, this, &OffsetTableWidget::resetCurrent);
    toolbar->addWidget(m_resetBtn);

    m_copyBtn = new QPushButton(tr("复制到..."), this);
    connect(m_copyBtn, &QPushButton::clicked, this, [this]() {
        bool ok;
        int target = QInputDialog::getInt(this, tr("复制坐标系"),
            tr("目标坐标系 (0=G54, 1=G55, ...):"), 0, 0, 8, 1, &ok);
        if (ok) {
            copyToSystem(target);
        }
    });
    toolbar->addWidget(m_copyBtn);

    toolbar->addStretch();

    mainLayout->addLayout(toolbar);

    // ---- 偏置表格 ----
    createOffsetTable();
    mainLayout->addWidget(m_offsetTable, 1);

    // ---- 详情面板 ----
    m_detailsGroup = new QGroupBox(tr("偏置详情"), this);
    auto *detailsLayout = new QGridLayout(m_detailsGroup);

    QStringList axisNames = {"X", "Y", "Z", "A", "B", "C"};
    for (int i = 0; i < 6; ++i) {
        detailsLayout->addWidget(new QLabel(
            QStringLiteral("%1 轴:").arg(axisNames[i]), this), i / 2, (i % 2) * 2);

        QDoubleSpinBox *spin = new QDoubleSpinBox(this);
        spin->setRange(-10000, 10000);
        spin->setDecimals(4);
        spin->setSuffix(" mm");
        m_offsetSpins.append(spin);
        detailsLayout->addWidget(spin, i / 2, (i % 2) * 2 + 1);
    }

    mainLayout->addWidget(m_detailsGroup);

    // 初始显示
    refresh();
}

void OffsetTableWidget::createOffsetTable()
{
    m_offsetTable = new QTableWidget(this);
    m_offsetTable->setColumnCount(8);
    m_offsetTable->setHorizontalHeaderLabels({
        tr("坐标系"), tr("描述"), tr("X"), tr("Y"), tr("Z"),
        tr("A"), tr("B"), tr("C")
    });

    m_offsetTable->setAlternatingRowColors(true);
    m_offsetTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_offsetTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_offsetTable->setSortingEnabled(false);

    // 列宽
    m_offsetTable->setColumnWidth(0, 60);
    m_offsetTable->setColumnWidth(1, 100);
    m_offsetTable->horizontalHeader()->setStretchLastSection(true);

    connect(m_offsetTable, &QTableWidget::itemSelectionChanged,
            this, &OffsetTableWidget::onSelectionChanged);
    connect(m_offsetTable, &QTableWidget::cellChanged,
            this, &OffsetTableWidget::onCellChanged);
}

// ============================================================================
// 数据操作
// ============================================================================

bool OffsetTableWidget::loadFromIni(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream in(&file);
    bool inKoordsSection = false;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        if (line == "[KINS]" || line == "[EMCIO]" || line.startsWith("[") && !inKoordsSection) {
            continue;
        }

        if (line.startsWith("[AXIS_")) {
            continue;
        }

        // 查找坐标系相关配置
        if (line.contains("G54") || line.contains("COORDINATE")) {
            inKoordsSection = true;
        }

        if (line.contains('=')) {
            QString key = line.section('=', 0, 0).trimmed();
            double value = line.section('=', 1, 1).trimmed().toDouble();

            // 解析 G54 ~ G59.3 偏置
            for (int i = 0; i < m_systems.size(); ++i) {
                QString prefix = QString("G54.%1").arg(i);
                if (i == 0) prefix = "G54";

                if (key.startsWith(prefix)) {
                    if (key.contains("X")) m_systems[i].offsets[0] = value;
                    else if (key.contains("Y")) m_systems[i].offsets[1] = value;
                    else if (key.contains("Z")) m_systems[i].offsets[2] = value;
                    else if (key.contains("A")) m_systems[i].offsets[3] = value;
                    else if (key.contains("B")) m_systems[i].offsets[4] = value;
                    else if (key.contains("C")) m_systems[i].offsets[5] = value;
                }
            }
        }
    }

    file.close();
    refresh();
    return true;
}

bool OffsetTableWidget::saveToIni(const QString &filename) const
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out << "# LinuxCNC Coordinate System Offsets\n\n";

    for (int i = 0; i < m_systems.size(); ++i) {
        const auto &cs = m_systems[i];
        QString prefix = QString("G54.%1").arg(i);
        if (i == 0) prefix = "G54";

        out << QString("# %1 - %2\n").arg(cs.name).arg(cs.description);
        out << QString("%1_X = %.6f\n").arg(prefix).arg(cs.offsets[0]);
        out << QString("%1_Y = %.6f\n").arg(prefix).arg(cs.offsets[1]);
        out << QString("%1_Z = %.6f\n").arg(prefix).arg(cs.offsets[2]);
        out << QString("%1_A = %.6f\n").arg(prefix).arg(cs.offsets[3]);
        out << QString("%1_B = %.6f\n").arg(prefix).arg(cs.offsets[4]);
        out << QString("%1_C = %.6f\n").arg(prefix).arg(cs.offsets[5]);
        out << "\n";
    }

    file.close();
    return true;
}

CoordinateSystem OffsetTableWidget::coordinateSystem(int index) const
{
    if (index >= 0 && index < m_systems.size()) {
        return m_systems[index];
    }
    return CoordinateSystem();
}

void OffsetTableWidget::setCoordinateSystem(int index, const double offsets[6])
{
    if (index < 0 || index >= m_systems.size()) return;

    for (int i = 0; i < 6; ++i) {
        m_systems[index].offsets[i] = offsets[i];
    }
    refresh();
}

int OffsetTableWidget::currentSystem() const
{
    return m_currentRow;
}

void OffsetTableWidget::clear()
{
    for (auto &cs : m_systems) {
        for (int i = 0; i < 6; ++i) {
            cs.offsets[i] = 0.0;
        }
    }
    refresh();
}

void OffsetTableWidget::refresh()
{
    m_offsetTable->setRowCount(m_systems.size());

    for (int i = 0; i < m_systems.size(); ++i) {
        const auto &cs = m_systems[i];

        m_offsetTable->setItem(i, 0, new QTableWidgetItem(cs.name));
        m_offsetTable->setItem(i, 1, new QTableWidgetItem(cs.description));

        for (int j = 0; j < 6; ++j) {
            m_offsetTable->setItem(i, j + 2,
                new QTableWidgetItem(QString::number(cs.offsets[j], 'f', 4)));
        }
    }
}

// ============================================================================
// 私有槽
// ============================================================================

void OffsetTableWidget::onSelectionChanged()
{
    updateDetails();
}

void OffsetTableWidget::onCellChanged(int row, int column)
{
    if (column < 2) return;  // 只处理偏置列

    int axis = column - 2;
    if (row < 0 || row >= m_systems.size() || axis < 0 || axis >= 6) return;

    QTableWidgetItem *item = m_offsetTable->item(row, column);
    if (item) {
        bool ok;
        double value = item->text().toDouble(&ok);
        if (ok) {
            m_systems[row].offsets[axis] = value;
            emit offsetChanged(row, axis, value);
        }
    }
}

void OffsetTableWidget::applyChanges()
{
    // 从详情面板应用修改
    if (m_currentRow >= 0 && m_currentRow < m_systems.size()) {
        for (int i = 0; i < 6; ++i) {
            m_systems[m_currentRow].offsets[i] = m_offsetSpins[i]->value();
        }
        refresh();
        emit offsetChanged(m_currentRow, -1, 0.0);  // -1 表示全部改变
    }
}

void OffsetTableWidget::resetCurrent()
{
    if (m_currentRow < 0 || m_currentRow >= m_systems.size()) return;

    auto reply = QMessageBox::question(this, tr("复位坐标系"),
        tr("确定要复位 %1 坐标系吗?").arg(m_systems[m_currentRow].name));
    if (reply != QMessageBox::Yes) return;

    for (int i = 0; i < 6; ++i) {
        m_systems[m_currentRow].offsets[i] = 0.0;
    }
    refresh();
    emit offsetChanged(m_currentRow, -1, 0.0);
}

void OffsetTableWidget::copyToSystem(int target)
{
    if (m_currentRow < 0 || m_currentRow >= m_systems.size()) return;
    if (target < 0 || target >= m_systems.size()) return;
    if (target == m_currentRow) return;

    auto reply = QMessageBox::question(this, tr("复制坐标系"),
        tr("确定要从 %1 复制到 %2 吗?")
            .arg(m_systems[m_currentRow].name)
            .arg(m_systems[target].name));
    if (reply != QMessageBox::Yes) return;

    for (int i = 0; i < 6; ++i) {
        m_systems[target].offsets[i] = m_systems[m_currentRow].offsets[i];
    }
    refresh();
    emit systemSelected(target);
}

void OffsetTableWidget::updateDetails()
{
    int row = m_offsetTable->currentRow();
    m_currentRow = row;

    for (int i = 0; i < 6; ++i) {
        m_offsetSpins[i]->blockSignals(true);
        if (row >= 0 && row < m_systems.size()) {
            m_offsetSpins[i]->setValue(m_systems[row].offsets[i]);
        } else {
            m_offsetSpins[i]->setValue(0.0);
        }
        m_offsetSpins[i]->blockSignals(false);
    }

    if (row >= 0) {
        emit systemSelected(row);
    }
}