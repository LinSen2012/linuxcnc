/********************************************************************
* Description: LcncStatusData.h
*   LcncStatusData — CNC machine state data structure for
*   linuxQtCnc. Defines all runtime state information for a CNC
*   machine control: machine state (EStop/On/Off), motion mode
*   (Manual/Auto/MDI), interpreter state (Idle/Running/Paused),
*   position coordinates (absolute/relative/work), feed rate,
*   spindle speed, current G/M/T codes, tool info, limit switch
*   state, and more. Uses Q_GADGET for QVariant transport with
*   Q_PROPERTY metadata.
*
*   Derived from a work by Fred Proctor & Will Shackleford
*
* Author: linuxQtCnc project
* License: GPL Version 2
* System: Linux / Windows
*
* Copyright (c) 2026 All rights reserved.
********************************************************************/

#ifndef LCNC_STATUS_DATA_H
#define LCNC_STATUS_DATA_H

#include <QString>
#include <QVector>

// ============================================================================
// 前向声明 - LinuxCNC 核心类型（实际集成时替换为真实头文件）
// ============================================================================
// 在 Windows 开发环境中，我们定义兼容类型
// 在 Linux 上集成时，这些类型将来自 LinuxCNC 核心头文件

/// 三维位姿结构体（兼容 LinuxCNC EmcPose）
/// 与 EmcPose 保持一致：tran 代表平动部分，a/b/c 为旋转轴
struct LcncTran {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct LcncPose {
    LcncTran tran;        ///< 平动部分 (X/Y/Z)
    double a = 0.0;       ///< A 轴坐标（旋转轴）
    double b = 0.0;       ///< B 轴坐标（旋转轴）
    double c = 0.0;       ///< C 轴坐标（旋转轴）
    double u = 0.0;       ///< U 轴坐标
    double v = 0.0;       ///< V 轴坐标
    double w = 0.0;       ///< W 轴坐标

    LcncPose() = default;
    LcncPose(double x_, double y_, double z_,
             double a_ = 0.0, double b_ = 0.0, double c_ = 0.0,
             double u_ = 0.0, double v_ = 0.0, double w_ = 0.0)
        : a(a_), b(b_), c(c_), u(u_), v(v_), w(w_) {
        tran.x = x_;
        tran.y = y_;
        tran.z = z_;
    }

    bool operator==(const LcncPose &other) const {
        return tran.x == other.tran.x && tran.y == other.tran.y
               && tran.z == other.tran.z && a == other.a && b == other.b
               && c == other.c && u == other.u && v == other.v
               && w == other.w;
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
    double a = 0.0;         ///< A 轴偏置
    double b = 0.0;         ///< B 轴偏置
    double c = 0.0;         ///< C 轴偏置
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

    // Q_PROPERTY (MEMBER) 声明 - 使用 MEMBER 直接绑定到字段，
    // 避免字段名与 getter 方法名冲突，同时保留字段直接访问能力
    Q_PROPERTY(int taskState      MEMBER taskState)
    Q_PROPERTY(int motionMode     MEMBER motionMode)
    Q_PROPERTY(int interpState    MEMBER interpState)
    Q_PROPERTY(int spindleState   MEMBER spindleState)
    Q_PROPERTY(double feedrate    MEMBER feedrate)
    Q_PROPERTY(double spindleSpeed MEMBER spindleSpeed)
    Q_PROPERTY(double spindleOverride MEMBER spindleOverride)
    Q_PROPERTY(double feedOverride MEMBER feedOverride)
    Q_PROPERTY(QString currentFile MEMBER m_currentFile)
    Q_PROPERTY(int currentLine    MEMBER m_currentLine)

public:
    LcncStatusData() = default;

    // ----- 机床状态 -----
    MachineState taskState = MachineState::ESTOP;

    // ----- 运动模式 -----
    MotionMode motionMode = MotionMode::MANUAL;

    // ----- 解释器状态 -----
    InterpState interpState = InterpState::IDLE;

    // ----- 主轴状态 -----
    SpindleState spindleState = SpindleState::STOPPED;

    // ----- 进给速率 -----
    double feedrate = 0.0;

    // ----- 主轴转速 -----
    double spindleSpeed = 0.0;

    // ----- 主轴倍率 -----
    double spindleOverride = 1.0;

    // ----- 进给倍率 -----
    double feedOverride = 1.0;

    // ----- 坐标位置 -----
    LcncPose absolutePos;    ///< 绝对坐标（机床坐标系）
    LcncPose relativePos;   ///< 相对坐标
    LcncPose workPos;       ///< 工件坐标系位置
    LcncPose distanceToGo; ///< 距目标点距离
    LcncPose g5xOffset;     ///< G5x 工件偏置
    LcncPose g92Offset;     ///< G92 偏置
    int g5xIndex = 0;       ///< 当前工件坐标系索引 (1=G54, 2=G55, ...)

    // ----- 当前 G/M 代码 -----
    ActiveGCodes activeGCodesData;
    ActiveMCodes activeMCodesData;

    // ----- 刀具信息 -----
    LcncToolData currentTool;  ///< 当前刀具
    int toolInSpindle = 0;     ///< 主轴中的刀具号

    // ----- 冷却液状态 -----
    bool coolantMist = false;  ///< 雾化冷却开启
    bool coolantFlood = false; ///< 喷流冷却开启

    // ----- 轴状态 -----
    QVector<LcncAxisStatus> axes;  ///< 各轴状态（最多 9 轴）

    // ----- 文件信息 -----
    QString m_currentFile;   ///< 当前加载的 G 代码文件
    int m_currentLine = 0;   ///< 当前行号

    // 便捷访问方法（为兼容旧代码保留，不会与字段名冲突）
    QString currentFile() const { return m_currentFile; }
    void setCurrentFile(const QString &v) { m_currentFile = v; }
    int currentLine() const { return m_currentLine; }
    void setCurrentLine(int v) { m_currentLine = v; }

    // ----- 程序信息 -----
    int programLine = 0;     ///< 程序总行数
    QString programUnits;    ///< 程序单位（G20/G21）

    // ----- 连接状态 -----
    bool connected = false;  ///< 是否已连接到 NML 服务器

    // ----- 错误信息 -----
    QString errorMessage;    ///< 最近错误信息

    // ----- 比较运算符（状态变化检测） -----
    bool operator==(const LcncStatusData &other) const {
        return taskState == other.taskState
            && motionMode == other.motionMode
            && interpState == other.interpState
            && spindleState == other.spindleState
            && feedrate == other.feedrate
            && spindleSpeed == other.spindleSpeed
            && spindleOverride == other.spindleOverride
            && feedOverride == other.feedOverride
            && m_currentFile == other.m_currentFile
            && m_currentLine == other.m_currentLine
            && toolInSpindle == other.toolInSpindle
            && connected == other.connected;
    }
    bool operator!=(const LcncStatusData &other) const { return !(*this == other); }
};

Q_DECLARE_METATYPE(LcncStatusData)

#endif // LCNC_STATUS_DATA_H
