/*
 * linuxQtCncGui - LinuxCNC Qt5 GUI 控制器
 * LcncCore 实现
 *
 * NML 通信封装的核心实现。
 * Windows 环境下使用模拟数据，Linux 上集成真实 NML 通信。
 */
#include "LcncCore.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDateTime>
#include <QTimer>
#include <cmath>

// ============================================================================
// Linux NML 头文件（仅在 Linux 下编译）
// ============================================================================
#ifdef Q_OS_LINUX
#include "emc_nml.hh"
#include "emc.hh"
#include "inifile.hh"
#include "rcs.hh"
#endif

// ============================================================================
// 单例实现
// ============================================================================

LcncCore *LcncCore::m_instance = nullptr;

LcncCore *LcncCore::instance()
{
    if (!m_instance) {
        m_instance = new LcncCore();
    }
    return m_instance;
}

LcncCore::LcncCore(QObject *parent)
    : QObject(parent)
    , m_pollTimer(new QTimer(this))
    , m_command(new LcncCommand(this))
{
    // 初始化轴状态（默认 3 轴：X/Y/Z）
    m_status.axes.resize(9);
    for (int i = 0; i < 9; ++i) {
        m_status.axes[i].minPosition = -1000.0;
        m_status.axes[i].maxPosition = 1000.0;
        m_status.axes[i].enabled = (i < 3);  // 默认只使能 XYZ
    }

    // 连接轮询定时器
    connect(m_pollTimer, &QTimer::timeout, this, &LcncCore::onPollTimer);

    qInfo().noquote() << "[LcncCore] 核心单例已创建";
}

LcncCore::~LcncCore()
{
    disconnectFromServer();
    m_instance = nullptr;
    qInfo().noquote() << "[LcncCore] 核心单例已销毁";
}

// ============================================================================
// 连接管理
// ============================================================================

