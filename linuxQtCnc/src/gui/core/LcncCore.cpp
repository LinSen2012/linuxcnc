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
#include <algorithm>

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

bool LcncCore::connectToServer(const QString &iniFile)
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
    INIFILE ini;
    if (ini.Open(iniFile.toLocal8Bit().constData()) != 0) {
        QString err = QString("无法打开 INI 文件: %1").arg(iniFile);
        qCritical().noquote() << "[LcncCore]" << err;
        emit errorOccurred(err);
        startSimulation();
        return true;  // 模拟模式也算成功连接
    }

    QString nmlFile;
    const char *nmlSection = "EMC";
    char buf[LINELEN];
    if (ini.Find(nmlSection, "NML_FILE", 1)) {
        iniString(nmlFile, buf, sizeof(buf));
        qInfo() << "[LcncCore] NML 文件:" << nmlFile;
    } else {
        nmlFile = "/etc/linuxcnc/linuxcnc.nml";
    }

    const char *channelType = "xemc";
    if (ini.Find(nmlSection, "CHANNEL", 1)) {
        iniString(buf, sizeof(buf));
        channelType = buf;
    }

    m_nmlCmd = new RCS_CMD_CHANNEL(
        nmlFile.toLocal8Bit().constData(), emcFormat, channelType, "xemc_cmd");
    m_nmlStat = new RCS_STAT_CHANNEL(
        nmlFile.toLocal8Bit().constData(), emcFormat, channelType, "xemc_stat");

    if (!m_nmlCmd || !m_nmlStat) {
        QString err = "NML 通道创建失败";
        qCritical().noquote() << "[LcncCore]" << err;
        emit errorOccurred(err);
        startSimulation();
        return true;
    }

    if (m_nmlCmd->error_type != 0 || m_nmlStat->error_type != 0) {
        QString err = QString("NML 通道错误");
        qCritical().noquote() << "[LcncCore]" << err;
        emit errorOccurred(err);
        startSimulation();
        return true;
    }

    m_emcStat = new EMC_STAT;
    NMLmsg *msg = m_nmlStat->peek();
    if (msg && msg->type == EMC_STAT_TYPE) {
        *m_emcStat = *(static_cast<EMC_STAT *>(msg));
    }

    m_command->setNmlChannel(m_nmlCmd);
    m_connected = true;
    m_simulation = false;
    m_status.connected = true;

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
    return true;

