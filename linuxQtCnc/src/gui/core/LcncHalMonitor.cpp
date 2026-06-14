/*
 * linuxQtCncGui - LinuxCNC Qt5 GUI 控制器
 * LcncHalMonitor - HAL 信号监视器实现
 *
 * 在 Linux 上通过 HAL API 读取真实信号值。
 * 在 Windows 上使用模拟数据进行开发测试。
 */

#include "LcncHalMonitor.h"

#include <QDebug>
#include <QDateTime>
#include <cmath>

// ============================================================================
// Linux HAL 头文件（仅在 Linux 下编译）
// ============================================================================
#ifdef Q_OS_LINUX
#include "hal.h"
#include "hal_lib.h"
#endif

// ============================================================================
// 静态成员初始化
// ============================================================================

LcncHalMonitor *LcncHalMonitor::m_instance = nullptr;

// ============================================================================
// 构造 / 析构
// ============================================================================

LcncHalMonitor::LcncHalMonitor(QObject *parent)
    : QObject(parent)
    , m_refreshTimer(new QTimer(this))
{
    qInfo().noquote() << "[LcncHalMonitor] HAL 监视器已创建";

#ifdef Q_OS_LINUX
    // 尝试初始化 Linux HAL
    if (!initLinuxHal()) {
        qWarning().noquote() << "[LcncHalMonitor] HAL 初始化失败，使用模拟数据";
        initSimulation();
    }
#else
    // Windows 下使用模拟数据
    initSimulation();
#endif

    // 连接刷新定时器
    connect(m_refreshTimer, &QTimer::timeout, this, &LcncHalMonitor::refresh);
}

LcncHalMonitor::~LcncHalMonitor()
{
    stop();
    m_instance = nullptr;
    qInfo().noquote() << "[LcncHalMonitor] HAL 监视器已销毁";
}

LcncHalMonitor *LcncHalMonitor::instance()
{
    if (!m_instance) {
        m_instance = new LcncHalMonitor();
    }
    return m_instance;
}

// ============================================================================
// Linux HAL 初始化
// ============================================================================

#ifdef Q_OS_LINUX
bool LcncHalMonitor::initLinuxHal()
{
    // 连接到 HAL 共享内存
    // HAL 使用 RTAPI/ULAPI 机制
    // 我们需要获取 HAL 组件的 ID 或者使用直接的共享内存访问

    // 尝试加载 hal_lib 并获取信号
    // 注意: 实际的 HAL 信号访问需要特殊权限和初始化
    // 这里提供一个框架，实际集成时需要完善

    // 示例: 初始化 HAL 组件（如果需要）
    // m_halCompId = hal_init("lcnc_hal_monitor");
    // if (m_halCompId < 0) {
    //     return false;
    // }

    m_connected = false;
    m_useSimulation = true;  // 暂时使用模拟，真实 HAL 需要进一步集成

    qInfo().noquote() << "[LcncHalMonitor] Linux HAL 框架已加载（模拟模式）";
    return true;
}

void LcncHalMonitor::readLinuxSignals()
{
    if (m_useSimulation || !m_connected) {
        readSimulation();
        return;
    }

    // 真实 HAL 信号读取
    // 使用 hal_get_signal_value_by_name() 读取每个信号
    // for (const auto &sig : m_signals) {
    //     hal_type_t type;
    //     hal_data_u *data;
    //     bool has_writers;
    //     if (hal_get_signal_value_by_name(sig.name.toLocal8Bit().constData(),
    //                                      &type, &data, &has_writers) == 0) {
    //         // 读取值并转换为 QVariant
    //         switch (type) {
    //         case HAL_BIT: sig.value = data->bit; break;
    //         case HAL_FLOAT: sig.value = data->float_data; break;
    //         case HAL_S32: sig.value = data->s32; break;
    //         case HAL_U32: sig.value = data->u32; break;
    //         ...
    //         }
    //     }
    // }
}
#endif

// ============================================================================
// 模拟数据
// ============================================================================