void LcncCore::connectToServer(const QString &iniFile)
{
    if (m_connected) {
        qWarning().noquote() << "[LcncCore] 已经连接，先断开再重连";
        disconnectFromServer();
    }

    m_iniFile = iniFile;

#ifdef Q_OS_LINUX
    // ============================================================
    // Linux: 真实 NML 连接
    // ============================================================
    // 步骤 1: 解析 INI 文件获取 NML 配置
    INIFILE ini;
    if (ini.Open(iniFile.toLocal8Bit().constData()) != 0) {
        QString err = QString("无法打开 INI 文件: %1").arg(iniFile);
        qCritical().noquote() << "[LcncCore]" << err;
        emit errorOccurred(err);
        // 回退到模拟模式
        startSimulation();
        return;
    }

    // 步骤 2: 读取 NML 配置文件路径（默认从 INI 查找 [EMC] NML_FILE）
    QString nmlFile;
    const char *nmlSection = "EMC";
    char buf[LINELEN];
    if (ini.Find(nmlSection, "NML_FILE", 1)) {
        iniString(nmlFile, buf, sizeof(buf));
        qInfo() << "[LcncCore] NML 文件:" << nmlFile;
    } else {
        nmlFile = "/etc/linuxcnc/linuxcnc.nml";
        qInfo() << "[LcncCore] 使用默认 NML 文件:" << nmlFile;
    }

    // 步骤 3: 创建 NML 命令通道
    // 默认 emcChannelType = "xemc"
    const char *channelType = "xemc";
    if (ini.Find(nmlSection, "CHANNEL", 1)) {
        iniString(buf, sizeof(buf));
        channelType = buf;
    }

    // 步骤 4: 创建 NML 通道
    // NML channel naming: emcFormat, emcChannelType, "xemc_cmd"
    m_nmlCmd = new RCS_CMD_CHANNEL(
        nmlFile.toLocal8Bit().constData(),
        emcFormat,
        channelType,
        "xemc_cmd"
    );

    m_nmlStat = new RCS_STAT_CHANNEL(
        nmlFile.toLocal8Bit().constData(),
        emcFormat,
        channelType,
        "xemc_stat"
    );

    // 步骤 5: 验证通道创建成功
    if (!m_nmlCmd || !m_nmlStat) {
        QString err = QString("NML 通道创建失败 (cmd=%1, stat=%2)")
                          .arg(m_nmlCmd != nullptr).arg(m_nmlStat != nullptr);
        qCritical().noquote() << "[LcncCore]" << err;
        emit errorOccurred(err);
        // 回退到模拟模式
        startSimulation();
        return;
    }

    if (m_nmlCmd->error_type != 0 || m_nmlStat->error_type != 0) {
        QString err = QString("NML 通道错误 (cmd_err=%1, stat_err=%2)")
                          .arg(m_nmlCmd->error_type)
                          .arg(m_nmlStat->error_type);
        qCritical().noquote() << "[LcncCore]" << err;
        emit errorOccurred(err);
        // 回退到模拟模式
        startSimulation();
        return;
    }

    // 步骤 6: 初始化 EMC_STAT 状态缓冲区
    m_emcStat = new EMC_STAT;

    // 步骤 7: 读取初始状态
    NMLmsg *msg = m_nmlStat->peek();
    if (msg && msg->type == EMC_STAT_TYPE) {
        *m_emcStat = *(static_cast<EMC_STAT *>(msg));
        qInfo() << "[LcncCore] 初始 EMC 状态读取成功";
    }

    // 步骤 8: 初始化 LcncCommand 的 NML 通道
    m_command->setNmlChannel(m_nmlCmd);

    // 步骤 9: 标记已连接并启动轮询
    m_connected = true;
    m_simulation = false;
    m_status.connected = true;

    // 轮询周期从 INI 读取，默认 50ms
    int pollMs = 50;
    if (ini.Find("DISPLAY", "CYCLE_TIME", 1)) {
        pollMs = static_cast<int>(iniFindDouble("CYCLE_TIME") * 1000.0);
        pollMs = std::clamp(pollMs, 10, 200);
    }
    m_pollTimer->start(pollMs);

    ini.Close();
    emit connected();
    qInfo().noquote() << "[LcncCore] Linux NML 连接成功，轮询周期"
                      << pollMs << "ms";

#else
    // Windows 开发环境 - 使用模拟模式
    qInfo().noquote() << "[LcncCore] Windows 环境，使用模拟数据";
    qInfo().noquote() << "[LcncCore] INI 文件（模拟）:" << iniFile;
    startSimulation();

#endif
}

void LcncCore::disconnectFromServer()
{
    m_pollTimer->stop();

#ifdef Q_OS_LINUX
    // 清理 NML 通道
    if (m_emcStat) { delete m_emcStat; m_emcStat = nullptr; }
    if (m_nmlCmd) { delete m_nmlCmd; m_nmlCmd = nullptr; }
    if (m_nmlStat) { delete m_nmlStat; m_nmlStat = nullptr; }
    m_command->setNmlChannel(nullptr);
#endif

    m_connected = false;
    m_simulation = false;
    m_status.connected = false;

    emit disconnected();
    qInfo().noquote() << "[LcncCore] 已断开连接";
}

bool LcncCore::isConnected() const
{
    return m_connected;
}

bool LcncCore::isSimulation() const
{
    return m_simulation;
}

void LcncCore::startSimulation()
{
    m_simulation = true;
    m_connected = true;
    m_status.connected = true;

    initSimulation();

    // 启动轮询定时器（100ms 模拟更新周期）
    m_pollTimer->start(100);

    emit connected();
    qInfo().noquote() << "[LcncCore] 模拟模式已启动，轮询周期 100ms";
}

// ============================================================================
// 状态轮询
// ============================================================================

void LcncCore::onPollTimer()
{
    if (m_simulation) {
        generateSimulatedStatus();
    } else {
        readNmlStatus();
    }

    // 检测状态变化，发出通知
    if (m_status != m_prevStatus) {
        emit statusUpdated(m_status);
        m_prevStatus = m_status;
    }
}

