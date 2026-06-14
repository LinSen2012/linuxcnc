/*
 * linuxQtCncGui - LinuxCNC Qt6 GUI 控制器
 * GCodeParser 实现
 */

#include "GCodeParser.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <cmath>
#include <limits>

// ============================================================================
// 构造 / 析构
// ============================================================================

GCodeParser::GCodeParser(QObject *parent)
    : QObject(parent)
{
}

GCodeParser::~GCodeParser() = default;

// ============================================================================
// 公共方法
// ============================================================================

bool GCodeParser::parseFile(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_errors.append(QStringLiteral("无法打开文件: %1").arg(filename));
        emit parseError(0, m_errors.last());
        return false;
    }

    QTextStream in(&file);
    return parseText(in.readAll());
}

bool GCodeParser::parseText(const QString &gcode)
{
    clear();

    const QStringList lines = gcode.split(QLatin1Char('\n'), Qt::SkipEmptyParts);

    for (int i = 0; i < lines.size(); ++i) {
        parseLine(lines[i].trimmed(), i + 1);
    }

    qInfo().noquote() << "[GCodeParser] 解析完成:"
                       << lines.size() << "行,"
                       << m_segments.size() << "段路径";

    emit parseComplete(m_segments.size());
    return m_errors.isEmpty();
}

const QVector<PathSegment> &GCodeParser::pathSegments() const
{
    return m_segments;
}

QPair<QPair<double, double>, QPair<double, double>> GCodeParser::boundingBox() const
{
    if (m_segments.isEmpty()) {
        return {{0, 0}, {0, 0}};
    }

    double minX = std::numeric_limits<double>::max();
    double minY = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double maxY = std::numeric_limits<double>::lowest();

    for (const auto &seg : m_segments) {
        minX = std::min({minX, seg.x0, seg.x1});
        minY = std::min({minY, seg.y0, seg.y1});
        maxX = std::max({maxX, seg.x0, seg.x1});
        maxY = std::max({maxY, seg.y0, seg.y1});
    }

    return {{minX, minY}, {maxX, maxY}};
}

void GCodeParser::clear()
{
    m_segments.clear();
    m_errors.clear();
    m_currentMotionMode = 0;
    m_currentX = 0.0;
    m_currentY = 0.0;
    m_currentZ = 0.0;
    m_currentFeedrate = 0.0;
    m_absoluteMode = true;
}

QStringList GCodeParser::errors() const
{
    return m_errors;
}

int GCodeParser::segmentCount() const
{
    return m_segments.size();
}

// ============================================================================
// 内部解析方法
// ============================================================================

QMap<QChar, double> GCodeParser::parseWords(const QString &line)
{
    QMap<QChar, double> words;
    QString processed = line.toUpper();

    // 移除注释（括号内和分号后）
    int commentStart = processed.indexOf('(');
    if (commentStart >= 0) {
        processed = processed.left(commentStart);
    }
    commentStart = processed.indexOf(';');
    if (commentStart >= 0) {
        processed = processed.left(commentStart);
    }

    // 解析字母+数值对
    int pos = 0;
    while (pos < processed.size()) {
        // 跳过空白
        while (pos < processed.size() && processed[pos].isSpace()) {
            ++pos;
        }
        if (pos >= processed.size()) break;

        // 读取字母
        QChar letter = processed[pos];
        if (!letter.isLetter()) {
            ++pos;
            continue;
        }
        ++pos;

        // 读取数值（可能带符号和小数点）
        QString numStr;
        if (pos < processed.size() && (processed[pos] == '+' || processed[pos] == '-')) {
            numStr += processed[pos];
            ++pos;
        }
        while (pos < processed.size() &&
               (processed[pos].isDigit() || processed[pos] == '.')) {
            numStr += processed[pos];
            ++pos;
        }

        if (!numStr.isEmpty()) {
            bool ok = false;
            double value = numStr.toDouble(&ok);
            if (ok) {
                words[letter] = value;
            }
        }
    }

    return words;
}

void GCodeParser::parseLine(const QString &line, int lineNumber)
{
    // 跳过空行和纯注释行
    if (line.isEmpty() || line.startsWith('(') || line.startsWith(';')) {
        return;
    }

    auto words = parseWords(line);

    // 处理模态 G 代码
    if (words.contains('G')) {
        int gCode = static_cast<int>(words.value('G'));
        processGCode(gCode, words, lineNumber);
    } else if (words.contains('X') || words.contains('Y') || words.contains('Z')) {
        // 没有显式 G 代码但有坐标，使用当前模态运动模式
        if (m_currentMotionMode > 0) {
            PathType type = PathType::Linear;
            switch (m_currentMotionMode) {
            case 0: type = PathType::Rapid; break;
            case 1: type = PathType::Linear; break;
            case 2: type = PathType::ArcCW; break;
            case 3: type = PathType::ArcCCW; break;
            default: type = PathType::Linear; break;
            }
            generateSegment(type, words, lineNumber);
        }
    }

    // 处理非运动指令
    if (words.contains('F')) {
        m_currentFeedrate = words.value('F');
    }
}

void GCodeParser::processGCode(int gCode, const QMap<QChar, double> &words, int lineNumber)
{
    switch (gCode) {
    // 运动指令
    case 0:
        m_currentMotionMode = 0;
        generateSegment(PathType::Rapid, words, lineNumber);
        break;
    case 1:
        m_currentMotionMode = 1;
        generateSegment(PathType::Linear, words, lineNumber);
        break;
    case 2:
        m_currentMotionMode = 2;
        generateSegment(PathType::ArcCW, words, lineNumber);
        break;
    case 3:
        m_currentMotionMode = 3;
        generateSegment(PathType::ArcCCW, words, lineNumber);
        break;

    // 坐标系模式
    case 90:
        m_absoluteMode = true;
        break;
    case 91:
        m_absoluteMode = false;
        break;

    // 其他 G 代码暂不处理
    default:
        break;
    }

    // 更新进给速率
    if (words.contains('F')) {
        m_currentFeedrate = words.value('F');
    }
}

void GCodeParser::generateSegment(PathType type, const QMap<QChar, double> &words, int lineNumber)
{
    PathSegment seg;
    seg.type = type;
    seg.lineNumber = lineNumber;
    seg.feedrate = m_currentFeedrate;

    // 起点 = 当前位置
    seg.x0 = m_currentX;
    seg.y0 = m_currentY;
    seg.z0 = m_currentZ;

    // 计算终点
    if (m_absoluteMode) {
        seg.x1 = words.value('X', m_currentX);
        seg.y1 = words.value('Y', m_currentY);
        seg.z1 = words.value('Z', m_currentZ);
    } else {
        // 增量模式
        seg.x1 = m_currentX + words.value('X', 0.0);
        seg.y1 = m_currentY + words.value('Y', 0.0);
        seg.z1 = m_currentZ + words.value('Z', 0.0);
    }

    // 圆弧参数
    if (type == PathType::ArcCW || type == PathType::ArcCCW) {
        seg.i = words.value('I', 0.0);
        seg.j = words.value('J', 0.0);
        seg.k = words.value('K', 0.0);
        seg.radius = words.value('R', 0.0);
    }

    // 只有坐标发生变化时才生成路径段
    if (!qFuzzyCompare(seg.x0, seg.x1) ||
        !qFuzzyCompare(seg.y0, seg.y1) ||
        !qFuzzyCompare(seg.z0, seg.z1)) {
        m_segments.append(seg);
    }

    // 更新当前位置
    m_currentX = seg.x1;
    m_currentY = seg.y1;
    m_currentZ = seg.z1;
}