#else
    // Windows 开发环境 - 使用模拟模式
    qInfo().noquote() << "[LcncCore] Windows 环境，使用模拟数据";
    qInfo().noquote() << "[LcncCore] INI 文件（模拟）:" << iniFile;
    startSimulation();
    return true;
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

    // 机床状态（注意：EMC_TASK_STATE 是 C++ enum class，必须用 :: 访问）
    switch (s->task.state) {
    case EMC_TASK_STATE::ESTOP: m_status.taskState = MachineState::ESTOP; break;
    case EMC_TASK_STATE::ESTOP_RESET: m_status.taskState = MachineState::ESTOP_RESET; break;
    case EMC_TASK_STATE::OFF: m_status.taskState = MachineState::OFF; break;
    case EMC_TASK_STATE::ON: m_status.taskState = MachineState::ON; break;
    default: m_status.taskState = MachineState::ESTOP; break;
    }

    // 运动模式（enum class）
    switch (s->task.mode) {
    case EMC_TASK_MODE::MANUAL: m_status.motionMode = MotionMode::MANUAL; break;
    case EMC_TASK_MODE::MDI: m_status.motionMode = MotionMode::MDI; break;
    case EMC_TASK_MODE::AUTO: m_status.motionMode = MotionMode::AUTO; break;
    default: m_status.motionMode = MotionMode::MANUAL; break;
    }

    // 解释器状态（enum class）
    switch (s->task.interpState) {
    case EMC_TASK_INTERP::IDLE: m_status.interpState = InterpState::IDLE; break;
    case EMC_TASK_INTERP::READING: m_status.interpState = InterpState::RUNNING; break;
    case EMC_TASK_INTERP::WAITING: m_status.interpState = InterpState::RUNNING; break;
    case EMC_TASK_INTERP::PAUSED: m_status.interpState = InterpState::PAUSED; break;
    default: m_status.interpState = InterpState::IDLE; break;
    }

    // 位置数据 - 从 EMC_TRAJ_STAT 和 EMC_JOINT_STAT 读取
    int joints = std::min(static_cast<int>(s->motion.traj.joints), 9);
    for (int i = 0; i < joints && i < MAX_DRO_AXES; ++i) {
        m_status.axes[i].position = s->motion.joint[i].input;
        m_status.axes[i].velocity = s->motion.joint[i].velocity;
        m_status.axes[i].enabled = s->motion.joint[i].enabled;
        m_status.axes[i].minPosition = s->motion.joint[i].minPositionLimit;
        m_status.axes[i].maxPosition = s->motion.joint[i].maxPositionLimit;

        // 限位状态
        if (s->motion.joint[i].minHardLimit || s->motion.joint[i].minSoftLimit) {
            m_status.axes[i].limitState = LimitState::MIN_LIMIT;
        } else if (s->motion.joint[i].maxHardLimit || s->motion.joint[i].maxSoftLimit) {
            m_status.axes[i].limitState = LimitState::MAX_LIMIT;
        } else {
            m_status.axes[i].limitState = LimitState::NONE;
        }

        // 回零状态
        if (s->motion.joint[i].homing) {
            m_status.axes[i].homingState = HomingState::HOMING;
        } else if (s->motion.joint[i].homed) {
            m_status.axes[i].homingState = HomingState::HOMED;
        } else {
            m_status.axes[i].homingState = HomingState::IDLE;
        }

        // 故障状态
        m_status.axes[i].fault = s->motion.joint[i].fault;
    }

    // XYZ 位置快捷访问（从 EmcPose 的 tran 成员访问）
    m_status.absolutePos.tran.x = s->motion.traj.position.tran.x;
    m_status.absolutePos.tran.y = s->motion.traj.position.tran.y;
    m_status.absolutePos.tran.z = s->motion.traj.position.tran.z;
    m_status.absolutePos.a = s->motion.traj.position.a;
    m_status.absolutePos.b = s->motion.traj.position.b;
    m_status.absolutePos.c = s->motion.traj.position.c;

    // 相对坐标 = 绝对坐标 - 工件偏置
    m_status.relativePos.tran.x = m_status.absolutePos.tran.x - s->task.g5x_offset.tran.x - s->task.g92_offset.tran.x;
    m_status.relativePos.tran.y = m_status.absolutePos.tran.y - s->task.g5x_offset.tran.y - s->task.g92_offset.tran.y;
    m_status.relativePos.tran.z = m_status.absolutePos.tran.z - s->task.g5x_offset.tran.z - s->task.g92_offset.tran.z;

    // 工件坐标系位置
    m_status.workPos = m_status.relativePos;

    // 距离目标点
    m_status.distanceToGo.tran.x = s->motion.traj.dtg.tran.x;
    m_status.distanceToGo.tran.y = s->motion.traj.dtg.tran.y;
    m_status.distanceToGo.tran.z = s->motion.traj.dtg.tran.z;

    // 主轴状态
    if (s->motion.spindle[0].speed > 0) {
        m_status.spindleState = SpindleState::FORWARD;
        m_status.spindleSpeed = s->motion.spindle[0].speed;
    } else if (s->motion.spindle[0].speed < 0) {
        m_status.spindleState = SpindleState::REVERSE;
        m_status.spindleSpeed = -s->motion.spindle[0].speed;
    } else {
        m_status.spindleState = SpindleState::STOPPED;
        m_status.spindleSpeed = 0.0;
    }

    // 进给倍率
    m_status.feedOverride = s->motion.traj.scale;
    m_status.spindleOverride = s->motion.spindle[0].spindle_scale;

    // 进给速率（从 activeSettings）
    if (s->task.activeSettings[0] > 0) {
        m_status.feedrate = s->task.activeSettings[0];
    }

    // 当前刀具
    m_status.toolInSpindle = s->io.tool.toolInSpindle;

    // 刀具偏置
    m_status.currentTool.xOffset = s->task.toolOffset.tran.x;
    m_status.currentTool.yOffset = s->task.toolOffset.tran.y;
    m_status.currentTool.zOffset = s->task.toolOffset.tran.z;
    m_status.currentTool.a = s->task.toolOffset.a;
    m_status.currentTool.b = s->task.toolOffset.b;
    m_status.currentTool.c = s->task.toolOffset.c;

    // G5x 偏置（G54-G59.3）
    m_status.g5xOffset.tran.x = s->task.g5x_offset.tran.x;
    m_status.g5xOffset.tran.y = s->task.g5x_offset.tran.y;
    m_status.g5xOffset.tran.z = s->task.g5x_offset.tran.z;
    m_status.g5xIndex = s->task.g5x_index;

    // G92 偏置
    m_status.g92Offset.tran.x = s->task.g92_offset.tran.x;
    m_status.g92Offset.tran.y = s->task.g92_offset.tran.y;
    m_status.g92Offset.tran.z = s->task.g92_offset.tran.z;

    // 程序信息
    m_status.m_currentLine = s->task.currentLine;
    m_status.programLine = s->task.readLine;

    // 当前 G 代码文件
    if (s->task.file[0] != '\0') {
        m_status.m_currentFile = QString::fromLocal8Bit(s->task.file);
    }

    // 当前活动的 G 代码
    m_status.activeGCodesData.codes.clear();
    for (int i = 0; i < 16 && s->task.activeGCodes[i] != -1; ++i) {
        m_status.activeGCodesData.codes.append(s->task.activeGCodes[i]);
    }

    // 冷却液状态
    m_status.coolantMist = (s->io.coolant.mist != 0);
    m_status.coolantFlood = (s->io.coolant.flood != 0);

    // 连接状态
    m_status.connected = true;
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
    m_status.m_currentFile.clear();
    m_status.m_currentLine = 0;
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
    m_status.absolutePos.tran.x = noise;
    m_status.absolutePos.tran.y = noise * 0.5;
    m_status.absolutePos.tran.z = 0.0;
    m_status.workPos.tran.x = m_status.absolutePos.tran.x;
    m_status.workPos.tran.y = m_status.absolutePos.tran.y;
    m_status.workPos.tran.z = 0.0;

    // 更新轴位置
    m_status.axes[0].position = m_status.absolutePos.tran.x;
    m_status.axes[1].position = m_status.absolutePos.tran.y;
    m_status.axes[2].position = m_status.absolutePos.tran.z;

    // 模拟主轴转速变化（如果主轴正在运转）
    if (m_status.spindleState == SpindleState::FORWARD) {
        m_status.spindleSpeed = 1000.0 + std::sin(m_simTime) * 5.0;
    } else if (m_status.spindleState == SpindleState::REVERSE) {
        m_status.spindleSpeed = -(1000.0 + std::sin(m_simTime) * 5.0);
    }

    // 模拟程序运行时行号递增
    if (m_status.interpState == InterpState::RUNNING && !m_status.m_currentFile.isEmpty()) {
        if (m_simStep % 3 == 0) {  // 每 300ms 递增一行
            m_status.m_currentLine++;
            if (m_status.m_currentLine > m_status.programLine) {
                m_status.m_currentLine = m_status.programLine;
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
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->estop();
#endif
    }
    emit commandSent(QStringLiteral("Estop"));
    qInfo().noquote() << "[LcncCore] 命令: 急停";
}

void LcncCore::sendEstopReset()
{
    if (m_simulation) {
        m_status.taskState = MachineState::ESTOP_RESET;
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->estopReset();
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
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->machineOn();
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
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->machineOff();
#endif
    }
    emit commandSent(QStringLiteral("MachineOff"));
    qInfo().noquote() << "[LcncCore] 命令: 机床关闭";
}

void LcncCore::sendModeAuto()
{
    if (m_simulation) {
        m_status.motionMode = MotionMode::AUTO;
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->modeAuto();
#endif
    }
    emit commandSent(QStringLiteral("ModeAuto"));
    qInfo().noquote() << "[LcncCore] 命令: 切换自动模式";
}

void LcncCore::sendModeManual()
{
    if (m_simulation) {
        m_status.motionMode = MotionMode::MANUAL;
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->modeManual();
#endif
    }
    emit commandSent(QStringLiteral("ModeManual"));
    qInfo().noquote() << "[LcncCore] 命令: 切换手动模式";
}

void LcncCore::sendModeMDI()
{
    if (m_simulation) {
        m_status.motionMode = MotionMode::MDI;
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->modeMDI();
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
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->jogCont(axis, velocity);
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
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->jogStop(axis);
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
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->jogCont(axis, velocity);
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
            case 0: m_status.absolutePos.tran.x = newPos; m_status.workPos.tran.x = newPos; break;
            case 1: m_status.absolutePos.tran.y = newPos; m_status.workPos.tran.y = newPos; break;
            case 2: m_status.absolutePos.tran.z = newPos; m_status.workPos.tran.z = newPos; break;
            default: break;
            }
        }
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->jogIncrement(axis, distance, velocity);
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
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->mdi(cmd);
#endif
    }
    emit commandSent(QStringLiteral("MDI: %1").arg(cmd));
    qInfo().noquote() << "[LcncCore] MDI 命令:" << cmd;
}