void LcncCore::readNmlStatus()
{
#ifdef Q_OS_LINUX
    if (!m_nmlStat || !m_emcStat) return;

    // 尝试读取 NML 消息
    NMLmsg *msg = m_nmlStat->read();
    if (!msg) return;

    switch (msg->type) {
    case EMC_STAT_TYPE: {
        EMC_STAT *s = static_cast<EMC_STAT *>(msg);
        *m_emcStat = *s;

        // 将 EMC_STAT 映射到 LcncStatusData
        mapEmcStatToLcnc(s);
        break;
    }
    case EMC_OPERATOR_ERROR_TYPE:
    case EMC_OPERATOR_TEXT_TYPE: {
        EMC_OPERATOR *op = static_cast<EMC_OPERATOR *>(msg);
        QString text = QString::fromLocal8Bit(op->error);
        if (msg->type == EMC_OPERATOR_ERROR_TYPE) {
            qWarning().noquote() << "[LcncCore] EMC 错误:" << text;
            emit errorOccurred(text);
        } else {
            qInfo().noquote() << "[LcncCore] EMC 信息:" << text;
        }
        break;
    }
    case EMC_TASK_ABORT_TYPE: {
        qWarning() << "[LcncCore] EMC_TASK_ABORT 收到";
        m_status.interpState = InterpState::IDLE;
        break;
    }
    case EMC_MOTION_SIM_TYPE: {
        // 运动仿真消息
        break;
    }
    default:
        break;
    }

    // 检测连接断开
    if (m_nmlStat->get_msg_count() < 0) {
        qWarning() << "[LcncCore] NML 连接丢失";
        m_connected = false;
        m_status.connected = false;
        emit disconnected();
    }
#endif
}

#ifdef Q_OS_LINUX
void LcncCore::mapEmcStatToLcnc(const void *raw)
{
    const EMC_STAT *s = static_cast<const EMC_STAT *>(raw);
    if (!s) return;

    // 机床状态
    switch (s->task.state) {
    case EMC_TASK_STATE_ESTOP: m_status.taskState = MachineState::ESTOP; break;
    case EMC_TASK_STATE_ESTOP_RESET: m_status.taskState = MachineState::ESTOP_RESET; break;
    case EMC_TASK_STATE_OFF: m_status.taskState = MachineState::OFF; break;
    case EMC_TASK_STATE_ON: m_status.taskState = MachineState::ON; break;
    default: m_status.taskState = MachineState::ESTOP; break;
    }

    // 运动模式
    switch (s->task.mode) {
    case EMC_TASK_MODE_MANUAL: m_status.motionMode = MotionMode::MANUAL; break;
    case EMC_TASK_MODE_MDI: m_status.motionMode = MotionMode::MDI; break;
    case EMC_TASK_MODE_AUTO: m_status.motionMode = MotionMode::AUTO; break;
    default: m_status.motionMode = MotionMode::MANUAL; break;
    }

    // 解释器状态
    switch (s->interp_state) {
    case EMC_INTERP_IDLE: m_status.interpState = InterpState::IDLE; break;
    case EMC_INTERP_READING: m_status.interpState = InterpState::RUNNING; break;
    case EMC_INTERP_WAITING: m_status.interpState = InterpState::RUNNING; break;
    case EMC_INTERP_PAUSED: m_status.interpState = InterpState::PAUSED; break;
    default: m_status.interpState = InterpState::IDLE; break;
    }

    // 位置数据
    int axes = std::min(static_cast<int>(s->motion.traj.axes), 9);
    for (int i = 0; i < axes; ++i) {
        if (i < MAX_DRO_AXES) {
            m_status.axes[i].position = s->motion.traj.position[i];
            m_status.axes[i].velocity = s->motion.traj.velocity;
            m_status.axes[i].enabled = (s->motion.traj.axis_mask & (1 << i));
            m_status.axes[i].minPosition = s->motion.traj.min_position_limit[i];
            m_status.axes[i].maxPosition = s->motion.traj.max_position_limit[i];
        }
    }

    // XYZ 位置快捷访问
    m_status.absolutePos.x = (axes > 0) ? s->motion.traj.position[0] : 0.0;
    m_status.absolutePos.y = (axes > 1) ? s->motion.traj.position[1] : 0.0;
    m_status.absolutePos.z = (axes > 2) ? s->motion.traj.position[2] : 0.0;

    // 主轴
    if (s->motion.spindle[0].spindle_speed > 0) {
        m_status.spindleState = SpindleState::FORWARD;
        m_status.spindleSpeed = s->motion.spindle[0].spindle_speed;
    } else if (s->motion.spindle[0].spindle_speed < 0) {
        m_status.spindleState = SpindleState::REVERSE;
        m_status.spindleSpeed = -s->motion.spindle[0].spindle_speed;
    } else {
        m_status.spindleState = SpindleState::STOPPED;
        m_status.spindleSpeed = 0.0;
    }

    // 进给倍率
    m_status.feedOverride = s->motion.traj.scale / 100.0;
    m_status.spindleOverride = s->motion.spindle[0].spindle_override / 100.0;

    // 当前刀具
    m_status.toolInSpindle = s->io.tool.tool_in_spindle;

    // 回零状态
    for (int i = 0; i < axes && i < MAX_DRO_AXES; ++i) {
        switch (s->motion.joint[i].homing_state) {
        case 1: m_status.axes[i].homingState = HomingState::HOMING; break;
        case 2: m_status.axes[i].homingState = HomingState::HOMED; break;
        default: m_status.axes[i].homingState = HomingState::UNHOMED; break;
        }
    }

    // 程序行号
    m_status.currentLine = s->task.callLevel > 0 ? s->task.line : 0;
    m_status.programLine = s->task.activeLine;
}
#endif

