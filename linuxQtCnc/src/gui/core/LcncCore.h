/*
 * linuxQtCncGui - LinuxCNC Qt6 GUI 控制器
 * LcncCore - NML 通信封装（核心单例）
 *
 * 封装与 LinuxCNC NML 服务器的通信，提供：
 * - 单例模式访问
 * - 连接/断开管理
 * - 定期状态轮询（QTimer）
 * - 命令发送接口
 * - 模拟模式支持（Windows 开发环境）
 *
 * 在 Windows 环境下使用模拟数据，实际 NML 连接留到 Linux 上集成。
 */

#ifndef LCNC_CORE_H
#define LCNC_CORE_H

#include <QObject>
#include <QTimer>
#include <QString>

#include "LcncStatusData.h"
#include "LcncCommand.h"

// ============================================================================
// 常量定义
// ============================================================================

/// 最大支持轴数
#define MAX_DRO_AXES 9

class LcncCore : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取 LcncCore 单例实例
     * @return 单例指针
     */
    static LcncCore *instance();

    /**
     * @brief 析构函数
     */
    ~LcncCore() override;

    // 禁止拷贝和移动
    LcncCore(const LcncCore &) = delete;
    LcncCore &operator=(const LcncCore &) = delete;
    LcncCore(LcncCore &&) = delete;
    LcncCore &operator=(LcncCore &&) = delete;

    // ========================================================================
    // 连接管理
    // ========================================================================

    /**
     * @brief 连接到 NML 服务器
     * @param iniFile INI 配置文件路径
     *
     * 在 Linux 上：读取 INI 文件，初始化 NML 通道
     * 在 Windows 上：进入模拟模式
     */
    void connectToServer(const QString &iniFile);

    /**
     * @brief 断开与 NML 服务器的连接
     */
    void disconnectFromServer();

    /**
     * @brief 是否已连接
     * @return 连接状态
     */
    bool isConnected() const;

    /**
     * @brief 是否处于模拟模式
     * @return 模拟模式标志
     */
    bool isSimulation() const;

    /**
     * @brief 启动模拟模式（不连接真实服务器）
     */
    void startSimulation();

    // ========================================================================
    // 机床控制命令
    // ========================================================================

    /**
     * @brief 发送急停命令
     */
    void sendEstop();

    /**
     * @brief 发送急停复位命令
     */
    void sendEstopReset();

    /**
     * @brief 发送机床开启命令
     */
    void sendMachineOn();

    /**
     * @brief 发送机床关闭命令
     */
    void sendMachineOff();

    /**
     * @brief 切换到自动模式
     */
    void sendModeAuto();

    /**
     * @brief 切换到手动模式
     */
    void sendModeManual();

    /**
     * @brief 切换到 MDI 模式
     */
    void sendModeMDI();

    /**
     * @brief 发送点动命令
     * @param axis 轴编号（0=X, 1=Y, 2=Z, ...）
     * @param velocity 点动速度（单位/秒）
     */
    void sendJog(int axis, double velocity);

    /**
     * @brief 停止点动
     * @param axis 轴编号
     */
    void sendJogStop(int axis);

    /**
     * @brief 发送连续点动命令
     * @param axis 轴编号
     * @param velocity 点动速度
     */
    void sendJogCont(int axis, double velocity);

    /**
     * @brief 发送增量点动命令
     * @param axis 轴编号
     * @param distance 移动距离
     * @param velocity 移动速度
     */
    void sendJogIncrement(int axis, double distance, double velocity);

    /**
     * @brief 发送 MDI 命令
     * @param cmd G 代码命令字符串（如 "G0 X10 Y20"）
     */
    void sendMdi(const QString &cmd);

    /**
     * @brief 打开 G 代码程序文件
     * @param filename 文件路径
     */
    void sendProgramOpen(const QString &filename);

    /**
     * @brief 开始运行程序
     */
    void sendProgramRun();

    /**
     * @brief 从指定行开始运行程序
     * @param line 起始行号
     */
    void sendProgramRun(int line);

    /**
     * @brief 暂停程序
     */
    void sendProgramPause();

    /**
     * @brief 恢复程序运行
     */
    void sendProgramResume();

    /**
     * @brief 停止程序
     */
    void sendProgramStop();

    /**
     * @brief 单步执行
     */
    void sendProgramStep();

    /**
     * @brief 回参考点
     * @param axis 轴编号（-1 表示全部轴）
     */
    void sendHome(int axis);

    // ========================================================================
    // 覆盖率控制
    // ========================================================================

    /**
     * @brief 设置进给倍率
     * @param rate 倍率值（0.0 ~ 2.0，1.0 = 100%）
     */
    void sendFeedOverride(double rate);

    /**
     * @brief 设置主轴倍率
     * @param rate 倍率值（0.0 ~ 2.0，1.0 = 100%）
     */
    void sendSpindleOverride(double rate);

    /**
     * @brief 设置最大进给速度
     * @param rate 最大进给速度
     */
    void sendMaxVelocity(double rate);

    // ========================================================================
    // 主轴控制
    // ========================================================================

    /**
     * @brief 主轴正转
     * @param speed 转速（RPM）
     */
    void sendSpindleForward(double speed);

    /**
     * @brief 主轴反转
     * @param speed 转速（RPM）
     */
    void sendSpindleReverse(double speed);

    /**
     * @brief 主轴停止
     */
    void sendSpindleStop();

    /**
     * @brief 主轴增加转速
     */
    void sendSpindleFaster();

    /**
     * @brief 主轴降低转速
     */
    void sendSpindleSlower();

    // ========================================================================
    // 冷却液控制
    // ========================================================================

    /**
     * @brief 开启冷却液（ Mist ）
     */
    void sendCoolantMistOn();

    /**
     * @brief 关闭冷却液（ Mist ）
     */
    void sendCoolantMistOff();

    /**
     * @brief 开启冷却液（ Flood ）
     */
    void sendCoolantFloodOn();

    /**
     * @brief 关闭冷却液（ Flood ）
     */
    void sendCoolantFloodOff();

    // ========================================================================
    // 状态查询
    // ========================================================================

    /**
     * @brief 获取最新的状态数据
     * @return 状态数据的常量引用
     */
    const LcncStatusData &statusData() const;

    /**
     * @brief 获取命令发送接口
     * @return LcncCommand 指针
     */
    LcncCommand *command() const;

