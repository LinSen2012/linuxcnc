/**
 * @file DROWidget.cpp
 * @brief 数字读数（DRO）控件实现 - 工业 LCD 风格坐标显示
 */

#include "DROWidget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QFontDatabase>
#include <QFrame>
#include <cmath>

// ============================================================================
// 工业风格颜色常量
// ============================================================================
namespace DROColors {
    // 背景色
    static const QColor BG_DARK       = QColor(30, 30, 35);       // 深色背景
    static const QColor BG_PANEL      = QColor(20, 22, 28);       // 面板背景
    static const QColor BG_LCD        = QColor(10, 12, 18);       // LCD 背景
    static const QColor BORDER        = QColor(60, 65, 75);        // 边框色

    // 正常状态 - 绿色
    static const QColor TEXT_NORMAL   = QColor(0, 220, 80);        // 正常绿色
    static const QColor LABEL_NORMAL  = QColor(120, 200, 140);     // 标签绿色

    // 警告状态 - 黄色（接近限位）
    static const QColor TEXT_WARNING  = QColor(255, 200, 0);       // 警告黄色
    static const QColor LABEL_WARNING = QColor(220, 180, 60);      // 标签黄色

    // 报警状态 - 红色（限位触发）
    static const QColor TEXT_ALARM     = QColor(255, 50, 50);       // 报警红色
    static const QColor LABEL_ALARM    = QColor(220, 80, 80);       // 标签红色

    // 未回零 - 灰色
    static const QColor TEXT_UNHOMED   = QColor(100, 100, 110);     // 未回零灰色

    // 进给/主轴信息
    static const QColor TEXT_INFO      = QColor(180, 200, 220);     // 信息文字
    static const QColor TEXT_VALUE     = QColor(0, 200, 255);       // 数值蓝色
}

// ============================================================================
// AxisDRO - 单轴数字读数
// ============================================================================
AxisDRO::AxisDRO(const QString &axisName, QWidget *parent)
    : QWidget(parent)
    , m_axisName(axisName)
    , m_limitStatus(0)
    , m_homed(false)
{
    setupUi();
    updateColors();
}

void AxisDRO::setupUi()
{
    // 使用固定宽度的等宽字体，模拟 LCD 显示效果
    QFont lcdFont("Consolas", 16, QFont::Bold);
    QFont labelFont("Segoe UI", 9, QFont::Normal);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 2, 4, 2);
    mainLayout->setSpacing(1);

    // 轴名称标签
    m_axisLabel = new QLabel(m_axisName);
    m_axisLabel->setFont(QFont("Segoe UI", 11, QFont::Bold));
    m_axisLabel->setAlignment(Qt::AlignCenter);
    m_axisLabel->setFixedHeight(20);
    mainLayout->addWidget(m_axisLabel);

    // 绝对坐标
    auto *absRow = new QHBoxLayout();
    auto *absLabel = new QLabel(tr("绝对"));
    absLabel->setFont(labelFont);
    absLabel->setFixedWidth(36);
    m_absValue = new QLabel("0.0000");
    m_absValue->setFont(lcdFont);
    m_absValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    absRow->addWidget(absLabel);
    absRow->addWidget(m_absValue);
    mainLayout->addLayout(absRow);

    // 相对坐标
    auto *relRow = new QHBoxLayout();
    auto *relLabel = new QLabel(tr("相对"));
    relLabel->setFont(labelFont);
    relLabel->setFixedWidth(36);
    m_relValue = new QLabel("0.0000");
    m_relValue->setFont(lcdFont);
    m_relValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    relRow->addWidget(relLabel);
    relRow->addWidget(m_relValue);
    mainLayout->addLayout(relRow);

    // 工件坐标（DTG）
    auto *dtgRow = new QHBoxLayout();
    auto *dtgLabel = new QLabel(tr("工件"));
    dtgLabel->setFont(labelFont);
    dtgLabel->setFixedWidth(36);
    m_dtgValue = new QLabel("0.0000");
    m_dtgValue->setFont(lcdFont);
    m_dtgValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    dtgRow->addWidget(dtgLabel);
    dtgRow->addWidget(m_dtgValue);
    mainLayout->addLayout(dtgRow);

    setFixedWidth(200);
    setFixedHeight(110);
}

