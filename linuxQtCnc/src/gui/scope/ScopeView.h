#ifndef SCOPEVIEW_H
#define SCOPEVIEW_H

#include <QWidget>
#include <QTimer>
#include <QVector>
#include <QMap>
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QToolBar>
#include <QComboBox>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QActionGroup>

#include "ScopeChannel.h"

// 前向声明 LcncCore
class LcncCore;

/**
 * @brief 多通道信号示波器视图
 *
 * 使用 QPainter 自定义绘制，实现示波器风格的网格和波形显示。
 * 替代 halscope，支持最多 8 个通道同时显示。
 *
 * 功能：
 *   - 多通道波形显示（最多 8 通道）
 *   - 水平轴：时间（可调时基）
 *   - 垂直轴：信号值（可调增益）
 *   - 触发模式：自由运行 / 单次 / 自动
 *   - 触发源选择：任一通道的上升沿/下降沿/电平
 *   - 采集控制：开始/停止/单次采集
 *   - 环形缓冲区（默认 4096 点）
 *   - 示波器风格网格背景
 *   - 鼠标悬停十字光标和值显示
 */
class ScopeView : public QWidget
{
    Q_OBJECT

public:
    // 触发模式枚举
    enum TriggerMode {
        TriggerFree = 0,     // 自由运行
        TriggerSingle = 1,  // 单次触发
        TriggerAuto = 2     // 自动触发
    };

    // 触发边沿枚举
    enum TriggerEdge {
        EdgeRising = 0,     // 上升沿
        EdgeFalling = 1,    // 下降沿
        EdgeLevel = 2       // 电平触发
    };

    // 最大通道数
    static constexpr int MAX_CHANNELS = 8;

    explicit ScopeView(QWidget *parent = nullptr);
    ~ScopeView() override;

    /**
     * @brief 设置 LcncCore 实例
     * @param core LcncCore 指针
     */
    void setCore(LcncCore *core);

    /**
     * @brief 添加通道
     * @param channel 通道对象
     */
    void addChannel(ScopeChannel *channel);

    /**
     * @brief 移除通道
     * @param index 通道索引
     */
    void removeChannel(int index);

    /**
     * @brief 获取通道数量
     */
    int channelCount() const { return m_channels.size(); }

    /**
     * @brief 获取指定通道
     */
    ScopeChannel* channel(int index) const;

public slots:
    /**
     * @brief 开始采集
     */
    void startCapture();

    /**
     * @brief 停止采集
     */
    void stopCapture();

    /**
     * @brief 单次采集
     */
    void singleCapture();

    /**
     * @brief 设置触发模式
     * @param mode 触发模式枚举
     */
    void setTriggerMode(int mode);

    /**
     * @brief 设置时基（每格微秒数）
     * @param usPerDiv 微秒/格
     */
    void setTimebase(double usPerDiv);

    /**
     * @brief 清除所有通道数据
     */
    void clearAllChannels();

private slots:
    /**
     * @brief 采样定时器超时处理
     */
    void onSampleTimer();

    /**
     * @brief 触发模式选择变化
     */
    void onTriggerModeChanged(int index);

    /**
     * @brief 触发通道选择变化
     */
    void onTriggerChannelChanged(int index);

    /**
     * @brief 触发边沿选择变化
     */
    void onTriggerEdgeChanged(int index);

    /**
     * @brief 触发电平值变化
     */
    void onTriggerLevelChanged(double value);

private:
    // 界面初始化
    void setupUi();
    void setupConnections();

    // 绘制
    void paintEvent(QPaintEvent *event) override;
    void drawGrid(QPainter &painter, const QRectF &plotArea);
    void drawWaveforms(QPainter &painter, const QRectF &plotArea);
    void drawChannelLabels(QPainter &painter, const QRectF &plotArea);
    void drawCrosshair(QPainter &painter, const QRectF &plotArea);
    void drawTriggerIndicator(QPainter &painter, const QRectF &plotArea);
    void drawTimeAxis(QPainter &painter, const QRectF &plotArea);
    void drawValueAxis(QPainter &painter, const QRectF &plotArea);

    // 鼠标事件
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

    // 触发检测
    bool checkTrigger();

    // 默认通道颜色
    static QVector<QColor> defaultChannelColors();

    // 核心接口
    LcncCore *m_core;

    // 通道列表
    QVector<ScopeChannel*> m_channels;

    // 采集状态
    bool m_capturing;          // 是否正在采集
    bool m_singleShotDone;     // 单次采集是否已完成

    // 触发参数
    TriggerMode m_triggerMode;
    int m_triggerChannel;      // 触发源通道索引
    TriggerEdge m_triggerEdge;
    double m_triggerLevel;      // 触发电平

    // 时基参数
    double m_timebase;         // 微秒/格
    double m_sampleRate;       // 采样率（Hz）
    int m_samplesPerGrid;      // 每格采样点数

    // 显示参数
    int m_divisionsH;          // 水平格数
    int m_divisionsV;          // 垂直格数
    double m_voltsPerDiv;      // 默认 V/div

    // 鼠标状态
    QPoint m_mousePos;
    bool m_showCrosshair;

    // 绘图区域边距
    int m_marginLeft;
    int m_marginRight;
    int m_marginTop;
    int m_marginBottom;

    // 采样定时器
    QTimer *m_sampleTimer;

    // UI 控件
    QToolBar *m_toolBar;
    QPushButton *m_startBtn;
    QPushButton *m_stopBtn;
    QPushButton *m_singleBtn;
    QComboBox *m_triggerModeCombo;
    QComboBox *m_triggerChannelCombo;
    QComboBox *m_triggerEdgeCombo;
    QDoubleSpinBox *m_triggerLevelSpin;
    QDoubleSpinBox *m_timebaseSpin;
    QLabel *m_statusLabel;

    // 采样周期（毫秒）
    static constexpr int SAMPLE_INTERVAL_MS = 1;
};

#endif // SCOPEVIEW_H