void LcncHalMonitor::initSimulation()
{
    m_useSimulation = true;
    m_connected = true;  // 模拟模式下视为已连接

    // 添加一些示例信号
    m_signals.clear();
    m_signalIndices.clear();

    // 运动相关信号
    addSignal("motion-command-handle", QStringLiteral("运动命令句柄"));
    addSignal("motion-analog-in-00", QStringLiteral("模拟输入0"));
    addSignal("joint-0-pos-fb", QStringLiteral("关节0位置反馈"));
    addSignal("joint-1-pos-fb", QStringLiteral("关节1位置反馈"));
    addSignal("joint-2-pos-fb", QStringLiteral("关节2位置反馈"));
    addSignal("spindle-vel-cmd", QStringLiteral("主轴速度命令"));
    addSignal("spindle-vel-fb", QStringLiteral("主轴速度反馈"));
    addSignal("coolant-flood", QStringLiteral("冷却液喷流"));
    addSignal("coolant-mist", QStringLiteral("冷却液雾化"));
    addSignal("probe-in", QStringLiteral("探头输入"));
    addSignal("estop-in", QStringLiteral("急停输入"));
    addSignal("home-0", QStringLiteral("原点开关0"));
    addSignal("home-1", QStringLiteral("原点开关1"));
    addSignal("home-2", QStringLiteral("原点开关2"));

    qInfo().noquote() << "[LcncHalMonitor] 模拟数据已初始化，共"
                       << m_signals.size() << "个信号";
}

void LcncHalMonitor::readSimulation()
{
    // 模拟数据更新 - 添加小幅随机变化
    for (auto &sig : m_signals) {
        QVariant oldValue = sig.value;

        switch (sig.type) {
        case HalType::BIT: {
            // 位信号偶尔翻转
            if ((QDateTime::currentMSecsSinceEpoch() % 1000) < 10) {
                sig.value = !sig.value.toBool();
            }
            break;
        }
        case HalType::FLOAT: {
            // 浮点信号添加噪声
            double val = sig.value.toDouble();
            double noise = (qrand() % 100 - 50) / 10000.0;
            sig.value = val + noise;
            break;
        }
        case HalType::S32:
        case HalType::U32:
        case HalType::S64:
        case HalType::U64: {
            // 整数信号添加小幅变化
            int val = sig.value.toInt();
            int noise = qrand() % 5 - 2;
            sig.value = val + noise;
            break;
        }
        }

        if (sig.value != oldValue) {
            emit signalValueChanged(sig.name, sig.value);
        }
    }
}

// ============================================================================
// 信号管理
// ============================================================================

bool LcncHalMonitor::addSignal(const QString &name, const QString &desc)
{
    if (m_signalIndices.contains(name)) {
        return false;  // 已存在
    }

    HalSignalInfo info;
    info.name = name;
    info.description = desc;
    info.type = HalType::FLOAT;  // 默认浮点
    info.dir = HalPinDir::IO;
    info.value = 0.0;

    int index = m_signals.size();
    m_signals.append(info);
    m_signalIndices[name] = index;

    qDebug().noquote() << "[LcncHalMonitor] 添加信号:" << name;
    return true;
}

void LcncHalMonitor::removeSignal(const QString &name)
{
    auto it = m_signalIndices.find(name);
    if (it != m_signalIndices.end()) {
        int index = it.value();
        m_signals.remove(index);
        m_signalIndices.erase(it);

        // 重建索引
        for (int i = 0; i < m_signals.size(); ++i) {
            m_signalIndices[m_signals[i].name] = i;
        }

        qDebug().noquote() << "[LcncHalMonitor] 移除信号:" << name;
    }
}

void LcncHalMonitor::clearSignals()
{
    m_signals.clear();
    m_signalIndices.clear();
    qDebug().noquote() << "[LcncHalMonitor] 已清空所有信号";
}

QVariant LcncHalMonitor::signalValue(const QString &name) const
{
    auto it = m_signalIndices.find(name);
    if (it != m_signalIndices.end()) {
        return m_signals[it.value()].value;
    }
    return QVariant();
}

// ============================================================================
// 刷新控制
// ============================================================================

void LcncHalMonitor::start(int intervalMs)
{
    if (m_refreshTimer->isActive()) {
        return;
    }

    m_refreshTimer->start(intervalMs);
    qInfo().noquote() << "[LcncHalMonitor] 开始监视，间隔" << intervalMs << "ms";
}

void LcncHalMonitor::stop()
{
    if (m_refreshTimer->isActive()) {
        m_refreshTimer->stop();
        qInfo().noquote() << "[LcncHalMonitor] 停止监视";
    }
}

void LcncHalMonitor::refresh()
{
#ifdef Q_OS_LINUX
    readLinuxSignals();
#else
    readSimulation();
#endif
}