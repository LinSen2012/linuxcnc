/*
 * linuxQtCncGui - LinuxCNC Qt6 GUI 控制器
 * StatusBar 实现
 */

#include "StatusBar.h"

#include <QHBoxLayout>
#include <QGridLayout>
#include <QFont>

// ============================================================================
// 构造 / 析构
// ============================================================================

StatusBarWidget::StatusBarWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

StatusBarWidget::~StatusBarWidget() = default;

// ============================================================================
// 公共方法
// ============================================================================

void StatusBarWidget::updateStatus(const LcncStatusData &data)
{
    // 机床状态
    m_machineStateLabel->setText(machineStateString(data.taskState));

    // 根据状态设置颜色
    switch (data.taskState) {
    case MachineState::ESTOP:
        m_machineStateLabel->setStyleSheet(
            QStringLiteral("color: #ff0000; font-weight: bold; font-size: 13px;"));
        break;
    case MachineState::ESTOP_RESET:
        m_machineStateLabel->setStyleSheet(
            QStringLiteral("color: #ffaa00; font-weight: bold; font-size: 13px;"));
        break;
    case MachineState::ON:
        m_machineStateLabel->setStyleSheet(
            QStringLiteral("color: #00ff88; font-weight: bold; font-size: 13px;"));
        break;
    case MachineState::OFF:
        m_machineStateLabel->setStyleSheet(
            QStringLiteral("color: #888888; font-size: 13px;"));
        break;
    }

    // 运动模式
    m_motionModeLabel->setText(motionModeString(data.motionMode));

    // 解释器状态
    m_interpStateLabel->setText(interpStateString(data.interpState));
    switch (data.interpState) {
    case InterpState::RUNNING:
        m_interpStateLabel->setStyleSheet(
            QStringLiteral("color: #00ff88; font-size: 12px;"));
        break;
    case InterpState::PAUSED:
        m_interpStateLabel->setStyleSheet(
            QStringLiteral("color: #ffaa00; font-size: 12px;"));
        break;
    case InterpState::IDLE:
        m_interpStateLabel->setStyleSheet(
            QStringLiteral("color: #888888; font-size: 12px;"));
        break;
    }

    // 进给速率
    m_feedrateLabel->setText(QStringLiteral("F: %1").arg(data.feedrate, 0, 'f', 0));
    m_feedOverrideLabel->setText(QStringLiteral("FOV: %1%")
                                  .arg(data.feedOverride * 100.0, 0, 'f', 0));

    // 主轴转速
    m_spindleSpeedLabel->setText(QStringLiteral("S: %1")
                                   .arg(std::abs(data.spindleSpeed), 0, 'f', 0));
    m_spindleOverrideLabel->setText(QStringLiteral("SOV: %1%")
                                     .arg(data.spindleOverride * 100.0, 0, 'f', 0));

    // G/M 代码
    m_gcodesLabel->setText(QStringLiteral("G: %1")
                             .arg(formatGCodes(data.activeGCodesData.codes)));
    m_mcodesLabel->setText(QStringLiteral("M: %1")
                             .arg(formatMCodes(data.activeMCodesData.codes)));

    // 刀具信息
    m_toolLabel->setText(QStringLiteral("T%1 D%2")
                          .arg(data.currentTool.toolNo)
                          .arg(data.currentTool.diameter, 0, 'f', 1));

    // 文件信息
    if (!data.m_currentFile.isEmpty()) {
        m_fileLabel->setText(data.m_currentFile);
        m_lineLabel->setText(QStringLiteral("行: %1/%2")
                              .arg(data.m_currentLine)
                              .arg(data.programLine));

        // 进度条
        if (data.programLine > 0) {
            m_progressBar->setMaximum(data.programLine);
            m_progressBar->setValue(data.m_currentLine);
            m_progressBar->setVisible(true);
        }
    } else {
        m_fileLabel->setText(QStringLiteral("--"));
        m_lineLabel->setText(QStringLiteral("--"));
        m_progressBar->setVisible(false);
    }
}

// ============================================================================
// 私有方法
// ============================================================================

void StatusBarWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 2, 4, 2);
    mainLayout->setSpacing(1);

    // 第一行：机床状态 + 运动模式 + 解释器状态
    auto *row1Layout = new QHBoxLayout();
    row1Layout->setSpacing(12);

    auto *stateTitle = new QLabel(QStringLiteral("状态:"), this);
    stateTitle->setStyleSheet(QStringLiteral("color: #00d2ff; font-weight: bold;"));
    row1Layout->addWidget(stateTitle);

    m_machineStateLabel = new QLabel(QStringLiteral("--"), this);
    m_machineStateLabel->setMinimumWidth(80);
    row1Layout->addWidget(m_machineStateLabel);

    auto *modeTitle = new QLabel(QStringLiteral("模式:"), this);
    modeTitle->setStyleSheet(QStringLiteral("color: #00d2ff; font-weight: bold;"));
    row1Layout->addWidget(modeTitle);

    m_motionModeLabel = new QLabel(QStringLiteral("--"), this);
    m_motionModeLabel->setMinimumWidth(60);
    row1Layout->addWidget(m_motionModeLabel);

    auto *interpTitle = new QLabel(QStringLiteral("解释器:"), this);
    interpTitle->setStyleSheet(QStringLiteral("color: #00d2ff; font-weight: bold;"));
    row1Layout->addWidget(interpTitle);

    m_interpStateLabel = new QLabel(QStringLiteral("--"), this);
    m_interpStateLabel->setMinimumWidth(60);
    row1Layout->addWidget(m_interpStateLabel);

    row1Layout->addStretch();

    // 刀具信息
    auto *toolTitle = new QLabel(QStringLiteral("刀具:"), this);
    toolTitle->setStyleSheet(QStringLiteral("color: #00d2ff; font-weight: bold;"));
    row1Layout->addWidget(toolTitle);

    m_toolLabel = new QLabel(QStringLiteral("--"), this);
    m_toolLabel->setMinimumWidth(80);
    row1Layout->addWidget(m_toolLabel);

    mainLayout->addLayout(row1Layout);

    // 第二行：进给 + 主轴 + G/M 代码 + 文件
    auto *row2Layout = new QHBoxLayout();
    row2Layout->setSpacing(12);

    m_feedrateLabel = new QLabel(QStringLiteral("F: --"), this);
    m_feedrateLabel->setMinimumWidth(60);
    row2Layout->addWidget(m_feedrateLabel);

    m_feedOverrideLabel = new QLabel(QStringLiteral("FOV: --"), this);
    m_feedOverrideLabel->setMinimumWidth(60);
    row2Layout->addWidget(m_feedOverrideLabel);

    m_spindleSpeedLabel = new QLabel(QStringLiteral("S: --"), this);
    m_spindleSpeedLabel->setMinimumWidth(60);
    row2Layout->addWidget(m_spindleSpeedLabel);

    m_spindleOverrideLabel = new QLabel(QStringLiteral("SOV: --"), this);
    m_spindleOverrideLabel->setMinimumWidth(60);
    row2Layout->addWidget(m_spindleOverrideLabel);

    row2Layout->addSpacing(8);

    m_gcodesLabel = new QLabel(QStringLiteral("G: --"), this);
    m_gcodesLabel->setMinimumWidth(120);
    row2Layout->addWidget(m_gcodesLabel);

    m_mcodesLabel = new QLabel(QStringLiteral("M: --"), this);
    m_mcodesLabel->setMinimumWidth(60);
    row2Layout->addWidget(m_mcodesLabel);

    row2Layout->addStretch();

    // 文件和行号
    m_fileLabel = new QLabel(QStringLiteral("--"), this);
    m_fileLabel->setMinimumWidth(100);
    row2Layout->addWidget(m_fileLabel);

    m_lineLabel = new QLabel(QStringLiteral("--"), this);
    m_lineLabel->setMinimumWidth(60);
    row2Layout->addWidget(m_lineLabel);

    // 进度条
    m_progressBar = new QProgressBar(this);
    m_progressBar->setMaximumWidth(150);
    m_progressBar->setMaximumHeight(14);
    m_progressBar->setTextVisible(false);
    m_progressBar->setVisible(false);
    row2Layout->addWidget(m_progressBar);

    mainLayout->addLayout(row2Layout);
}

QString StatusBarWidget::formatGCodes(const QVector<int> &codes)
{
    QStringList list;
    for (int code : codes) {
        list.append(QString::number(code));
    }
    return list.join(QStringLiteral(" "));
}

QString StatusBarWidget::formatMCodes(const QVector<int> &codes)
{
    if (codes.isEmpty()) return QStringLiteral("--");
    QStringList list;
    for (int code : codes) {
        list.append(QString::number(code));
    }
    return list.join(QStringLiteral(" "));
}

QString StatusBarWidget::machineStateString(MachineState state)
{
    switch (state) {
    case MachineState::ESTOP:       return QStringLiteral("急停");
    case MachineState::ESTOP_RESET: return QStringLiteral("急停复位");
    case MachineState::ON:          return QStringLiteral("运行");
    case MachineState::OFF:         return QStringLiteral("关闭");
    }
    return QStringLiteral("--");
}

QString StatusBarWidget::motionModeString(MotionMode mode)
{
    switch (mode) {
    case MotionMode::MANUAL: return QStringLiteral("手动");
    case MotionMode::AUTO:   return QStringLiteral("自动");
    case MotionMode::MDI:    return QStringLiteral("MDI");
    }
    return QStringLiteral("--");
}

QString StatusBarWidget::interpStateString(InterpState state)
{
    switch (state) {
    case InterpState::IDLE:    return QStringLiteral("空闲");
    case InterpState::RUNNING: return QStringLiteral("运行中");
    case InterpState::PAUSED:  return QStringLiteral("已暂停");
    }
    return QStringLiteral("--");
}