void LcncCore::initSimulation()
{
    // 初始化模拟状态
    m_simStep = 0;
    m_simTime = 0.0;

    m_status.taskState = MachineState::ESTOP_RESET;
    m_status.motionMode = MotionMode::MANUAL;
    m_status.interpState = InterpState::IDLE;
    m_status.spindleState = SpindleState::STOPPED;
    m_status.feedrate = 100.0;
    m_status.spindleSpeed = 0.0;
    m_status.spindleOverride = 1.0;
    m_status.feedOverride = 1.0;
    m_status.toolInSpindle = 1;

    // 初始位置
    m_status.absolutePos = LcncPose(0.0, 0.0, 0.0);
    m_status.relativePos = LcncPose(0.0, 0.0, 0.0);
    m_status.workPos = LcncPose(0.0, 0.0, 0.0);
    m_status.distanceToGo = LcncPose(0.0, 0.0, 0.0);

    // 初始 G 代码
    m_status.activeGCodesData.codes = {54, 17, 21, 40, 49, 80, 90, 94};
    m_status.activeMCodesData.codes = {};

    // 刀具信息
    m_status.currentTool.toolNo = 1;
    m_status.currentTool.diameter = 6.0;
    m_status.currentTool.zOffset = 10.0;

    // 轴状态
    for (int i = 0; i < 9; ++i) {
        m_status.axes[i].position = 0.0;
        m_status.axes[i].velocity = 0.0;
        m_status.axes[i].homingState = HomingState::HOMED;
    }

    // 文件信息
    m_status.currentFile.clear();
    m_status.currentLine = 0;
    m_status.programLine = 0;
    m_status.programUnits = QStringLiteral("mm");

    m_prevStatus = m_status;
}

