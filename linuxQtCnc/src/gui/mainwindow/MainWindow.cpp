/**
 * @file MainWindow.cpp
 * @brief 主窗口实现 - 整合所有 GUI 组件，连接 LcncCore 信号
 *
 * 布局：QSplitter 分割布局
 *   - 左侧：QDockWidget 包含 GCodeEditor
 *   - 中央：ToolpathView（3D 刀具路径预览）
 *   - 右侧上：DROWidget（数字读数）
 *   - 右侧下：JogPanel（手轮/点动面板）
 *   - 底部：StatusBar（状态栏）
 */

#include "MainWindow.h"

#include "LcncCore.h"
#include "LcncStatusData.h"

// GUI 控件
#include "../widgets/DROWidget.h"
#include "../widgets/GCodeEditor.h"
#include "../widgets/JogPanel.h"
#include "../widgets/StatusBar.h"

// 预览模块（由 lqt_gui_preview 提供，前向声明使用）
// #include "ToolpathView.h"

// HAL 监视器（由 lqt_gui_halmon 提供）
// #include "HalMonitorWidget.h"

// 示波器（由 lqt_gui_scope 提供）
// #include "ScopeWidget.h"

#include <QKeyEvent>
#include <QCloseEvent>
#include <QFileDialog>
#include <QApplication>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

// ============================================================================
// 工业风格全局颜色常量
// ============================================================================
namespace ThemeColors {
    static const QColor BG_MAIN       = QColor(25, 27, 32);      // 主背景
    static const QColor BG_DARK       = QColor(20, 22, 28);      // 深色背景
    static const QColor BG_PANEL      = QColor(30, 32, 38);      // 面板背景
    static const QColor BG_DOCK        = QColor(28, 30, 36);      // Dock 背景
    static const QColor BG_TOOLBAR     = QColor(30, 32, 38);      // 工具栏背景
    static const QColor BG_MENUBAR     = QColor(28, 30, 36);      // 菜单栏背景
    static const QColor BORDER        = QColor(50, 55, 65);       // 边框
    static const QColor TEXT_NORMAL    = QColor(200, 210, 220);    // 正常文字
    static const QColor TEXT_DIM       = QColor(120, 130, 140);    // 暗淡文字
    static const QColor ACCENT_BLUE    = QColor(0, 128, 208);      // 强调蓝色
    static const QColor ACCENT_GREEN  = QColor(0, 200, 100);      // 强调绿色
    static const QColor ACCENT_RED     = QColor(255, 50, 50);       // 强调红色
    static const QColor ACCENT_YELLOW  = QColor(255, 200, 0);       // 强调黄色
}

// ============================================================================
// MainWindow 构造/析构
// ============================================================================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_core(nullptr)
    , m_droWidget(nullptr)
    , m_gcodeEditor(nullptr)
    , m_jogPanel(nullptr)
    , m_statusBar(nullptr)
    , m_toolpathView(nullptr)
    , m_gcodeDock(nullptr)
    , m_droDock(nullptr)
    , m_jogDock(nullptr)
    , m_mainSplitter(nullptr)
    , m_statusTimer(nullptr)
{
    // 设置窗口属性
    setWindowTitle(tr("LinuxQtCNC - CNC 控制器"));
    setMinimumSize(1200, 800);
    resize(1400, 900);

    // 初始化各部分
    setupCentralWidget();
    setupDockWidgets();
    setupMenus();
    setupToolbars();
    setupStatusBar();
    setupConnections();
    setupStatusTimer();
    applyIndustrialStyle();
}

MainWindow::~MainWindow()
{
    if (m_statusTimer) {
        m_statusTimer->stop();
        delete m_statusTimer;
    }
}

void MainWindow::setCore(LcncCore *core)
{
    m_core = core;

    if (m_core) {
        // 连接 LcncCore 的信号到各控件
        connect(m_core, &LcncCore::statusUpdated,
                m_droWidget, &DROWidget::onStatusUpdate);
        connect(m_core, &LcncCore::statusUpdated,
                m_statusBar, &StatusBar::onStatusUpdate);

        // 通过 statusUpdated 信号跟踪执行行
        connect(m_core, &LcncCore::statusUpdated,
                this, [this](const LcncStatusData &data) {
                    if (data.currentLine > 0) {
                        m_gcodeEditor->highlightLine(data.currentLine);
                    }
                });

        // 启动状态刷新
        m_statusTimer->start(100);
    }
}

