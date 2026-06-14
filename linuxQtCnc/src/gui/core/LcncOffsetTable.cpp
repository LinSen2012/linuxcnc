/********************************************************************
* Description: LcncOffsetTable.cpp
*   LcncOffsetTable — Work Offset Table Implementation for
*   linuxQtCnc. Inherits the original LinuxCNC coordinate
*   system management logic and flow:
*   - Nine predefined coordinate systems (G54-G59.3)
*   - G92 temporary coordinate system offset overlay
*   - Load from / save to INI file
*   - Update from EMC_STAT status
*
*   Derived from a work by Fred Proctor & Will Shackleford
*
* Author: linuxQtCnc project
* License: GPL Version 2
* System: Linux / Windows
*
* Copyright (c) 2026 All rights reserved.
********************************************************************/

#include "LcncOffsetTable.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QRegularExpression>

// ============================================================================
// 静态成员初始化
// ============================================================================

LcncOffsetTable *LcncOffsetTable::m_instance = nullptr;

// ============================================================================
// 构造 / 析构
// ============================================================================

LcncOffsetTable::LcncOffsetTable(QObject *parent)
    : QObject(parent)
    , m_refreshTimer(new QTimer(this))
{
    qInfo().noquote() << "[LcncOffsetTable] 坐标系偏置表已创建";
    initDefaultSystems();

    connect(m_refreshTimer, &QTimer::timeout, this, [this]() {
        emit dataUpdated();
    });
}

LcncOffsetTable::~LcncOffsetTable()
{
    m_instance = nullptr;
    qInfo().noquote() << "[LcncOffsetTable] 坐标系偏置表已销毁";
}

LcncOffsetTable *LcncOffsetTable::instance()
{
    if (!m_instance) {
        m_instance = new LcncOffsetTable();
    }
    return m_instance;
}

// ============================================================================
// 默认坐标系初始化（继承 LinuxCNC 预定义逻辑）
// ============================================================================

void LcncOffsetTable::initDefaultSystems()
{
    m_coordinateSystems.clear();
    m_indexMap.clear();

    // 预定义 9 个坐标系 (G54, G55, G56, G57, G58, G59, G59.1, G59.2, G59.3)
    QStringList names = {"G54", "G55", "G56", "G57", "G58", "G59", "G59.1", "G59.2", "G59.3"};
    QStringList descs = {
        "工件坐标系 1", "工件坐标系 2", "工件坐标系 3",
        "工件坐标系 4", "工件坐标系 5", "工件坐标系 6",
        "工件坐标系 7", "工件坐标系 8", "工件坐标系 9"
    };

    for (int i = 0; i < 9; ++i) {
        CoordinateSystem cs;
        cs.index = i + 1;      // 索引从 1 开始 (G54=1)
        cs.name = names[i];
        cs.description = descs[i];
        // 所有偏置默认 0
        cs.x = 0.0;
        cs.y = 0.0;
        cs.z = 0.0;
        cs.a = 0.0;
        cs.b = 0.0;
        cs.c = 0.0;
        cs.u = 0.0;
        cs.v = 0.0;
        cs.w = 0.0;
        m_coordinateSystems.append(cs);
        m_indexMap[cs.index] = i;
    }

    m_activeIndex = 1;    // 默认激活 G54
    m_g92.active = false;
    m_g92.x = 0.0;
    m_g92.y = 0.0;
    m_g92.z = 0.0;
    m_g92.a = 0.0;
    m_g92.b = 0.0;
    m_g92.c = 0.0;
    m_g92.u = 0.0;
    m_g92.v = 0.0;
    m_g92.w = 0.0;

    qInfo().noquote() << "[LcncOffsetTable] 已初始化"
                    << m_coordinateSystems.size()
                    << "个坐标系";
}

// ============================================================================
// 坐标系数据访问
// ============================================================================