void LcncCore::generateSimulatedStatus()
{
    m_simStep++;
    m_simTime += 0.1;  // 每 100ms 增加 0.1 秒

    // 模拟微小的位置变化（模拟震动/噪声）
    double noise = std::sin(m_simTime * 3.14159 * 2.0) * 0.001;
    m_status.absolutePos.x = noise;
    m_status.absolutePos.y = noise * 0.5;
    m_status.absolutePos.z = 0.0;
    m_status.workPos = m_status.absolutePos;

    // 更新轴位置
    m_status.axes[0].position = m_status.absolutePos.x;
    m_status.axes[1].position = m_status.absolutePos.y;
    m_status.axes[2].position = m_status.absolutePos.z;

    // 模拟主轴转速变化（如果主轴正在运转）
    if (m_status.spindleState == SpindleState::FORWARD) {
        m_status.spindleSpeed = 1000.0 + std::sin(m_simTime) * 5.0;
    } else if (m_status.spindleState == SpindleState::REVERSE) {
        m_status.spindleSpeed = -(1000.0 + std::sin(m_simTime) * 5.0);
    }

    // 模拟程序运行时行号递增
    if (m_status.interpState == InterpState::RUNNING && !m_status.currentFile.isEmpty()) {
        if (m_simStep % 3 == 0) {  // 每 300ms 递增一行
            m_status.currentLine++;
            if (m_status.currentLine > m_status.programLine) {
                m_status.currentLine = m_status.programLine;
                m_status.interpState = InterpState::IDLE;
            }
        }
    }
}

// ============================================================================
// 机床控制命令
// ============================================================================

void LcncCore::sendEstop()
{
    if (m_simulation) {
        m_status.taskState = MachineState::ESTOP;
        m_status.spindleState = SpindleState::STOPPED;
        m_status.spindleSpeed = 0.0;
        m_status.interpState = InterpState::IDLE;
    } else {
#ifdef Q_OS_LINUX
        // TODO: m_command->sendEstop();
#endif
    }
    emit commandSent(QStringLiteral("Estop"));
    qInfo().noquote() << "[LcncCore] 命令: 急停";
}

void LcncCore::sendEstopReset()
{
    if (m_simulation) {
        m_status.taskState = MachineState::ESTOP_RESET;
    } else {
#ifdef Q_OS_LINUX
        // TODO: m_command->sendEstopReset();
#endif
    }
    emit commandSent(QStringLiteral("EstopReset"));
    qInfo().noquote() << "[LcncCore] 命令: 急停复位";
}

void LcncCore::sendMachineOn()
{
    if (m_simulation) {
        if (m_status.taskState == MachineState::ESTOP_RESET) {
            m_status.taskState = MachineState::ON;
        }
    } else {
#ifdef Q_OS_LINUX
        // TODO: m_command->sendMachineOn();
#endif
    }
    emit commandSent(QStringLiteral("MachineOn"));
    qInfo().noquote() << "[LcncCore] 命令: 机床开启";
}

void LcncCore::sendMachineOff()
{
    if (m_simulation) {
        m_status.taskState = MachineState::OFF;
        m_status.spindleState = SpindleState::STOPPED;
        m_status.spindleSpeed = 0.0;
    } else {
#ifdef Q_OS_LINUX
        // TODO: m_command->sendMachineOff();
#endif
    }
    emit commandSent(QStringLiteral("MachineOff"));
    qInfo().noquote() << "[LcncCore] 命令: 机床关闭";
}

void LcncCore::sendModeAuto()
{
    if (m_simulation) {
        m_status.motionMode = MotionMode::AUTO;
    } else {
#ifdef Q_OS_LINUX
        // TODO: m_command->sendModeAuto();
#endif
    }
    emit commandSent(QStringLiteral("ModeAuto"));
    qInfo().noquote() << "[LcncCore] 命令: 切换自动模式";
}

void LcncCore::sendModeManual()
{
    if (m_simulation) {
        m_status.motionMode = MotionMode::MANUAL;
    } else {
#ifdef Q_OS_LINUX
        // TODO: m_command->sendModeManual();
#endif
    }
    emit commandSent(QStringLiteral("ModeManual"));
    qInfo().noquote() << "[LcncCore] 命令: 切换手动模式";
}

void LcncCore::sendModeMDI()
{
    if (m_simulation) {
        m_status.motionMode = MotionMode::MDI;
    } else {
#ifdef Q_OS_LINUX
        // TODO: m_command->sendModeMDI();
#endif
    }
    emit commandSent(QStringLiteral("ModeMDI"));
    qInfo().noquote() << "[LcncCore] 命令: 切换 MDI 模式";
}

