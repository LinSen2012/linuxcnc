/*
 * linuxQtCncGui - LinuxCNC Qt6 GUI 控制器
 * JogPanel 实现
 */

#include "JogPanel.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>

// ============================================================================
// 构造 / 析构
// ============================================================================

JogPanel::JogPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

JogPanel::~JogPanel() = default;

// ============================================================================
// 公共方法
// ============================================================================

void JogPanel::setAxisCount(int count)
{
    m_axisCount = std::clamp(count, 1, MAX_AXES);

    // 显示/隐藏轴点动组
    for (int i = 0; i < MAX_AXES; ++i) {
        if (m_jogNegButtons[i]) {
            m_jogNegButtons[i]->setVisible(i < m_axisCount);
        }
        if (m_jogPosButtons[i]) {
            m_jogPosButtons[i]->setVisible(i < m_axisCount);
        }
        if (m_homeButtons[i]) {
            m_homeButtons[i]->setVisible(i < m_axisCount);
        }
    }
}

void JogPanel::setJogSpeedRange(double minSpeed, double maxSpeed, double defaultSpeed)
{
    m_jogSpeedSpin->setRange(minSpeed, maxSpeed);
    m_jogSpeedSpin->setValue(defaultSpeed);
    m_jogSpeed = defaultSpeed;
}

// ============================================================================
// UI 初始化
// ============================================================================

void JogPanel::setupUi()
{
    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(4, 2, 4, 2);
    mainLayout->setSpacing(8);

    // ---- 左侧：轴点动按钮组 ----
    auto *jogGroup = new QGroupBox(QStringLiteral("点动控制"), this);
    auto *jogLayout = new QHBoxLayout(jogGroup);
    jogLayout->setSpacing(6);

    static const char *axisNames[] = {"X", "Y", "Z", "A", "B", "C", "U", "V", "W"};
    for (int i = 0; i < MAX_AXES; ++i) {
        auto *axisWidget = createAxisJogGroup(
            QStringLiteral(axisNames[i]), i);
        jogLayout->addWidget(axisWidget);
    }

    mainLayout->addWidget(jogGroup);

    // ---- 中间：速度和增量设置 ----
    auto *settingsGroup = new QGroupBox(QStringLiteral("设置"), this);
    auto *settingsLayout = new QVBoxLayout(settingsGroup);
    settingsLayout->setSpacing(4);

    // 点动速度
    auto *speedLayout = new QHBoxLayout();
    auto *speedLabel = new QLabel(QStringLiteral("速度:"), this);
    speedLayout->addWidget(speedLabel);

    m_jogSpeedSpin = new QDoubleSpinBox(this);
    m_jogSpeedSpin->setRange(1.0, 5000.0);
    m_jogSpeedSpin->setValue(50.0);
    m_jogSpeedSpin->setSuffix(QStringLiteral(" mm/min"));
    m_jogSpeedSpin->setDecimals(0);
    speedLayout->addWidget(m_jogSpeedSpin);
    settingsLayout->addLayout(speedLayout);

    // 增量距离
    auto *incLayout = new QHBoxLayout();
    auto *incLabel = new QLabel(QStringLiteral("增量:"), this);
    incLayout->addWidget(incLabel);

    m_incrementCombo = new QComboBox(this);
    m_incrementCombo->addItem(QStringLiteral("连续"), 0.0);
    m_incrementCombo->addItem(QStringLiteral("1.0 mm"), 1.0);
    m_incrementCombo->addItem(QStringLiteral("0.1 mm"), 0.1);
    m_incrementCombo->addItem(QStringLiteral("0.01 mm"), 0.01);
    m_incrementCombo->addItem(QStringLiteral("0.001 mm"), 0.001);
    m_incrementCombo->setCurrentIndex(0);
    incLayout->addWidget(m_incrementCombo);
    settingsLayout->addLayout(incLayout);

    // 全轴回参考点按钮
    auto *homeAllBtn = new QPushButton(QStringLiteral("全轴回零"), this);
    homeAllBtn->setStyleSheet(QStringLiteral(
        "background-color: #006633; border: 1px solid #00cc66; "
        "border-radius: 4px; padding: 6px; font-weight: bold;"));
    settingsLayout->addWidget(homeAllBtn);
    connect(homeAllBtn, &QPushButton::clicked, this, [this]() {
        emit homeRequested(-1);
    });

    settingsLayout->addStretch();
    mainLayout->addWidget(settingsGroup);

    // ---- 右侧：主轴和冷却液控制 ----
    auto *auxGroup = new QGroupBox(QStringLiteral("辅助控制"), this);
    auto *auxLayout = new QVBoxLayout(auxGroup);
    auxLayout->setSpacing(4);

    // 主轴控制
    auto *spindleLabel = new QLabel(QStringLiteral("主轴:"), this);
    spindleLabel->setStyleSheet(QStringLiteral("font-weight: bold; color: #00d2ff;"));
    auxLayout->addWidget(spindleLabel);

    auto *spindleBtnLayout = new QHBoxLayout();
    m_spindleFwdBtn = new QPushButton(QStringLiteral("正转"), this);
    m_spindleRevBtn = new QPushButton(QStringLiteral("反转"), this);
    m_spindleStopBtn = new QPushButton(QStringLiteral("停止"), this);
    spindleBtnLayout->addWidget(m_spindleFwdBtn);
    spindleBtnLayout->addWidget(m_spindleRevBtn);
    spindleBtnLayout->addWidget(m_spindleStopBtn);
    auxLayout->addLayout(spindleBtnLayout);

    connect(m_spindleFwdBtn, &QPushButton::clicked, this, [this]() {
        emit spindleControl(true, false);
    });
    connect(m_spindleRevBtn, &QPushButton::clicked, this, [this]() {
        emit spindleControl(false, false);
    });
    connect(m_spindleStopBtn, &QPushButton::clicked, this, [this]() {
        emit spindleControl(false, true);
    });

    auxLayout->addSpacing(8);

    // 冷却液控制
    auto *coolantLabel = new QLabel(QStringLiteral("冷却液:"), this);
    coolantLabel->setStyleSheet(QStringLiteral("font-weight: bold; color: #00d2ff;"));
    auxLayout->addWidget(coolantLabel);

    auto *coolantBtnLayout = new QHBoxLayout();
    m_coolantFloodBtn = new QPushButton(QStringLiteral("喷流"), this);
    m_coolantMistBtn = new QPushButton(QStringLiteral("雾化"), this);
    m_coolantFloodBtn->setCheckable(true);
    m_coolantMistBtn->setCheckable(true);
    coolantBtnLayout->addWidget(m_coolantFloodBtn);
    coolantBtnLayout->addWidget(m_coolantMistBtn);
    auxLayout->addLayout(coolantBtnLayout);

    auxLayout->addStretch();
    mainLayout->addWidget(auxGroup);

    // 速度改变信号
    connect(m_jogSpeedSpin, &QDoubleSpinBox::valueChanged,
            this, &JogPanel::onJogSpeedChanged);

    // 初始显示
    setAxisCount(3);
}