CoordinateSystem LcncOffsetTable::systemByIndex(int index) const
{
    auto it = m_indexMap.find(index);
    if (it != m_indexMap.end()) {
        return m_coordinateSystems[it.value()];
    }
    return CoordinateSystem();
}

CoordinateSystem LcncOffsetTable::systemByName(const QString &name) const
{
    for (const auto &cs : m_coordinateSystems) {
        if (cs.name == name) {
            return cs;
        }
    }
    return CoordinateSystem();
}

// ============================================================================
// 文件操作（继承 LinuxCNC INI 文件格式）
// ============================================================================

bool LcncOffsetTable::loadFromIni(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit errorOccurred(QString("无法打开文件: %1").arg(filename));
        return false;
    }

    QTextStream in(&file);

    // 搜索坐标系相关配置
    // LinuxCNC INI 格式：G54_X = 10.0   (在 [AXIS_X] 节
    // 简化格式：[AXIS_X] HOME = 10.0
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        // 跳过注释和空行
        if (line.isEmpty() || line.startsWith('#') || line.startsWith(';')) {
            continue;
        }

        // 检查节名
        if (line.startsWith('[') && line.endsWith(']')) {
            // 节名，不处理
            continue;
        }

        // 处理 "G54_X = 10.0 格式
        int eq = line.indexOf('=');
        if (eq < 0) continue;

        QString key = line.left(eq).trimmed();
        QString valueStr = line.mid(eq + 1).trimmed();

        bool ok;
        double value = valueStr.toDouble(&ok);
        if (!ok) continue;

        // 匹配 G54_X 格式
        for (int i = 0; i < m_coordinateSystems.size(); ++i) {
            const QString &name = m_coordinateSystems[i].name;
            if (key.startsWith(name)) {
                QString axisPart = key.mid(name.length());
                if (axisPart.startsWith('_')) {
                    axisPart = axisPart.mid(1);
                }
                char axisChar = axisPart.toUpper().at(0).toLatin1();
                int axis = 0;
                switch (axisChar) {
                    case 'X': axis = 0; break;
                    case 'Y': axis = 1; break;
                    case 'Z': axis = 2; break;
                    case 'A': axis = 3; break;
                    case 'B': axis = 4; break;
                    case 'C': axis = 5; break;
                    case 'U': axis = 6; break;
                    case 'V': axis = 7; break;
                    default: axis = -1;
                }
                if (axis >= 0) {
                    switch (axis) {
                        case 0: m_coordinateSystems[i].x = value; break;
                        case 1: m_coordinateSystems[i].y = value; break;
                        case 2: m_coordinateSystems[i].z = value; break;
                        case 3: m_coordinateSystems[i].a = value; break;
                        case 4: m_coordinateSystems[i].b = value; break;
                        case 5: m_coordinateSystems[i].c = value; break;
                        case 6: m_coordinateSystems[i].u = value; break;
                        case 7: m_coordinateSystems[i].v = value; break;
                    }
                }
            }
        }
    }

    file.close();
    qInfo().noquote() << "[LcncOffsetTable] 已从 INI 加载: " << filename;
    emit dataUpdated();
    return true;
}

bool LcncOffsetTable::saveToIni(const QString &filename) const
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit errorOccurred(QString("无法创建文件: %1").arg(filename));
        return false;
    }

    QTextStream out(&file);

    // 写入 INI 格式
    out << "# LinuxCNC 工件坐标系偏置表\n";
    out << "# 格式: G54_X = 值\n\n";

    for (const auto &cs : m_coordinateSystems) {
        out << QString("[%1]\n").arg(cs.name);
        out << QString("%1_X = %2\n").arg(cs.name).arg(cs.x, 0, 'f', 6);
        out << QString("%1_Y = %2\n").arg(cs.name).arg(cs.y, 0, 'f', 6);
        out << QString("%1_Z = %2\n").arg(cs.name).arg(cs.z, 0, 'f', 6);
        out << QString("%1_A = %2\n").arg(cs.name).arg(cs.a, 0, 'f', 6);
        out << QString("%1_B = %2\n").arg(cs.name).arg(cs.b, 0, 'f', 6);
        out << QString("%1_C = %2\n").arg(cs.name).arg(cs.c, 0, 'f', 6);
        out << QString("# %1\n").arg(cs.description);
        out << "\n";
    }

    file.close();
    qInfo().noquote() << "[LcncOffsetTable] 已保存到 INI: " << filename;
    return true;
}

