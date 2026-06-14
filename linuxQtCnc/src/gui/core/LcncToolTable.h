/*
 * linuxQtCncGui - LinuxCNC Qt5 GUI 控制器
 * LcncToolTable - 刀具表管理器
 *
 * 封装 LinuxCNC 刀具表的读取和管理。
 * 在 Linux 上通过 NML 或共享内存访问刀具表数据，
 * 在 Windows 上使用模拟数据进行开发测试。
 *
 * LinuxCNC 刀具表数据结构 (CANON_TOOL_TABLE):
 *   - toolno: 刀具号
 *   - pocketno: 刀位号
 *   - offset: 偏置 (EmcPose，包含 tran.xyz 和 a/b/c)
 *   - diameter: 直径
 *   - frontangle: 前角
 *   - backangle: 后角
 *   - orientation: 方向
 *   - comment: 注释
 */

#ifndef LCNC_TOOL_TABLE_H
#define LCNC_TOOL_TABLE_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QTimer>

// ============================================================================
// 刀具数据结构
// ============================================================================

/**
 * @brief 刀具数据
 */
struct ToolItem {
    int toolNo = 0;           ///< 刀具号
    int pocketNo = 0;         ///< 刀位号
    double xOffset = 0.0;     ///< X 偏置
    double yOffset = 0.0;     ///< Y 偏置
    double zOffset = 0.0;     ///< Z 偏置
    double aOffset = 0.0;     ///< A 轴偏置
    double bOffset = 0.0;     ///< B 轴偏置
    double cOffset = 0.0;     ///< C 轴偏置
    double diameter = 0.0;    ///< 直径
    double frontAngle = 0.0;  ///< 前角
    double backAngle = 0.0;   ///< 后角
    int orientation = 0;       ///< 方向
    QString comment;          ///< 注释
};

/**
 * @brief 刀具表管理器类
 *
 * 管理 LinuxCNC 刀具表，支持：
 * - 从刀具表文件加载
 * - 保存到刀具表文件
 * - 实时刀具数据更新
 */
class LcncToolTable : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     */
    static LcncToolTable *instance();

    /**
     * @brief 析构函数
     */
    ~LcncToolTable() override;

    // ========================================================================
    // 刀具数据访问
    // ========================================================================

    /**
     * @brief 获取刀具数量
     */
    int count() const { return m_tools.size(); }

    /**
     * @brief 获取指定位置的刀具
     * @param index 索引 (0-based)
     */
    ToolItem toolAt(int index) const;

    /**
     * @brief 获取指定刀位号的刀具
     * @param pocket 刀位号
     */
    ToolItem toolByPocket(int pocket) const;

    /**
     * @brief 获取指定刀具号的刀具
     * @param toolNo 刀具号
     */
    ToolItem toolByNumber(int toolNo) const;

    /**
     * @brief 获取主轴中的刀具
     */
    int toolInSpindle() const { return m_toolInSpindle; }

    /**
     * @brief 获取所有刀具列表
     */
    QVector<ToolItem> tools() const { return m_tools; }

    // ========================================================================
    // 文件操作
    // ========================================================================

    /**
     * @brief 从文件加载刀具表
     * @param filename 刀具表文件路径
     * @return 是否成功
     */
    bool loadFromFile(const QString &filename);

    /**
     * @brief 保存刀具表到文件
     * @param filename 刀具表文件路径
     * @return 是否成功
     */
    bool saveToFile(const QString &filename) const;

    // ========================================================================
    // NML/状态更新
    // ========================================================================

    /**
     * @brief 从 EMC_TOOL_STAT 更新刀具数据
     * @param toolInSpindle 主轴中的刀具号
     * @param toolTable 刀具表数组
     * @param count 刀具数量
     */
    void updateFromEmc(int toolInSpindle, const void *toolTable, int count);

signals:
    /**
     * @brief 刀具改变信号
     * @param toolNo 新刀具号
     */
    void toolChanged(int toolNo);

    /**
     * @brief 数据更新信号
     */
    void dataUpdated();

    /**
     * @brief 错误信号
     * @param error 错误信息
     */
    void errorOccurred(const QString &error);

public slots:
    /**
     * @brief 开始监视
     * @param intervalMs 刷新间隔
     */
    void start(int intervalMs = 500);

    /**
     * @brief 停止监视
     */
    void stop();

private:
    /**
     * @brief 构造函数
     */
    explicit LcncToolTable(QObject *parent = nullptr);

    /**
     * @brief 初始化模拟数据
     */
    void initSimulation();

    /**
     * @brief 解析刀具表行
     * @param line 行内容
     * @param tool 输出刀具数据
     * @return 是否成功
     */
    bool parseLine(const QString &line, ToolItem &tool);

    /**
     * @brief 格式化刀具表行
     * @param tool 刀具数据
     * @return 格式化后的行
     */
    QString formatLine(const ToolItem &tool) const;

    /**
     * @brief 将刀具添加到索引映射
     * @param tool 刀具数据
     * @return 是否成功
     */
    bool addToolToIndex(const ToolItem &tool);

    static LcncToolTable *m_instance;

    QVector<ToolItem> m_tools;
    QMap<int, int> m_pocketIndex;   // pocket -> index
    QMap<int, int> m_toolIndex;     // toolNo -> index
    int m_toolInSpindle = 0;
    QString m_currentFile;
    QTimer *m_refreshTimer = nullptr;
    bool m_useSimulation = true;
};

#endif // LCNC_TOOL_TABLE_H