void LcncCore::sendProgramOpen(const QString &filename)
{
    if (m_simulation) {
        m_status.m_currentFile = filename;
        m_status.m_currentLine = 0;
        m_status.programLine = 100;  // 模拟 100 行程序
        m_status.interpState = InterpState::IDLE;
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->programOpen(filename);
#endif
    }
    emit commandSent(QStringLiteral("ProgramOpen: %1").arg(filename));
    qInfo().noquote() << "[LcncCore] 打开程序:" << filename;
}

void LcncCore::sendProgramRun()
{
    if (m_simulation) {
        if (!m_status.m_currentFile.isEmpty()) {
            m_status.motionMode = MotionMode::AUTO;
            m_status.interpState = InterpState::RUNNING;
            m_status.m_currentLine = 0;
        }
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->programRun();
#endif
    }
    emit commandSent(QStringLiteral("ProgramRun"));
}

void LcncCore::sendProgramRun(int line)
{
    if (m_simulation) {
        if (!m_status.m_currentFile.isEmpty()) {
            m_status.motionMode = MotionMode::AUTO;
            m_status.interpState = InterpState::RUNNING;
            m_status.m_currentLine = line;
        }
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->programRun(line);
#endif
    }
    emit commandSent(QStringLiteral("ProgramRun from line %1").arg(line));
}

