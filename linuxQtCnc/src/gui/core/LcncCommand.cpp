/*
 * linuxQtCncGui - LinuxCNC Qt6 GUI 控制器
 * LcncCommand 实现
 *
 * 命令发送封装的实现。
 * Windows 环境下为接口桩实现，Linux 上集成真实 NML 命令发送。
 */

#include "LcncCommand.h"

#include <QDebug>

// ============================================================================
// 构造 / 析构
// ============================================================================

LcncCommand::LcncCommand(QObject *parent)
    : QObject(parent)
{
    qInfo().noquote() << "[LcncCommand] 命令接口已创建";
}

LcncCommand::~LcncCommand()
{
    qInfo().noquote() << "[LcncCommand] 命令接口已销毁";
}

// ============================================================================
// 内部命令发送
// ============================================================================

bool LcncCommand::sendCommand(const QString &commandName)
{
#ifdef Q_OS_LINUX
    // TODO: 在 Linux 上实现真实的 NML 命令发送
    //
    // 示例代码结构：
    //   if (!m_nmlCmd) {
    //       emit commandError(commandName, "NML 命令通道未初始化");
    //       return false;
    //   }
    //
    //   // 构造 NML 消息
    //   EMC_TASK_MODE_ENUM mode = EMC_TASK_MODE_AUTO;
    //   RCS_CMD_MSG msg;
    //   // ... 设置消息内容 ...
    //
    //   int result = m_nmlCmd->write(&msg);
    //   if (result <= 0) {
    //       emit commandError(commandName, "NML 写入失败");
    //       return false;
    //   }
    //
    //   emit commandSent(commandName, true);
    //   return true;
    //
    // 每种命令类型对应不同的 NML 消息结构：
    //   - EMC_TASK_ABORT         -> 急停
    //   - EMC_TASK_ESTOP         -> 急停
    //   - EMC_TASK_ESTOP_RESET   -> 急停复位
    //   - EMC_MACHINE_ON         -> 机床开启
    //   - EMC_MACHINE_OFF        -> 机床关闭
    //   - EMC_TASK_MODE_MANUAL   -> 手动模式
    //   - EMC_TASK_MODE_AUTO     -> 自动模式
    //   - EMC_TASK_MODE_MDI      -> MDI 模式
    //   - EMC_JOG_CONT           -> 连续点动
    //   - EMC_JOG_INCR           -> 增量点动
    //   - EMC_JOG_ABORT          -> 停止点动
    //   - EMC_TASK_PLAN_OPEN     -> 打开程序
    //   - EMC_TASK_PLAN_RUN      -> 运行程序
    //   - EMC_TASK_PLAN_PAUSE    -> 暂停
    //   - EMC_TASK_PLAN_RESUME   -> 恢复
    //   - EMC_TASK_PLAN_END      -> 停止
    //   - EMC_TASK_PLAN_STEP     -> 单步
    //   - EMC_SPINDLE_ON         -> 主轴开启
    //   - EMC_SPINDLE_OFF        -> 主轴关闭
    //   - EMC_COOLANT_MIST_ON    -> 雾化冷却开
    //   - EMC_COOLANT_FLOOD_ON   -> 喷流冷却开
    //   - EMC_TRAJ_SET_SCALE     -> 设置进给倍率
    //   - EMC_SPINDLE_SET_SCALE  -> 设置主轴倍率
    //   等等...

    qWarning().noquote() << "[LcncCommand] Linux NML 命令发送尚未实现:" << commandName;
    emit commandSent(commandName, false);
    return false;
#else
    // Windows 开发环境 - 仅记录日志
    qDebug().noquote() << "[LcncCommand] 模拟命令:" << commandName;
    emit commandSent(commandName, true);
    return true;
#endif
}

// ============================================================================
// 机床控制命令
// ============================================================================

bool LcncCommand::estop()
{
    return sendCommand(QStringLiteral("ESTOP"));
}

bool LcncCommand::estopReset()
{
    return sendCommand(QStringLiteral("ESTOP_RESET"));
}

bool LcncCommand::machineOn()
{
    return sendCommand(QStringLiteral("MACHINE_ON"));
}

bool LcncCommand::machineOff()
{
    return sendCommand(QStringLiteral("MACHINE_OFF"));
}

// ============================================================================
// 模式切换命令
// ============================================================================

bool LcncCommand::modeManual()
{
    return sendCommand(QStringLiteral("MODE_MANUAL"));
}

bool LcncCommand::modeAuto()
{
    return sendCommand(QStringLiteral("MODE_AUTO"));
}

bool LcncCommand::modeMDI()
{
    return sendCommand(QStringLiteral("MODE_MDI"));
}

// ============================================================================
// 点动命令
// ============================================================================

bool LcncCommand::jogCont(int axis, double velocity)
{
    return sendCommand(QStringLiteral("JOG_CONT axis=%1 vel=%2").arg(axis).arg(velocity));
}

bool LcncCommand::jogStop(int axis)
{
    return sendCommand(QStringLiteral("JOG_STOP axis=%1").arg(axis));
}

bool LcncCommand::jogIncrement(int axis, double distance, double velocity)
{
    return sendCommand(QStringLiteral("JOG_INCR axis=%1 dist=%2 vel=%3")
                       .arg(axis).arg(distance).arg(velocity));
}

