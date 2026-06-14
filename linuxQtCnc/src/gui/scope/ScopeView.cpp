#include "ScopeView.h"
#include "LcncCore.h"

#include <cmath>
#include <algorithm>

// ============================================================
// 构造 / 析构
// ============================================================

ScopeView::ScopeView(QWidget *parent)
    : QWidget(parent)
    , m_core(nullptr)
    , m_capturing(false)
    , m_singleShotDone(false)
    , m_triggerMode(TriggerAuto)
    , m_triggerChannel(0)
    , m_triggerEdge(EdgeRising)
    , m_triggerLevel(0.0)
    , m_timebase(1000.0)           // 默认 1000us/div = 1ms/div
    , m_sampleRate(10000.0)        // 默认 10kHz
    , m_samplesPerGrid(10)
    , m_divisionsH(10)            // 水平 10 格
    , m_divisionsV(8)             // 垂直 8 格
    , m_voltsPerDiv(1.0)
    , m_showCrosshair(false)
    , m_marginLeft(60)
    , m_marginRight(20)
    , m_marginTop(30)
    , m_marginBottom(30)
    , m_sampleTimer(nullptr)
{
    setupUi();
    setupConnections();

    // 创建采样定时器
    m_sampleTimer = new QTimer(this);
    m_sampleTimer->setInterval(SAMPLE_INTERVAL_MS);
}

ScopeView::~ScopeView()
{
    if (m_sampleTimer) {
        m_sampleTimer->stop();
    }
    qDeleteAll(m_channels);
}

// ============================================================
// 公共接口
// ============================================================

void ScopeView::setCore(LcncCore *core)
{
    m_core = core;
}

void ScopeView::addChannel(ScopeChannel *channel)
{
    if (!channel || m_channels.size() >= MAX_CHANNELS) {
        return;
    }

    channel->setIndex(m_channels.size());

    // 如果未设置颜色，使用默认颜色
    auto colors = defaultChannelColors();
    if (channel->color() == QColor(0, 255, 0) && m_channels.size() > 0) {
        channel->setColor(colors.at(m_channels.size() % colors.size()));
    }

    m_channels.append(channel);

    // 更新触发通道下拉框
    m_triggerChannelCombo->addItem(channel->name(), channel->index());

    update();
}

void ScopeView::removeChannel(int index)
{
    if (index < 0 || index >= m_channels.size()) {
        return;
    }

    delete m_channels.takeAt(index);

    // 重新编号
    for (int i = 0; i < m_channels.size(); ++i) {
        m_channels[i]->setIndex(i);
    }

    // 更新触发通道下拉框
    m_triggerChannelCombo->clear();
    for (const auto *ch : m_channels) {
        m_triggerChannelCombo->addItem(ch->name(), ch->index());
    }

    update();
}

ScopeChannel* ScopeView::channel(int index) const
{
    if (index >= 0 && index < m_channels.size()) {
        return m_channels.at(index);
    }
    return nullptr;
}

void ScopeView::startCapture()
{
    m_capturing = true;
    m_singleShotDone = false;
    m_startBtn->setEnabled(false);
    m_stopBtn->setEnabled(true);
    m_singleBtn->setEnabled(false);
    m_sampleTimer->start();
    m_statusLabel->setText("采集中...");
}

void ScopeView::stopCapture()
{
    m_capturing = false;
    m_sampleTimer->stop();
    m_startBtn->setEnabled(true);
    m_stopBtn->setEnabled(false);
    m_singleBtn->setEnabled(true);
    m_statusLabel->setText("已停止");
    update();
}

void ScopeView::singleCapture()
{
    // 清除旧数据
    clearAllChannels();
    m_capturing = true;
    m_singleShotDone = false;
    m_startBtn->setEnabled(false);
    m_stopBtn->setEnabled(true);
    m_singleBtn->setEnabled(false);
    m_sampleTimer->start();
    m_statusLabel->setText("单次采集中...");
}

void ScopeView::setTriggerMode(int mode)
{
    m_triggerMode = static_cast<TriggerMode>(mode);
    m_triggerModeCombo->setCurrentIndex(mode);
}

