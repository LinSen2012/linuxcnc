/*
 * linuxQtCncGui - LinuxCNC Qt6 GUI 控制器
 * JogPanel - 手轮/点动面板
 *
 * 提供机床手动控制功能：
 * - 各轴点动按钮（正/负方向）
 * - 点动速度调节
 * - 增量点动距离选择
 * - 回参考点按钮
 * - 主轴控制
 * - 冷却液控制
 */

#ifndef JOGPANEL_H
#define JOGPANEL_H

#include <QWidget>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QComboBox>

/**
 * @brief JogPanel - 手轮/点动控制面板
 *
 * 提供完整的机床手动操作界面。
 * 支持连续点动、增量点动和回参考点操作。
 */
class JogPanel : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父组件
     */
    explicit JogPanel(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~JogPanel() override;

    /**
     * @brief 设置轴数量
     * @param count 轴数量（1~9，默认 3）
     */
    void setAxisCount(int count);

    /**
     * @brief 设置点动速度范围
     * @param minSpeed 最小速度
     * @param maxSpeed 最大速度
     * @param defaultSpeed 默认速度
     */
    void setJogSpeedRange(double minSpeed, double maxSpeed, double defaultSpeed);

signals:
    /**
     * @brief 点动请求信号
     * @param axis 轴编号
     * @param velocity 速度（正值=正方向，负值=负方向）
     */
    void jogRequested(int axis, double velocity);

    /**
     * @brief 停止点动信号
     * @param axis 轴编号
     */
    void jogStopRequested(int axis);

    /**
     * @brief 回参考点请求信号
     * @param axis 轴编号（-1 表示全部轴）
     */
    void homeRequested(int axis);

    /**
     * @brief 主轴控制信号
     * @param forward true=正转, false=反转, stop=停止
     */
    void spindleControl(bool forward, bool stop = false);

private:
    /**
     * @brief 初始化 UI
     */
    void setupUi();

    /**
     * @brief 创建轴点动按钮组
     * @param axisName 轴名称
     * @param axisIndex 轴编号
     * @return 包含按钮的布局
     */
    QWidget *createAxisJogGroup(const QString &axisName, int axisIndex);

private slots:
    /**
     * @brief 点动按钮按下
     */
    void onJogButtonPressed();

    /**
     * @brief 点动按钮释放
     */
    void onJogButtonReleased();

    /**
     * @brief 增量点动按钮点击
     */
    void onIncrementJogClicked();

    /**
     * @brief 速度值改变
     */
    void onJogSpeedChanged(double value);

private:
    // 配置
    int m_axisCount = 3;       ///< 轴数量
    double m_jogSpeed = 50.0;  ///< 当前点动速度

    // UI 组件
    QDoubleSpinBox *m_jogSpeedSpin = nullptr;   ///< 点动速度输入
    QComboBox *m_incrementCombo = nullptr;       ///< 增量距离选择

    // 点动按钮（每个轴一对：正/负）
    static const int MAX_AXES = 9;
    QPushButton *m_jogNegButtons[MAX_AXES] = {}; ///< 负方向点动按钮
    QPushButton *m_jogPosButtons[MAX_AXES] = {}; ///< 正方向点动按钮
    QPushButton *m_homeButtons[MAX_AXES] = {};  ///< 回参考点按钮

    // 主轴控制按钮
    QPushButton *m_spindleFwdBtn = nullptr;     ///< 主轴正转
    QPushButton *m_spindleRevBtn = nullptr;     ///< 主轴反转
    QPushButton *m_spindleStopBtn = nullptr;    ///< 主轴停止

    // 冷却液控制按钮
    QPushButton *m_coolantFloodBtn = nullptr;   ///< 喷流冷却
    QPushButton *m_coolantMistBtn = nullptr;    ///< 雾化冷却
};

#endif // JOGPANEL_H
