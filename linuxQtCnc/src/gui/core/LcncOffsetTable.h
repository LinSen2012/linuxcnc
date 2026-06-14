/*
 * linuxQtCncGui - LinuxCNC Qt5 GUI 控制器
 * LcncOffsetTable - 工件坐标系偏置表管理器
 *
 * 封装 LinuxCNC 坐标系偏置 (G54-G59.3) 的管理。
 * 每个坐标系存储 X/Y/Z/A/B/C/U/V/W 轴的偏置值。
 *
 * 继承 LinuxCNC 的逻辑：
 * - 坐标系索引：1=G54, 2=G55, 3=G56, 4=G57, 5=G58, 6=G59
 *               7=G59.1, 8=G59.2, 9=G59.3
 * - 坐标偏置存储在工件坐标系表中，由 INI 文件加载或在运行时设置
 * - 运行时也可通过 G10 L2 Pn X... Y... Z... 设置
 * - G92 偏置是临时的、叠加在当前坐标系上
 */

#ifndef LCNC_OFFSET_TABLE_H
#define LCNC_OFFSET_TABLE_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QTimer>

// ============================================================================
// 坐标系数据结构（继承 LinuxCNC 的坐标表示）
// ============================================================================

/**
 * @brief 单个坐标系的偏置数据
 * 继承 LinuxCNC EmcPose 概念：tran.x/y/z 代表平动偏置，
 * a/b/c 代表旋转轴偏置
 */
struct CoordinateSystem {
    int index = 0;          ///< 坐标系索引 (1-9)
    QString name;           ///< 坐标系名称 (G54, G55, ...)
    QString description;    ///< 描述
    double x = 0.0;         ///< X 轴偏置
    double y = 0.0;         ///< Y 轴偏置
    double z = 0.0;         ///< Z 轴偏置
    double a = 0.0;         ///< A 轴偏置
    double b = 0.0;         ///< B 轴偏置
    double c = 0.0;         ///< C 轴偏置
    double u = 0.0;         ///< U 轴偏置
    double v = 0.0;         ///< V 轴偏置
    double w = 0.0;         ///< W 轴偏置
};

/**
 * @brief G92 临时坐标系偏置
 * 与 G54-G59.3 不同，G92 是叠加在当前坐标系之上的临时偏置
 */
struct G92Offset {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double a = 0.0;
    double b = 0.0;
    double c = 0.0;
    bool active = false;    ///< G92 是否激活
};

/**
 * @brief LcncOffsetTable - 坐标系偏置管理器
 *
 * 继承 LinuxCNC 旧有的坐标系管理逻辑与流程：
 * - 从 INI 文件加载初始偏置
 * - 支持 9 个工件坐标系 (G54~G59.3)
 * - 支持 G92 临时坐标系偏置
 * - 通过 NML/状态轮询更新坐标偏置数据
 */
class LcncOffsetTable : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     */
    static LcncOffsetTable *instance();

    /**
     * @brief 析构函数
     */
    ~LcncOffsetTable() override;

    // ========================================================================
    // 坐标系数据访问
    // ========================================================================

    /**
     * @brief 获取坐标系数量 (固定 9 个)
     */
    int count() const { return m_coordinateSystems.size(); }

    /**
     * @brief 获取指定索引的坐标系
     * @param index 坐标系索引 (1-9，对应 G54=1 到 G59.3=9)
     */
    CoordinateSystem systemByIndex(int index) const;

    /**
     * @brief 获取指定名称的坐标系
     * @param name 坐标系名称 ("G54", "G55", ...)
     */
    CoordinateSystem systemByName(const QString &name) const;

    /**
     * @brief 当前激活的坐标系索引
     */
    int activeIndex() const { return m_activeIndex; }

    /**
     * @brief 获取当前 G92 临时偏置
     */
    G92Offset g92Offset() const { return m_g92; }

    /**
     * @brief 获取所有坐标系数据（只读）
     */
    QVector<CoordinateSystem> coordinateSystems() const { return m_coordinateSystems; }

    // ========================================================================
    // 文件操作
    // ========================================================================

    /**
     * @brief 从 INI 配置文件加载坐标系偏置
     * 继承 LinuxCNC 旧逻辑：从 [AXIS_n] 节的 HOME 偏移读取
     * @param filename INI 文件路径
     * @return 是否成功
     */
    bool loadFromIni(const QString &filename);

    /**
     * @brief 保存坐标系偏置到 INI 文件
     * @param filename INI 文件路径
     * @return 是否成功
     */
    bool saveToIni(const QString &filename) const;

signals:
    /**
     * @brief 坐标系偏置改变信号
     * @param index 坐标系索引
     */
    void offsetChanged(int index);

    /**
     * @brief 当前激活坐标系切换信号
     * @param newIndex 新坐标系索引
     */
    void systemChanged(int newIndex);

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
     * @brief 从状态数据更新坐标系偏置
     * 继承 LinuxCNC 旧逻辑：从 EMC_STAT 的 task.g5x_offset/g92_offset 更新
     * @param activeIndex 当前激活坐标系
     * @param x/y/z/a/b/c 各轴的当前坐标系偏置
     */
    void updateFromStatus(int activeIndex,
                          double x, double y, double z,
                          double a = 0, double b = 0, double c = 0);

    /**
     * @brief 更新 G92 临时偏置
     */
    void updateG92(const G92Offset &offset);

    /**
     * @brief 复位 G92 偏置（模拟 G92.1）
     */
    void resetG92();

    /**
     * @brief 设置单个坐标系的某个轴偏置
     */
    void setAxisOffset(int index, int axis, double value);

    /**
     * @brief 开始监视
     */
    void start(int intervalMs = 500);

    /**
     * @brief 停止监视
     */
    void stop();

private:
    /**
     * @brief 构造函数（私有，单例模式）
     */
    explicit LcncOffsetTable(QObject *parent = nullptr);

    /**
     * @brief 初始化 9 个默认坐标系
     * 继承 LinuxCNC 预定义坐标系的逻辑
     */
    void initDefaultSystems();

    /**
     * @brief 将 INI 键值对解析为偏置值
     * @param line INI 中的一行，如 "G54_X = 10.0" 或 "OFFSET_AV_RATIO = 0.1"
     * @param[out] index 坐标系索引
     * @param[out] axis 轴索引 (0=X, 1=Y, 2=Z, 3=A, 4=B, 5=C)
     * @param[out] value 偏置值
     * @return 是否成功解析
     */
    bool parseKeyValue(const QString &key, const QString &value,
                       int &index, int &axis, double &value_out);

    static LcncOffsetTable *m_instance;

    QVector<CoordinateSystem> m_coordinateSystems;  ///< 9 个坐标系
    QMap<int, int> m_indexMap;                      ///< index -> array index
    int m_activeIndex = 1;                          ///< 当前激活坐标系 (G54=1)
    G92Offset m_g92;                                ///< G92 临时偏置
    QTimer *m_refreshTimer = nullptr;
    bool m_useSimulation = true;
};

#endif // LCNC_OFFSET_TABLE_H