/*
 * linuxQtCncGui - LinuxCNC Qt5 GUI 控制器
 * LcncHalMonitor - HAL 信号监视器封装
 *
 * 封装 HAL 信号的读取和监视。
 * Windows 下使用模拟数据，Linux 下集成真实 HAL 通信。
 *
 * 在 Linux 上，HAL 信号通过共享内存访问，需要：
 * 1. 加载 HAL 库 (hal_lib.so)
 * 2. 通过 HAL API 读取信号值
 * 3. 使用定时器轮询更新
 */

#ifndef LCNC_HAL_MONITOR_H
#define LCNC_HAL_MONITOR_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QVariant>
#include <QTimer>
#include <QMap>

// ============================================================================
// HAL 数据类型和结构
// ============================================================================

/**
 * @brief HAL 数据类型
 */
enum class HalType {
    BIT = 0,    ///< 位类型 (hal_bit_t)
    FLOAT = 1,  ///< 浮点类型 (hal_float_t)
    S32 = 2,    ///< 有符号32位整数 (hal_s32_t)
    U32 = 3,    ///< 无符号32位整数 (hal_u32_t)
    S64 = 4,    ///< 有符号64位整数 (hal_s64_t)
    U64 = 5     ///< 无符号64位整数 (hal_u64_t)
};

/**
 * @brief HAL 引脚/信号方向
 */
enum class HalPinDir {
    IN = 1,     ///< 输入
    OUT = 2,    ///< 输出
    IO = 3      ///< 双向
};

/**
 * @brief HAL 信号信息
 */
struct HalSignalInfo {
    QString name;           ///< 信号名称
    HalType type;          ///< 数据类型
    HalPinDir dir;         ///< 方向
    QVariant value;        ///< 当前值
    QString description;    ///< 描述
    bool hasWriters = false; ///< 是否有写入者
};

/**
 * @brief HAL 监视器类
 *
 * 封装 HAL 信号的读取和监视功能。
 * 在 Linux 上通过 HAL API 读取真实信号值，
 * 在 Windows 上使用模拟数据进行开发测试。
 */
class LcncHalMonitor : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     */
    static LcncHalMonitor *instance();

    /**
     * @brief 析构函数
     */
    ~LcncHalMonitor() override;

    // ========================================================================
    // 信号管理
    // ========================================================================

    /**
     * @brief 添加要监视的信号
     * @param name 信号名称
     * @param desc 描述（可选）
     * @return 是否成功
     */
    bool addSignal(const QString &name, const QString &desc = QString());

    /**
     * @brief 移除监视的信号
     * @param name 信号名称
     */
    void removeSignal(const QString &name);

    /**
     * @brief 清空所有监视的信号
     */
    void clearSignals();

    /**
     * @brief 获取所有信号列表
     */
    QVector<HalSignalInfo> signals() const { return m_signals; }

    /**
     * @brief 获取信号值
     * @param name 信号名称
     * @return 信号值，无则返回无效 QVariant
     */
    QVariant signalValue(const QString &name) const;

    /**
     * @brief 是否已连接到 HAL
     */
    bool isConnected() const { return m_connected; }

signals:
    /**
     * @brief 信号值改变信号
     * @param name 信号名称
     * @param value 新值
     */
    void signalValueChanged(const QString &name, const QVariant &value);

    /**
     * @brief 连接状态改变信号
     * @param connected 是否已连接
     */
    void connectionChanged(bool connected);

    /**
     * @brief 错误信号
     * @param error 错误信息
     */
    void errorOccurred(const QString &error);

public slots:
    /**
     * @brief 开始监视
     * @param intervalMs 刷新间隔（毫秒）
     */
    void start(int intervalMs = 100);

    /**
     * @brief 停止监视
     */
    void stop();

    /**
     * @brief 刷新所有信号值
     */
    void refresh();

private:
    /**
     * @brief 构造函数
     */
    explicit LcncHalMonitor(QObject *parent = nullptr);

    /**
     * @brief Linux 下初始化 HAL 连接
     */
    bool initLinuxHal();

    /**
     * @brief Linux 下读取信号值
     */
    void readLinuxSignals();

    /**
     * @brief 模拟数据初始化（Windows 或 Linux 无 HAL）
     */
    void initSimulation();

    /**
     * @brief 读取模拟信号值
     */
    void readSimulation();

private:
    static LcncHalMonitor *m_instance;

    QVector<HalSignalInfo> m_signals;
    QMap<QString, int> m_signalIndices;
    QTimer *m_refreshTimer = nullptr;
    bool m_connected = false;
    bool m_useSimulation = true;

#ifdef Q_OS_LINUX
    int m_halCompId = -1;  // HAL 组件ID
#endif
};

#endif // LCNC_HAL_MONITOR_H