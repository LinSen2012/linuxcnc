/*
 * linuxQtCncGui - LinuxCNC Qt6 GUI 控制器
 * GCodeParser - G 代码解析器
 *
 * 将 G 代码文件解析为刀具路径数据，供 ToolpathView 渲染。
 * 支持：
 * - 基本运动指令（G0/G1/G2/G3）
 * - 坐标解析（X/Y/Z/A/B/C）
 * - 进给速率（F）
 * - 主轴转速（S）
 * - 路径段类型标记（快移/直线/圆弧）
 */

#ifndef GCODEPARSER_H
#define GCODEPARSER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QPair>

/**
 * @brief 路径段类型
 */
enum class PathType {
    Rapid,      ///< 快速移动（G0）
    Linear,     ///< 直线插补（G1）
    ArcCW,      ///< 顺时针圆弧（G2）
    ArcCCW      ///< 逆时针圆弧（G3）
};

/**
 * @brief 单条路径段数据
 */
struct PathSegment {
    PathType type = PathType::Linear;  ///< 路径类型
    double x0 = 0.0;  ///< 起点 X
    double y0 = 0.0;  ///< 起点 Y
    double z0 = 0.0;  ///< 起点 Z
    double x1 = 0.0;  ///< 终点 X
    double y1 = 0.0;  ///< 终点 Y
    double z1 = 0.0;  ///< 终点 Z
    double feedrate = 0.0;  ///< 进给速率
    int lineNumber = 0;  ///< 源代码行号

    // 圆弧参数（仅 G2/G3 有效）
    double i = 0.0;  ///< 圆心 X 偏移（相对于起点）
    double j = 0.0;  ///< 圆心 Y 偏移（相对于起点）
    double k = 0.0;  ///< 圆心 Z 偏移（相对于起点）
    double radius = 0.0;  ///< 圆弧半径（R 格式）
};

/**
 * @brief GCodeParser - G 代码解析器
 *
 * 将 G 代码文本解析为 PathSegment 序列。
 */
class GCodeParser : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit GCodeParser(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~GCodeParser() override;

    /**
     * @brief 解析 G 代码文件
     * @param filename 文件路径
     * @return 是否解析成功
     */
    bool parseFile(const QString &filename);

    /**
     * @brief 解析 G 代码文本
     * @param gcode G 代码文本
     * @return 是否解析成功
     */
    bool parseText(const QString &gcode);

    /**
     * @brief 获取解析后的路径段列表
     * @return 路径段向量
     */
    const QVector<PathSegment> &pathSegments() const;

    /**
     * @brief 获取路径边界框
     * @return (minX, minY, maxX, maxY)
     */
    QPair<QPair<double, double>, QPair<double, double>> boundingBox() const;

    /**
     * @brief 清除解析结果
     */
    void clear();

    /**
     * @brief 获取解析错误信息
     * @return 错误信息列表
     */
    QStringList errors() const;

    /**
     * @brief 获取路径段数量
     * @return 段数量
     */
    int segmentCount() const;

signals:
    /**
     * @brief 解析完成信号
     * @param segmentCount 路径段数量
     */
    void parseComplete(int segmentCount);

    /**
     * @brief 解析错误信号
     * @param line 行号
     * @param message 错误描述
     */
    void parseError(int line, const QString &message);

private:
    /**
     * @brief 解析单行 G 代码
     * @param line 行内容
     * @param lineNumber 行号
     */
    void parseLine(const QString &line, int lineNumber);

    /**
     * @brief 处理 G 代码指令
     * @param gCode G 代码编号
     * @param words 该行所有字（字母+数值对）
     * @param lineNumber 行号
     */
    void processGCode(int gCode, const QMap<QChar, double> &words, int lineNumber);

    /**
     * @brief 生成路径段
     * @param type 路径类型
     * @param words 该行所有字
     * @param lineNumber 行号
     */
    void generateSegment(PathType type, const QMap<QChar, double> &words, int lineNumber);

    /**
     * @brief 解析一行中的所有字（字母+数值对）
     * @param line 行内容
     * @return 字映射
     */
    static QMap<QChar, double> parseWords(const QString &line);

private:
    QVector<PathSegment> m_segments;  ///< 路径段列表
    QStringList m_errors;             ///< 错误信息

    // 当前解析状态
    int m_currentMotionMode = 0;      ///< 当前运动模式（G0/G1/G2/G3）
    double m_currentX = 0.0;          ///< 当前 X 位置
    double m_currentY = 0.0;          ///< 当前 Y 位置
    double m_currentZ = 0.0;          ///< 当前 Z 位置
    double m_currentFeedrate = 0.0;   ///< 当前进给速率
    bool m_absoluteMode = true;       ///< 绝对/增量模式（G90/G91）
};

#endif // GCODEPARSER_H
