/********************************************************************
* Description: LcncHalMonitor.h
*   LcncHalMonitor — HAL Signal Monitor for linuxQtCnc.
*   Encapsulates reading and monitoring of HAL signal values.
*   On Windows, simulated data is used; on Linux, real HAL
*   communication is integrated via shared memory.
*   On Linux, HAL signals are accessed via shared memory:
*   1. Load HAL library (hal_lib.so)
*   2. Read signal values via the HAL API
*   3. Use a timer for periodic polling updates
*
*   Derived from a work by Fred Proctor & Will Shackleford
*
* Author: linuxQtCnc project
* License: GPL Version 2
* System: Linux / Windows
*
* Copyright (c) 2026 All rights reserved.
********************************************************************/

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