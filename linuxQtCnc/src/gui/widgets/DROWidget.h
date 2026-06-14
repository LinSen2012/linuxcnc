/*
 * linuxQtCncGui - LinuxCNC Qt6 GUI 控制器
 * DROWidget - 数字读数显示（Digital ReadOut）
 *
 * 显示机床各轴的坐标位置，支持：
 * - 绝对坐标（机床坐标系）
 * - 相对坐标
 * - 工件坐标系（G54 等）
 * - 高对比度 LED 风格数字显示
 * - 可配置的小数位数和轴数量
 */

#ifndef DROWIDGET_H
#define DROWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QGroupBox>
#include <QGridLayout>
#include <QComboBox>

#include "core/LcncStatusData.h"

/**
 * @brief DROWidget - 数字读数显示组件
 *
 * 以工业 DRO 风格显示各轴坐标位置。
 * 支持绝对坐标、相对坐标和工件坐标系三种显示模式。
 */
class DROWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父组件
     */
    explicit DROWidget(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~DROWidget() override;

    /**
     * @brief 更新坐标位置显示
     * @param absPos 绝对坐标
     * @param relPos 相对坐标
     * @param workPos 工件坐标系坐标
     */
    void updatePosition(const LcncPose &absPos,
                         const LcncPose &relPos,
                         const LcncPose &workPos);

    /**
     * @brief 设置显示的小数位数
     * @param digits 小数位数（默认 4）
     */
    void setDecimalDigits(int digits);

    /**
     * @brief 设置显示的轴数量
     * @param count 轴数量（1~9，默认 3）
     */
    void setAxisCount(int count);

private:
    /**
     * @brief 初始化 UI
     */
    void setupUi();

    /**
     * @brief 创建轴显示行
     * @param axisName 轴名称（X/Y/Z/A/B/C/U/V/W）
     * @param row 行号
     */
    void createAxisRow(const QString &axisName, int row);

    /**
     * @brief 格式化坐标值
     * @param value 坐标值
     * @return 格式化字符串
     */
    QString formatValue(double value) const;

    /**
     * @brief 更新所有轴的显示
     */
    void refreshDisplay();

private slots:
    /**
     * @brief 坐标系切换回调
     */
    void onCoordinateSystemChanged(int index);

private:
    // 坐标数据
    LcncPose m_absPos;     ///< 绝对坐标
    LcncPose m_relPos;     ///< 相对坐标
    LcncPose m_workPos;    ///< 工件坐标系坐标

    // UI 组件
    QGridLayout *m_gridLayout = nullptr;  ///< 网格布局
    QComboBox *m_coordSystemCombo = nullptr; ///< 坐标系选择下拉框

    // 轴标签和数值标签
    static const int MAX_AXES = 9;
    QLabel *m_axisLabels[MAX_AXES] = {};     ///< 轴名称标签
    QLabel *m_valueLabels[MAX_AXES] = {};   ///< 坐标值标签

    // 配置
    int m_decimalDigits = 4;  ///< 小数位数
    int m_axisCount = 3;      ///< 显示轴数量

    // 坐标系类型
    enum class CoordSystem {
        Absolute,  ///< 绝对坐标
        Relative,  ///< 相对坐标
        Work       ///< 工件坐标系
    };
    CoordSystem m_coordSystem = CoordSystem::Work;
};

#endif // DROWIDGET_H