// ============================================================================
// 菜单栏设置
// ============================================================================
void MainWindow::setupMenus()
{
    // ---- File 菜单 ----
    m_fileMenu = menuBar()->addMenu(tr("文件(&F)"));

    m_openFileAction = m_fileMenu->addAction(tr("打开文件(&O)..."));
    m_openFileAction->setShortcut(QKeySequence::Open);
    m_openFileAction->setStatusTip(tr("打开 G 代码文件 (.ngc)"));

    m_fileMenu->addSeparator();

    m_exitAction = m_fileMenu->addAction(tr("退出(&Q)"));
    m_exitAction->setShortcut(QKeySequence::Quit);
    m_exitAction->setStatusTip(tr("退出程序"));

    // ---- View 菜单 ----
    m_viewMenu = menuBar()->addMenu(tr("视图(&V)"));

    m_toggleGCodeAction = m_viewMenu->addAction(tr("G 代码编辑器"));
    m_toggleGCodeAction->setCheckable(true);
    m_toggleGCodeAction->setChecked(true);

    m_toggleDROAction = m_viewMenu->addAction(tr("数字读数 (DRO)"));
    m_toggleDROAction->setCheckable(true);
    m_toggleDROAction->setChecked(true);

    m_toggleJogAction = m_viewMenu->addAction(tr("点动面板"));
    m_toggleJogAction->setCheckable(true);
    m_toggleJogAction->setChecked(true);

    // ---- Machine 菜单 ----
    m_machineMenu = menuBar()->addMenu(tr("机器(&M)"));

    m_connectAction = m_machineMenu->addAction(tr("连接(&C)"));
    m_connectAction->setStatusTip(tr("连接到 LinuxCNC 后端"));

    m_disconnectAction = m_machineMenu->addAction(tr("断开连接(&D)"));
    m_disconnectAction->setStatusTip(tr("断开与 LinuxCNC 后端的连接"));

    m_machineMenu->addSeparator();

    m_estopAction = m_machineMenu->addAction(tr("急停(&E)"));
    m_estopAction->setShortcut(Qt::Key_F1);
    m_estopAction->setStatusTip(tr("紧急停止 (F1)"));

    m_machineOnAction = m_machineMenu->addAction(tr("机器开(&N)"));
    m_machineOnAction->setShortcut(Qt::Key_F2);
    m_machineOnAction->setStatusTip(tr("开启机器 (F2)"));

    m_machineMenu->addSeparator();

    // 模式切换组
    m_modeActionGroup = new QActionGroup(this);
    m_modeActionGroup->setExclusive(true);

    m_modeAutoAction = m_machineMenu->addAction(tr("AUTO 模式"));
    m_modeAutoAction->setCheckable(true);
    m_modeAutoAction->setChecked(true);
    m_modeActionGroup->addAction(m_modeAutoAction);

    m_modeManualAction = m_machineMenu->addAction(tr("MANUAL 模式"));
    m_modeManualAction->setCheckable(true);
    m_modeActionGroup->addAction(m_modeManualAction);

    m_modeMDIAction = m_machineMenu->addAction(tr("MDI 模式"));
    m_modeMDIAction->setCheckable(true);
    m_modeActionGroup->addAction(m_modeMDIAction);

    // ---- Tools 菜单 ----
    m_toolsMenu = menuBar()->addMenu(tr("工具(&T)"));

    m_halMonitorAction = m_toolsMenu->addAction(tr("HAL 监视器"));
    m_halMonitorAction->setStatusTip(tr("打开 HAL 引脚/信号监视器"));

    m_scopeAction = m_toolsMenu->addAction(tr("示波器"));
    m_scopeAction->setStatusTip(tr("打开 HAL 示波器"));

    // ---- Help 菜单 ----
    m_helpMenu = menuBar()->addMenu(tr("帮助(&H)"));

    m_aboutAction = m_helpMenu->addAction(tr("关于(&A)..."));
    m_aboutAction->setStatusTip(tr("关于 LinuxQtCNC"));
}