void ScopeView::setTimebase(double usPerDiv)
{
    m_timebase = usPerDiv;
    m_timebaseSpin->setValue(usPerDiv);

    // 更新每格采样点数
    // samplesPerGrid = timebase(us) * sampleRate(Hz) / 1e6
    m_samplesPerGrid = qMax(1, static_cast<int>(
        m_timebase * m_sampleRate / 1e6));

    update();
}

void ScopeView::clearAllChannels()
{
    for (auto *ch : m_channels) {
        ch->clear();
    }
    update();
}

// ============================================================
// 私有槽函数
// ============================================================

void ScopeView::onSampleTimer()
{
    if (!m_capturing) return;

    // 从 LcncCore 读取各通道数据
    for (auto *ch : m_channels) {
        if (!ch->enabled()) continue;

        double value = 0.0;
        // TODO: 从 LcncCore 获取 HAL pin 值
        // if (m_core) {
        //     m_core->getHalPinValue(ch->pinName(), &value);
        // }
        ch->addSample(value);
    }

    // 检查触发条件（单次模式）
    if (m_triggerMode == TriggerSingle && !m_singleShotDone) {
        if (checkTrigger()) {
            m_singleShotDone = true;
            stopCapture();
            m_statusLabel->setText("单次采集完成");
        }
    }

    // 检查缓冲区是否已满（单次模式）
    if (m_triggerMode == TriggerSingle && !m_singleShotDone) {
        bool allFull = true;
        for (auto *ch : m_channels) {
            if (ch->enabled() && ch->sampleCount() < ch->bufferSize()) {
                allFull = false;
                break;
            }
        }
        if (allFull) {
            m_singleShotDone = true;
            stopCapture();
            m_statusLabel->setText("单次采集完成（缓冲区满）");
        }
    }

    update();
}

void ScopeView::onTriggerModeChanged(int index)
{
    m_triggerMode = static_cast<TriggerMode>(index);
}

void ScopeView::onTriggerChannelChanged(int index)
{
    m_triggerChannel = index;
}

void ScopeView::onTriggerEdgeChanged(int index)
{
    m_triggerEdge = static_cast<TriggerEdge>(index);
}

void ScopeView::onTriggerLevelChanged(double value)
{
    m_triggerLevel = value;
    update();
}

// ============================================================
// 触发检测
// ============================================================

bool ScopeView::checkTrigger()
{
    if (m_triggerChannel < 0 || m_triggerChannel >= m_channels.size()) {
        return false;
    }

    ScopeChannel *ch = m_channels.at(m_triggerChannel);
    if (!ch || ch->sampleCount() < 2) {
        return false;
    }

    int n = ch->sampleCount();
    double prev = ch->valueAt(n - 2);
    double curr = ch->valueAt(n - 1);

    switch (m_triggerEdge) {
    case EdgeRising:
        return prev < m_triggerLevel && curr >= m_triggerLevel;
    case EdgeFalling:
        return prev > m_triggerLevel && curr <= m_triggerLevel;
    case EdgeLevel:
        return qAbs(curr - m_triggerLevel) < 0.001;
    default:
        return false;
    }
}

// ============================================================
// 绘制
// ============================================================

void ScopeView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 背景
    painter.fillRect(rect(), QColor(20, 20, 30));

    // 计算绘图区域
    QRectF plotArea(
        m_marginLeft,
        m_marginTop,
        width() - m_marginLeft - m_marginRight,
        height() - m_marginTop - m_marginBottom
    );

    if (plotArea.width() <= 0 || plotArea.height() <= 0) {
        return;
    }

    // 绘制网格
    drawGrid(painter, plotArea);

    // 绘制波形
    drawWaveforms(painter, plotArea);

    // 绘制通道标签
    drawChannelLabels(painter, plotArea);

    // 绘制触发指示器
    drawTriggerIndicator(painter, plotArea);

    // 绘制时间轴
    drawTimeAxis(painter, plotArea);

    // 绘制值轴
    drawValueAxis(painter, plotArea);

    // 绘制十字光标
    if (m_showCrosshair) {
        drawCrosshair(painter, plotArea);
    }
}

