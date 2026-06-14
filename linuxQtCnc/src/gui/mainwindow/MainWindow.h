/*
 * linuxQtCncGui - LinuxCNC Qt6 GUI 控制器
 * MainWindow - 主窗口
 *
 * CNC 控制器的主界面窗口，采用纯代码布局（无 .ui 文件）。
 * 布局结构：
 * - 顶部：菜单栏 + 工具栏
 * - 中央：分割布局
 *   - 左侧：3D 刀具路径预览 (ToolpathView)
 *   - 右侧上：DRO 数字读数 + 状态栏
 *   - 右侧下：G 代码编辑器
 * - 底部：点动面板 (JogPanel)
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QToolBar>
#include <QMenuBar>
#include <QStatusBar>

class DROWidget;
class GCodeEditor;
class JogPanel;
class StatusBarWidget;
class ToolpathView;
class LcncCore;

/**
 * @brief MainWindow - CNC 控制器主窗口
 *
 * 管理所有子组件的布局和交互。
 * 纯代码布局，不使用 .ui 文件。
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口
     */
    explicit MainWindow(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~MainWindow() override;

public slots:
    /**
     * @brief 启动模拟模式
     */
    void startSimulation();

    /**
     * @brief 连接到指定的 INI 文件
     * @param iniFile INI 配置文件路径
     */
    void connectToIni(const QString &iniFile);

private slots:
    /**
     * @brief 状态数据更新回调
     */
    void onStatusUpdated();

    /**
     * @brief 连接状态变化回调
     */
    void onConnected();

    /**
     * @brief 断开连接回调
     */
    void onDisconnected();

    /**
     * @brief 错误回调
     * @param message 错误信息
     */
    void onError(const QString &message);

private:
    /**
     * @brief 初始化 UI 布局
     */
    void setupUi();

    /**
     * @brief 创建菜单栏
     */
    void setupMenus();

    /**
     * @brief 创建工具栏
     */
    void setupToolBars();

    /**
     * @brief 创建中央布局
     */
    void setupCentralWidget();

    /**
     * @brief 创建底部面板
     */
    void setupBottomPanel();

    /**
     * @brief 连接信号/槽
     */
    void setupConnections();

private:
    // 核心引用
    LcncCore *m_core = nullptr;

    // 子组件
    DROWidget *m_droWidget = nullptr;           ///< 数字读数显示
    GCodeEditor *m_gcodeEditor = nullptr;       ///< G 代码编辑器
    JogPanel *m_jogPanel = nullptr;             ///< 点动面板
    StatusBarWidget *m_statusBarWidget = nullptr; ///< 状态栏组件
    ToolpathView *m_toolpathView = nullptr;     ///< 3D 刀具路径视图

    // 布局组件
    QSplitter *m_mainSplitter = nullptr;        ///< 主分割器
    QSplitter *m_rightSplitter = nullptr;        ///< 右侧分割器

    // 菜单和工具栏
    QMenu *m_fileMenu = nullptr;                ///< 文件菜单
    QMenu *m_machineMenu = nullptr;             ///< 机床菜单
    QMenu *m_viewMenu = nullptr;                ///< 视图菜单
    QMenu *m_helpMenu = nullptr;                ///< 帮助菜单
    QToolBar *m_machineToolBar = nullptr;        ///< 机床控制工具栏
    QToolBar *m_programToolBar = nullptr;         ///< 程序控制工具栏

    // 工具栏动作
    QAction *m_estopAction = nullptr;            ///< 急停动作
    QAction *m_machineOnAction = nullptr;        ///< 机床开启动作
    QAction *m_machineOffAction = nullptr;       ///< 机床关闭动作
    QAction *m_homeAllAction = nullptr;          ///< 全轴回参考点
    QAction *m_modeAutoAction = nullptr;         ///< 自动模式
    QAction *m_modeManualAction = nullptr;       ///< 手动模式
    QAction *m_modeMdiAction = nullptr;          ///< MDI 模式
    QAction *m_programRunAction = nullptr;       ///< 运行程序
    QAction *m_programPauseAction = nullptr;     ///< 暂停程序
    QAction *m_programStopAction = nullptr;      ///< 停止程序
    QAction *m_programOpenAction = nullptr;      ///< 打开程序
};

#endif // MAINWINDOW_H