void LcncCore::sendJog(int axis, double velocity)
{
    if (m_simulation) {
        if (axis >= 0 && axis < m_status.axes.size()) {
            m_status.axes[axis].velocity = velocity;
        }
    } else {
#ifdef Q_OS_LINUX
        // TODO: m_command->sendJog(axis, velocity);
#endif
    }
    emit commandSent(QStringLiteral("Jog axis=%1 vel=%2").arg(axis).arg(velocity));
}

void LcncCore::sendJogStop(int axis)
{
    if (m_simulation) {
        if (axis >= 0 && axis < m_status.axes.size()) {
            m_status.axes[axis].velocity = 0.0;
        }
    } else {
#ifdef Q_OS_LINUX
        // TODO: m_command->sendJogStop(axis);
#endif
    }
    emit commandSent(QStringLiteral("JogStop axis=%1").arg(axis));
}

void LcncCore::sendJogCont(int axis, double velocity)
{
    if (m_simulation) {
        if (axis >= 0 && axis < m_status.axes.size()) {
            m_status.axes[axis].velocity = velocity;
        }
    } else {
#ifdef Q_OS_LINUX
        // TODO: m_command->sendJogCont(axis, velocity);
#endif
    }
    emit commandSent(QStringLiteral("JogCont axis=%1 vel=%2").arg(axis).arg(velocity));
}

void LcncCore::sendJogIncrement(int axis, double distance, double velocity)
{
    if (m_simulation) {
        if (axis >= 0 && axis < m_status.axes.size()) {
            double newPos = m_status.axes[axis].position + distance;
            newPos = std::clamp(newPos,
                                m_status.axes[axis].minPosition,
                                m_status.axes[axis].maxPosition);
            m_status.axes[axis].position = newPos;

            // 同步更新坐标
            switch (axis) {
            case 0: m_status.absolutePos.x = newPos; m_status.workPos.x = newPos; break;
            case 1: m_status.absolutePos.y = newPos; m_status.workPos.y = newPos; break;
            case 2: m_status.absolutePos.z = newPos; m_status.workPos.z = newPos; break;
            default: break;
            }
        }
    } else {
#ifdef Q_OS_LINUX
        // TODO: m_command->sendJogIncrement(axis, distance, velocity);
#endif
    }
    emit commandSent(QStringLiteral("JogInc axis=%1 dist=%2 vel=%3")
                     .arg(axis).arg(distance).arg(velocity));
}

void LcncCore::sendMdi(const QString &cmd)
{
    if (m_simulation) {
        m_status.motionMode = MotionMode::MDI;
        m_status.interpState = InterpState::RUNNING;
        // 模拟执行完成后恢复空闲
        QTimer::singleShot(500, this, [this]() {
            m_status.interpState = InterpState::IDLE;
        });
    } else {
#ifdef Q_OS_LINUX
        // TODO: m_command->sendMdi(cmd);
#endif
    }
    emit commandSent(QStringLiteral("MDI: %1").arg(cmd));
    qInfo().noquote() << "[LcncCore] MDI 命令:" << cmd;
}

void LcncCore::sendProgramOpen(const QString &filename)
{
    if (m_simulation) {
        m_status.currentFile = filename;
        m_status.currentLine = 0;
        m_status.programLine = 100;  // 模拟 100 行程序
        m_status.interpState = InterpState::IDLE;
    } else {
#ifdef Q_OS_LINUX
        // TODO: m_command->sendProgramOpen(filename);
#endif
    }
    emit commandSent(QStringLiteral("ProgramOpen: %1").arg(filename));
    qInfo().noquote() << "[LcncCore] 打开程序:" << filename;
}

