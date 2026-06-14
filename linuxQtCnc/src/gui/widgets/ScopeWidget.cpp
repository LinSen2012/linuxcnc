/*
 * linuxQtCncGui - LinuxCNC Qt5 GUI 控制器
 * ScopeWidget - 示波器组件实现
 */

#include "ScopeWidget.h"

#include <QPainter>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDebug>
#include <QDateTime>
#include <cstdlib>
#include <cmath>

// ============================================================================
// ScopeWidget 实现
// ============================================================================

ScopeWidget::ScopeWidget(QWidget *parent)
    : QWidget(parent)
    , m_refreshTimer(new QTimer(this))
{
    setupUi();

    // 默认添加 3 个通道
    addChannel(tr("通道1"), Qt::red);
    addChannel(tr("通道2"), Qt::green);
    addChannel(tr("通道3"), Qt::blue);

    connect(m_refreshTimer, &QTimer::timeout, this, &ScopeWidget::onRefreshTimer);
}

ScopeWidget::~ScopeWidget() = default;

void ScopeWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    // ---- 控制栏 ----
    auto *controls = new QHBoxLayout();

    controls->addWidget(new QLabel(tr("时基:"), this));
    m_timeBaseSpin = new QDoubleSpinBox(this);
    m_timeBaseSpin->setRange(0.001, 10.0);
    m_timeBaseSpin->setValue(0.1);
    m_timeBaseSpin->setSuffix(" s/div");
    connect(m_timeBaseSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ScopeWidget::onTimeBaseChanged);
    controls->addWidget(m_timeBaseSpin);

    controls->addSpacing(10);

    controls->addWidget(new QLabel(tr("触发通道:"), this));
    m_triggerChannelCombo = new QComboBox(this);
    m_triggerChannelCombo->addItem(tr("无"), -1);
    connect(m_triggerChannelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { m_triggerChannel = m_triggerChannelCombo->currentData().toInt(); });
    controls->addWidget(m_triggerChannelCombo);

    controls->addWidget(new QLabel(tr("触发电平:"), this));
    m_triggerLevelSpin = new QDoubleSpinBox(this);
    m_triggerLevelSpin->setRange(-100.0, 100.0);
    m_triggerLevelSpin->setValue(0.0);
    m_triggerLevelSpin->setSuffix(" V");
    connect(m_triggerLevelSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ScopeWidget::onTriggerLevelChanged);
    controls->addWidget(m_triggerLevelSpin);

    controls->addStretch();

    m_runStopBtn = new QPushButton(tr("运行"), this);
    m_runStopBtn->setCheckable(true);
    connect(m_runStopBtn, &QPushButton::clicked, this, &ScopeWidget::onRunStopClicked);
    controls->addWidget(m_runStopBtn);

    m_clearBtn = new QPushButton(tr("清零"), this);
    connect(m_clearBtn, &QPushButton::clicked, this, &ScopeWidget::onClearClicked);
    controls->addWidget(m_clearBtn);

    mainLayout->addLayout(controls);

    // ---- 状态栏 ----
    auto *statusLayout = new QHBoxLayout();
    m_statusLabel = new QLabel(tr("停止"), this);
    statusLayout->addWidget(m_statusLabel);
    statusLayout->addStretch();
    mainLayout->addLayout(statusLayout);

    // ---- 占位符（实际使用 QCustomPlot 替代）----
    auto *placeholder = new QLabel(
        tr("示波器显示区域\n(需要集成 QCustomPlot)"), this);
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setStyleSheet(R"(
        QLabel {
            background-color: #1a1a2e;
            color: #00ff00;
            border: 1px solid #333;
            font-size: 14px;
        }
    )");
    mainLayout->addWidget(placeholder, 1);
}

int ScopeWidget::addChannel(const QString &name, const QColor &color)
{
    ScopeChannel ch;
    ch.name = name;
    ch.color = color.isValid() ? color : QColor(Qt::white);
    ch.data.reserve(MAX_SAMPLES);
    m_channels.append(ch);

    m_triggerChannelCombo->addItem(name, m_channels.size() - 1);
    return m_channels.size() - 1;
}

void ScopeWidget::addSample(int channel, double value)
{
    if (channel < 0 || channel >= m_channels.size()) return;

    auto &ch = m_channels[channel];
    ch.data.append(value);

    // 限制数据量
    while (ch.data.size() > MAX_SAMPLES) {
        ch.data.removeFirst();
    }
}