// ============================================================================
// 工具栏设置
// ============================================================================
void MainWindow::setupToolbars()
{
    // ---- 机器控制工具栏 ----
    m_machineToolbar = addToolBar(tr("机器控制"));
    m_machineToolbar->setMovable(false);
    m_machineToolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    // 急停按钮 - 红色醒目
    m_estopAction->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
    m_machineToolbar->addAction(m_estopAction);

    m_machineToolbar->addSeparator();

    // 机器开按钮
    m_machineOnAction->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
    m_machineToolbar->addAction(m_machineOnAction);

    m_machineToolbar->addSeparator();

    // 模式切换按钮
    m_machineToolbar->addAction(m_modeAutoAction);
    m_machineToolbar->addAction(m_modeManualAction);
    m_machineToolbar->addAction(m_modeMDIAction);

    // ---- 程序控制工具栏 ----
    m_programToolbar = addToolBar(tr("程序控制"));
    m_programToolbar->setMovable(false);
    m_programToolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    m_progStartAction = m_programToolbar->addAction(tr("启动"));
    m_progStartAction->setStatusTip(tr("启动程序运行"));
    m_progStartAction->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));

    m_progPauseAction = m_programToolbar->addAction(tr("暂停"));
    m_progPauseAction->setStatusTip(tr("暂停/继续程序 (Space)"));
    m_progPauseAction->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    m_progPauseAction->setShortcut(Qt::Key_Space);

    m_progStopAction = m_programToolbar->addAction(tr("停止"));
    m_progStopAction->setStatusTip(tr("停止程序运行"));
    m_progStopAction->setIcon(style()->standardIcon(QStyle::SP_MediaStop));

    m_progStepAction = m_programToolbar->addAction(tr("单步"));
    m_progStepAction->setStatusTip(tr("单步执行"));
    m_progStepAction->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
}

// ============================================================================
// DockWidget 设置
// ============================================================================
void MainWindow::setupDockWidgets()
{
    // ---- 左侧：G 代码编辑器 ----
    m_gcodeDock = new QDockWidget(tr("G 代码编辑器"), this);
    m_gcodeDock->setFeatures(QDockWidget::DockWidgetMovable |
                             QDockWidget::DockWidgetFloatable |
                             QDockWidget::DockWidgetClosable);
    m_gcodeDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_gcodeEditor = new GCodeEditor(this);
    m_gcodeDock->setWidget(m_gcodeEditor);
    addDockWidget(Qt::LeftDockWidgetArea, m_gcodeDock);

    // ---- 右侧上：DRO ----
    m_droDock = new QDockWidget(tr("数字读数 (DRO)"), this);
    m_droDock->setFeatures(QDockWidget::DockWidgetMovable |
                            QDockWidget::DockWidgetFloatable |
                            QDockWidget::DockWidgetClosable);
    m_droDock->setAllowedAreas(Qt::LeftDockWidgetArea |
                               Qt::RightDockWidgetArea |
                               Qt::BottomDockWidgetArea);
    m_droWidget = new DROWidget(this);
    m_droDock->setWidget(m_droWidget);
    addDockWidget(Qt::RightDockWidgetArea, m_droDock);

    // ---- 右侧下：JogPanel ----
    m_jogDock = new QDockWidget(tr("点动面板"), this);
    m_jogDock->setFeatures(QDockWidget::DockWidgetMovable |
                            QDockWidget::DockWidgetFloatable |
                            QDockWidget::DockWidgetClosable);
    m_jogDock->setAllowedAreas(Qt::LeftDockWidgetArea |
                               Qt::RightDockWidgetArea |
                               Qt::BottomDockWidgetArea);
    m_jogPanel = new JogPanel(this);
    m_jogDock->setWidget(m_jogPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_jogDock);

    // 将 DRO 和 JogPanel 垂直排列在右侧
    tabifyDockWidget(m_droDock, m_jogDock);
    // 不使用 tabify，改为垂直分割
    splitDockWidget(m_droDock, m_jogDock, Qt::Vertical);
}