void ScopeView::drawGrid(QPainter &painter, const QRectF &plotArea)
{
    painter.save();

    // 绘图区域背景
    painter.fillRect(plotArea, QColor(10, 10, 20));

    // 主网格线
    QPen gridPen(QColor(60, 60, 80), 1.0);
    painter.setPen(gridPen);

    // 垂直网格线
    float gridW = plotArea.width() / m_divisionsH;
    for (int i = 0; i <= m_divisionsH; ++i) {
        float x = plotArea.left() + i * gridW;
        painter.drawLine(QPointF(x, plotArea.top()), QPointF(x, plotArea.bottom()));
    }

    // 水平网格线
    float gridH = plotArea.height() / m_divisionsV;
    for (int i = 0; i <= m_divisionsV; ++i) {
        float y = plotArea.top() + i * gridH;
        painter.drawLine(QPointF(plotArea.left(), y), QPointF(plotArea.right(), y));
    }

    // 细分网格线（每格 5 等分）
    QPen subGridPen(QColor(40, 40, 55), 0.5);
    painter.setPen(subGridPen);

    for (int i = 0; i < m_divisionsH; ++i) {
        float baseX = plotArea.left() + i * gridW;
        for (int j = 1; j < 5; ++j) {
            float x = baseX + j * gridW / 5.0f;
            painter.drawLine(QPointF(x, plotArea.top()), QPointF(x, plotArea.bottom()));
        }
    }

    for (int i = 0; i < m_divisionsV; ++i) {
        float baseY = plotArea.top() + i * gridH;
        for (int j = 1; j < 5; ++j) {
            float y = baseY + j * gridH / 5.0f;
            painter.drawLine(QPointF(plotArea.left(), y), QPointF(plotArea.right(), y));
        }
    }

    // 绘图区域边框
    QPen borderPen(QColor(100, 100, 120), 2.0);
    painter.setPen(borderPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(plotArea);

    painter.restore();
}

void ScopeView::drawWaveforms(QPainter &painter, const QRectF &plotArea)
{
    painter.save();

    // 裁剪到绘图区域
    painter.setClipRect(plotArea);

    for (auto *ch : m_channels) {
        if (!ch || !ch->enabled()) continue;

        int sampleCount = ch->sampleCount();
        if (sampleCount < 2) continue;

        // 计算显示参数
        double scale = ch->scale();
        double offset = ch->offset();

        // 每个采样点在 X 方向的像素位置
        float totalSamples = m_divisionsH * m_samplesPerGrid;
        float dx = plotArea.width() / totalSamples;

        // 绘制波形
        QPen waveformPen(ch->color(), 1.5);
        painter.setPen(waveformPen);

        QPainterPath path;
        bool pathStarted = false;

        // 从最新的数据开始显示
        int displayCount = qMin(sampleCount, static_cast<int>(totalSamples));

        for (int i = 0; i < displayCount; ++i) {
            double value = ch->valueAt(sampleCount - displayCount + i);

            // 将值映射到 Y 坐标
            // 中心线在绘图区域中间
            float normalizedValue = (value - offset) / scale;
            float y = plotArea.center().y() - normalizedValue * (plotArea.height() / m_divisionsV / 2.0f);

            float x = plotArea.left() + i * dx;

            if (!pathStarted) {
                path.moveTo(x, y);
                pathStarted = true;
            } else {
                path.lineTo(x, y);
            }
        }

        painter.drawPath(path);
    }

    painter.restore();
}

void ScopeView::drawChannelLabels(QPainter &painter, const QRectF &plotArea)
{
    painter.save();

    QFont labelFont("Monospace", 9);
    painter.setFont(labelFont);

    for (int i = 0; i < m_channels.size(); ++i) {
        auto *ch = m_channels.at(i);
        if (!ch || !ch->enabled()) continue;

        // 在绘图区域左上角显示通道信息
        float y = plotArea.top() + 5 + i * 18;
        if (y > plotArea.bottom() - 10) break;

        QString label = QString("CH%1: %2 (%3V/div)")
                            .arg(i + 1)
                            .arg(ch->name())
                            .arg(ch->scale(), 0, 'f', 2);

        // 背景矩形
        QRectF labelRect(plotArea.left() + 5, y - 12, 180, 16);
        painter.fillRect(labelRect, QColor(0, 0, 0, 180));

        // 文字
        painter.setPen(ch->color());
        painter.drawText(labelRect, Qt::AlignLeft | Qt::AlignVCenter, label);
    }

    painter.restore();
}

void ScopeView::drawTriggerIndicator(QPainter &painter, const QRectF &plotArea)
{
    painter.save();

    if (m_triggerMode == TriggerFree) {
        painter.restore();
        return;
    }

    // 绘制触发电平线
    if (m_triggerChannel >= 0 && m_triggerChannel < m_channels.size()) {
        auto *ch = m_channels.at(m_triggerChannel);
        if (ch) {
            double normalizedLevel = (m_triggerLevel - ch->offset()) / ch->scale();
            float y = plotArea.center().y() - normalizedLevel * (plotArea.height() / m_divisionsV / 2.0f);

            // 触发电平线（黄色三角 + 虚线）
            QPen triggerPen(QColor(255, 200, 0), 1.0, Qt::DashLine);
            painter.setPen(triggerPen);
            painter.drawLine(QPointF(plotArea.left(), y), QPointF(plotArea.right(), y));

            // 三角标记
            QPolygonF triangle;
            triangle << QPointF(plotArea.left() - 8, y - 5)
                     << QPointF(plotArea.left() - 8, y + 5)
                     << QPointF(plotArea.left(), y);
            painter.setBrush(QColor(255, 200, 0));
            painter.setPen(Qt::NoPen);
            painter.drawPolygon(triangle);
        }
    }

    painter.restore();
}

void ScopeView::drawTimeAxis(QPainter &painter, const QRectF &plotArea)
{
    painter.save();

    QFont axisFont("Monospace", 8);
    painter.setFont(axisFont);
    painter.setPen(QColor(180, 180, 180));

    float gridW = plotArea.width() / m_divisionsH;

    // 时间标签
    for (int i = 0; i <= m_divisionsH; ++i) {
        float x = plotArea.left() + i * gridW;
        double time = i * m_timebase;  // 微秒

        QString label;
        if (time >= 1e6) {
            label = QString("%1s").arg(time / 1e6, 0, 'f', 2);
        } else if (time >= 1e3) {
            label = QString("%1ms").arg(time / 1e3, 0, 'f', 2);
        } else {
            label = QString("%1us").arg(time, 0, 'f', 1);
        }

        QRectF labelRect(x - gridW / 2, plotArea.bottom() + 2, gridW, 20);
        painter.drawText(labelRect, Qt::AlignCenter, label);
    }

    // 时基标签
    QString timebaseLabel;
    if (m_timebase >= 1e6) {
        timebaseLabel = QString("时基: %1s/div").arg(m_timebase / 1e6, 0, 'f', 2);
    } else if (m_timebase >= 1e3) {
        timebaseLabel = QString("时基: %1ms/div").arg(m_timebase / 1e3, 0, 'f', 2);
    } else {
        timebaseLabel = QString("时基: %1us/div").arg(m_timebase, 0, 'f', 1);
    }
    painter.drawText(QRectF(plotArea.left(), plotArea.bottom() + 18,
                             plotArea.width(), 20),
                      Qt::AlignCenter, timebaseLabel);

    painter.restore();
}

void ScopeView::drawValueAxis(QPainter &painter, const QRectF &plotArea)
{
    painter.save();

    QFont axisFont("Monospace", 8);
    painter.setFont(axisFont);
    painter.setPen(QColor(180, 180, 180));

    float gridH = plotArea.height() / m_divisionsV;

    // 值标签（左侧）
    for (int i = 0; i <= m_divisionsV; ++i) {
        float y = plotArea.top() + i * gridH;
        double value = (m_divisionsV / 2.0 - i) * m_voltsPerDiv;

        QString label = QString("%1").arg(value, 0, 'f', 1);
        QRectF labelRect(0, y - gridH / 2, m_marginLeft - 5, gridH);
        painter.drawText(labelRect, Qt::AlignRight | Qt::AlignVCenter, label);
    }

    painter.restore();
}

void ScopeView::drawCrosshair(QPainter &painter, const QRectF &plotArea)
{
    if (!plotArea.contains(m_mousePos)) {
        m_showCrosshair = false;
        return;
    }

    painter.save();

    // 十字光标线
    QPen crosshairPen(QColor(200, 200, 200, 150), 1.0, Qt::DashLine);
    painter.setPen(crosshairPen);

    // 垂直线
    painter.drawLine(QPointF(m_mousePos.x(), plotArea.top()),
                     QPointF(m_mousePos.x(), plotArea.bottom()));

    // 水平线
    painter.drawLine(QPointF(plotArea.left(), m_mousePos.y()),
                     QPointF(plotArea.right(), m_mousePos.y()));

    // 值标签
    QFont valFont("Monospace", 9);
    painter.setFont(valFont);

    // 时间值
    float relX = (m_mousePos.x() - plotArea.left()) / plotArea.width();
    double time = relX * m_divisionsH * m_timebase;
    QString timeStr;
    if (time >= 1e6) {
        timeStr = QString("t=%1s").arg(time / 1e6, 0, 'f', 3);
    } else if (time >= 1e3) {
        timeStr = QString("t=%1ms").arg(time / 1e3, 0, 'f', 3);
    } else {
        timeStr = QString("t=%1us").arg(time, 0, 'f', 1);
    }

    // 各通道值
    float relY = (m_mousePos.y() - plotArea.top()) / plotArea.height();
    double val = (0.5 - relY) * m_divisionsV * m_voltsPerDiv;

    QString valStr = QString("v=%1").arg(val, 0, 'f', 3);

    // 绘制标签背景
    QRectF labelRect(m_mousePos.x() + 10, m_mousePos.y() - 30, 120, 40);
    painter.fillRect(labelRect, QColor(0, 0, 0, 200));

    painter.setPen(QColor(200, 200, 200));
    painter.drawText(QPointF(m_mousePos.x() + 12, m_mousePos.y() - 15), timeStr);
    painter.drawText(QPointF(m_mousePos.x() + 12, m_mousePos.y() + 5), valStr);

    painter.restore();
}

// ============================================================
// 鼠标事件
// ============================================================

void ScopeView::mouseMoveEvent(QMouseEvent *event)
{
    m_mousePos = event->pos();
    m_showCrosshair = true;
    update();
}

void ScopeView::wheelEvent(QWheelEvent *event)
{
    // 滚轮调整时基
    float delta = event->angleDelta().y() / 120.0f;
    double newTimebase = m_timebase * (1.0 - delta * 0.2);

    // 限制时基范围：1us ~ 100s
    newTimebase = std::clamp(newTimebase, 1.0, 1e8);
    setTimebase(newTimebase);
}

// ============================================================
// 界面初始化
// ============================================================

void ScopeView::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(2, 2, 2, 2);
    mainLayout->setSpacing(2);

    // ---- 工具栏 ----
    m_toolBar = new QToolBar("示波器控制");
    m_toolBar->setMovable(false);
    m_toolBar->setIconSize(QSize(16, 16));

    // 采集控制按钮
    m_startBtn = new QPushButton("开始");
    m_stopBtn = new QPushButton("停止");
    m_stopBtn->setEnabled(false);
    m_singleBtn = new QPushButton("单次");

    m_toolBar->addWidget(m_startBtn);
    m_toolBar->addWidget(m_stopBtn);
    m_toolBar->addWidget(m_singleBtn);
    m_toolBar->addSeparator();

    // 触发模式
    m_toolBar->addWidget(new QLabel("触发:"));
    m_triggerModeCombo = new QComboBox();
    m_triggerModeCombo->addItem("自由运行", TriggerFree);
    m_triggerModeCombo->addItem("单次", TriggerSingle);
    m_triggerModeCombo->addItem("自动", TriggerAuto);
    m_triggerModeCombo->setFixedWidth(80);
    m_toolBar->addWidget(m_triggerModeCombo);

    // 触发通道
    m_toolBar->addWidget(new QLabel("源:"));
    m_triggerChannelCombo = new QComboBox();
    m_triggerChannelCombo->setFixedWidth(80);
    m_toolBar->addWidget(m_triggerChannelCombo);

    // 触发边沿
    m_toolBar->addWidget(new QLabel("边沿:"));
    m_triggerEdgeCombo = new QComboBox();
    m_triggerEdgeCombo->addItem("上升", EdgeRising);
    m_triggerEdgeCombo->addItem("下降", EdgeFalling);
    m_triggerEdgeCombo->addItem("电平", EdgeLevel);
    m_triggerEdgeCombo->setFixedWidth(60);
    m_toolBar->addWidget(m_triggerEdgeCombo);

    // 触发电平
    m_toolBar->addWidget(new QLabel("电平:"));
    m_triggerLevelSpin = new QDoubleSpinBox();
    m_triggerLevelSpin->setRange(-1000.0, 1000.0);
    m_triggerLevelSpin->setSingleStep(0.1);
    m_triggerLevelSpin->setDecimals(3);
    m_triggerLevelSpin->setFixedWidth(80);
    m_toolBar->addWidget(m_triggerLevelSpin);

    m_toolBar->addSeparator();

    // 时基
    m_toolBar->addWidget(new QLabel("时基:"));
    m_timebaseSpin = new QDoubleSpinBox();
    m_timebaseSpin->setRange(1.0, 1e8);
    m_timebaseSpin->setSingleStep(100.0);
    m_timebaseSpin->setDecimals(1);
    m_timebaseSpin->setValue(m_timebase);
    m_timebaseSpin->setSuffix(" us/div");
    m_timebaseSpin->setFixedWidth(120);
    m_toolBar->addWidget(m_timebaseSpin);

    m_toolBar->addSeparator();

    // 状态标签
    m_statusLabel = new QLabel("就绪");
    m_statusLabel->setFixedWidth(100);
    m_toolBar->addWidget(m_statusLabel);

    mainLayout->addWidget(m_toolBar);

    // ---- 示波器显示区域（由 paintEvent 绘制） ----
    // 整个 ScopeView 就是绘图区域，通过 paintEvent 自定义绘制
    setMinimumSize(600, 400);
}

