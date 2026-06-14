/*
 * linuxQtCncGui - LinuxCNC Qt5 GUI 控制器
 * HalMonitorWidget - HAL 信号监视器
 *
 * 显示 HAL 引脚和信号的实时状态。
 * 支持搜索、过滤和信号追踪。
 */

#ifndef HALMONITORWIDGET_H
#define HALMONITORWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QLineEdit>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>

class HalMonitorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HalMonitorWidget(QWidget *parent = nullptr);
    ~HalMonitorWidget() override;

    /**
     * @brief 刷新 HAL 信号列表
     */
    void refresh();

    /**
     * @brief 设置过滤关键字
     * @param filter 过滤字符串
     */
    void setFilter(const QString &filter);

    /**
     * @brief 获取选中的信号名称
     * @return 信号名称，未选中则为空
     */
    QString selectedSignal() const;

signals:
    /**
     * @brief 信号选中信号
     * @param name 信号名称
     */
    void signalSelected(const QString &name);

    /**
     * @brief 信号值改变信号
     * @param name 信号名称
     * @param value 新的值
     */
    void signalValueChanged(const QString &name, double value);

private:
    /**
     * @brief 初始化 UI
     */
    void setupUi();

    /**
     * @brief 创建信号列表
     */
    void createSignalTable();

private slots:
    /**
     * @brief 过滤文本改变
     */
    void onFilterChanged(const QString &text);

    /**
     * @brief 表格选择改变
     */
    void onSelectionChanged();

    /**
     * @brief 刷新定时器
     */
    void onRefreshTimer();

private:
    // UI 组件
    QTableWidget *m_signalTable = nullptr;
    QLineEdit *m_filterEdit = nullptr;
    QComboBox *m_typeFilter = nullptr;
    QLabel *m_signalCountLabel = nullptr;

    // 数据
    QString m_currentFilter;
    int m_refreshRate = 250;  // ms
};

#endif // HALMONITORWIDGET_H