// ============================================================================
// 中央控件设置
// ============================================================================
void MainWindow::setupCentralWidget()
{
    // 中央区域使用 QSplitter
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);

    // 3D 刀具路径预览（占位 - 实际由 lqt_gui_preview 提供）
    // 当 ToolpathView 可用时替换为真实组件
    auto *previewPlaceholder = new QWidget();
    previewPlaceholder->setMinimumSize(400, 300);
    auto *previewLayout = new QVBoxLayout(previewPlaceholder);
    auto *previewLabel = new QLabel(tr("3D 刀具路径预览\n(ToolpathView)"));
    previewLabel->setAlignment(Qt::AlignCenter);
    previewLabel->setStyleSheet(
        "color: #505868;"
        "font-size: 18px;"
        "font-weight: bold;"
    );
    previewLayout->addWidget(previewLabel);
    previewPlaceholder->setStyleSheet(
        "background-color: #1A1C22;"
        "border: 2px dashed #3C414B;"
    );

    m_mainSplitter->addWidget(previewPlaceholder);
    setCentralWidget(m_mainSplitter);

    // 设置分割器初始比例
    m_mainSplitter->setStretchFactor(0, 1);
}

// ============================================================================
// 状态栏设置
// ============================================================================
void MainWindow::setupStatusBar()
{
    m_statusBar = new StatusBar(this);
    setStatusBar(m_statusBar);
}

// ============================================================================
// 状态刷新定时器
// ============================================================================
void MainWindow::setupStatusTimer()
{
    m_statusTimer = new QTimer(this);
    m_statusTimer->setInterval(100);  // 100ms 刷新周期

    connect(m_statusTimer, &QTimer::timeout,
            this, &MainWindow::onRefreshStatus);
}

// ============================================================================
// 信号连接
// ============================================================================
void MainWindow::setupConnections()
{
    // ---- 菜单动作 ----
    connect(m_openFileAction, &QAction::triggered,
            this, &MainWindow::onOpenFile);
    connect(m_exitAction, &QAction::triggered,
            this, &MainWindow::onExit);
    connect(m_connectAction, &QAction::triggered,
            this, &MainWindow::onConnect);
    connect(m_disconnectAction, &QAction::triggered,
            this, &MainWindow::onDisconnect);
    connect(m_estopAction, &QAction::triggered,
            this, &MainWindow::onEstop);
    connect(m_machineOnAction, &QAction::triggered,
            this, &MainWindow::onMachineOn);
    connect(m_modeAutoAction, &QAction::triggered,
            this, &MainWindow::onModeAuto);
    connect(m_modeManualAction, &QAction::triggered,
            this, &MainWindow::onModeManual);
    connect(m_modeMDIAction, &QAction::triggered,
            this, &MainWindow::onModeMDI);
    connect(m_aboutAction, &QAction::triggered,
            this, &MainWindow::onAbout);

    // ---- 视图切换 ----
    connect(m_toggleGCodeAction, &QAction::toggled,
            this, &MainWindow::onToggleGCodeEditor);
    connect(m_toggleDROAction, &QAction::toggled,
            this, &MainWindow::onToggleDRO);
    connect(m_toggleJogAction, &QAction::toggled,
            this, &MainWindow::onToggleJogPanel);

    // ---- 程序控制 ----
    connect(m_progStartAction, &QAction::triggered,
            this, &MainWindow::onProgramStart);
    connect(m_progPauseAction, &QAction::triggered,
            this, &MainWindow::onProgramPause);
    connect(m_progStopAction, &QAction::triggered,
            this, &MainWindow::onProgramStop);
    connect(m_progStepAction, &QAction::triggered,
            this, &MainWindow::onProgramStep);

    // ---- 工具 ----
    connect(m_halMonitorAction, &QAction::triggered,
            this, &MainWindow::onOpenHalMonitor);
    connect(m_scopeAction, &QAction::triggered,
            this, &MainWindow::onOpenScope);

    // ---- JogPanel 信号转发到 LcncCore ----
    connect(m_jogPanel, &JogPanel::jogAxis,
            this, &MainWindow::onJogAxis);
    connect(m_jogPanel, &JogPanel::jogStop,
            this, &MainWindow::onJogStop);
    connect(m_jogPanel, &JogPanel::homeAxis,
            this, &MainWindow::onHomeAxis);
    connect(m_jogPanel, &JogPanel::spindleStart,
            this, &MainWindow::onSpindleStart);
    connect(m_jogPanel, &JogPanel::spindleStop,
            this, &MainWindow::onSpindleStop);

    // ---- DockWidget 可见性同步到菜单 ----
    connect(m_gcodeDock, &QDockWidget::visibilityChanged,
            m_toggleGCodeAction, &QAction::setChecked);
    connect(m_droDock, &QDockWidget::visibilityChanged,
            m_toggleDROAction, &QAction::setChecked);
    connect(m_jogDock, &QDockWidget::visibilityChanged,
            m_toggleJogAction, &QAction::setChecked);
}

