/*
 * linuxQtCncGui - LinuxCNC Qt5 GUI 控制器
 * LcncCommand 实现
 *
 * 命令发送封装的实现。
 * Windows 环境下为接口桩实现，Linux 上集成真实 NML 命令发送。
 */

#include "LcncCommand.h"

#include <QDebug>
#include <cmath>
#include <cstring>

// ============================================================================
// Linux NML 头文件（仅在 Linux 下编译）
// ============================================================================
#ifdef Q_OS_LINUX
#include "emc.hh"
#include "emc_nml.hh"
#include "rcs.hh"
#endif

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
    qDebug().noquote() << "[LcncCommand] NML 命令:" << commandName;
#else
    qDebug().noquote() << "[LcncCommand] 模拟命令:" << commandName;
#endif
    emit commandSent(commandName, true);
    return true;
}

// ============================================================================
// 机床控制命令
// ============================================================================

bool LcncCommand::estop()
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("ESTOP"), "NML 命令通道未初始化");
        return false;
    }
    EMC_TASK_ESTOP emc_cmd;
    emc_cmd.mode = 0;
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("ESTOP"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: ESTOP";
    return true;
#else
    return sendCommand(QStringLiteral("ESTOP"));
#endif
}

bool LcncCommand::estopReset()
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("ESTOP_RESET"), "NML 命令通道未初始化");
        return false;
    }
    EMC_TASK_ESTOP_RESET emc_cmd;
    emc_cmd.mode = 0;
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("ESTOP_RESET"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: ESTOP_RESET";
    return true;
#else
    return sendCommand(QStringLiteral("ESTOP_RESET"));
#endif
}

bool LcncCommand::machineOn()
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("MACHINE_ON"), "NML 命令通道未初始化");
        return false;
    }
    EMC_TASK_SET_STATE emc_cmd;
    emc_cmd.state = EMC_TASK_STATE_ON;
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("MACHINE_ON"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: MACHINE_ON";
    return true;
#else
    return sendCommand(QStringLiteral("MACHINE_ON"));
#endif
}

bool LcncCommand::machineOff()
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("MACHINE_OFF"), "NML 命令通道未初始化");
        return false;
    }
    EMC_TASK_SET_STATE emc_cmd;
    emc_cmd.state = EMC_TASK_STATE_OFF;
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("MACHINE_OFF"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: MACHINE_OFF";
    return true;
#else
    return sendCommand(QStringLiteral("MACHINE_OFF"));
#endif
}

// ============================================================================
// 模式切换命令
// ============================================================================

bool LcncCommand::modeManual()
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("MODE_MANUAL"), "NML 命令通道未初始化");
        return false;
    }
    EMC_TASK_SET_MODE emc_cmd;
    emc_cmd.mode = EMC_TASK_MODE_MANUAL;
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("MODE_MANUAL"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: MODE_MANUAL";
    return true;
#else
    return sendCommand(QStringLiteral("MODE_MANUAL"));
#endif
}

bool LcncCommand::modeAuto()
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("MODE_AUTO"), "NML 命令通道未初始化");
        return false;
    }
    EMC_TASK_SET_MODE emc_cmd;
    emc_cmd.mode = EMC_TASK_MODE_AUTO;
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("MODE_AUTO"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: MODE_AUTO";
    return true;
#else
    return sendCommand(QStringLiteral("MODE_AUTO"));
#endif
}

bool LcncCommand::modeMDI()
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("MODE_MDI"), "NML 命令通道未初始化");
        return false;
    }
    EMC_TASK_SET_MODE emc_cmd;
    emc_cmd.mode = EMC_TASK_MODE::MDI;
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("MODE_MDI"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: MODE_MDI";
    return true;
#else
    return sendCommand(QStringLiteral("MODE_MDI"));
#endif
}

// ============================================================================
// 点动命令
// ============================================================================

bool LcncCommand::jogCont(int axis, double velocity)
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("JOG_CONT"), "NML 命令通道未初始化");
        return false;
    }
    EMC_TRAJ_JOG_CONT emc_cmd;
    emc_cmd.axis = axis;
    emc_cmd.vel = velocity;
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("JOG_CONT"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: JOG_CONT axis=" << axis << " vel=" << velocity;
    return true;
#else
    return sendCommand(QStringLiteral("JOG_CONT axis=%1 vel=%2").arg(axis).arg(velocity));
#endif
}

