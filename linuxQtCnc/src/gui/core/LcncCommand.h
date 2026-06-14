/********************************************************************
* Description: LcncCommand.h
*   LcncCommand - NML Command Interface for linuxQtCnc.
*   Encapsulates the construction and sending of NML command
*   messages, providing a type-safe command API.
*   On Windows this provides the interface definition; the actual
*   NML command sending is integrated on Linux.
*
*   Derived from a work by Fred Proctor & Will Shackleford
*
* Author: linuxQtCnc project
* License: GPL Version 2
* System: Linux / Windows
*
* Copyright (c) 2026 All rights reserved.
********************************************************************/

#ifndef LCNC_COMMAND_H
#define LCNC_COMMAND_H

#include <QObject>
#include <QString>

class LcncCore;

/**
 * @brief LcncCommand - CNC 命令发送封装
 *
 * 负责将高层命令转换为 NML 消息格式并发送。
 * 每个命令方法对应一个 NML 命令类型。
 *
 * 在 Windows 开发环境下，命令仅记录日志；
 * 在 Linux 上集成时，通过 NML 通道发送实际命令。
 */
class LcncCommand : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象（通常为 LcncCore）
     */
    explicit LcncCommand(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~LcncCommand() override;

    // ========================================================================
    // 机床控制命令
    // ========================================================================

    /**
     * @brief 发送急停命令
     * @return true 命令已发送
     */
    bool estop();

    /**
     * @brief 发送急停复位命令
     * @return true 命令已发送
     */
    bool estopReset();

    /**
     * @brief 发送机床开启命令
     * @return true 命令已发送
     */
    bool machineOn();

    /**
     * @brief 发送机床关闭命令
     * @return true 命令已发送
     */
    bool machineOff();

    // ========================================================================
    // 模式切换命令
    // ========================================================================

    /**
     * @brief 切换到手动模式
     * @return true 命令已发送
     */
    bool modeManual();

    /**
     * @brief 切换到自动模式
     * @return true 命令已发送
     */
    bool modeAuto();

    /**
     * @brief 切换到 MDI 模式
     * @return true 命令已发送
     */
    bool modeMDI();

    // ========================================================================
    // 点动命令
    // ========================================================================

    /**
     * @brief 连续点动
     * @param axis 轴编号
     * @param velocity 速度
     * @return true 命令已发送
     */
    bool jogCont(int axis, double velocity);

    /**
     * @brief 停止点动
     * @param axis 轴编号
     * @return true 命令已发送
     */
    bool jogStop(int axis);

    /**
     * @brief 增量点动
     * @param axis 轴编号
     * @param distance 移动距离
     * @param velocity 移动速度
     * @return true 命令已发送
     */
    bool jogIncrement(int axis, double distance, double velocity);

    // ========================================================================
    // 程序控制命令
    // ========================================================================

    /**
     * @brief 打开程序文件
     * @param filename 文件路径
     * @return true 命令已发送
     */
    bool programOpen(const QString &filename);

    /**
     * @brief 运行程序
     * @return true 命令已发送
     */
    bool programRun();

    /**
     * @brief 从指定行运行程序
     * @param line 起始行号
     * @return true 命令已发送
     */
    bool programRun(int line);

    /**
     * @brief 暂停程序
     * @return true 命令已发送
     */
    bool programPause();

    /**
     * @brief 恢复程序
     * @return true 命令已发送
     */
    bool programResume();

    /**
     * @brief 停止程序
     * @return true 命令已发送
     */
    bool programStop();

    /**
     * @brief 单步执行
     * @return true 命令已发送
     */
    bool programStep();

    // ========================================================================
    // MDI 命令
    // ========================================================================

    /**
     * @brief 发送 MDI 命令
     * @param command G 代码命令字符串
     * @return true 命令已发送
     */
    bool mdi(const QString &command);

    // ========================================================================
    // 回参考点命令
    // ========================================================================

    /**
     * @brief 回参考点
     * @param axis 轴编号（-1 表示全部轴）
     * @return true 命令已发送
     */
    bool home(int axis);

    // ========================================================================
    // 主轴控制命令
    // ========================================================================

    /**
     * @brief 主轴正转
     * @param speed 转速（RPM），0 表示使用当前设定值
     * @return true 命令已发送
     */
    bool spindleForward(double speed = 0.0);

    /**
     * @brief 主轴反转
     * @param speed 转速（RPM），0 表示使用当前设定值
     * @return true 命令已发送
     */
    bool spindleReverse(double speed = 0.0);

    /**
     * @brief 主轴停止
     * @return true 命令已发送
     */
    bool spindleStop();

    /**
     * @brief 主轴加速
     * @return true 命令已发送
     */
    bool spindleFaster();

    /**
     * @brief 主轴减速
     * @return true 命令已发送
     */
    bool spindleSlower();

    /**
     * @brief 主轴恒速控制开启
     * @return true 命令已发送
     */
    bool spindleConstantOn();

    /**
     * @brief 主轴恒速控制关闭
     * @return true 命令已发送
     */
    bool spindleConstantOff();

    /**
     * @brief 主轴增加转速
     * @param increment 增量（RPM）
     * @return true 命令已发送
     */
    bool spindleIncrease(double increment);

    /**
     * @brief 主轴降低转速
     * @param decrement 减量（RPM）
     * @return true 命令已发送
     */
    bool spindleDecrease(double decrement);

    /**
     * @brief 主轴制动
     * @return true 命令已发送
     */
    bool spindleBrakeOn();

    /**
     * @brief 释放主轴制动
     * @return true 命令已发送
     */
    bool spindleBrakeOff();

    // ========================================================================
    // 冷却液控制命令
    // ========================================================================

    /**
     * @brief 开启雾化冷却
     * @return true 命令已发送
     */
    bool coolantMistOn();

    /**
     * @brief 关闭雾化冷却
     * @return true 命令已发送
     */
    bool coolantMistOff();

    /**
     * @brief 开启喷流冷却
     * @return true 命令已发送
     */
    bool coolantFloodOn();

    /**
     * @brief 关闭喷流冷却
     * @return true 命令已发送
     */
    bool coolantFloodOff();

    // ========================================================================
    // 覆盖率命令
    // ========================================================================

    /**
     * @brief 设置进给倍率
     * @param rate 倍率（0.0 ~ 2.0）
     * @return true 命令已发送
     */
    bool feedOverride(double rate);

    /**
     * @brief 设置主轴倍率
     * @param rate 倍率（0.0 ~ 2.0）
     * @return true 命令已发送
     */
    bool spindleOverride(double rate);

    /**
     * @brief 设置最大进给速度
     * @param velocity 最大速度
     * @return true 命令已发送
     */
    bool maxVelocity(double velocity);

    /**
     * @brief 设置快移倍率
     * @param rate 倍率（0.0 ~ 1.0）
     * @return true 命令已发送
     */
    bool rapidOverride(double rate);

    // ========================================================================
    // 刀具命令
    // ========================================================================

    /**
     * @brief 换刀（自动）
     * @param pocket 刀位编号
     * @return true 命令已发送
     */
    bool toolChange(int pocket);

    /**
     * @brief 换刀（手动）
     * @param toolNo 刀具编号
     * @return true 命令已发送
     */
    bool toolChangeManual(int toolNo);

    // ========================================================================
    // 坐标系命令
    // ========================================================================

    /**
     * @brief 设置工件坐标系原点（G10 L2）
     * @param axis 轴编号
     * @param value 坐标值
     * @param coordinateSystem 坐标系编号（G54=1, G55=2, ...）
     * @return true 命令已发送
     */
    bool setWorkCoordinate(int axis, double value, int coordinateSystem = 1);

signals:
    /**
     * @brief 命令已发送信号
     * @param commandName 命令名称
     * @param success 是否成功
     */
    void commandSent(const QString &commandName, bool success);

    /**
     * @brief 命令错误信号
     * @param commandName 命令名称
     * @param errorMessage 错误描述
     */
    void commandError(const QString &commandName, const QString &errorMessage);

private:
    /**
     * @brief 内部命令发送方法
     * @param commandName 命令名称（用于日志和信号）
     * @return true 命令已发送
     */
    bool sendCommand(const QString &commandName);

public:
    // ========================================================================
    // Linux NML 通道设置（仅 Linux 构建）
    // ========================================================================
#ifdef Q_OS_LINUX
    /**
     * @brief 设置 NML 命令通道
     * @param ch 命令通道指针（由 LcncCore 初始化后传入）
     */
    void setNmlChannel(void *ch);
#else
    void setNmlChannel(void *ch) { (void)ch; }
#endif

#ifdef Q_OS_LINUX
    void *m_nmlCmd = nullptr;  // RCS_CMD_CHANNEL*
#endif
};

#endif // LCNC_COMMAND_H