// ============================================================================
// 槽函数实现 - 文件操作
// ============================================================================
void MainWindow::onOpenFile()
{
    QString path = QFileDialog::getOpenFileName(
        this,
        tr("打开 G 代码文件"),
        QString(),
        tr("G 代码文件 (*.ngc *.nc *.gcode *.tap);;所有文件 (*)")
    );

    if (!path.isEmpty()) {
        m_gcodeEditor->openFile(path);
        setWindowTitle(tr("LinuxQtCNC - %1").arg(path));

        // 通知 LcncCore 加载文件
        if (m_core) {
            // m_core->loadProgram(path);
        }
    }
}

void MainWindow::onExit()
{
    close();
}

// ============================================================================
// 槽函数实现 - 视图切换
// ============================================================================
void MainWindow::onToggleGCodeEditor()
{
    m_gcodeDock->setVisible(m_toggleGCodeAction->isChecked());
}

void MainWindow::onToggleDRO()
{
    m_droDock->setVisible(m_toggleDROAction->isChecked());
}

void MainWindow::onToggleJogPanel()
{
    m_jogDock->setVisible(m_toggleJogAction->isChecked());
}

// ============================================================================
// 槽函数实现 - 机器控制
// ============================================================================
void MainWindow::onConnect()
{
    if (m_core) {
        // m_core->connect();
        m_statusBar->showMessage(tr("正在连接到 LinuxCNC 后端..."), 3000);
    } else {
        m_statusBar->showMessage(tr("警告: LcncCore 未初始化"), 5000);
    }
}

void MainWindow::onDisconnect()
{
    if (m_core) {
        // m_core->disconnect();
        m_statusBar->showMessage(tr("已断开连接"), 3000);
    }
}

void MainWindow::onEstop()
{
    if (m_core) {
        // m_core->estop();
        m_statusBar->showMessage(tr("!!! 急停已触发 !!!"), 0);
    }
}

void MainWindow::onMachineOn()
{
    if (m_core) {
        // m_core->machineOn();
        m_statusBar->showMessage(tr("机器已开启"), 2000);
    }
}

void MainWindow::onModeAuto()
{
    if (m_core) {
        // m_core->setMode(LcncCore::MODE_AUTO);
    }
}

void MainWindow::onModeManual()
{
    if (m_core) {
        // m_core->setMode(LcncCore::MODE_MANUAL);
    }
}

void MainWindow::onModeMDI()
{
    if (m_core) {
        // m_core->setMode(LcncCore::MODE_MDI);
    }
}

// ============================================================================
// 槽函数实现 - 程序控制
// ============================================================================
void MainWindow::onProgramStart()
{
    if (m_core) {
        // m_core->programStart();
        m_statusBar->showMessage(tr("程序开始运行"), 2000);
    }
}

void MainWindow::onProgramPause()
{
    if (m_core) {
        // m_core->programPause();
    }
}