bool LcncCommand::jogStop(int axis)
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("JOG_STOP"), "NML 命令通道未初始化");
        return false;
    }
    EMC_JOG_STOP emc_cmd;
    emc_cmd.joint_or_axis = axis;
    emc_cmd.jjogmode = 0;
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("JOG_STOP"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: JOG_STOP axis=" << axis;
    return true;
#else
    return sendCommand(QStringLiteral("JOG_STOP axis=%1").arg(axis));
#endif
}

bool LcncCommand::jogIncrement(int axis, double distance, double velocity)
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("JOG_INCR"), "NML 命令通道未初始化");
        return false;
    }
    EMC_TRAJ_JOG_INCR emc_cmd;
    emc_cmd.axis = axis;
    emc_cmd.vel = velocity;
    emc_cmd.incr = distance;
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("JOG_INCR"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: JOG_INCR axis=" << axis << " dist=" << distance << " vel=" << velocity;
    return true;
#else
    return sendCommand(QStringLiteral("JOG_INCR axis=%1 dist=%2 vel=%3")
                       .arg(axis).arg(distance).arg(velocity));
#endif
}

// ============================================================================
// 程序控制命令
// ============================================================================

bool LcncCommand::programOpen(const QString &filename)
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("PROGRAM_OPEN"), "NML 命令通道未初始化");
        return false;
    }
    EMC_TASK_OPEN_PROGRAM emc_cmd;
    // 复制文件名到命令结构
    strncpy(emc_cmd.file, filename.toLocal8Bit().constData(), sizeof(emc_cmd.file) - 1);
    emc_cmd.file[sizeof(emc_cmd.file) - 1] = '\0';
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("PROGRAM_OPEN"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: PROGRAM_OPEN" << filename;
    return true;
#else
    return sendCommand(QStringLiteral("PROGRAM_OPEN: %1").arg(filename));
#endif
}

bool LcncCommand::programRun()
{
    return programRun(-1);  // -1 表示从头开始
}

bool LcncCommand::programRun(int line)
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("PROGRAM_RUN"), "NML 命令通道未初始化");
        return false;
    }
    EMC_TASK_PLAN_RUN emc_cmd;
    emc_cmd.line = line;
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("PROGRAM_RUN"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: PROGRAM_RUN line=" << line;
    return true;
#else
    if (line < 0) {
        return sendCommand(QStringLiteral("PROGRAM_RUN"));
    } else {
        return sendCommand(QStringLiteral("PROGRAM_RUN line=%1").arg(line));
    }
#endif
}

bool LcncCommand::programPause()
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("PROGRAM_PAUSE"), "NML 命令通道未初始化");
        return false;
    }
    EMC_TASK_PAUSE emc_cmd;
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("PROGRAM_PAUSE"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: PROGRAM_PAUSE";
    return true;
#else
    return sendCommand(QStringLiteral("PROGRAM_PAUSE"));
#endif
}

bool LcncCommand::programResume()
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("PROGRAM_RESUME"), "NML 命令通道未初始化");
        return false;
    }
    EMC_TASK_RESUME emc_cmd;
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("PROGRAM_RESUME"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: PROGRAM_RESUME";
    return true;
#else
    return sendCommand(QStringLiteral("PROGRAM_RESUME"));
#endif
}

bool LcncCommand::programStop()
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("PROGRAM_STOP"), "NML 命令通道未初始化");
        return false;
    }
    EMC_TASK_STOP emc_cmd;
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("PROGRAM_STOP"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: PROGRAM_STOP";
    return true;
#else
    return sendCommand(QStringLiteral("PROGRAM_STOP"));
#endif
}

bool LcncCommand::programStep()
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("PROGRAM_STEP"), "NML 命令通道未初始化");
        return false;
    }
    EMC_TASK_STEP emc_cmd;
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("PROGRAM_STEP"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: PROGRAM_STEP";
    return true;
#else
    return sendCommand(QStringLiteral("PROGRAM_STEP"));
#endif
}

// ============================================================================
// MDI 命令
// ============================================================================

bool LcncCommand::mdi(const QString &command)
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("MDI"), "NML 命令通道未初始化");
        return false;
    }
    EMC_TASK_PLAN_EXECUTE emc_cmd;
    // 复制命令字符串到命令结构
    strncpy(emc_cmd.command, command.toLocal8Bit().constData(), sizeof(emc_cmd.command) - 1);
    emc_cmd.command[sizeof(emc_cmd.command) - 1] = '\0';
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("MDI"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: MDI" << command;
    return true;
#else
    return sendCommand(QStringLiteral("MDI: %1").arg(command));
#endif
}