void AxisDRO::updateColors()
{
    QColor textColor;
    QColor labelColor;

    if (!m_homed) {
        textColor = DROColors::TEXT_UNHOMED;
        labelColor = DROColors::TEXT_UNHOMED;
    } else if (m_limitStatus == 2) {
        textColor = DROColors::TEXT_ALARM;
        labelColor = DROColors::LABEL_ALARM;
    } else if (m_limitStatus == 1) {
        textColor = DROColors::TEXT_WARNING;
        labelColor = DROColors::LABEL_WARNING;
    } else {
        textColor = DROColors::TEXT_NORMAL;
        labelColor = DROColors::LABEL_NORMAL;
    }

    // 设置样式表
    QString style = QString(
        "QLabel {"
        "  color: %1;"
        "  background: transparent;"
        "  border: none;"
        "}"
    ).arg(textColor.name());

    m_absValue->setStyleSheet(style);
    m_relValue->setStyleSheet(style);
    m_dtgValue->setStyleSheet(style);

    m_axisLabel->setStyleSheet(QString(
        "QLabel {"
        "  color: %1;"
        "  background: transparent;"
        "  border: none;"
        "}"
    ).arg(labelColor.name()));
}

void AxisDRO::setAxisName(const QString &name)
{
    m_axisName = name;
    m_axisLabel->setText(name);
}

void AxisDRO::updatePosition(double absolute, double relative, double dtg)
{
    m_absValue->setText(QString::number(absolute, 'f', 4));
    m_relValue->setText(QString::number(relative, 'f', 4));
    m_dtgValue->setText(QString::number(dtg, 'f', 4));
}

void AxisDRO::setLimitStatus(int status)
{
    if (m_limitStatus != status) {
        m_limitStatus = status;
        updateColors();
        update();
    }
}

void AxisDRO::setHomed(bool homed)
{
    if (m_homed != homed) {
        m_homed = homed;
        updateColors();
        update();
    }
}

QSize AxisDRO::sizeHint() const
{
    return QSize(200, 110);
}

void AxisDRO::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制 LCD 面板背景
    QColor bgColor = DROColors::BG_LCD;
    if (m_limitStatus == 2) {
        bgColor = QColor(40, 15, 15);  // 红色报警背景
    } else if (m_limitStatus == 1) {
        bgColor = QColor(35, 30, 10);  // 黄色警告背景
    }

    painter.fillRect(rect(), bgColor);

    // 绘制边框
    QPen borderPen(DROColors::BORDER);
    borderPen.setWidth(1);
    painter.setPen(borderPen);
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}

// ============================================================================
// DROWidget - 数字读数主控件
// ============================================================================
DROWidget::DROWidget(QWidget *parent)
    : QWidget(parent)
    , m_axisCount(3)
{
    setupUi();
    applyIndustrialStyle();
}

AxisDRO *DROWidget::axisDRO(int index) const
{
    if (index >= 0 && index < m_axisDROs.size())
        return m_axisDROs[index];
    return nullptr;
}

void DROWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(4);

    // 轴 DRO 区域 - 使用网格布局，每行最多 3 个轴
    auto *axisGrid = new QGridLayout();
    axisGrid->setSpacing(4);

    const QStringList axisNames = {"X", "Y", "Z", "A", "B", "U"};

    // 创建 3 轴（默认）
    for (int i = 0; i < m_axisCount; ++i) {
        auto *dro = new AxisDRO(axisNames[i], this);
        m_axisDROs.append(dro);
        int row = i / 3;
        int col = i % 3;
        axisGrid->addWidget(dro, row, col);
    }

    mainLayout->addLayout(axisGrid);

    // 分隔线
    auto *separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    separator->setStyleSheet("color: #3C414B;");
    mainLayout->addWidget(separator);

    // ---- 进给速率区域 ----
    auto *feedLayout = new QHBoxLayout();
    m_feedLabel = new QLabel(tr("进给速率:"));
    m_feedLabel->setStyleSheet("color: #B4C8DC; font-size: 11px;");
    m_feedValue = new QLabel("0.0");
    m_feedValue->setStyleSheet(
        "color: #00C8FF; font-family: Consolas; font-size: 14px; font-weight: bold;"
    );
    auto *feedUnit = new QLabel(tr("mm/min"));
    feedUnit->setStyleSheet("color: #7888A0; font-size: 10px;");
    feedLayout->addWidget(m_feedLabel);
    feedLayout->addWidget(m_feedValue);
    feedLayout->addWidget(feedUnit);
    feedLayout->addStretch();
    mainLayout->addLayout(feedLayout);

    // ---- 主轴状态区域 ----
    auto *spindleLayout = new QHBoxLayout();
    m_spindleLabel = new QLabel(tr("主轴:"));
    m_spindleLabel->setStyleSheet("color: #B4C8DC; font-size: 11px;");
    m_spindleSpeedValue = new QLabel("0");
    m_spindleSpeedValue->setStyleSheet(
        "color: #00C8FF; font-family: Consolas; font-size: 14px; font-weight: bold;"
    );
    m_spindleDirValue = new QLabel(tr("停止"));
    m_spindleDirValue->setStyleSheet("color: #FF5050; font-size: 11px;");
    auto *rpmUnit = new QLabel(tr("RPM"));
    rpmUnit->setStyleSheet("color: #7888A0; font-size: 10px;");
    spindleLayout->addWidget(m_spindleLabel);
    spindleLayout->addWidget(m_spindleSpeedValue);
    spindleLayout->addWidget(rpmUnit);
    spindleLayout->addSpacing(10);
    spindleLayout->addWidget(m_spindleDirValue);
    spindleLayout->addStretch();
    mainLayout->addLayout(spindleLayout);

    // ---- G/M 代码状态区域 ----
    auto *codeLayout = new QHBoxLayout();
    m_gcodeLabel = new QLabel(tr("G代码:"));
    m_gcodeLabel->setStyleSheet("color: #B4C8DC; font-size: 11px;");
    m_gcodeValue = new QLabel("G0");
    m_gcodeValue->setStyleSheet(
        "color: #00DC50; font-family: Consolas; font-size: 13px; font-weight: bold;"
    );
    codeLayout->addWidget(m_gcodeLabel);
    codeLayout->addWidget(m_gcodeValue);
    codeLayout->addSpacing(20);
    m_mcodeLabel = new QLabel(tr("M代码:"));
    m_mcodeLabel->setStyleSheet("color: #B4C8DC; font-size: 11px;");
    m_mcodeValue = new QLabel("M5");
    m_mcodeValue->setStyleSheet(
        "color: #FFC800; font-family: Consolas; font-size: 13px; font-weight: bold;"
    );
    codeLayout->addWidget(m_mcodeLabel);
    codeLayout->addWidget(m_mcodeValue);
    codeLayout->addStretch();
    mainLayout->addLayout(codeLayout);
}