void ScopeView::setupConnections()
{
    // 采集控制
    connect(m_startBtn, &QPushButton::clicked, this, &ScopeView::startCapture);
    connect(m_stopBtn, &QPushButton::clicked, this, &ScopeView::stopCapture);
    connect(m_singleBtn, &QPushButton::clicked, this, &ScopeView::singleCapture);

    // 触发设置
    connect(m_triggerModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ScopeView::onTriggerModeChanged);
    connect(m_triggerChannelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ScopeView::onTriggerChannelChanged);
    connect(m_triggerEdgeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ScopeView::onTriggerEdgeChanged);
    connect(m_triggerLevelSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ScopeView::onTriggerLevelChanged);

    // 时基设置
    connect(m_timebaseSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double val) { setTimebase(val); });

    // 采样定时器
    connect(m_sampleTimer, &QTimer::timeout, this, &ScopeView::onSampleTimer);
}

// ============================================================
// 辅助函数
// ============================================================

QVector<QColor> ScopeView::defaultChannelColors()
{
    return {
        QColor(0, 255, 0),     // CH1: 绿色
        QColor(255, 255, 0),   // CH2: 黄色
        QColor(0, 255, 255),   // CH3: 青色
        QColor(255, 0, 255),   // CH4: 品红
        QColor(255, 128, 0),   // CH5: 橙色
        QColor(128, 0, 255),   // CH6: 紫色
        QColor(255, 255, 255), // CH7: 白色
        QColor(0, 128, 255)    // CH8: 蓝色
    };
}