// ============================================================================
// 回参考点命令
// ============================================================================

bool LcncCommand::home(int axis)
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("HOME"), "NML 命令通道未初始化");
        return false;
    }
    // 全部回零（axis == -1）时对每个关节发送；否则发送单关节回零
    if (axis == -1) {
        // 遍历所有轴（默认最多9轴）
        bool ok = true;
        for (int i = 0; i < 9; ++i) {
            EMC_JOINT_HOME emc_cmd;
            emc_cmd.joint = i;
            if (static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd) != 0) {
                ok = false;
                break;
            }
        }
        if (!ok) {
            emit commandError(QStringLiteral("HOME"), "NML 写入失败");
            return false;
        }
    } else {
        EMC_JOINT_HOME emc_cmd;
        emc_cmd.joint = axis;
        int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
        if (result != 0) {
            emit commandError(QStringLiteral("HOME"), QString("NML 写入失败: %1").arg(result));
            return false;
        }
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: HOME axis=" << axis;
    return true;
#else
    return sendCommand(QStringLiteral("HOME axis=%1").arg(axis));
#endif
}

// ============================================================================
// 主轴控制命令
// ============================================================================

bool LcncCommand::spindleForward(double speed)
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("SPINDLE_FORWARD"), "NML 命令通道未初始化");
        return false;
    }
    EMC_SPINDLE_ON emc_cmd;
    emc_cmd.speed = static_cast<double>(speed);
    emc_cmd.dir = 1;  // 正转
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("SPINDLE_FORWARD"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: SPINDLE_FORWARD speed=" << speed;
    return true;
#else
    return sendCommand(QStringLiteral("SPINDLE_FORWARD speed=%1").arg(speed));
#endif
}

bool LcncCommand::spindleReverse(double speed)
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("SPINDLE_REVERSE"), "NML 命令通道未初始化");
        return false;
    }
    EMC_SPINDLE_ON emc_cmd;
    emc_cmd.speed = static_cast<double>(-std::abs(speed));  // 负值表示反转
    emc_cmd.dir = -1;  // 反转
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("SPINDLE_REVERSE"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: SPINDLE_REVERSE speed=" << speed;
    return true;
#else
    return sendCommand(QStringLiteral("SPINDLE_REVERSE speed=%1").arg(speed));
#endif
}

bool LcncCommand::spindleStop()
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("SPINDLE_STOP"), "NML 命令通道未初始化");
        return false;
    }
    EMC_SPINDLE_OFF emc_cmd;
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("SPINDLE_STOP"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: SPINDLE_STOP";
    return true;
#else
    return sendCommand(QStringLiteral("SPINDLE_STOP"));
#endif
}

bool LcncCommand::spindleFaster()
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("SPINDLE_FASTER"), "NML 命令通道未初始化");
        return false;
    }
    EMC_SPINDLE_INCREASE emc_cmd;
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("SPINDLE_FASTER"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: SPINDLE_INCREASE";
    return true;
#else
    return sendCommand(QStringLiteral("SPINDLE_FASTER"));
#endif
}

bool LcncCommand::spindleSlower()
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("SPINDLE_SLOWER"), "NML 命令通道未初始化");
        return false;
    }
    EMC_SPINDLE_DECREASE emc_cmd;
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("SPINDLE_SLOWER"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: SPINDLE_DECREASE";
    return true;
#else
    return sendCommand(QStringLiteral("SPINDLE_SLOWER"));
#endif
}

bool LcncCommand::spindleConstantOn()
{
    // LinuxCNC 没有直接的恒速命令，这个实现使用进给倍率模拟
    return feedOverride(1.0);
}

bool LcncCommand::spindleConstantOff()
{
    return spindleOverride(1.0);
}

bool LcncCommand::spindleIncrease(double increment)
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("SPINDLE_INCREASE"), "NML 命令通道未初始化");
        return false;
    }
    EMC_SPINDLE_INCREASE emc_cmd;
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("SPINDLE_INCREASE"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: SPINDLE_INCREASE";
    return true;
#else
    return sendCommand(QStringLiteral("SPINDLE_INCREASE %1").arg(increment));
#endif
}

