/*
 * linuxQtCncGui - LinuxCNC Qt6 GUI 控制器
 * StatusBar - 状态栏组件
 *
 * 显示机床运行状态信息：
 * - 机床状态（急停/开启/关闭）
 * - 运动模式（手动/自动/MDI）
 * - 解释器状态（空闲/运行/暂停）
 * - 进给速率和倍率
 * - 主轴转速和倍率
 * - 当前 G/M 代码
 * - 刀具信息
 * - 程序进度
 */

#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QWidget>
#include <QLabel>
#include <QProgressBar>

#include "core/LcncStatusData.h"

/**
 * @brief StatusBarWidget - 机床状态显示栏
 *
 * 以紧凑的工业风格显示机床关键状态信息。
 */
class StatusBarWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父组件
     */
    explicit StatusBarWidget(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~StatusBarWidget() override;

    /**
     * @brief 更新状态显示
     * @param data 最新状态数据
     */
    void updateStatus(const LcncStatusData &data);

private:
    /**
     * @brief 初始化 UI
     */
    void setupUi();

    /**
     * @brief 格式化 G 代码列表
     * @param codes G 代码向量
     * @return 格式化字符串
     */
    static QString formatGCodes(const QVector<int> &codes);

    /**
     * @brief 格式化 M 代码列表
     * @param codes M 代码向量
     * @return 格式化字符串
     */
    static QString formatMCodes(const QVector<int> &codes);

    /**
     * @brief 获取机床状态字符串
     * @param state 机床状态
     * @return 状态描述字符串
     */
    static QString machineStateString(MachineState state);

    /**
     * @brief 获取运动模式字符串
     * @param mode 运动模式
     * @return 模式描述字符串
     */
    static QString motionModeString(MotionMode mode);

    /**
     * @brief 获取解释器状态字符串
     * @param state 解释器状态
     * @return 状态描述字符串
     */
    static QString interpStateString(InterpState state);

private:
    // 状态标签
    QLabel *m_machineStateLabel = nullptr;     ///< 机床状态
    QLabel *m_motionModeLabel = nullptr;       ///< 运动模式
    QLabel *m_interpStateLabel = nullptr;      ///< 解释器状态
    QLabel *m_feedrateLabel = nullptr;         ///< 进给速率
    QLabel *m_feedOverrideLabel = nullptr;     ///< 进给倍率
    QLabel *m_spindleSpeedLabel = nullptr;     ///< 主轴转速
    QLabel *m_spindleOverrideLabel = nullptr;  ///< 主轴倍率
    QLabel *m_gcodesLabel = nullptr;           ///< 当前 G 代码
    QLabel *m_mcodesLabel = nullptr;           ///< 当前 M 代码
    QLabel *m_toolLabel = nullptr;             ///< 刀具信息
    QLabel *m_fileLabel = nullptr;             ///< 当前文件
    QLabel *m_lineLabel = nullptr;             ///< 当前行号
    QProgressBar *m_progressBar = nullptr;     ///< 程序进度条
};

#endif // STATUSBAR_H