signals:
    /**
     * @brief 状态数据更新信号
     * @param data 最新的机床状态数据
     */
    void statusUpdated(const LcncStatusData &data);

    /**
     * @brief 已连接到 NML 服务器
     */
    void connected();

    /**
     * @brief 已断开与 NML 服务器的连接
     */
    void disconnected();

    /**
     * @brief 错误发生
     * @param message 错误描述
     */
    void errorOccurred(const QString &message);

    /**
     * @brief 命令已发送
     * @param commandType 命令类型描述
     */
    void commandSent(const QString &commandType);

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    explicit LcncCore(QObject *parent = nullptr);

    // ========================================================================
    // 内部方法
    // ========================================================================

    /**
     * @brief 状态轮询定时器回调
     */
    void onPollTimer();

    /**
     * @brief 读取 NML 状态（Linux 实际实现）
     */
    void readNmlStatus();

    /**
     * @brief 生成模拟状态数据（Windows 开发环境）
     */
    void generateSimulatedStatus();

    /**
     * @brief 初始化模拟状态
     */
    void initSimulation();

private:
    // mapEmcStatToLcnc: Linux-only. Declaration in .cpp using forward void*.
    // On Windows, the Linux-only overload in .cpp is unreachable (m_nmlStat is nullptr).
    void mapEmcStatToLcnc(const void *s);

private:
    static LcncCore *m_instance;   ///< 单例实例指针

    QTimer *m_pollTimer = nullptr; ///< 状态轮询定时器
    LcncCommand *m_command = nullptr; ///< 命令发送器

    LcncStatusData m_status;       ///< 当前状态数据
    LcncStatusData m_prevStatus;   ///< 上一次状态数据（用于变化检测）

    bool m_connected = false;       ///< 连接标志
    bool m_simulation = false;     ///< 模拟模式标志
    QString m_iniFile;             ///< INI 文件路径

    // 模拟数据计数器
    int m_simStep = 0;             ///< 模拟步进计数
    double m_simTime = 0.0;        ///< 模拟时间

    // Linux NML 通道（仅在 Linux 构建时使用）
#ifdef Q_OS_LINUX
    void *m_nmlCmd = nullptr;         // RCS_CMD_CHANNEL*
    void *m_nmlStat = nullptr;         // RCS_STAT_CHANNEL*
    void *m_emcStat = nullptr;         // EMC_STAT*
#endif
};

#endif // LCNC_CORE_H