void LcncCore::sendProgramRun()
{
    if (m_simulation) {
        if (!m_status.currentFile.isEmpty()) {
            m_status.motionMode = MotionMode::AUTO;
            m_status.interpState = InterpState::RUNNING;
            m_status.currentLine = 0;
        }
    } else {
#ifdef Q_OS_LINUX
        // TODO: m_command->sendProgramRun();
#endif
    }
    emit commandSent(QStringLiteral("ProgramRun"));
}

void LcncCore::sendProgramRun(int line)
{
    if (m_simulation) {
        if (!m_status.currentFile.isEmpty()) {
            m_status.motionMode = MotionMode::AUTO;
            m_status.interpState = InterpState::RUNNING;
            m_status.currentLine = line;
        }
    } else {
#ifdef Q_OS_LINUX
        // TODO: m_command->sendProgramRun(line);
#endif
    }
    emit commandSent(QStringLiteral("ProgramRun from line %1").arg(line));
}

void LcncCore::sendProgramPause()
{
    if (m_simulation) {
        m_status.interpState = InterpState::PAUSED;
    } else {
#ifdef Q_OS_LINUX
        // TODO: m_command->sendProgramPause();
#endif
    }
    emit commandSent(QStringLiteral("ProgramPause"));
}

void LcncCore::sendProgramResume()
{
    if (m_simulation) {
        m_status.interpState = InterpState::RUNNING;
    } else {
#ifdef Q_OS_LINUX
        // TODO: m_command->sendProgramResume();
#endif
    }
    emit commandSent(QStringLiteral("ProgramResume"));
}

void LcncCore::sendProgramStop()
{
    if (m_simulation) {
        m_status.interpState = InterpState::IDLE;
        m_status.currentLine = 0;
    } else {
#ifdef Q_OS_LINUX
        // TODO: m_command->sendProgramStop();
#endif
    }
    emit commandSent(QStringLiteral("ProgramStop"));
}

void LcncCore::sendProgramStep()
{
    if (m_simulation) {
        if (!m_status.currentFile.isEmpty()) {
            m_status.interpState = InterpState::RUNNING;
            m_status.currentLine++;
            QTimer::singleShot(200, this, [this]() {
                m_status.interpState = InterpState::PAUSED;
            });
        }
    } else {
#ifdef Q_OS_LINUX
        // TODO: m_command->sendProgramStep();
#endif
    }
    emit commandSent(QStringLiteral("ProgramStep"));
}

void LcncCore::sendHome(int axis)
{
    if (m_simulation) {
        if (axis == -1) {
            // 所有轴回参考点
            for (int i = 0; i < 9; ++i) {
                if (m_status.axes[i].enabled) {
                    m_status.axes[i].position = 0.0;
                    m_status.axes[i].homingState = HomingState::HOMING;
                }
            }
            m_status.absolutePos = LcncPose(0, 0, 0);
            m_status.workPos = LcncPose(0, 0, 0);

            // 模拟回参考点完成
            QTimer::singleShot(1000, this, [this]() {
                for (int i = 0; i < 9; ++i) {
                    m_status.axes[i].homingState = HomingState::HOMED;
                }
            });
        } else if (axis >= 0 && axis < m_status.axes.size()) {
            m_status.axes[axis].position = 0.0;
            m_status.axes[axis].homingState = HomingState::HOMING;

            switch (axis) {
            case 0: m_status.absolutePos.x = 0; m_status.workPos.x = 0; break;
            case 1: m_status.absolutePos.y = 0; m_status.workPos.y = 0; break;
            case 2: m_status.absolutePos.z = 0; m_status.workPos.z = 0; break;
            default: break;
            }

            QTimer::singleShot(500, this, [this, axis]() {
                if (axis < m_status.axes.size()) {
                    m_status.axes[axis].homingState = HomingState::HOMED;
                }
            });
        }
    } else {
#ifdef Q_OS_LINUX
        // TODO: m_command->sendHome(axis);
#endif
    }
    emit commandSent(QStringLiteral("Home axis=%1").arg(axis));
}

// ============================================================================
// 覆盖率控制
// ============================================================================