bool LcncCommand::spindleDecrease(double decrement)
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("SPINDLE_DECREASE"), "NML 命令通道未初始化");
        return false;
    }
    EMC_SPINDLE_DECREASE emc_cmd;
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("SPINDLE_DECREASE"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: SPINDLE_DECREASE";
    return true;
#else
    return sendCommand(QStringLiteral("SPINDLE_DECREASE %1").arg(decrement));
#endif
}

bool LcncCommand::spindleBrakeOn()
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("SPINDLE_BRAKE_ON"), "NML 命令通道未初始化");
        return false;
    }
    EMC_SPINDLE_BRAKE_ENGAGE emc_cmd;
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("SPINDLE_BRAKE_ON"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: SPINDLE_BRAKE_ENGAGE";
    return true;
#else
    return sendCommand(QStringLiteral("SPINDLE_BRAKE_ON"));
#endif
}

bool LcncCommand::spindleBrakeOff()
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("SPINDLE_BRAKE_OFF"), "NML 命令通道未初始化");
        return false;
    }
    EMC_SPINDLE_BRAKE_RELEASE emc_cmd;
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("SPINDLE_BRAKE_OFF"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: SPINDLE_BRAKE_RELEASE";
    return true;
#else
    return sendCommand(QStringLiteral("SPINDLE_BRAKE_OFF"));
#endif
}

// ============================================================================
// 冷却液控制命令
// ============================================================================

bool LcncCommand::coolantMistOn()
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("COOLANT_MIST_ON"), "NML 命令通道未初始化");
        return false;
    }
    EMC_COOLANT_MIST_ON emc_cmd;
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("COOLANT_MIST_ON"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: COOLANT_MIST_ON";
    return true;
#else
    return sendCommand(QStringLiteral("COOLANT_MIST_ON"));
#endif
}

bool LcncCommand::coolantMistOff()
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("COOLANT_MIST_OFF"), "NML 命令通道未初始化");
        return false;
    }
    EMC_COOLANT_MIST_OFF emc_cmd;
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("COOLANT_MIST_OFF"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: COOLANT_MIST_OFF";
    return true;
#else
    return sendCommand(QStringLiteral("COOLANT_MIST_OFF"));
#endif
}

bool LcncCommand::coolantFloodOn()
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("COOLANT_FLOOD_ON"), "NML 命令通道未初始化");
        return false;
    }
    EMC_COOLANT_FLOOD_ON emc_cmd;
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("COOLANT_FLOOD_ON"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: COOLANT_FLOOD_ON";
    return true;
#else
    return sendCommand(QStringLiteral("COOLANT_FLOOD_ON"));
#endif
}

bool LcncCommand::coolantFloodOff()
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("COOLANT_FLOOD_OFF"), "NML 命令通道未初始化");
        return false;
    }
    EMC_COOLANT_FLOOD_OFF emc_cmd;
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("COOLANT_FLOOD_OFF"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: COOLANT_FLOOD_OFF";
    return true;
#else
    return sendCommand(QStringLiteral("COOLANT_FLOOD_OFF"));
#endif
}

// ============================================================================
// 覆盖率命令
// ============================================================================

bool LcncCommand::feedOverride(double rate)
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("FEED_OVERRIDE"), "NML 命令通道未初始化");
        return false;
    }
    EMC_TRAJ_SET_SCALE emc_cmd;
    emc_cmd.scale = static_cast<double>(rate);
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("FEED_OVERRIDE"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: FEED_OVERRIDE" << rate;
    return true;
#else
    return sendCommand(QStringLiteral("FEED_OVERRIDE %1").arg(rate));
#endif
}

bool LcncCommand::spindleOverride(double rate)
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("SPINDLE_OVERRIDE"), "NML 命令通道未初始化");
        return false;
    }
    EMC_TRAJ_SET_SPINDLE_SCALE emc_cmd;
    emc_cmd.scale = static_cast<double>(rate);
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("SPINDLE_OVERRIDE"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: SPINDLE_OVERRIDE" << rate;
    return true;
#else
    return sendCommand(QStringLiteral("SPINDLE_OVERRIDE %1").arg(rate));
#endif
}

bool LcncCommand::maxVelocity(double velocity)
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("MAX_VELOCITY"), "NML 命令通道未初始化");
        return false;
    }
    EMC_TRAJ_SET_MAX_VELOCITY emc_cmd;
    emc_cmd.velocity = static_cast<double>(velocity);
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("MAX_VELOCITY"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: MAX_VELOCITY" << velocity;
    return true;
#else
    return sendCommand(QStringLiteral("MAX_VELOCITY %1").arg(velocity));
#endif
}