// ============================================================================
// 状态更新（继承 LinuxCNC 旧逻辑：从 EMC_STAT 更新坐标偏置）
// ============================================================================

void LcncOffsetTable::updateFromStatus(int activeIndex,
                                        double x, double y, double z,
                                        double a, double b, double c)
{
    m_activeIndex = activeIndex;

    // 更新当前激活坐标系的偏置
    auto it = m_indexMap.find(activeIndex);
    if (it != m_indexMap.end()) {
        int arrIdx = it.value();
        m_coordinateSystems[arrIdx].x = x;
        m_coordinateSystems[arrIdx].y = y;
        m_coordinateSystems[arrIdx].z = z;
        m_coordinateSystems[arrIdx].a = a;
        m_coordinateSystems[arrIdx].b = b;
        m_coordinateSystems[arrIdx].c = c;
        emit offsetChanged(activeIndex);
        emit systemChanged(activeIndex);
        emit dataUpdated();
    }
}

void LcncOffsetTable::updateG92(const G92Offset &offset)
{
    m_g92 = offset;
    emit dataUpdated();
}

void LcncOffsetTable::resetG92()
{
    m_g92.x = 0.0;
    m_g92.y = 0.0;
    m_g92.z = 0.0;
    m_g92.a = 0.0;
    m_g92.b = 0.0;
    m_g92.c = 0.0;
    m_g92.u = 0.0;
    m_g92.v = 0.0;
    m_g92.w = 0.0;
    m_g92.active = false;
    emit dataUpdated();
}

void LcncOffsetTable::setAxisOffset(int index, int axis, double value)
{
    auto it = m_indexMap.find(index);
    if (it == m_indexMap.end()) return;

    int arrIdx = it.value();
    switch (axis) {
        case 0: m_coordinateSystems[arrIdx].x = value; break;
        case 1: m_coordinateSystems[arrIdx].y = value; break;
        case 2: m_coordinateSystems[arrIdx].z = value; break;
        case 3: m_coordinateSystems[arrIdx].a = value; break;
        case 4: m_coordinateSystems[arrIdx].b = value; break;
        case 5: m_coordinateSystems[arrIdx].c = value; break;
        case 6: m_coordinateSystems[arrIdx].u = value; break;
        case 7: m_coordinateSystems[arrIdx].v = value; break;
        case 8: m_coordinateSystems[arrIdx].w = value; break;
    }

    emit offsetChanged(index);
    emit dataUpdated();
}

// ============================================================================
// 内部解析 INI 键值（辅助函数）
// ============================================================================

bool LcncOffsetTable::parseKeyValue(const QString &key, const QString &value,
                                    int &index, int &axis, double &value_out)
{
    Q_UNUSED(key);
    Q_UNUSED(value);
    Q_UNUSED(index);
    Q_UNUSED(axis);
    Q_UNUSED(value_out);

    // 简化实现：在 loadFromIni 中直接解析
    return false;
}

// ============================================================================
// 开始/停止监视
// ============================================================================

void LcncOffsetTable::start(int intervalMs)
{
    if (!m_refreshTimer->isActive()) {
        m_refreshTimer->start(intervalMs);
        qInfo().noquote() << "[LcncOffsetTable] 开始监视，间隔" << intervalMs << "ms";
    }
}

void LcncOffsetTable::stop()
{
    if (m_refreshTimer->isActive()) {
        m_refreshTimer->stop();
        qInfo().noquote() << "[LcncOffsetTable] 停止监视";
    }
}