void LcncCore::sendProgramPause()
{
    if (m_simulation) {
        m_status.interpState = InterpState::PAUSED;
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->programPause();
#endif
    }
    emit commandSent(QStringLiteral("ProgramPause"));
}

void LcncCore::sendProgramResume()
{
    if (m_simulation) {
        m_status.interpState = InterpState::RUNNING;
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->programResume();
#endif
    }
    emit commandSent(QStringLiteral("ProgramResume"));
}

void LcncCore::sendProgramStop()
{
    if (m_simulation) {
        m_status.interpState = InterpState::IDLE;
        m_status.m_currentLine = 0;
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->programStop();
#endif
    }
    emit commandSent(QStringLiteral("ProgramStop"));
}

void LcncCore::sendProgramStep()
{
    if (m_simulation) {
        if (!m_status.m_currentFile.isEmpty()) {
            m_status.interpState = InterpState::RUNNING;
            m_status.m_currentLine++;
            QTimer::singleShot(200, this, [this]() {
                m_status.interpState = InterpState::PAUSED;
            });
        }
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->programStep();
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
            case 0: m_status.absolutePos.tran.x = 0; m_status.workPos.tran.x = 0; break;
            case 1: m_status.absolutePos.tran.y = 0; m_status.workPos.tran.y = 0; break;
            case 2: m_status.absolutePos.tran.z = 0; m_status.workPos.tran.z = 0; break;
            default: break;
            }

            QTimer::singleShot(500, this, [this, axis]() {
                if (axis < m_status.axes.size()) {
                    m_status.axes[axis].homingState = HomingState::HOMED;
                }
            });
        }
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->home(axis);
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
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->feedOverride(rate);
#endif
    }
    emit commandSent(QStringLiteral("FeedOverride: %1").arg(rate));
}

void LcncCore::sendSpindleOverride(double rate)
{
    if (m_simulation) {
        m_status.spindleOverride = std::clamp(rate, 0.0, 2.0);
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->spindleOverride(rate);
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
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->maxVelocity(rate);
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
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->spindleForward(speed);
#endif
    }
    emit commandSent(QStringLiteral("SpindleForward: %1 RPM").arg(speed));
}

void LcncCore::sendSpindleReverse(double speed)
{
    if (m_simulation) {
        m_status.spindleState = SpindleState::REVERSE;
        m_status.spindleSpeed = -speed;
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->spindleReverse(speed);
#endif
    }
    emit commandSent(QStringLiteral("SpindleReverse: %1 RPM").arg(speed));
}

void LcncCore::sendSpindleStop()
{
    if (m_simulation) {
        m_status.spindleState = SpindleState::STOPPED;
        m_status.spindleSpeed = 0.0;
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->spindleStop();
#endif
    }
    emit commandSent(QStringLiteral("SpindleStop"));
}

