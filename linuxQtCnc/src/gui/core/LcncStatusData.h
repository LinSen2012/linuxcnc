/*
 * linuxQtCncGui - LinuxCNC Qt6 GUI 控制器
 * LcncStatusData - 机床状态数据结构
 *
 * 定义 CNC 机床运行时的所有状态信息，包括：
 * - 机床状态（急停/开启/关闭）
 * - 运动模式（手动/自动/MDI）
 * - 解释器状态（空闲/运行/暂停）
 * - 坐标位置（绝对/相对/工件坐标系）
 * - 进给速率、主轴转速
 * - 当前 G/M/T 代码
 * - 刀具信息、限位开关状态等
 *
 * 使用 Q_GADGET 使其可在 QVariant 中传递，支持 Q_PROPERTY
 */

#ifndef LCNC_STATUS_DATA_H
#define LCNC_STATUS_DATA_H

#include <QObject>
#include <QString>
#include <QVector>

// ============================================================================
// 前向声明 - LinuxCNC 核心类型（实际集成时替换为真实头文件）
// ============================================================================
// 在 Windows 开发环境中，我们定义兼容类型
// 在 Linux 上集成时，这些类型将来自 LinuxCNC 核心头文件

/// 三维位姿结构体（兼容 LinuxCNC EmcPose）
struct LcncPose {
    double x = 0.0;  ///< X 轴坐标
    double y = 0.0;  ///< Y 轴坐标
    double z = 0.0;  ///< Z 轴坐标
    double a = 0.0;  ///< A 轴坐标（旋转轴）
    double b = 0.0;  ///< B 轴坐标（旋转轴）
    double c = 0.0;  ///< C 轴坐标（旋转轴）
    double u = 0.0;  ///< U 轴坐标
    double v = 0.0;  ///< V 轴坐标
    double w = 0.0;  ///< W 轴坐标

    LcncPose() = default;
    LcncPose(double x_, double y_, double z_,
             double a_ = 0.0, double b_ = 0.0, double c_ = 0.0,
             double u_ = 0.0, double v_ = 0.0, double w_ = 0.0)
        : x(x_), y(y_), z(z_), a(a_), b(b_), c(c_), u(u_), v(v_), w(w_) {}

    bool operator==(const LcncPose &other) const {
        return x == other.x && y == other.y && z == other.z
            && a == other.a && b == other.b && c == other.c
            && u == other.u && v == other.v && w == other.w;
    }
    bool operator!=(const LcncPose &other) const { return !(*this == other); }
};

// ============================================================================
// 枚举定义
// ============================================================================

/**
 * @brief 机床状态枚举
 * 对应 LinuxCNC 中的 EMC_TASK_INTERP_STATE
 */
enum class MachineState {
    ESTOP,          ///< 急停状态 - 机床已急停
    ESTOP_RESET,    ///< 急停复位 - 急停已复位但机床未开启
    ON,             ///< 机床开启 - 机床已上电就绪
    OFF             ///< 机床关闭
};

/**
 * @brief 运动模式枚举
 * 对应 LinuxCNC 中的运动模式
 */
enum class MotionMode {
    MANUAL,         ///< 手动模式（JOG/MPG）
    AUTO,           ///< 自动模式（执行 G 代码程序）
    MDI             ///< MDI 模式（手动数据输入）
};

/**
 * @brief 解释器状态枚举
 * 对应 LinuxCNC 中的 EMC_TASK_INTERP_STATE
 */
enum class InterpState {
    IDLE,           ///< 空闲 - 等待命令
    RUNNING,        ///< 运行中 - 正在执行 G 代码
    PAUSED          ///< 暂停
};

/**
 * @brief 主轴状态枚举
 */
enum class SpindleState {
    STOPPED,        ///< 主轴停止
    FORWARD,        ///< 主轴正转（顺时针）
    REVERSE         ///< 主轴反转（逆时针）
};

/**
 * @brief 限制开关状态
 */
enum class LimitState {
    NONE,           ///< 无限制状态
    MIN_LIMIT,      ///< 到达最小限位
    MAX_LIMIT,      ///< 到达最大限位
    BOTH            ///< 同时到达两端限位（异常）
};

/**
 * @brief 回参考点状态
 */
enum class HomingState {
    IDLE,           ///< 未开始回参考点
    HOMING,         ///< 正在回参考点
    HOMED           ///< 已回参考点
};

// ============================================================================
// 刀具信息结构
// ============================================================================

/**
 * @brief 刀具数据
 * 对应 LinuxCNC 中的 CANON_TOOL_TABLE
 */
struct LcncToolData {
    int toolNo = 0;         ///< 刀具编号
    int pocketNo = 0;       ///< 刀位编号
    double xOffset = 0.0;   ///< X 方向偏置
    double yOffset = 0.0;   ///< Y 方向偏置
    double zOffset = 0.0;   ///< Z 方向偏置
    double diameter = 0.0;  ///< 刀具直径
    double frontAngle = 0.0;///< 前角
    double backAngle = 0.0; ///< 后角
    int orientation = 0;    ///< 刀具方向
};

// ============================================================================
// 当前活动 G/M 代码
// ============================================================================

/**
 * @brief 当前活动的 G 代码
 */
struct ActiveGCodes {
    QVector<int> codes;  ///< 当前活动的 G 代码列表
};

/**
 * @brief 当前活动的 M 代码
 */
struct ActiveMCodes {
    QVector<int> codes;  ///< 当前活动的 M 代码列表
};

// ============================================================================
// 轴状态信息
// ============================================================================

/**
 * @brief 单轴状态
 */