void DROWidget::setAxisCount(int count)
{
    if (count < 3) count = 3;
    if (count > 6) count = 6;
    if (count == m_axisCount) return;

    // 清除旧的轴 DRO
    qDeleteAll(m_axisDROs);
    m_axisDROs.clear();
    m_axisCount = count;

    const QStringList axisNames = {"X", "Y", "Z", "A", "B", "U"};

    // 重新创建
    auto *axisGrid = findChild<QGridLayout *>();
    if (!axisGrid) return;

    // 清除网格中的旧项
    QLayoutItem *item;
    while ((item = axisGrid->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    for (int i = 0; i < m_axisCount; ++i) {
        auto *dro = new AxisDRO(axisNames[i], this);
        m_axisDROs.append(dro);
        int row = i / 3;
        int col = i % 3;
        axisGrid->addWidget(dro, row, col);
    }

    updateGeometry();
}

void DROWidget::updateStatus(const LcncStatusData &data)
{
    // 从 LcncPose 提取各轴坐标数据
    // LcncPose 包含 x/y/z/a/b/c/u/v/w 字段
    const double absCoords[] = {
        data.absolutePos.x, data.absolutePos.y, data.absolutePos.z,
        data.absolutePos.a, data.absolutePos.b, data.absolutePos.u
    };
    const double relCoords[] = {
        data.relativePos.x, data.relativePos.y, data.relativePos.z,
        data.relativePos.a, data.relativePos.b, data.relativePos.u
    };
    const double dtgCoords[] = {
        data.distanceToGo.x, data.distanceToGo.y, data.distanceToGo.z,
        data.distanceToGo.a, data.distanceToGo.b, data.distanceToGo.u
    };

    // 更新各轴坐标
    for (int i = 0; i < m_axisCount && i < m_axisDROs.size(); ++i) {
        m_axisDROs[i]->updatePosition(absCoords[i], relCoords[i], dtgCoords[i]);

        // 从 axes 数组获取限位和回零状态
        if (i < data.axes.size()) {
            const auto &axis = data.axes[i];
            // 限位状态：NONE=0, MIN_LIMIT=1, MAX_LIMIT=2, BOTH=3
            int limitStatus = 0;
            if (axis.limitState == LimitState::BOTH) {
                limitStatus = 2;
            } else if (axis.limitState == LimitState::MIN_LIMIT ||
                       axis.limitState == LimitState::MAX_LIMIT) {
                limitStatus = 1;
            }
            m_axisDROs[i]->setLimitStatus(limitStatus);

            // 回零状态
            bool homed = (axis.homingState == HomingState::HOMED);
            m_axisDROs[i]->setHomed(homed);
        }
    }

    // 更新进给速率
    m_feedValue->setText(QString::number(data.feedrate, 'f', 1));

    // 更新主轴状态
    m_spindleSpeedValue->setText(QString::number(data.spindleSpeed));
    if (data.spindleState == SpindleState::FORWARD) {
        m_spindleDirValue->setText(tr("正转"));
        m_spindleDirValue->setStyleSheet("color: #00DC50; font-size: 11px;");
    } else if (data.spindleState == SpindleState::REVERSE) {
        m_spindleDirValue->setText(tr("反转"));
        m_spindleDirValue->setStyleSheet("color: #FFC800; font-size: 11px;");
    } else {
        m_spindleDirValue->setText(tr("停止"));
        m_spindleDirValue->setStyleSheet("color: #FF5050; font-size: 11px;");
    }

    // 更新 G/M 代码
    if (!data.activeGCodesData.codes.isEmpty()) {
        QStringList gList;
        for (int code : data.activeGCodesData.codes) {
            gList << QString("G%1").arg(code);
        }
        m_gcodeValue->setText(gList.join(" "));
    }
    if (!data.activeMCodesData.codes.isEmpty()) {
        QStringList mList;
        for (int code : data.activeMCodesData.codes) {
            mList << QString("M%1").arg(code);
        }
        m_mcodeValue->setText(mList.join(" "));
    }
}

void DROWidget::onStatusUpdate(const LcncStatusData &data)
{
    updateStatus(data);
}

void DROWidget::applyIndustrialStyle()
{
    setStyleSheet(
        "DROWidget {"
        "  background-color: #1E1E23;"
        "  border: 1px solid #3C414B;"
        "  border-radius: 4px;"
        "}"
    );
}

QSize DROWidget::sizeHint() const
{
    int rows = (m_axisCount + 2) / 3;
    return QSize(640, rows * 120 + 100);
}