QWidget *JogPanel::createAxisJogGroup(const QString &axisName, int axisIndex)
{
    auto *widget = new QWidget(this);
    auto *layout = new QVBoxLayout(widget);
    layout->setSpacing(2);
    layout->setContentsMargins(2, 2, 2, 2);

    // 轴名称标签
    auto *label = new QLabel(axisName, this);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet(QStringLiteral(
        "font-weight: bold; font-size: 14px; color: #00d2ff;"));
    layout->addWidget(label);

    // 正方向按钮
    auto *posBtn = new QPushButton(QStringLiteral("+"), this);
    posBtn->setFixedSize(40, 40);
    posBtn->setStyleSheet(QStringLiteral(
        "font-size: 18px; font-weight: bold;"));
    posBtn->setProperty("axis", axisIndex);
    posBtn->setProperty("direction", 1);
    layout->addWidget(posBtn);
    m_jogPosButtons[axisIndex] = posBtn;

    // 负方向按钮
    auto *negBtn = new QPushButton(QStringLiteral("-"), this);
    negBtn->setFixedSize(40, 40);
    negBtn->setStyleSheet(QStringLiteral(
        "font-size: 18px; font-weight: bold;"));
    negBtn->setProperty("axis", axisIndex);
    negBtn->setProperty("direction", -1);
    layout->addWidget(negBtn);
    m_jogNegButtons[axisIndex] = negBtn;

    // 回参考点按钮
    auto *homeBtn = new QPushButton(QStringLiteral("H"), this);
    homeBtn->setFixedSize(40, 24);
    homeBtn->setToolTip(QStringLiteral("回参考点"));
    homeBtn->setProperty("axis", axisIndex);
    layout->addWidget(homeBtn);
    m_homeButtons[axisIndex] = homeBtn;

    // 连接信号
    connect(posBtn, &QPushButton::pressed, this, &JogPanel::onJogButtonPressed);
    connect(posBtn, &QPushButton::released, this, &JogPanel::onJogButtonReleased);
    connect(negBtn, &QPushButton::pressed, this, &JogPanel::onJogButtonPressed);
    connect(negBtn, &QPushButton::released, this, &JogPanel::onJogButtonReleased);
    connect(homeBtn, &QPushButton::clicked, this, [this, axisIndex]() {
        emit homeRequested(axisIndex);
    });

    return widget;
}

// ============================================================================
// 槽函数
// ============================================================================

void JogPanel::onJogButtonPressed()
{
    auto *btn = qobject_cast<QPushButton *>(sender());
    if (!btn) return;

    int axis = btn->property("axis").toInt();
    int direction = btn->property("direction").toInt();
    double velocity = m_jogSpeed * direction;

    emit jogRequested(axis, velocity);
}

void JogPanel::onJogButtonReleased()
{
    auto *btn = qobject_cast<QPushButton *>(sender());
    if (!btn) return;

    int axis = btn->property("axis").toInt();
    emit jogStopRequested(axis);
}

void JogPanel::onIncrementJogClicked()
{
    // 增量点动通过按钮点击实现
    auto *btn = qobject_cast<QPushButton *>(sender());
    if (!btn) return;

    int axis = btn->property("axis").toInt();
    int direction = btn->property("direction").toInt();
    double distance = m_incrementCombo->currentData().toDouble();

    if (distance > 0.0) {
        // 增量点动：发送一个短时的连续点动
        emit jogRequested(axis, m_jogSpeed * direction);
        // 短时间后自动停止（由 MainWindow 或 LcncCore 处理）
        QTimer::singleShot(100, this, [this, axis]() {
            emit jogStopRequested(axis);
        });
    }
}

void JogPanel::onJogSpeedChanged(double value)
{
    m_jogSpeed = value;
}