struct LcncAxisStatus {
    double position = 0.0;       ///< 当前位置
    double velocity = 0.0;       ///< 当前速度
    double minPosition = 0.0;    ///< 最小位置限制
    double maxPosition = 0.0;    ///< 最大位置限制
    LimitState limitState = LimitState::NONE;  ///< 限位开关状态
    HomingState homingState = HomingState::IDLE; ///< 回参考点状态
    bool enabled = true;         ///< 轴是否使能
    bool fault = false;         ///< 轴故障标志
};

// ============================================================================
// 主状态数据结构
// ============================================================================

/**
 * @brief LcncStatusData - 机床完整状态数据
 *
 * 使用 Q_GADGET 使其可在 QVariant 中传递，支持信号/槽参数传递。
 * 所有属性通过 Q_PROPERTY 暴露，可用于 QML。
 */
class LcncStatusData
{
    Q_GADGET

    // Q_PROPERTY 声明
    Q_PROPERTY(int taskState      READ taskState      WRITE setTaskState)
    Q_PROPERTY(int motionMode     READ motionMode     WRITE setMotionMode)
    Q_PROPERTY(int interpState    READ interpState    WRITE setInterpState)
    Q_PROPERTY(int spindleState   READ spindleState   WRITE setSpindleState)
    Q_PROPERTY(double feedrate    READ feedrate       WRITE setFeedrate)
    Q_PROPERTY(double spindleSpeed READ spindleSpeed    WRITE setSpindleSpeed)
    Q_PROPERTY(double spindleOverride READ spindleOverride WRITE setSpindleOverride)
    Q_PROPERTY(double feedOverride READ feedOverride   WRITE setFeedOverride)
    Q_PROPERTY(int activeGCodes   READ activeGCodes   WRITE setActiveGCodes)
    Q_PROPERTY(int activeMCodes   READ activeMCodes   WRITE setActiveMCodes)
    Q_PROPERTY(QString currentFile READ currentFile    WRITE setCurrentFile)
    Q_PROPERTY(int currentLine    READ currentLine     WRITE setCurrentLine)

public:
    LcncStatusData() = default;

    // ----- 机床状态 -----
    MachineState taskState = MachineState::ESTOP;
    int taskState() const { return static_cast<int>(taskState); }
    void setTaskState(int v) { taskState = static_cast<MachineState>(v); }

    // ----- 运动模式 -----
    MotionMode motionMode = MotionMode::MANUAL;
    int motionMode() const { return static_cast<int>(motionMode); }
    void setMotionMode(int v) { motionMode = static_cast<MotionMode>(v); }

    // ----- 解释器状态 -----
    InterpState interpState = InterpState::IDLE;
    int interpState() const { return static_cast<int>(interpState); }
    void setInterpState(int v) { interpState = static_cast<InterpState>(v); }

    // ----- 主轴状态 -----
    SpindleState spindleState = SpindleState::STOPPED;
    int spindleState() const { return static_cast<int>(spindleState); }
    void setSpindleState(int v) { spindleState = static_cast<SpindleState>(v); }

    // ----- 进给速率 -----
    double feedrate = 0.0;
    double feedrate() const { return feedrate; }
    void setFeedrate(double v) { feedrate = v; }

    // ----- 主轴转速 -----
    double spindleSpeed = 0.0;
    double spindleSpeed() const { return spindleSpeed; }
    void setSpindleSpeed(double v) { spindleSpeed = v; }

    // ----- 主轴倍率 -----
    double spindleOverride = 1.0;
    double spindleOverride() const { return spindleOverride; }
    void setSpindleOverride(double v) { spindleOverride = v; }

    // ----- 进给倍率 -----
    double feedOverride = 1.0;
    double feedOverride() const { return feedOverride; }
    void setFeedOverride(double v) { feedOverride = v; }

    // ----- 坐标位置 -----
    LcncPose absolutePos;    ///< 绝对坐标（机床坐标系）
    LcncPose relativePos;    ///< 相对坐标
    LcncPose workPos;        ///< 工件坐标系位置
    LcncPose distanceToGo;    ///< 距目标点距离

    // ----- 当前 G/M 代码 -----
    ActiveGCodes activeGCodesData;
    ActiveMCodes activeMCodesData;
    int activeGCodes() const { return 0; /* 实际通过 activeGCodesData 访问 */ }
    void setActiveGCodes(int) {}
    int activeMCodes() const { return 0; }
    void setActiveMCodes(int) {}

    // ----- 刀具信息 -----
    LcncToolData currentTool;  ///< 当前刀具
    int toolInSpindle = 0;     ///< 主轴中的刀具号

    // ----- 轴状态 -----
    QVector<LcncAxisStatus> axes;  ///< 各轴状态（最多 9 轴）

    // ----- 文件信息 -----
    QString currentFile;     ///< 当前加载的 G 代码文件
    int currentLine = 0;     ///< 当前行号
    QString currentFile() const { return currentFile; }
    void setCurrentFile(const QString &v) { currentFile = v; }
    int currentLine() const { return currentLine; }
    void setCurrentLine(int v) { currentLine = v; }

    // ----- 程序信息 -----
    int programLine = 0;     ///< 程序总行数
    QString programUnits;    ///< 程序单位（G20/G21）

    // ----- 连接状态 -----
    bool connected = false;  ///< 是否已连接到 NML 服务器

    // ----- 错误信息 -----
    QString errorMessage;    ///< 最近错误信息
};

Q_DECLARE_METATYPE(LcncStatusData)

#endif // LCNC_STATUS_DATA_H