void ScopeWidget::clear()
{
    for (auto &ch : m_channels) {
        ch.data.clear();
    }
    updatePlot();
}

void ScopeWidget::setChannelVisible(int channel, bool visible)
{
    if (channel < 0 || channel >= m_channels.size()) return;
    m_channels[channel].visible = visible;
}

void ScopeWidget::setTimeBase(double secs)
{
    m_timeBase = secs;
}

void ScopeWidget::setTriggerLevel(double level)
{
    m_triggerLevel = level;
}

void ScopeWidget::setTriggerChannel(int channel)
{
    m_triggerChannel = channel;
}

void ScopeWidget::start()
{
    m_running = true;
    m_refreshTimer->start(m_refreshRate);
    m_statusLabel->setText(tr("运行中"));
    m_runStopBtn->setText(tr("停止"));
}

void ScopeWidget::stop()
{
    m_running = false;
    m_refreshTimer->stop();
    m_statusLabel->setText(tr("停止"));
    m_runStopBtn->setText(tr("运行"));
}

void ScopeWidget::updatePlot()
{
    // TODO: 使用 QCustomPlot 绘制波形
    update();
}

void ScopeWidget::onTimeBaseChanged(double value)
{
    setTimeBase(value);
}

void ScopeWidget::onTriggerLevelChanged(double value)
{
    setTriggerLevel(value);
}

void ScopeWidget::onRunStopClicked()
{
    if (m_running) {
        stop();
    } else {
        start();
    }
}

void ScopeWidget::onClearClicked()
{
    clear();
}

void ScopeWidget::onRefreshTimer()
{
    // 模拟采样数据
    for (int i = 0; i < m_channels.size(); ++i) {
        double value = sin(QDateTime::currentMSecsSinceEpoch() / 1000.0 * (i + 1)) * (i + 1) * 10;
        addSample(i, value);
    }
    updatePlot();
}

// ============================================================================
// SimpleScopeWidget 实现（简化版，无外部依赖）
// ============================================================================

SimpleScopeWidget::SimpleScopeWidget(QWidget *parent)
    : QWidget(parent)
    , m_timer(new QTimer(this))
{
    setBackgroundRole(QPalette::Dark);
    setAutoFillBackground(true);

    connect(m_timer, &QTimer::timeout, this, QOverload<>::of(&QWidget::update));
}

SimpleScopeWidget::~SimpleScopeWidget() = default;

void SimpleScopeWidget::addChannel(const QString &name, const QColor &color)
{
    ScopeChannel ch;
    ch.name = name;
    ch.color = color.isValid() ? color : QColor(Qt::white);
    ch.data.reserve(500);
    m_channels.append(ch);
}

void SimpleScopeWidget::addSample(int channel, double value)
{
    if (channel < 0 || channel >= m_channels.size()) return;
    auto &ch = m_channels[channel];
    ch.data.append(value);
    while (ch.data.size() > 500) {
        ch.data.removeFirst();
    }
}

void SimpleScopeWidget::clear()
{
    for (auto &ch : m_channels) {
        ch.data.clear();
    }
    update();
}

void SimpleScopeWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRect rect = this->rect();
    int w = rect.width();
    int h = rect.height();

    // 绘制网格
    p.setPen(QColor(50, 50, 50));
    for (int x = 0; x < w; x += 50) {
        p.drawLine(x, 0, x, h);
    }
    for (int y = 0; y < h; y += 50) {
        p.drawLine(0, y, w, y);
    }

    // 绘制通道波形
    int chIndex = 0;
    for (const auto &ch : m_channels) {
        if (!ch.visible || ch.data.isEmpty()) {
            ++chIndex;
            continue;
        }

        p.setPen(ch.color);
        QVector<QPointF> points;
        double dx = static_cast<double>(w) / ch.data.size();
        double yScale = h / 20.0;  // 缩放因子

        for (int i = 0; i < ch.data.size(); ++i) {
            double x = i * dx;
            double y = h / 2.0 - ch.data[i] * yScale / ch.scale + ch.offset;
            points.append(QPointF(x, y));
        }

        if (points.size() > 1) {
            QPainterPath path;
            path.moveTo(points[0]);
            for (int i = 1; i < points.size(); ++i) {
                path.lineTo(points[i]);
            }
            p.drawPath(path);
        }
        ++chIndex;
    }

    // 绘制中心线
    p.setPen(QColor(80, 80, 80));
    p.drawLine(0, h / 2, w, h / 2);
}