void MainWindow::onProgramStop()
{
    if (m_core) {
        // m_core->programStop();
        m_statusBar->showMessage(tr("程序已停止"), 2000);
    }
}

void MainWindow::onProgramStep()
{
    if (m_core) {
        // m_core->programStep();
    }
}

// ============================================================================
// 槽函数实现 - 工具
// ============================================================================
void MainWindow::onOpenHalMonitor()
{
    // TODO: 创建并显示 HalMonitorWidget 窗口
    m_statusBar->showMessage(tr("HAL 监视器 - 功能开发中"), 3000);
}

void MainWindow::onOpenScope()
{
    // TODO: 创建并显示 ScopeWidget 窗口
    m_statusBar->showMessage(tr("示波器 - 功能开发中"), 3000);
}

// ============================================================================
// 槽函数实现 - 帮助
// ============================================================================
void MainWindow::onAbout()
{
    QMessageBox::about(this, tr("关于 LinuxQtCNC"),
        tr("<h2>LinuxQtCNC</h2>"
           "<p>版本 0.1.0</p>"
           "<p>基于 Qt6 的 CNC 控制器图形界面</p>"
           "<p>LinuxCNC 的现代化重构版本</p>"
           "<p><b>功能特点:</b></p>"
           "<ul>"
           "<li>深色工业风格界面</li>"
           "<li>实时坐标显示 (DRO)</li>"
           "<li>G 代码编辑与语法高亮</li>"
           "<li>3D 刀具路径预览</li>"
           "<li>手轮/点动控制面板</li>"
           "<li>HAL 监视与示波器</li>"
           "</ul>"));
}

// ============================================================================
// 槽函数实现 - JogPanel 信号转发
// ============================================================================
void MainWindow::onJogAxis(int axis, int direction)
{
    if (m_core) {
        // m_core->jogAxis(axis, direction);
    }
}

void MainWindow::onJogStop()
{
    if (m_core) {
        // m_core->jogStop();
    }
}

void MainWindow::onHomeAxis(int axis)
{
    if (m_core) {
        // m_core->homeAxis(axis);
    }
}

void MainWindow::onSpindleStart(bool clockwise)
{
    if (m_core) {
        // m_core->spindleStart(clockwise);
    }
}

void MainWindow::onSpindleStop()
{
    if (m_core) {
        // m_core->spindleStop();
    }
}

// ============================================================================
// 状态刷新（100ms 定时器驱动）
// ============================================================================
void MainWindow::onRefreshStatus()
{
    if (m_core) {
        // m_core->requestStatusUpdate();
        // 状态更新通过 LcncCore 的 statusUpdated 信号异步返回
        // 已在 setCore() 中连接到各控件
    }
}

// ============================================================================
// 键盘快捷键
// ============================================================================
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_F1:
        // 急停
        onEstop();
        event->accept();
        break;

    case Qt::Key_F2:
        // 机器开
        onMachineOn();
        event->accept();
        break;

    case Qt::Key_Space:
        // 暂停/继续
        onProgramPause();
        event->accept();
        break;

    default:
        QMainWindow::keyPressEvent(event);
        break;
    }
}

// ============================================================================
// 关闭事件
// ============================================================================
void MainWindow::closeEvent(QCloseEvent *event)
{
    // 停止状态刷新定时器
    if (m_statusTimer) {
        m_statusTimer->stop();
    }

    // 断开连接
    if (m_core) {
        // m_core->disconnect();
    }

    event->accept();
}

// ============================================================================
// 深色工业风格主题
// ============================================================================
void MainWindow::applyDarkTheme()
{
    applyIndustrialStyle();
}

