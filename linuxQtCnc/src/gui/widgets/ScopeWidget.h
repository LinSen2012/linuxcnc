/*
 * linuxQtCncGui - LinuxCNC Qt5 GUI 控制器
 * ScopeWidget - 示波器组件
 *
 * 基于 QCustomPlot 的波形显示示波器。
 * 支持多通道、时基调节、触发设置。
 */

#ifndef SCOPEWIDGET_H
#define SCOPEWIDGET_H

#include <QWidget>
#include <QVector>
#include <QColor>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QTimer>

// ============================================================================
// 示波器通道数据
// ============================================================================

struct ScopeChannel {
    QString name;              // 通道名称
    QVector<double> data;     // 采样数据
    QColor color;             // 波形颜色
    bool visible = true;      // 显示状态
    double scale = 1.0;       // 垂直缩放
    double offset = 0.0;      // 垂直偏移
    int penWidth = 1;         // 线宽
};

class ScopeWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父组件
     */
    explicit ScopeWidget(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~ScopeWidget() override;

    // =========================================================================
    // 通道管理
    // =========================================================================

    /**
     * @brief 添加示波器通道
     * @param name 通道名称
     * @param color 波形颜色
     * @return 通道索引
     */
    int addChannel(const QString &name, const QColor &color = QColor());

    /**
     * @brief 添加采样数据到通道
     * @param channel 通道索引
     * @param value 采样值
     */
    void addSample(int channel, double value);

    /**
     * @brief 清除所有通道数据
     */
    void clear();

    /**
     * @brief 设置通道可见性
     * @param channel 通道索引
     * @param visible 是否可见
     */
    void setChannelVisible(int channel, bool visible);

    /**
     * @brief 设置时基（每格时间）
     * @param secs 秒数
     */
    void setTimeBase(double secs);

    /**
     * @brief 设置触发电平
     * @param level 触发电平值
     */
    void setTriggerLevel(double level);

    /**
     * @brief 设置触发通道
     * @param channel 通道索引，-1 表示无触发
     */
    void setTriggerChannel(int channel);

    /**
     * @brief 启动示波器
     */
    void start();

    /**
     * @brief 停止示波器
     */
    void stop();

    /**
     * @brief 是否正在运行
     * @return 运行状态
     */
    bool isRunning() const { return m_running; }

signals:
    /**
     * @brief 触发信号
     * @param channel 触发通道
     */
    void triggered(int channel);

private:
    /**
     * @brief 初始化 UI
     */
    void setupUi();

    /**
     * @brief 更新绘图
     */
    void updatePlot();

private slots:
    /**
     * @brief 时基改变
     */
    void onTimeBaseChanged(double value);

    /**
     * @brief 触发电平改变
     */
    void onTriggerLevelChanged(double value);

    /**
     * @brief 运行/停止按钮
     */
    void onRunStopClicked();

    /**
     * @brief 清零按钮
     */
    void onClearClicked();

    /**
     * @brief 刷新定时器
     */
    void onRefreshTimer();

private:
    // UI 组件
    QVector<ScopeChannel> m_channels;    // 通道数据
    QComboBox *m_triggerChannelCombo = nullptr;
    QDoubleSpinBox *m_timeBaseSpin = nullptr;
    QDoubleSpinBox *m_triggerLevelSpin = nullptr;
    QPushButton *m_runStopBtn = nullptr;
    QPushButton *m_clearBtn = nullptr;
    QLabel *m_statusLabel = nullptr;

    // 配置
    static const int MAX_SAMPLES = 1000;
    double m_timeBase = 0.1;  // 100ms 每格
    double m_triggerLevel = 0.0;
    int m_triggerChannel = -1;
    bool m_running = false;

    // 刷新定时器
    QTimer *m_refreshTimer = nullptr;
    int m_refreshRate = 50;  // ms
};

// ============================================================================
// 简化版示波器（不使用 QCustomPlot，仅用 QPainter）
// ============================================================================

class SimpleScopeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SimpleScopeWidget(QWidget *parent = nullptr);
    ~SimpleScopeWidget() override;

    void addChannel(const QString &name, const QColor &color);
    void addSample(int channel, double value);
    void clear();
    void setTimeBase(double secs) { m_timeBase = secs; update(); }
    void start() { m_running = true; }
    void stop() { m_running = false; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<ScopeChannel> m_channels;
    double m_timeBase = 1.0;
    bool m_running = false;
    QTimer *m_timer = nullptr;
};

#endif // SCOPEWIDGET_H