// ============================================================================
// 程序控制命令
// ============================================================================

bool LcncCommand::programOpen(const QString &filename)
{
    return sendCommand(QStringLiteral("PROGRAM_OPEN: %1").arg(filename));
}

bool LcncCommand::programRun()
{
    return sendCommand(QStringLiteral("PROGRAM_RUN"));
}

bool LcncCommand::programRun(int line)
{
    return sendCommand(QStringLiteral("PROGRAM_RUN line=%1").arg(line));
}

bool LcncCommand::programPause()
{
    return sendCommand(QStringLiteral("PROGRAM_PAUSE"));
}

bool LcncCommand::programResume()
{
    return sendCommand(QStringLiteral("PROGRAM_RESUME"));
}

bool LcncCommand::programStop()
{
    return sendCommand(QStringLiteral("PROGRAM_STOP"));
}

bool LcncCommand::programStep()
{
    return sendCommand(QStringLiteral("PROGRAM_STEP"));
}

// ============================================================================
// MDI 命令
// ============================================================================

bool LcncCommand::mdi(const QString &command)
{
    return sendCommand(QStringLiteral("MDI: %1").arg(command));
}

// ============================================================================
// 回参考点命令
// ============================================================================

bool LcncCommand::home(int axis)
{
    return sendCommand(QStringLiteral("HOME axis=%1").arg(axis));
}

// ============================================================================
// 主轴控制命令
// ============================================================================

bool LcncCommand::spindleForward(double speed)
{
    return sendCommand(QStringLiteral("SPINDLE_FORWARD speed=%1").arg(speed));
}

bool LcncCommand::spindleReverse(double speed)
{
    return sendCommand(QStringLiteral("SPINDLE_REVERSE speed=%1").arg(speed));
}

bool LcncCommand::spindleStop()
{
    return sendCommand(QStringLiteral("SPINDLE_STOP"));
}

bool LcncCommand::spindleFaster()
{
    return sendCommand(QStringLiteral("SPINDLE_FASTER"));
}

bool LcncCommand::spindleSlower()
{
    return sendCommand(QStringLiteral("SPINDLE_SLOWER"));
}

bool LcncCommand::spindleConstantOn()
{
    return sendCommand(QStringLiteral("SPINDLE_CSS_ON"));
}

bool LcncCommand::spindleConstantOff()
{
    return sendCommand(QStringLiteral("SPINDLE_CSS_OFF"));
}

bool LcncCommand::spindleIncrease(double increment)
{
    return sendCommand(QStringLiteral("SPINDLE_INCREASE %1").arg(increment));
}

bool LcncCommand::spindleDecrease(double decrement)
{
    return sendCommand(QStringLiteral("SPINDLE_DECREASE %1").arg(decrement));
}

bool LcncCommand::spindleBrakeOn()
{
    return sendCommand(QStringLiteral("SPINDLE_BRAKE_ON"));
}

bool LcncCommand::spindleBrakeOff()
{
    return sendCommand(QStringLiteral("SPINDLE_BRAKE_OFF"));
}

// ============================================================================
// 冷却液控制命令
// ============================================================================

bool LcncCommand::coolantMistOn()
{
    return sendCommand(QStringLiteral("COOLANT_MIST_ON"));
}

bool LcncCommand::coolantMistOff()
{
    return sendCommand(QStringLiteral("COOLANT_MIST_OFF"));
}

bool LcncCommand::coolantFloodOn()
{
    return sendCommand(QStringLiteral("COOLANT_FLOOD_ON"));
}

bool LcncCommand::coolantFloodOff()
{
    return sendCommand(QStringLiteral("COOLANT_FLOOD_OFF"));
}

// ============================================================================
// 覆盖率命令
// ============================================================================

bool LcncCommand::feedOverride(double rate)
{
    return sendCommand(QStringLiteral("FEED_OVERRIDE %1").arg(rate));
}

bool LcncCommand::spindleOverride(double rate)
{
    return sendCommand(QStringLiteral("SPINDLE_OVERRIDE %1").arg(rate));
}

bool LcncCommand::maxVelocity(double velocity)
{
    return sendCommand(QStringLiteral("MAX_VELOCITY %1").arg(velocity));
}

bool LcncCommand::rapidOverride(double rate)
{
    return sendCommand(QStringLiteral("RAPID_OVERRIDE %1").arg(rate));
}

// ============================================================================
// 刀具命令
// ============================================================================

bool LcncCommand::toolChange(int pocket)
{
    return sendCommand(QStringLiteral("TOOL_CHANGE pocket=%1").arg(pocket));
}

bool LcncCommand::toolChangeManual(int toolNo)
{
    return sendCommand(QStringLiteral("TOOL_CHANGE_MANUAL tool=%1").arg(toolNo));
}

// ============================================================================
// 坐标系命令
// ============================================================================

bool LcncCommand::setWorkCoordinate(int axis, double value, int coordinateSystem)
{
    return sendCommand(QStringLiteral("SET_WORK_COORD axis=%1 value=%2 coord_sys=%3")
                       .arg(axis).arg(value).arg(coordinateSystem));
}
