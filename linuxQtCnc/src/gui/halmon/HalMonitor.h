#ifndef HALMONITOR_H
#define HALMONITOR_H

#include <QWidget>
#include <QTimer>
#include <QTreeView>
#include <QLineEdit>
#include <QTableWidget>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QHeaderView>
#include <QTextBrowser>
#include <QMap>

#include "HalTreeModel.h"

// 前向声明 LcncCore（由 core 模块提供）
class LcncCore;

/**
 * @brief HAL 监视器窗口
 *
 * 实时显示 HAL pin/signal/param 树形结构，替代 halshow.tcl。
 * 可作为独立窗口或嵌入 QDockWidget 使用。
 *
 * 布局：
 *   顶部：搜索过滤框
 *   左侧：QTreeView 显示 HAL 组件/引脚/信号/参数树
 *   右侧：选中项的详细信息面板
 *   底部：Watch 列表（用户添加关注的引脚）
 */
class HalMonitor : public QWidget
{
    Q_OBJECT

public:
    explicit HalMonitor(QWidget *parent = nullptr);
    ~HalMonitor() override;

    /**
     * @brief 设置 LcncCore 实例，用于获取 HAL 数据
     * @param core LcncCore 指针
     */
    void setCore(LcncCore *core);

public slots:
    /**
     * @brief 刷新 HAL 数据
     */
    void refreshData();

    /**
     * @brief 添加引脚到 Watch 列表
     * @param pinName 引脚名称
     */
    void addToWatch(const QString &pinName);

    /**
     * @brief 从 Watch 列表移除引脚
     * @param pinName 引脚名称
     */
    void removeFromWatch(const QString &pinName);

private slots:
    /**
     * @brief 搜索框文本变化处理
     * @param text 搜索文本
     */
    void onSearchTextChanged(const QString &text);

    /**
     * @brief 树视图选择变化处理
     */
    void onTreeSelectionChanged();

    /**
     * @brief 双击树节点处理（编辑引脚值）
     * @param index 点击的模型索引
     */
    void onTreeDoubleClicked(const QModelIndex &index);

    /**
     * @brief 添加到 Watch 按钮
     */
    void onAddToWatch();

    /**
     * @brief 从 Watch 列表移除
     */
    void onRemoveFromWatch();

private:
    /**
     * @brief 初始化界面布局
     */
    void setupUi();

    /**
     * @brief 初始化信号连接
     */
    void setupConnections();

    /**
     * @brief 更新详细信息面板
     */
    void updateDetailPanel(const HalPinInfo &info);

    /**
     * @brief 更新 Watch 列表中的值
     */
    void updateWatchValues();

    // 核心接口
    LcncCore *m_core;

    // UI 组件
    QLineEdit *m_searchEdit;           // 搜索框
    QTreeView *m_treeView;             // 树形视图
    HalTreeModel *m_treeModel;         // 树形模型
    QTextBrowser *m_detailPanel;       // 详细信息面板
    QTableWidget *m_watchTable;        // Watch 列表
    QPushButton *m_addWatchBtn;        // 添加到 Watch 按钮
    QPushButton *m_removeWatchBtn;     // 从 Watch 移除按钮
    QSplitter *m_mainSplitter;         // 主分割器
    QSplitter *m_bottomSplitter;       // 底部分割器

    // 刷新定时器
    QTimer *m_refreshTimer;

    // Watch 列表数据：引脚名 -> 值
    QMap<QString, double> m_watchPins;

    // 刷新周期（毫秒）
    static constexpr int REFRESH_INTERVAL_MS = 100;
};

#endif // HALMONITOR_H