void LcncCore::sendSpindleFaster()
{
    if (m_simulation) {
        m_status.spindleOverride = std::min(m_status.spindleOverride + 0.1, 2.0);
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->spindleFaster();
#endif
    }
    emit commandSent(QStringLiteral("SpindleFaster"));
}

void LcncCore::sendSpindleSlower()
{
    if (m_simulation) {
        m_status.spindleOverride = std::max(m_status.spindleOverride - 0.1, 0.0);
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->spindleSlower();
#endif
    }
    emit commandSent(QStringLiteral("SpindleSlower"));
}

// ============================================================================
// 冷却液控制
// ============================================================================

void LcncCore::sendCoolantMistOn()
{
    if (m_simulation) {
        m_status.coolantMist = true;
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->coolantMistOn();
#endif
    }
    emit commandSent(QStringLiteral("CoolantMistOn"));
}

void LcncCore::sendCoolantMistOff()
{
    if (m_simulation) {
        m_status.coolantMist = false;
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->coolantMistOff();
#endif
    }
    emit commandSent(QStringLiteral("CoolantMistOff"));
}

void LcncCore::sendCoolantFloodOn()
{
    if (m_simulation) {
        m_status.coolantFlood = true;
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->coolantFloodOn();
#endif
    }
    emit commandSent(QStringLiteral("CoolantFloodOn"));
}

void LcncCore::sendCoolantFloodOff()
{
    if (m_simulation) {
        m_status.coolantFlood = false;
    } else if (m_command) {
#ifdef Q_OS_LINUX
        m_command->coolantFloodOff();
#endif
    }
    emit commandSent(QStringLiteral("CoolantFloodOff"));
}

// ============================================================================
// 状态查询
// ============================================================================

const LcncStatusData *LcncCore::statusData() const
{
    return &m_status;
}

LcncCommand *LcncCore::command() const
{
    return m_command;
}

// ============================================================================
// 模式切换
// ============================================================================

void LcncCore::setMode(Mode mode)
{
    m_mode = mode;
    switch (mode) {
    case Mode::ModeAuto:   sendModeAuto();   break;
    case Mode::ModeManual: sendModeManual(); break;
    case Mode::ModeMdi:    sendModeMDI();    break;
    }
}

LcncCore::Mode LcncCore::mode() const
{
    return m_mode;
}

// ============================================================================
// 高层命令（MainWindow 直接调用）
// ============================================================================

bool LcncCore::estop()
{
    sendEstop();
    return true;
}

bool LcncCore::estopReset()
{
    sendEstopReset();
    return true;
}

bool LcncCore::machineOn()
{
    sendMachineOn();
    return true;
}

bool LcncCore::machineOff()
{
    sendMachineOff();
    return true;
}

bool LcncCore::homeAxis(int axis)
{
    sendHome(axis);
    return true;
}

bool LcncCore::runProgram(const QString &program)
{
    if (!m_simulation && m_command) {
#ifdef Q_OS_LINUX
        // Linux: 实际打开程序并运行
        // program 是 G 代码字符串，需要写入临时文件或直接传递
        Q_UNUSED(program);
        // TODO: 实现真实 NML 程序运行流程
        return false;
#endif
    }

    // 模拟模式
    if (!program.isEmpty()) {
        m_status.m_currentFile = QStringLiteral("memory_program");
        m_status.m_currentLine = 0;
        m_status.interpState = InterpState::RUNNING;
        m_status.motionMode = MotionMode::AUTO;
        emit statusUpdated(m_status);
        emit commandSent(QStringLiteral("ProgramRun"));
        return true;
    }
    return false;
}

void LcncCore::pauseProgram()
{
    if (m_simulation) {
        if (m_status.interpState == InterpState::RUNNING) {
            m_status.interpState = InterpState::PAUSED;
            emit statusUpdated(m_status);
        }
    } else {
        sendProgramPause();
    }
}

void LcncCore::stopProgram()
{
    if (m_simulation) {
        m_status.interpState = InterpState::IDLE;
        m_status.m_currentLine = 0;
        emit statusUpdated(m_status);
    } else {
        sendProgramStop();
    }
}

void LcncCore::resumeProgram()
{
    if (m_simulation) {
        if (m_status.interpState == InterpState::PAUSED) {
            m_status.interpState = InterpState::RUNNING;
            emit statusUpdated(m_status);
        }
    } else {
        sendProgramResume();
    }
}