bool LcncCommand::rapidOverride(double rate)
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("RAPID_OVERRIDE"), "NML 命令通道未初始化");
        return false;
    }
    EMC_TRAJ_SET_RAPID_SCALE emc_cmd;
    emc_cmd.scale = static_cast<double>(rate);
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("RAPID_OVERRIDE"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: RAPID_OVERRIDE" << rate;
    return true;
#else
    return sendCommand(QStringLiteral("RAPID_OVERRIDE %1").arg(rate));
#endif
}

// ============================================================================
// 刀具命令
// ============================================================================

bool LcncCommand::toolChange(int pocket)
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("TOOL_CHANGE"), "NML 命令通道未初始化");
        return false;
    }
    EMC_TOOL_LOAD emc_cmd;
    emc_cmd.pocket = pocket;
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("TOOL_CHANGE"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: TOOL_CHANGE pocket=" << pocket;
    return true;
#else
    return sendCommand(QStringLiteral("TOOL_CHANGE pocket=%1").arg(pocket));
#endif
}

bool LcncCommand::toolChangeManual(int toolNo)
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("TOOL_CHANGE_MANUAL"), "NML 命令通道未初始化");
        return false;
    }
    EMC_TOOL_LOAD emc_cmd;
    emc_cmd.pocket = toolNo;
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("TOOL_CHANGE_MANUAL"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: TOOL_CHANGE_MANUAL tool=" << toolNo;
    return true;
#else
    return sendCommand(QStringLiteral("TOOL_CHANGE_MANUAL tool=%1").arg(toolNo));
#endif
}

// ============================================================================
// 坐标系命令
// ============================================================================

bool LcncCommand::setWorkCoordinate(int axis, double value, int coordinateSystem)
{
#ifdef Q_OS_LINUX
    if (!m_nmlCmd) {
        emit commandError(QStringLiteral("SET_WORK_COORD"), "NML 命令通道未初始化");
        return false;
    }
    EMC_TRAJ_SET_G5X emc_cmd;
    // coordinateSystem: 1=G54, 2=G55, ... 6=G59
    // axis: 0=X, 1=Y, 2=Z, 3=A, 4=B, 5=C
    int g5x_index = coordinateSystem - 1;  // 转为 0 基索引
    if (g5x_index < 0 || g5x_index >= 6 || axis < 0 || axis > 5) {
        emit commandError(QStringLiteral("SET_WORK_COORD"), "无效的坐标系或轴编号");
        return false;
    }
    emc_cmd.g5x_index = g5x_index;
    // 初始化所有轴偏移
    emc_cmd.origin.tran.x = 0.0;
    emc_cmd.origin.tran.y = 0.0;
    emc_cmd.origin.tran.z = 0.0;
    emc_cmd.origin.a = 0.0;
    emc_cmd.origin.b = 0.0;
    emc_cmd.origin.c = 0.0;
    // 设置目标轴偏移 (0:x, 1:y, 2:z, 3:a, 4:b, 5:c)
    switch (axis) {
    case 0: emc_cmd.origin.tran.x = value; break;
    case 1: emc_cmd.origin.tran.y = value; break;
    case 2: emc_cmd.origin.tran.z = value; break;
    case 3: emc_cmd.origin.a = value; break;
    case 4: emc_cmd.origin.b = value; break;
    case 5: emc_cmd.origin.c = value; break;
    }
    int result = static_cast<RCS_CMD_CHANNEL *>(m_nmlCmd)->write(emc_cmd);
    if (result != 0) {
        emit commandError(QStringLiteral("SET_WORK_COORD"), QString("NML 写入失败: %1").arg(result));
        return false;
    }
    qInfo().noquote() << "[LcncCommand] NML 发送: SET_WORK_COORD axis=" << axis
                      << " value=" << value << " coord_sys=" << coordinateSystem;
    return true;
#else
    return sendCommand(QStringLiteral("SET_WORK_COORD axis=%1 value=%2 coord_sys=%3")
                       .arg(axis).arg(value).arg(coordinateSystem));
#endif
}

// ============================================================================
// Linux NML 通道设置
// ============================================================================
#ifdef Q_OS_LINUX
void LcncCommand::setNmlChannel(void *ch)
{
    m_nmlCmd = ch;
    qInfo() << "[LcncCommand] NML 命令通道已设置:" << (ch ? "OK" : "NULL");
}
#endif