void LcncCore::sendFeedOverride(double rate)
{
    if (m_simulation) {
        m_status.feedOverride = std::clamp(rate, 0.0, 2.0);
    } else {
#ifdef Q_OS_LINUX
        // TODO: m_command->sendFeedOverride(rate);
#endif
    }
    emit commandSent(QStringLiteral("FeedOverride: %1").arg(rate));
}

void LcncCore::sendSpindleOverride(double rate)
{
    if (m_simulation) {
        m_status.spindleOverride = std::clamp(rate, 0.0, 2.0);
    } else {
#ifdef Q_OS_LINUX
        // TODO: m_command->sendSpindleOverride(rate);
#endif
    }
    emit commandSent(QStringLiteral("SpindleOverride: %1").arg(rate));
}

void LcncCore::sendMaxVelocity(double rate)
{
    if (m_simulation) {
        // 模拟设置最大速度
        for (auto &axis : m_status.axes) {
            axis.velocity = std::min(axis.velocity, rate);
        }
    } else {
#ifdef Q_OS_LINUX
        // TODO: m_command->sendMaxVelocity(rate);
#endif
    }
    emit commandSent(QStringLiteral("MaxVelocity: %1").arg(rate));
}

// ============================================================================
// 主轴控制
// ============================================================================

void LcncCore::sendSpindleForward(double speed)
{
    if (m_simulation) {
        m_status.spindleState = SpindleState::FORWARD;
        m_status.spindleSpeed = speed;
    } else {
#ifdef Q_OS_LINUX
        // TODO: m_command->sendSpindleForward(speed);
#endif
    }
    emit commandSent(QStringLiteral("SpindleForward: %1 RPM").arg(speed));
}

void LcncCore::sendSpindleReverse(double speed)
{
    if (m_simulation) {
        m_status.spindleState = SpindleState::REVERSE;
        m_status.spindleSpeed = -speed;
    } else {
#ifdef Q_OS_LINUX
        // TODO: m_command->sendSpindleReverse(speed);
#endif
    }
    emit commandSent(QStringLiteral("SpindleReverse: %1 RPM").arg(speed));
}

void LcncCore::sendSpindleStop()
{
    if (m_simulation) {
        m_status.spindleState = SpindleState::STOPPED;
        m_status.spindleSpeed = 0.0;
    } else {
#ifdef Q_OS_LINUX
        // TODO: m_command->sendSpindleStop();
#endif
    }
    emit commandSent(QStringLiteral("SpindleStop"));
}

void LcncCore::sendSpindleFaster()
{
    if (m_simulation) {
        m_status.spindleOverride = std::min(m_status.spindleOverride + 0.1, 2.0);
    } else {
#ifdef Q_OS_LINUX
        // TODO: m_command->sendSpindleFaster();
#endif
    }
    emit commandSent(QStringLiteral("SpindleFaster"));
}

void LcncCore::sendSpindleSlower()
{
    if (m_simulation) {
        m_status.spindleOverride = std::max(m_status.spindleOverride - 0.1, 0.0);
    } else {
#ifdef Q_OS_LINUX
        // TODO: m_command->sendSpindleSlower();
#endif
    }
    emit commandSent(QStringLiteral("SpindleSlower"));
}

// ============================================================================
// 冷却液控制
// ============================================================================

void LcncCore::sendCoolantMistOn()
{
    emit commandSent(QStringLiteral("CoolantMistOn"));
}

void LcncCore::sendCoolantMistOff()
{
    emit commandSent(QStringLiteral("CoolantMistOff"));
}

void LcncCore::sendCoolantFloodOn()
{
    emit commandSent(QStringLiteral("CoolantFloodOn"));
}

void LcncCore::sendCoolantFloodOff()
{
    emit commandSent(QStringLiteral("CoolantFloodOff"));
}

// ============================================================================
// 状态查询
// ============================================================================

const LcncStatusData &LcncCore::statusData() const
{
    return m_status;
}

LcncCommand *LcncCore::command() const
{
    return m_command;
}
