/********************************************************************
* Description: LcncToolTable.cpp
*   LcncToolTable — Tool Table Manager Implementation for
*   linuxQtCnc. On Linux, tool table data is accessed via NML
*   or shared memory. On Windows, simulated data is used for
*   development testing.
*
*   Derived from a work by Fred Proctor & Will Shackleford
*
* Author: linuxQtCnc project
* License: GPL Version 2
* System: Linux / Windows
*
* Copyright (c) 2026 All rights reserved.
********************************************************************/

#include "LcncToolTable.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QRegularExpression>

// ============================================================================
// 静态成员初始化
// ============================================================================

LcncToolTable *LcncToolTable::m_instance = nullptr;

// ============================================================================
// 构造 / 析构
// ============================================================================

LcncToolTable::LcncToolTable(QObject *parent)
    : QObject(parent)
    , m_refreshTimer(new QTimer(this))
{
    qInfo().noquote() << "[LcncToolTable] 刀具表管理器已创建";

    // 初始化模拟数据
    initSimulation();

    // 连接刷新定时器
    connect(m_refreshTimer, &QTimer::timeout, this, [this]() {
        emit dataUpdated();
    });
}

LcncToolTable::~LcncToolTable()
{
    m_instance = nullptr;
    qInfo().noquote() << "[LcncToolTable] 刀具表管理器已销毁";
}

LcncToolTable *LcncToolTable::instance()
{
    if (!m_instance) {
        m_instance = new LcncToolTable();
    }
    return m_instance;
}

// ============================================================================
// 模拟数据初始化
// ============================================================================

void LcncToolTable::initSimulation()
{
    m_useSimulation = true;
    m_tools.clear();
    m_pocketIndex.clear();
    m_toolIndex.clear();

    // 添加一些示例刀具
    QVector<ToolItem> sampleTools = {
        // T1: 10mm 平底铣刀
        {1, 1, 0.0, 0.0, -50.0, 0, 0, 0, 10.0, 0.0, 0.0, 0, "10mm flat endmill"},
        // T2: 5mm 球头铣刀
        {2, 2, 0.0, 0.0, -50.0, 0, 0, 0, 5.0, 0.0, 0.0, 0, "5mm ball endmill"},
        // T3: 3mm 钻头
        {3, 3, 0.0, 0.0, -40.0, 0, 0, 0, 3.0, 118.0, 0.0, 0, "3mm drill"},
        // T4: 8mm 面铣刀
        {4, 4, 0.0, 0.0, -50.0, 0, 0, 0, 8.0, 0.0, 0.0, 0, "8mm face mill"},
        // T5: 1/4" V-槽铣刀
        {5, 5, 0.0, 0.0, -50.0, 0, 0, 0, 6.35, 90.0, 10.0, 0, "1/4 V-bit"},
    };

    for (const auto &tool : sampleTools) {
        addToolToIndex(tool);
    }

    m_toolInSpindle = 1;  // T1 在主轴

    qInfo().noquote() << "[LcncToolTable] 模拟刀具表已初始化，共"
                       << m_tools.size() << "把刀具";
}

bool LcncToolTable::addToolToIndex(const ToolItem &tool)
{
    int index = m_tools.size();
    m_tools.append(tool);
    m_pocketIndex[tool.pocketNo] = index;
    m_toolIndex[tool.toolNo] = index;
    return true;
}

// ============================================================================
// 刀具数据访问
// ============================================================================

ToolItem LcncToolTable::toolAt(int index) const
{
    if (index >= 0 && index < m_tools.size()) {
        return m_tools[index];
    }
    return ToolItem();
}

ToolItem LcncToolTable::toolByPocket(int pocket) const
{
    auto it = m_pocketIndex.find(pocket);
    if (it != m_pocketIndex.end()) {
        return m_tools[it.value()];
    }
    return ToolItem();
}

ToolItem LcncToolTable::toolByNumber(int toolNo) const
{
    auto it = m_toolIndex.find(toolNo);
    if (it != m_toolIndex.end()) {
        return m_tools[it.value()];
    }
    return ToolItem();
}

// ============================================================================
// 文件操作
// ============================================================================

bool LcncToolTable::loadFromFile(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit errorOccurred(QString("无法打开文件: %1").arg(filename));
        return false;
    }

    m_tools.clear();
    m_pocketIndex.clear();
    m_toolIndex.clear();

    QTextStream in(&file);
    int lineNum = 0;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        ++lineNum;

        // 跳过空行和注释
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        ToolItem tool;
        if (parseLine(line, tool)) {
            addToolToIndex(tool);
        } else {
            qWarning().noquote() << "[LcncToolTable] 解析行" << lineNum << "失败:" << line;
        }
    }

    file.close();
    m_currentFile = filename;

    qInfo().noquote() << "[LcncToolTable] 已加载刀具表:" << filename
                       << ", 共" << m_tools.size() << "把刀具";
    emit dataUpdated();
    return true;
}