void MainWindow::applyIndustrialStyle()
{
    // ---- 全局样式表 ----
    setStyleSheet(
        // 主窗口
        "QMainWindow {"
        "  background-color: #191B20;"
        "}"

        // 菜单栏
        "QMenuBar {"
        "  background-color: #1C1E24;"
        "  color: #B4C8DC;"
        "  border-bottom: 1px solid #323640;"
        "  padding: 2px;"
        "}"
        "QMenuBar::item {"
        "  background: transparent;"
        "  padding: 4px 12px;"
        "  border-radius: 3px;"
        "}"
        "QMenuBar::item:selected {"
        "  background-color: #2A3040;"
        "}"
        "QMenuBar::item:pressed {"
        "  background-color: #0064A0;"
        "}"

        // 下拉菜单
        "QMenu {"
        "  background-color: #1E2028;"
        "  color: #C8D2DC;"
        "  border: 1px solid #3C414B;"
        "  border-radius: 4px;"
        "  padding: 4px;"
        "}"
        "QMenu::item {"
        "  padding: 6px 30px 6px 20px;"
        "  border-radius: 3px;"
        "}"
        "QMenu::item:selected {"
        "  background-color: #0064A0;"
        "}"
        "QMenu::separator {"
        "  height: 1px;"
        "  background: #3C414B;"
        "  margin: 4px 8px;"
        "}"

        // 工具栏
        "QToolBar {"
        "  background-color: #1C1E24;"
        "  border-bottom: 1px solid #323640;"
        "  padding: 2px;"
        "  spacing: 4px;"
        "}"
        "QToolBar::separator {"
        "  width: 1px;"
        "  background-color: #3C414B;"
        "  margin: 4px 2px;"
        "}"

        // 工具栏按钮
        "QToolButton {"
        "  background-color: #323640;"
        "  color: #C8D2DC;"
        "  border: 1px solid #4A5060;"
        "  border-radius: 3px;"
        "  padding: 4px 8px;"
        "  font-size: 11px;"
        "}"
        "QToolButton:hover {"
        "  background-color: #41465A;"
        "}"
        "QToolButton:pressed {"
        "  background-color: #0064A0;"
        "}"

        // DockWidget
        "QDockWidget {"
        "  color: #B4C8DC;"
        "  font-size: 11px;"
        "}"
        "QDockWidget::title {"
        "  background-color: #1C1E24;"
        "  padding: 6px;"
        "  border-bottom: 1px solid #323640;"
        "}"
        "QDockWidget::close-button, QDockWidget::float-button {"
        "  background: transparent;"
        "  border: none;"
        "  padding: 2px;"
        "}"
        "QDockWidget::close-button:hover, QDockWidget::float-button:hover {"
        "  background-color: #41465A;"
        "  border-radius: 2px;"
        "}"

        // 分割器
        "QSplitter::handle {"
        "  background-color: #323640;"
        "  width: 3px;"
        "}"
        "QSplitter::handle:hover {"
        "  background-color: #0064A0;"
        "}"

        // 滚动条
        "QScrollBar:vertical {"
        "  background: #1A1C22;"
        "  width: 10px;"
        "  border: none;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #4A5060;"
        "  border-radius: 5px;"
        "  min-height: 20px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: #5A6070;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0;"
        "}"
        "QScrollBar:horizontal {"
        "  background: #1A1C22;"
        "  height: 10px;"
        "  border: none;"
        "}"
        "QScrollBar::handle:horizontal {"
        "  background: #4A5060;"
        "  border-radius: 5px;"
        "  min-width: 20px;"
        "}"
        "QScrollBar::handle:horizontal:hover {"
        "  background: #5A6070;"
        "}"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
        "  width: 0;"
        "}"

        // 工具提示
        "QToolTip {"
        "  background-color: #2A2E38;"
        "  color: #C8D2DC;"
        "  border: 1px solid #4A5060;"
        "  border-radius: 3px;"
        "  padding: 4px;"
        "}"

        // 消息框
        "QMessageBox {"
        "  background-color: #1E2028;"
        "}"
        "QMessageBox QLabel {"
        "  color: #C8D2DC;"
        "}"
        "QMessageBox QPushButton {"
        "  background-color: #323640;"
        "  color: #C8D2DC;"
        "  border: 1px solid #4A5060;"
        "  border-radius: 3px;"
        "  padding: 6px 16px;"
        "  min-width: 80px;"
        "}"
        "QMessageBox QPushButton:hover {"
        "  background-color: #41465A;"
        "}"
    );
}
