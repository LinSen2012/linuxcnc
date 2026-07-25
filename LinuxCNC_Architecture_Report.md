# LinuxCNC 架构技术报告

**文档版本**: 1.0
**生成日期**: 2026-06-14
**目标芯片**: STM32F407 (Cortex-M4, 168MHz, 192KB RAM, 1MB Flash)

---

## 目录

1. [工程概述](#1-工程概述)
2. [文件层级结构](#2-文件层级结构)
3. [功能模块划分](#3-功能模块划分)
4. [核心数据结构](#4-核心数据结构)
5. [数据流程](#5-数据流程)
6. [核心算法](#6-核心算法)
7. [STM32F407 移植可行性分析](#7-stm32f407-移植可行性分析)
8. [移植建议](#8-移植建议)

---

## 1. 工程概述

### 1.1 LinuxCNC 简介

LinuxCNC 是一个开源的数控系统软件，最初基于美国 NIST（国家标准技术研究所）的研究项目开发。经过二十多年的发展，已成为工业级 CNC 控制系统的事实标准之一。

**核心技术特点**：
- 实时运动控制（支持 RT-PREEMPT、RTAI、Xenomai 等实时内核）
- 硬件抽象层（HAL）支持灵活的系统配置
- NML 通信机制实现模块间解耦
- RS-274 NGC G-code 解释器
- 高级轨迹规划（支持 S-curve 加速、圆弧拟合、混合前瞻）

### 1.2 系统架构图

```
┌─────────────────────────────────────────────────────────────────┐
│                         用户界面层 (UI)                          │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐  │
│  │  AXIS   │ │  Touchy │ │  GMOCCAPY │ │ TkLinuxCNC │ │  QtVCP  │  │
│  └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘  │
└───────┼───────────┼───────────┼───────────┼───────────┼──────────┘
        │           │           │           │           │
        └───────────┴─────┬─────┴───────────┴───────────┘
                          │
┌─────────────────────────┼─────────────────────────────────────────┐
│                    NML 通信层 (TCP/共享内存)                       │
│                         │                                          │
│  ┌──────────────────────┴──────────────────────────────┐         │
│  │                    EMC TASK                         │         │
│  │  ┌────────────┐  ┌────────────┐  ┌────────────────┐ │         │
│  │  │  解释器    │  │  任务控制   │  │   IO 控制      │ │         │
│  │  │ rs274ngc   │◄─┤ emctask.cc │◄─┤   emcio.cc    │ │         │
│  │  └────────────┘  └─────┬──────┘  └────────────────┘ │         │
│  └────────────────────────┼─────────────────────────────┘         │
│                           │                                        │
│  ┌────────────────────────┴──────────────────────────────────┐    │
│  │              运动控制层 (Motion Controller)                │    │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐  │    │
│  │  │ 轨迹规划器  │◄─┤  运动学      │◄─┤  位置控制器    │  │    │
│  │  │    tp.c    │  │ kins_*.c    │  │    axis.c      │  │    │
│  │  └──────┬──────┘  └─────────────┘  └────────┬────────┘  │    │
│  └─────────┼────────────────────────────────────┼───────────┘    │
│            │                                    │                 │
│  ┌────────┴────────────────────────────────────┴───────────┐    │
│  │              HAL 硬件抽象层                              │    │
│  │  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐  │    │
│  │  │  PID    │  │ Stepgen │  │  Encoder │  │ Parport │  │    │
│  │  └────┬────┘  └────┬────┘  └────┬────┘  └────┬────┘  │    │
│  └───────┼────────────┼────────────┼────────────┼─────────┘    │
└───────────┼────────────┼────────────┼────────────┼──────────────┘
            │            │            │            │
    ┌───────┴────────────┴────────────┴────────────┴───────┐
    │              硬件层 (FPGA/PCI/Parallel Port)          │
    │  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐  │
    │  │ Mesa    │  │  Parport │  │  PWM    │  │  GPIO   │  │
    │  │  cards  │  │         │  │ 输出    │  │         │  │
    │  └─────────┘  └─────────┘  └─────────┘  └─────────┘  │
    └─────────────────────────────────────────────────────────┘
```

---

## 2. 文件层级结构

### 2.1 顶层目录

```
src/
├── emc/          # 核心运动控制
├── hal/          # 硬件抽象层
├── libnml/       # NML 通信库
├── rtapi/        # 实时 API
├── module_helper/# 内核模块辅助
└── m4/           # autoconf 宏
```

### 2.2 EMC 核心目录 (`src/emc/`)

```
emc/
├── linuxcnc.h              # 全局宏定义 (LINELEN, MM_PER_INCH 等)
│
├── nml_intf/               # NML 接口定义
│   ├── emc.hh             # EMC NML 词汇表 (命令/状态类型定义)
│   ├── emc_nml.hh         # EMC NML 消息格式
│   ├── emcglb.h           # EMC 全局变量
│   ├── emcpose.h          # EmcPose 位置结构
│   ├── canon.hh           # 规范命令接口
│   ├── interpl.hh         # 解释器列表
│   └── interp_return.hh    # 解释器返回值
│
├── task/                   # 任务控制器
│   ├── emctask.cc         # 任务状态机 (模式/状态管理)
│   ├── emctaskmain.cc     # 任务主循环
│   ├── emcsvr.cc          # NML 服务器
│   ├── emccanon.cc        # 规范命令实现
│   ├── task.hh            # 任务类接口
│   ├── taskintf.cc        # 任务接口实现
│   └── taskclass.cc       # 任务类
│
├── motion/                 # 运动控制器
│   ├── motion.c           # 运动控制主循环
│   ├── motion.h
│   ├── axis.c             # 单轴 PID 控制
│   ├── axis.h
│   ├── command.c          # 运动命令处理
│   ├── control.c          # 运动控制逻辑
│   ├── homing.c           # 零点回归
│   ├── homemod.c         # 原点修饰
│   ├── dbuf.c             # 运动缓冲队列
│   ├── stashf.c           # 状态暂存
│   └── motion_struct.h     # 运动数据结构
│
├── tp/                     # 轨迹规划器
│   ├── tp.c               # 轨迹规划主循环
│   ├── tp.h
│   ├── tp_types.h          # TP 数据结构
│   ├── tc.c               # 轨迹段控制
│   ├── tc.h
│   ├── tc_types.h
│   ├── tcq.c               # 轨迹队列
│   ├── tcq.h
│   ├── blendmath.c         # 混合数学 (圆弧拟合)
│   ├── sp_scurve.c         # S 曲线加速
│   ├── tpmod.c             # TP 模块接口
│   └── cruckig/            # Ruckig 轨迹规划
│       ├── cruckig.c
│       ├── profile.c
│       ├── trajectory.c
│       └── ...
│
├── rs274ngc/               # G-code 解释器 (RS-274NGC)
│   ├── rs274ngc.hh         # 解释器接口
│   ├── rs274ngc_pre.cc     # 预读解析
│   ├── rs274ngc_interp.hh  # 解释器内部
│   ├── interp_convert.cc    # G/M 代码转换
│   ├── interp_execute.cc   # 解释执行
│   ├── interp_find.cc      # 查找和识别
│   ├── interp_cycles.cc    # 循环代码 (G81-G89)
│   ├── interp_arc.cc       # 圆弧插补
│   ├── interp_remap.cc     # 代码重映射
│   ├── interp_o_word.cc    # O-word 子程序
│   ├── interp_namedparams.cc # 命名参数 (#<name>)
│   ├── interp_setup.cc     # 解释器设置
│   ├── interp_base.cc      # 基础解释器
│   ├── interp_internal.hh  # 解释器内部结构
│   └── modal_state.hh      # 模态状态
│
├── kinematics/             # 运动学
│   ├── trivkins.c         # 三轴线性 (默认)
│   ├── corexykins.c        # CoreXY 结构
│   ├── lineardeltakins.c  # Delta 结构
│   ├── genserkins.c       # 通用串行机器人
│   ├── maxkins.c          # 5 轴
│   └── ...                 # 其他运动学模型
│
├── ini/                    # INI 文件解析
│   ├── inifile.cc/hh      # INI 解析器
│   ├── inihal.cc/hh        # HAL 配置解析
│   ├── initraj.cc/hh      # 轨迹配置解析
│   ├── iniaxis.cc/hh      # 轴配置解析
│   └── inijoint.cc/hh      # 关节配置解析
│
├── sai/                    # SAI (Simple Automation Interface)
│   └── saicanon.cc
│
└── tooldata/               # 刀具数据管理
    ├── tooldata_mmap.cc    # 共享内存方式
    └── tooldata_nml.cc     # NML 方式
```

### 2.3 HAL 目录 (`src/hal/`)

```
hal/
├── hal_lib.c               # HAL 核心库
├── hal.h                   # HAL 公共 API
├── hal_priv.h              # HAL 私有定义
├── halmodule.cc            # Python 模块接口
│
├── components/              # HAL 组件
│   ├── pid.c               # PID 控制器
│   ├── stepgen.c           # 步进电机生成器
│   ├── encoder.c            # 编码器计数器
│   ├── siggen.c             # 信号发生器
│   ├── watchdog.c           # 看门狗
│   ├── limit2.c             # 限位开关
│   ├── abs.c                # 绝对值
│   ├── charge_pump.c         # 充电泵
│   └── ...
│
├── drivers/                 # 硬件驱动
│   ├── hal_gm.c            # General Mechatronics
│   ├── pluto_common.h       # Pluto-SR 驱动
│   ├── motenc.h             # Motenc 驱动
│   └── ...
│
└── utils/                   # HAL 工具
    ├── halcmd.c             # HAL 命令行
    ├── halrmt.c             # 远程 HAL
    ├── halsh.c              # HAL Shell
    ├── scope.c               # 示波器
    └── ...
```

### 2.4 libnml 目录 (`src/libnml/`)

```
libnml/
├── nml/                     # NML 核心
│   ├── nml.cc/hh            # NML 类实现
│   ├── nmlmsg.cc/hh         # NML 消息基类
│   └── nml_oi.cc/hh         # OI (Operator Interface)
│
├── cms/                     # CMS (Communication Management System)
│   ├── cms.cc/hh            # CMS 实现
│   ├── cms_in.cc            # 输入通道
│   ├── cms_up.cc            # 上行通道
│   └── cms_pm.cc            # 性能监控
│
└── rcs/
    └── rcs.hh               # RCS (Realtome Control System) 基础
```

### 2.5 rtapi 目录 (`src/rtapi/`)

```
rtapi/
├── rtapi.h                  # RTAPI 主头文件
├── rtapi_app.h              # RTAPI 应用接口
├── rtapi_common.h           # 通用定义
├── rtapi_math.h             # 数学函数
├── rtapi_string.h           # 字符串函数
├── rtapi_atomic.h           # 原子操作
├── rtapi_mutex.h            # 互斥锁
├── rtapi_list.h             # 链表
├── rtapi_pci.h              # PCI 接口
├── rtapi_io.h               # I/O 操作
├── rtapi_pci.cc             # PCI 实现
│
├── uspace_rtai.cc           # RTAI 用户空间接口
├── uspace_xenomai.cc        # Xenomai 接口
├── uspace_posix.cc          # POSIX 接口
└── uspace_common.h          # 用户空间通用
```

---

## 3. 功能模块划分

### 3.1 模块总览

| 模块名称 | 层级 | 功能描述 | 实时性要求 |
|---------|------|---------|-----------|
| NML 通信 | 用户态 | 命令/状态消息传递 | 非实时 |
| EMC Task | 用户态 | 任务协调、模式切换 | 非实时 |
| G-code 解释器 | 用户态 | G/M/T/S 代码解析 | 非实时 |
| EMC IO | 用户态 | 刀具、冷却液、夹具控制 | 软实时 |
| Motion Controller | 内核态 | 运动命令协调 | 硬实时 |
| Trajectory Planner | 内核态 | 轨迹规划、速度规划 | 硬实时 |
| Axis Control | 内核态 | 单轴 PID 控制 | 硬实时 |
| HAL | 内核/用户态 | 硬件抽象、信号连接 | 视组件而定 |
| Kinematics | 内核态 | 坐标变换 | 硬实时 |

### 3.2 各模块详细说明

#### 3.2.1 G-code 解释器 (rs274ngc)

**功能**：将 NGC 文件中的 G-code 转换为规范命令 (Canon Commands)

**核心文件**：
- `rs274ngc_pre.cc` - 预读解析，识别 G/M 代码
- `interp_convert.cc` - 代码转换执行
- `interp_cycles.cc` - 固定循环 (G81-G89)
- `interp_remap.cc` - 用户自定义代码重映射

**解释流程**：
```
NGC 文件 → 词法分析 → 语法分析 → 语义分析 → 规范命令
         ↓
    read() 函数    ← 单行读取
         ↓
    parse() 函数   ← 解析 G/M/T/S/F... 代码
         ↓
    execute() 函数 ← 执行规范命令 (CANON_*)
```

#### 3.2.2 任务控制器 (EMC Task)

**功能**：协调解释器、运动控制器、IO 模块的状态和时序

**核心文件**：
- `emctask.cc` - 状态机实现
- `emctaskmain.cc` - 主循环
- `emcsvr.cc` - NML 服务器

**状态机**：
```
EMC_TASK_STATE:
├── ESTOP         # 急停
├── ESTOP_RESET   # 急停复位
├── OFF           # 关机
└── ON            # 开机

EMC_TASK_MODE:
├── MANUAL        # 手动模式
├── AUTO          # 自动模式
└── MDI           # 手动数据输入
```

#### 3.2.3 轨迹规划器 (Trajectory Planner)

**功能**：计算最优轨迹，生成平滑的速度曲线

**核心算法**：
- **TP_STRUCT** - 轨迹规划器主状态
- **TC (Trajectory Segment)** - 单个运动段
- **Blend Math** - 过渡混合算法

**关键参数**：
- `vMax` - 最大速度
- `aMax` - 最大加速度
- `jerk` - 加加速度 (加加速度)
- `tolerance` - 混合容差

#### 3.2.4 轴控制器 (Axis Control)

**功能**：单轴的位置/速度闭环控制

**控制结构**：
```c
typedef struct {
    double pos_cmd;        // 位置命令
    double pos_fb;         // 位置反馈
    double vel_cmd;        // 速度命令
    double vel_fb;         // 速度反馈
    double following_error;// 跟随误差
    double home_pos;       // 原点位置
    double min_limit;      // 软件限位最小
    double max_limit;      // 软件限位最大
    int    enable;         // 使能标志
    int    homed;          // 回零完成标志
} EMC_JOINT_STAT;
```

#### 3.2.5 硬件抽象层 (HAL)

**核心概念**：
- **Signal（信号）**：数据载体
- **Pin（引脚）**：组件的连接点
- **Component（组件）**：功能模块

**组件类型**：

| 组件 | 功能 | 实时性 |
|-----|------|--------|
| pid | PID 控制器 | RT |
| stepgen | 步进脉冲生成 | RT |
| encoder | 编码器计数 | RT |
| pwmgen | PWM 生成 | RT |
| watchdog | 看门狗 | RT |
| charge_pump | 充电泵 | RT |
| abs | 绝对值计算 | RT |
| offset | 偏移补偿 | RT |

---

## 4. 核心数据结构

### 4.1 EmcPose - 位置结构

```c
// src/emc/nml_intf/emcpose.h
struct EmcPose {
    double tran.x;     // X 方向平移 (mm)
    double tran.y;     // Y 方向平移 (mm)
    double tran.z;     // Z 方向平移 (mm)
    double a;          // A 轴角度 (度)
    double b;          // B 轴角度 (度)
    double c;          // C 轴角度 (度)
    double u;          // U 轴 (辅助轴)
    double v;          // V 轴
    double w;          // W 轴
};
```

### 4.2 TP_STRUCT - 轨迹规划器状态

```c
// src/emc/tp/tp_types.h
typedef struct {
    TC_QUEUE_STRUCT queue;      // 运动队列
    tp_spindle_t spindle;       // 主轴同步数据

    EmcPose currentPos;         // 当前位置
    EmcPose goalPos;           // 目标位置

    int queueSize;             // 队列大小
    double cycleTime;          // 周期时间 (秒)

    double vMax;               // 最大速度 (units/sec)
    double ini_maxvel;         // 初始最大速度
    double vLimit;            // 速度限制

    double aMax;               // 最大加速度
    double ini_maxjerk;        // 初始最大加加速度
    double aMaxCartesian;     // 笛卡尔最大加速度

    int nextId;               // 下一个运动 ID
    int execId;               // 当前执行 ID
    struct state_tag_t execTag;

    int termCond;              // 终止条件 (STOP/EXACT/BLEND)
    int done;                  // 运动完成标志
    int depth;                 // 队列深度
    int activeDepth;           // 混合深度
    int aborting;              // 中止标志
    int pausing;                // 暂停标志

    double tolerance;          // 混合容差
    int synchronized;          // 主轴同步标志
} TP_STRUCT;
```

### 4.3 EMC_STAT - 主状态结构

```c
// src/emc/nml_intf/emc.hh (简化版)
class EMC_STAT {
public:
    // 任务状态
    EMC_TASK_STAT task;

    // 轨迹状态
    EMC_TRAJ_STAT motion;

    // IO 状态
    EMC_IO_STAT io;

    // 时间戳
    double time;              // 时间戳 (秒)
    int echo_serial_number;   // 回显序列号
    int command_serial_number;// 命令序列号
};
```

### 4.4 EMC_TASK_STAT - 任务状态

```c
typedef struct {
    // 状态
    EMC_TASK_STATE state;     // ESTOP/ESTOP_RESET/OFF/ON
    EMC_TASK_MODE mode;       // MANUAL/AUTO/MDI
    EMC_TASK_EXEC execState;  // 执行状态
    EMC_TASK_INTERP interpState; // 解释器状态

    // 程序执行
    int callLevel;            // 子程序调用深度
    char file[LINELEN];       // 当前文件
    int lineNumber;           // 当前行号
    int id;                   // 当前运动 ID

    // 主动作
    double currentVel;        // 当前速度
    EmcPose currentWorldPosition; // 世界坐标位置

    // 坐标系偏置
    int g5xIndex;            // 当前坐标系 (1=G54, 2=G55, ...)
    EmcPose g5xOffset;       // 坐标系偏置
    EmcPose g92Offset;       // G92 偏置

    // 模态状态
    int activeGCodes[ACTIVE_G_CODES];   // 主动 G 代码
    int activeMCodes[ACTIVE_M_CODES];   // 主动 M 代码
    double feedRate;         // 进给率
    double spindleSpeed;      // 主轴速度
    int toolNumber;           // 刀具号

    // 错误
    int interpreterErrcode;   // 解释器错误码
} EMC_TASK_STAT;
```

### 4.5 EMC_TRAJ_STAT - 轨迹状态

```c
typedef struct {
    // 运动状态
    EMC_TRAJ_MODE mode;      // FREE/COORD/TELEOP
    double velocity;          // 当前速度
    double acceleration;      // 当前加速度
    EmcPose position;        // 当前位置 (笛卡尔)

    // 规划器状态
    int queueSize;          // 队列大小
    int activeQueueSize;     // 活跃队列大小
    int id;                  // 当前运动 ID
    int paused;             // 暂停标志

    // 限位
    double maxVelocity;       // 最大速度限制
    double maxAcceleration;   // 最大加速度限制

    // 坐标系
    int joints;              // 关节数
    EmcPose origin;         // 原点偏移

    // 主轴
    int spindleSpeed;         // 主轴速度
    int spindleDir;          // 主轴方向
    double spindleScale;     // 主轴倍率

    // 同步
    int synchronized;        // 主轴同步标志
} EMC_TRAJ_STAT;
```

### 4.6 EMC_JOINT_STAT - 轴状态

```c
typedef struct {
    // 位置
    double position;         // 当前位置
    double velocity;         // 当前速度
    double acceleration;      // 当前加速度

    // 命令
    double posCmd;           // 位置命令
    double velCmd;           // 速度命令

    // 反馈
    double posFb;            // 位置反馈
    double velFb;            // 速度反馈

    // 误差
    double followingError;   // 跟随误差
    double maxFollowingError;// 最大允许跟随误差

    // 限位
    double minPositionLimit; // 最小位置限制
    double maxPositionLimit; // 最大位置限制
    int minSoftLimit;        // 软限位触发
    int maxSoftLimit;        // 软限位触发
    int minHardLimit;        // 硬限位触发
    int maxHardLimit;        // 硬限位触发

    // 原点
    double homePosition;     // 原点位置
    int homed;              // 回零完成
    int homing;             // 回零中
    int fault;              // 故障标志

    // PID 参数
    double P;                // 比例系数
    double I;                // 积分系数
    double D;                // 微分系数
    double FF0;              // 前馈 0
    double FF1;              // 前馈 1
    double FF2;              // 前馈 2
    double backtrace;        // 滞后量
} EMC_JOINT_STAT;
```

---

## 5. 数据流程

### 5.1 G-code 执行流程

```
┌──────────────────────────────────────────────────────────────────┐
│                        用户界面 (AXIS/QtVCP)                       │
│  1. 加载 NGC 文件 / 输入 MDI 命令                                  │
└────────────────────────────────┬─────────────────────────────────┘
                                 │ NML 命令
┌────────────────────────────────┴─────────────────────────────────┐
│                        EMC Task Controller                        │
│  2. emctask.cc 接收命令                                          │
│  3. 状态机处理 (ESTOP/MANUAL/AUTO/MDI)                          │
└─────────────────────────────┬───────────────────────────────────┘
                              │ 解释器调用
┌─────────────────────────────┴───────────────────────────────────┐
│                      G-code Interpreter                           │
│  4. rs274ngc_pre.cc 预读解析                                     │
│  5. 识别 G/M/T/S/F 代码                                          │
│  6. 调用规范命令 CANON_*()                                       │
│                                                                     │
│  示例命令流：                                                       │
│  CANON_LINEAR_MOVE(x, y, z, feed)   # 直线移动                   │
│  CANON_ARC_MOVE(center, end, ...)    # 圆弧移动                   │
│  CANON_SET_SPINDLE_SPEED(rpm)         # 设置主轴速度               │
│  CANON_TOOL_LOAD(toolno)              # 换刀                        │
└─────────────────────────────┬───────────────────────────────────┘
                              │ 运动命令
┌─────────────────────────────┴───────────────────────────────────┐
│                      Motion Controller                             │
│  7. emctaskmain.cc 处理规范命令                                   │
│  8. 生成运动命令发送到 TP                                          │
└─────────────────────────────┬───────────────────────────────────┘
                              │ 轨迹规划
┌─────────────────────────────┴───────────────────────────────────┐
│                      Trajectory Planner                            │
│  9. tp.c 轨迹规划                                                │
│  10. 速度优化、混合计算                                           │
│  11. 输出轨迹段到 TC 队列                                         │
└─────────────────────────────┬───────────────────────────────────┘
                              │ 轨迹段
┌─────────────────────────────┴───────────────────────────────────┐
│                      Axis Controllers                             │
│  12. axis.c 每个轴独立控制                                        │
│  13. PID 控制循环                                                 │
│  14. 输出 PWM/STEP 信号                                          │
└─────────────────────────────┬───────────────────────────────────┘
                              │ PWM/STEP
┌─────────────────────────────┴───────────────────────────────────┐
│                           HAL Layer                                │
│  15. stepgen/pid 等组件                                          │
│  16. 信号连接和管理                                               │
└─────────────────────────────┬───────────────────────────────────┘
                              │ GPIO
┌─────────────────────────────┴───────────────────────────────────┐
│                        Hardware                                   │
│  17. 驱动器 / 电机                                                │
└──────────────────────────────────────────────────────────────────┘
```

### 5.2 NML 通信流程

```
┌────────────────┐         NML          ┌────────────────┐
│   GUI 进程     │ ◄─────────────────► │   EMC Task     │
│   (AXIS)      │    命令/状态通道      │   进程          │
└────────────────┘                      └───────┬────────┘
                                                │
                           ┌────────────────────┼────────────────────┐
                           │                    │                    │
                           ▼                    ▼                    ▼
                    ┌──────────┐        ┌──────────┐        ┌──────────┐
                    │ Motion   │        │ IO       │        │ NML      │
                    │ 控制器   │        │ 控制器   │        │ 服务器   │
                    └────┬─────┘        └────┬─────┘        └────┬─────┘
                         │                   │                    │
                         │ 共享内存            │ 共享内存           │ 监听
                         ▼                   ▼                    │
                    ┌────────────────────────────────┐             │
                    │      运动控制内核模块            │             │
                    │      (HAL + RTAPI)              │             │
                    └────────────────────────────────┘             │
                                                                     │
                                                                     │
                    ┌────────────────────────────────┐             │
                    │      硬件层                      │◄────────────┘
                    │  (PWM / Encoder / GPIO)         │
                    └────────────────────────────────┘
```

### 5.3 轨迹规划数据流

```
输入 (来自解释器):
┌─────────────────────────────────┐
│  EmcPose target     目标位置     │
│  double velocity    目标速度     │
│  double accel       加速度      │
│  int motionType     运动类型    │
│  (LINEAR/ARC/HOME)             │
└─────────────────┬───────────────┘
                  │
                  ▼
┌─────────────────────────────────┐
│       tp.c 轨迹规划器            │
│  ┌─────────────────────────┐    │
│  │ 1. 运动学生成            │    │
│  │ 2. 速度限制计算          │    │
│  │ 3. 混合点计算 (可选)     │    │
│  │ 4. 加加速度限制 (jerk)   │    │
│  └─────────────────────────┘    │
└─────────────────┬───────────────┘
                  │
                  ▼
┌─────────────────────────────────┐
│       TC 轨迹段控制器            │
│  ┌─────────────────────────┐    │
│  │ 1. 轨迹段出队           │    │
│  │ 2. 位置/速度计算        │    │
│  │ 3. 插补周期输出         │    │
│  └─────────────────────────┘    │
└─────────────────┬───────────────┘
                  │
                  ▼
┌─────────────────────────────────┐
│       axis.c 轴控制器            │
│  ┌─────────────────────────┐    │
│  │ 1. 位置环 PID           │    │
│  │ 2. 速度前馈             │    │
│  │ 3. 输出限幅             │    │
│  └─────────────────────────┘    │
└─────────────────┬───────────────┘
                  │
                  ▼
┌─────────────────────────────────┐
│       HAL Signal                │
│  axis.N.motor-pos-cmd          │
│  axis.N.motor-pos-fb           │
│  axis.N.motor-output           │
└─────────────────────────────────┘
```

---

## 6. 核心算法

### 6.1 轨迹规划算法

#### 6.1.1 TP 循环 (tp.c)

```c
// 核心循环，每周期调用一次
int tpRunCycle(TP_STRUCT *tp, long period_nsec)
{
    // 1. 检查队列状态
    if (tp->aborting) {
        tpAbort(tp);
        return TP_ERR_FAIL;
    }

    // 2. 暂停处理
    if (tp->pausing) {
        tpPause(tp);
        return TP_ERR_WAITING;
    }

    // 3. 获取下一个轨迹段
    if (tp->done) {
        // 检查队列，必要时加载新运动
        tpGetNext(tp);
    }

    // 4. 计算当前位置
    tpUpdateCurrentPos(tp);

    // 5. 速度规划
    tpPlanVelocity(tp);

    // 6. 输出到 TC
    tpOutput(tp);

    return TP_ERR_OK;
}
```

#### 6.1.2 S-Curve 加速 (sp_scurve.c)

```c
// S 曲线生成状态机
typedef enum {
    SCURVE_PHASE_START,      // 启动阶段
    SCURVE_PHASE_ACCEL_UP,    // 加速中
    SCURVE_PHASE_CONST_JERK, // 恒加加速
    SCURVE_PHASE_ACCEL_DOWN, // 减速中
    SCURVE_PHASE_CONST_VEL,  // 匀速
    SCURVE_PHASE_DECEL_DOWN, // 减速中
    SCURVE_PHASE_DECEL_CONST,// 恒减加速
    SCURVE_PHASE_DECEL_UP,   // 加速中
    SCURVE_PHASE_END         // 结束
} scurve_phase_t;
```

### 6.2 PID 控制算法 (axis.c)

```c
// 标准 PID 控制
double calculate_pid(double cmd, double fb, double *error_sum, double dt)
{
    double error = cmd - fb;

    // 比例
    double P = error * Kp;

    // 积分 (带抗饱和)
    *error_sum += error * dt;
    double I = *error_sum * Ki;

    // 微分
    double D = (error - prev_error) / dt * Kd;

    // 输出
    double output = P + I + D;

    // 输出限幅
    if (output > max_output) output = max_output;
    if (output < min_output) output = min_output;

    prev_error = error;
    return output;
}
```

### 6.3 回零算法 (homing.c)

```c
// 回零流程状态机
typedef enum {
    HOMING_NOT_STARTED,      // 未开始
    HOMING_SEARCH,            // 搜索阶段
    HOMING_LATCH,            // 捕获阶段
    HOMING_FALL,             // 下降沿
    HOMING_LOCK,             // 锁定
    HOMING_SYNC_LOCK,        // 同步锁定
    HOMING_FINISHED,         // 完成
    HOMING_ABORT             // 中止
} homing_state_t;

// 回零过程:
// 1. SEARCH: 快速搜索限位开关
// 2. LATCH: 低速捕获索引脉冲
// 3. LOCK: 设置位置
```

### 6.4 运动学算法 (kins_*.c)

```c
// 示例: 三轴线性运动学 (trivkins.c)
int kinematicsForward(const double *joints, EmcPose *world,
                      const KINEMATICS_FORWARD_FLAGS *fflags,
                      KINEMATICS_INVERSE_FLAGS *iflags)
{
    // 关节空间 -> 世界空间 (笛卡尔)
    world->tran.x = joints[0];  // X
    world->tran.y = joints[1];  // Y
    world->tran.z = joints[2];  // Z
    world->a = 0.0;
    world->b = 0.0;
    world->c = 0.0;
    return 0;
}

int kinematicsInverse(const EmcPose *world, double *joints,
                     const KINEMATICS_INVERSE_FLAGS *iflags,
                     KINEMATICS_FORWARD_FLAGS *fflags)
{
    // 世界空间 -> 关节空间
    joints[0] = world->tran.x;
    joints[1] = world->tran.y;
    joints[2] = world->tran.z;
    return 0;
}
```

---

## 7. STM32F407 移植可行性分析

### 7.1 STM32F407 资源评估

| 资源 | STM32F407 规格 | LinuxCNC 需求 | 可行性 |
|------|--------------|--------------|--------|
| 主频 | 168 MHz (Cortex-M4) | 需要确定性 | ⚠️ 需要 RTX/OS |
| RAM | 192 KB | ~50-100KB (裁剪后) | ⚠️ 紧张 |
| Flash | 1 MB | ~200-500KB | ⚠️ 紧张 |
| FPU | 单精度浮点 | 需要 | ✅ 支持 |
| GPIO | 140+ | 足够 | ✅ 足够 |
| 定时器 | 12 x 16-bit, 2 x 32-bit | 位置环控制 | ✅ 足够 |
| DMA | 16 通道 | 数据传输 | ✅ 足够 |
| CAN | 有 | 可选 | ✅ 支持 |

### 7.2 移植分层设计

```
┌─────────────────────────────────────────────────────────────────┐
│                      应用层 (CNC 控制器)                          │
│  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐      │
│  │ G-code 解释器 │  │ 轨迹规划器    │  │ 任务控制器    │      │
│  │  (简化版)    │  │  (嵌入式版)   │  │              │      │
│  └───────────────┘  └───────────────┘  └───────────────┘      │
└─────────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────┴───────────────────────────────────┐
│                      中间层 (抽象接口)                            │
│  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐      │
│  │ NML → 消息队列 │  │ HAL 简化版   │  │ 定时器抽象    │      │
│  └───────────────┘  └───────────────┘  └───────────────┘      │
└─────────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────┴───────────────────────────────────┐
│                      驱动层 (STM32 HAL/LL)                       │
│  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐      │
│  │ PWM 输出      │  │ Encoder 接口  │  │ GPIO 控制    │      │
│  └───────────────┘  └───────────────┘  └───────────────┘      │
└─────────────────────────────────────────────────────────────────┘
```

### 7.3 组件裁剪/替代方案

| LinuxCNC 组件 | STM32 替代方案 | 说明 |
|--------------|---------------|------|
| NML 通信 | FreeRTOS 队列/消息 | 进程内通信，无需网络 |
| G-code 解释器 | 简化版解释器 | 预编译为查找表 |
| TP 轨迹规划 | 嵌入式 TP | 减少混合算法复杂度 |
| HAL 组件 | 直接驱动调用 | 无需软件 HAL |
| 运动学 | 固定运动学 | 移除动态加载 |
| Python UI | 串口/USB 上位机 | 外部控制器 |

### 7.4 推荐架构

```
┌──────────────────────────────────────────────┐
│            STM32F407 CNC 控制器               │
│                                              │
│  ┌────────────────────────────────────────┐ │
│  │      FreeRTOS 实时操作系统              │ │
│  ├────────────────────────────────────────┤ │
│  │                                         │ │
│  │   ┌─────────┐  ┌─────────┐            │ │
│  │   │轨迹规划 │  │ PID     │  1-2ms    │ │
│  │   │  (TP)   │  │ 控制    │  周期     │ │
│  │   └────┬────┘  └────┬────┘            │ │
│  │        │            │                 │ │
│  │   ┌────┴────────────┴─────┐          │ │
│  │   │   位置/速度规划        │          │ │
│  │   └───────────┬───────────┘          │ │
│  │               │                       │ │
│  │   ┌───────────┴───────────┐           │ │
│  │   │   运动学正逆解        │           │ │
│  │   └───────────┬───────────┘           │ │
│  │               │                       │ │
│  │   ┌───────────┴───────────┐           │ │
│  │   │   限位/安全检查        │           │ │
│  │   └───────────┬───────────┘           │ │
│  │               │                       │ │
│  │   ┌───────────┴───────────┐           │ │
│  │   │   PWM/编码器驱动      │  ≤100μs  │ │
│  │   └───────────┬───────────┘           │ │
│  │               │                       │ │
│  └───────────────┼───────────────────────┘ │
│                  │                         │
│  ┌───────────────┼───────────────────────┐ │
│  │   UART/CAN   │   外部通信             │ │
│  └───────────────┴───────────────────────┘ │
└──────────────────────────────────────────────┘

外部设备:
┌──────────────────────────────────────────────┐
│            PC 上位机 (可选)                    │
│  ┌────────────┐  ┌────────────┐              │
│  │ G-code 编辑 │  │ 状态显示   │              │
│  └──────┬─────┘  └──────┬─────┘              │
│         │                │                    │
│         └───────┬────────┘                    │
│                 │ RS232/USB                   │
└─────────────────┼──────────────────────────────┘
```

---

## 8. 移植建议

### 8.1 分阶段移植计划

#### 阶段 1: 基础驱动 (1-2 周)
- PWM 输出 (TIM1/TIM8)
- 编码器接口 (TIM2-5)
- GPIO 限位输入
- 串口通信

#### 阶段 2: 运动控制核心 (2-4 周)
- PID 控制闭环
- 基础轨迹规划 (梯形/S 曲线)
- 运动学正逆解
- 原点回归

#### 阶段 3: G-code 支持 (4-8 周)
- 简化 G-code 解释器
- 固定循环支持
- 坐标系管理
- 刀具补偿

#### 阶段 4: 系统集成 (2-4 周)
- FreeRTOS 任务调度
- 参数存储 (Flash)
- 安全功能
- 外部通信

### 8.2 关键技术点

1. **实时性保证**
   - 使用 FreeRTOS + CMSIS-RTOS
   - 运动控制任务最高优先级
   - 禁用所有浮点中断

2. **内存管理**
   - 静态内存分配 (无 malloc)
   - 轨迹队列预分配
   - 参数存储在 Flash

3. **数值精度**
   - 使用 `float` 而非 `double` (FPU 优化)
   - 定标处理整数运算
   - 固定点运算可选

4. **代码组织**
   ```c
   // 目录结构
   stm32_cnc/
   ├── Core/
   │   ├── Inc/          # 头文件
   │   └── Src/          # 源文件
   ├── Drivers/          # HAL 驱动
   ├── Middlewares/
   │   ├── CNC/          # CNC 核心
   │   │   ├── tp/       # 轨迹规划
   │   │   ├── interp/   # 解释器
   │   │   ├── motion/   # 运动控制
   │   │   └── kins/     # 运动学
   │   └── HAL/          # 硬件抽象
   └── Application/      # 应用
   ```

### 8.3 建议的开发板

| 开发板 | MCU | 特点 | 推荐度 |
|-------|-----|------|--------|
| STM32F407 Discovery | F407VGT6 | 板载 ST-LINK | ⭐⭐⭐⭐⭐ |
| STM32F429 Discovery | F429ZIT6 | 更多资源 | ⭐⭐⭐⭐ |
| 自定义板 | F407IGT6 | 工业级 | ⭐⭐⭐⭐⭐ |

### 8.4 性能估算

| 功能 | STM32F407 能力 | LinuxCNC 原需求 | 满足度 |
|------|---------------|----------------|--------|
| 轨迹周期 | 1-2 ms | 1 ms | ⚠️ 勉强 |
| 位置环周期 | 100-200 μs | 50-100 μs | ⚠️ 可接受 |
| 轴数 | 3-6 轴 | 最多 9 轴 | ✅ 足够 |
| 轨迹段缓冲 | 10-20 段 | 32 段 | ✅ 足够 |
| G-code 解析 | ~1000 行/秒 | 实时 | ✅ 足够 |

---

## 附录 A: 文件索引

### 核心头文件

| 文件 | 描述 | 优先级 |
|-----|------|--------|
| `emc/nml_intf/emc.hh` | EMC NML 词汇表 | 🔴 必须 |
| `emc/tp/tp_types.h` | TP 数据结构 | 🔴 必须 |
| `emc/motion/motion.h` | 运动控制接口 | 🔴 必须 |
| `hal/hal.h` | HAL API | 🟡 推荐 |
| `libnml/nml/nml.hh` | NML 接口 | 🟡 推荐 |

### 核心源文件

| 文件 | 描述 | 优先级 |
|-----|------|--------|
| `emc/task/emctask.cc` | 任务控制器 | 🔴 必须 |
| `emc/tp/tp.c` | 轨迹规划器 | 🔴 必须 |
| `emc/motion/axis.c` | 轴控制器 | 🔴 必须 |
| `emc/rs274ngc/interp_convert.cc` | G-code 转换 | 🟡 推荐 |

---

## 附录 B: 联系方式

**项目主页**: https://github.com/LinuxCNC/linuxcnc
**文档**: https://linuxcnc.org/docs/

---

*本文档由 AI 辅助生成，基于 LinuxCNC 源码分析*
*生成时间: 2026-06-14*