bool LcncToolTable::saveToFile(const QString &filename) const
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit errorOccurred(QString("无法创建文件: %1").arg(filename));
        return false;
    }

    QTextStream out(&file);

    // 写入头部注释
    out << "# LinuxCNC Tool Table\n";
    out << "# Format: Pocket  T  X  Y  Z  A  B  C  Diameter  Front  Back  Orient  Comment\n";
    out << "#\n";

    // 写入刀具数据
    for (const auto &tool : m_tools) {
        out << formatLine(tool) << "\n";
    }

    file.close();

    qInfo().noquote() << "[LcncToolTable] 已保存刀具表:" << filename;
    return true;
}

bool LcncToolTable::parseLine(const QString &line, ToolItem &tool)
{
    // LinuxCNC 刀具表格式:
    // Pocket  T  X  Y  Z  A  B  C  Diameter  Front  Back  Orient  Comment
    // 示例:   1  1  0.0  0.0  -50.0  0.0  0.0  0.0  10.0  0.0  0.0  0  10mm flat endmill

    QStringList parts = line.split(QRegularExpression("\\s+"));
    if (parts.size() < 10) {
        return false;
    }

    bool ok;
    int index = 0;

    tool.pocketNo = parts[index++].toInt(&ok);
    if (!ok) return false;

    tool.toolNo = parts[index++].toInt(&ok);
    if (!ok) return false;

    tool.xOffset = parts[index++].toDouble(&ok);
    if (!ok) return false;

    tool.yOffset = parts[index++].toDouble(&ok);
    if (!ok) return false;

    tool.zOffset = parts[index++].toDouble(&ok);
    if (!ok) return false;

    tool.aOffset = parts[index++].toDouble(&ok);
    if (!ok) return false;

    tool.bOffset = parts[index++].toDouble(&ok);
    if (!ok) return false;

    tool.cOffset = parts[index++].toDouble(&ok);
    if (!ok) return false;

    tool.diameter = parts[index++].toDouble(&ok);
    if (!ok) return false;

    tool.frontAngle = parts[index++].toDouble(&ok);
    if (!ok) return false;

    tool.backAngle = parts[index++].toDouble(&ok);
    if (!ok) return false;

    tool.orientation = parts[index++].toInt(&ok);
    if (!ok) return false;

    // 剩余部分是注释
    if (index < parts.size()) {
        tool.comment = parts.mid(index).join(" ");
    }

    return true;
}

QString LcncToolTable::formatLine(const ToolItem &tool) const
{
    // 格式化: Pocket T X Y Z A B C Diameter Front Back Orient Comment
    QString line = QString("%1 %2 %3 %4 %5 %6 %7 %8 %9 %10 %11 %12 %3")
        .arg(tool.pocketNo, 6)
        .arg(tool.toolNo, 6)
        .arg(tool.xOffset, 10, 'f', 4)
        .arg(tool.yOffset, 10, 'f', 4)
        .arg(tool.zOffset, 10, 'f', 4)
        .arg(tool.aOffset, 10, 'f', 4)
        .arg(tool.bOffset, 10, 'f', 4)
        .arg(tool.cOffset, 10, 'f', 4)
        .arg(tool.diameter, 10, 'f', 4)
        .arg(tool.frontAngle, 7, 'f', 2)
        .arg(tool.backAngle, 7, 'f', 2)
        .arg(tool.orientation, 7);

    if (!tool.comment.isEmpty()) {
        line += " " + tool.comment;
    }

    return line;
}

// ============================================================================
// NML/状态更新
// ============================================================================

void LcncToolTable::updateFromEmc(int toolInSpindle, const void *toolTable, int count)
{
#ifdef Q_OS_LINUX
    // Linux 下从 EMC_TOOL_STAT 更新刀具数据
    // toolTable 是 CANON_TOOL_TABLE 数组
    // count 是刀具数量

    m_toolInSpindle = toolInSpindle;
    m_tools.clear();
    m_pocketIndex.clear();
    m_toolIndex.clear();

    const CANON_TOOL_TABLE *tools =
        static_cast<const CANON_TOOL_TABLE *>(toolTable);

    for (int i = 0; i < count; ++i) {
        const auto &src = tools[i];
        if (src.toolno == 0 && i > 0) {
            // 刀具号 0 表示空刀位，跳过（除非是第一个）
            continue;
        }

        ToolItem tool;
        tool.toolNo = src.toolno;
        tool.pocketNo = src.pocketno;
        tool.xOffset = src.offset.tran.x;
        tool.yOffset = src.offset.tran.y;
        tool.zOffset = src.offset.tran.z;
        tool.aOffset = src.offset.a;
        tool.bOffset = src.offset.b;
        tool.cOffset = src.offset.c;
        tool.diameter = src.diameter;
        tool.frontAngle = src.frontangle;
        tool.backAngle = src.backangle;
        tool.orientation = src.orientation;
        tool.comment = QString::fromLocal8Bit(src.comment);

        addToolToIndex(tool);
    }

    emit dataUpdated();
#endif

    Q_UNUSED(toolTable)
    Q_UNUSED(count)
}

void LcncToolTable::start(int intervalMs)
{
    if (!m_refreshTimer->isActive()) {
        m_refreshTimer->start(intervalMs);
        qInfo().noquote() << "[LcncToolTable] 开始监视，间隔" << intervalMs << "ms";
    }
}

void LcncToolTable::stop()
{
    if (m_refreshTimer->isActive()) {
        m_refreshTimer->stop();
        qInfo().noquote() << "[LcncToolTable] 停止监视";
    }
}