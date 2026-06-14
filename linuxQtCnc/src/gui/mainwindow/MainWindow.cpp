/*
 * linuxQtCncGui - LinuxCNC Qt5 GUI 控制器
 * MainWindow - 主窗口
 *
 * CNC 控制器的主界面窗口，采用纯代码布局（无 .ui 文件）。
 * 布局结构：
 *   顶部：菜单栏 + 工具栏
 *   中央：主分割器（水平）
 *     左侧（60%）：ToolpathView 刀具路径预览（3D OpenGL）
 *     右侧（40%）：垂直分割器
 *       右上：DRO 数字读数 + StatusBar
 *       右下：GCodeEditor（G代码编辑器）
 *   底部：JogPanel 点动面板
 *
 * LcncCore 单例通过构造函数注入，所有组件通过信号/槽自动响应状态变化。
 */
#include "MainWindow.h"

#include "LcncCore.h"
#include "LcncStatusData.h"

#include "widgets/DROWidget.h"
#include "widgets/GCodeEditor.h"
#include "widgets/JogPanel.h"
#include "widgets/StatusBar.h"
#include "preview/ToolpathView.h"

#include <QAction>
#include <QMenuBar>
#include <QMenu>
#include <QToolBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QDockWidget>
#include <QTextStream>
#include <QFile>
#include <QDateTime>
#include <QCloseEvent>
#include <QApplication>

// ============================================================================
// 构造 / 析构
// ============================================================================

MainWindow::MainWindow(LcncCore *core, QWidget *parent)
    : QMainWindow(parent)
    , m_core(core)
{
    // 窗口基本属性
    setWindowTitle("linuxQtCnc - CNC Controller");
    resize(1400, 900);
    setMinimumSize(1024, 768);

    // 设置布局
    setupUi();
    setupMenus();
    setupToolBars();
    setupConnections();

    // 初始化默认状态
    onStatusUpdated();
    onDisconnected();

    qInfo().noquote() << "[MainWindow] 主窗口初始化完成";
}

MainWindow::~MainWindow() = default;

// ============================================================================
// UI 初始化
// ============================================================================

void MainWindow::setupUi()
{
    // ---- 中央组件 ----
    // DRO 显示
    m_droWidget = new DROWidget(this);
    m_droWidget->setAxisCount(6); // 默认 6 轴

    // 3D 刀具路径视图（真实 OpenGL 组件）
    m_toolpathView = new ToolpathView(this);
    m_toolpathView->setMinimumSize(600, 500);

    // G代码编辑器
    m_gcodeEditor = new GCodeEditor(this);
    m_gcodeEditor->setMinimumHeight(200);

    // 点动面板
    m_jogPanel = new JogPanel(this);
    m_jogPanel->setAxisCount(6);
    m_jogPanel->setMinimumHeight(120);

    // 状态栏
    m_statusBarWidget = new StatusBarWidget(this);

    // ---- 布局 ----
    // 右侧垂直分割：上方 DRO，下方 G代码编辑器
    m_rightSplitter = new QSplitter(Qt::Vertical);
    m_rightSplitter->addWidget(m_droWidget);
    m_rightSplitter->addWidget(m_gcodeEditor);
    m_rightSplitter->setStretchFactor(0, 0); // DRO 不拉伸
    m_rightSplitter->setStretchFactor(1, 1); // G代码编辑器占满剩余空间
    m_rightSplitter->setSizes({250, 400});

    // 主水平分割：左侧刀具路径，右侧
    m_mainSplitter = new QSplitter(Qt::Horizontal);
    m_mainSplitter->addWidget(m_toolpathView);
    m_mainSplitter->addWidget(m_rightSplitter);
    m_mainSplitter->setStretchFactor(0, 3); // 刀具路径占 3/4
    m_mainSplitter->setStretchFactor(1, 2); // 右侧占 1/4
    m_mainSplitter->setSizes({700, 400});

    setCentralWidget(m_mainSplitter);

    // ---- 底部点动面板 ----
    QDockWidget *jogDock = new QDockWidget(tr("Jog"), this);
    jogDock->setWidget(m_jogPanel);
    jogDock->setMaximumHeight(160);
    jogDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::BottomDockWidgetArea, jogDock);

    // ---- 状态栏 ----
    statusBar()->addPermanentWidget(m_statusBarWidget, 1);
    setStatusBar(statusBar());
}

void MainWindow::setupMenus()
{
    // ---- 文件菜单 ----
    m_fileMenu = menuBar()->addMenu(tr("文件(&F)"));

    m_programOpenAction = new QAction(QIcon::fromTheme("document-open"), tr("打开 G代码..."), this);
    m_programOpenAction->setShortcut(QKeySequence::Open);
    m_programOpenAction->setStatusTip(tr("打开 G代码文件"));
    connect(m_programOpenAction, &QAction::triggered, this, &MainWindow::onOpenProgram);
    m_fileMenu->addAction(m_programOpenAction);

    m_fileMenu->addSeparator();

    QAction *exitAction = new QAction(tr("退出"), this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
    m_fileMenu->addAction(exitAction);

    // ---- 机床菜单 ----
    m_machineMenu = menuBar()->addMenu(tr("机床(&M)"));

    m_estopAction = new QAction(QIcon::fromTheme("process-stop"), tr("急停"), this);
    m_estopAction->setShortcut(Qt::Key_F1);
    m_estopAction->setStatusTip(tr("触发急停"));
    connect(m_estopAction, &QAction::triggered, this, &MainWindow::onEstop);
    m_machineMenu->addAction(m_estopAction);

    m_machineOnAction = new QAction(tr("机床开"), this);
    m_machineOnAction->setShortcut(Qt::Key_F2);
    m_machineOnAction->setStatusTip(tr("开启机床"));
    connect(m_machineOnAction, &QAction::triggered, this, &MainWindow::onMachineOn);
    m_machineMenu->addAction(m_machineOnAction);

    m_machineOffAction = new QAction(tr("机床关"), this);
    m_machineOffAction->setStatusTip(tr("关闭机床"));
    connect(m_machineOffAction, &QAction::triggered, this, &MainWindow::onMachineOff);
    m_machineMenu->addAction(m_machineOffAction);

    m_machineMenu->addSeparator();

    m_homeAllAction = new QAction(tr("全轴回零"), this);
    m_homeAllAction->setShortcut(Qt::Key_Home);
    m_homeAllAction->setStatusTip(tr("所有轴回参考点"));
    connect(m_homeAllAction, &QAction::triggered, this, &MainWindow::onHomeAll);
    m_machineMenu->addAction(m_homeAllAction);

    m_machineMenu->addSeparator();

    QAction *iniOpenAction = new QAction(tr("打开 INI 配置..."), this);
    connect(iniOpenAction, &QAction::triggered, this, &MainWindow::onOpenIni);
    m_machineMenu->addAction(iniOpenAction);

    // ---- 程序菜单 ----
    QMenu *progMenu = menuBar()->addMenu(tr("程序(&P)"));

    m_modeAutoAction = new QAction(tr("自动模式"), this);
    m_modeAutoAction->setCheckable(true);
    m_modeAutoAction->setShortcut(Qt::Key_Tab);
    connect(m_modeAutoAction, &QAction::triggered, this, &MainWindow::onModeAuto);
    progMenu->addAction(m_modeAutoAction);

    m_modeManualAction = new QAction(tr("手动模式"), this);
    m_modeManualAction->setCheckable(true);
    connect(m_modeManualAction, &QAction::triggered, this, &MainWindow::onModeManual);
    progMenu->addAction(m_modeManualAction);

    m_modeMdiAction = new QAction(tr("MDI 模式"), this);
    m_modeMdiAction->setCheckable(true);
    m_modeMdiAction->setShortcut(Qt::Key_F3);
    connect(m_modeMdiAction, &QAction::triggered, this, &MainWindow::onModeMdi);
    progMenu->addAction(m_modeMdiAction);

    progMenu->addSeparator();

    m_programRunAction = new QAction(QIcon::fromTheme("media-playback-start"), tr("运行"), this);
    m_programRunAction->setShortcut(Qt::Key_F5);
    connect(m_programRunAction, &QAction::triggered, this, &MainWindow::onProgramRun);
    progMenu->addAction(m_programRunAction);

    m_programPauseAction = new QAction(QIcon::fromTheme("media-playback-pause"), tr("暂停"), this);
    m_programPauseAction->setShortcut(Qt::Key_F6);
    connect(m_programPauseAction, &QAction::triggered, this, &MainWindow::onProgramPause);
    progMenu->addAction(m_programPauseAction);

    m_programStopAction = new QAction(QIcon::fromTheme("media-playback-stop"), tr("停止"), this);
    m_programStopAction->setShortcut(Qt::Key_F7);
    connect(m_programStopAction, &QAction::triggered, this, &MainWindow::onProgramStop);
    progMenu->addAction(m_programStopAction);

    // ---- 视图菜单 ----
    m_viewMenu = menuBar()->addMenu(tr("视图(&V)"));
    QAction *zoominAction = new QAction(tr("放大"), this);
    connect(zoominAction, &QAction::triggered, m_toolpathView, &ToolpathView::zoomIn);
    m_viewMenu->addAction(zoominAction);

    QAction *zoomoutAction = new QAction(tr("缩小"), this);
    connect(zoomoutAction, &QAction::triggered, m_toolpathView, &ToolpathView::zoomOut);
    m_viewMenu->addAction(zoomoutAction);

    QAction *resetViewAction = new QAction(tr("重置视图"), this);
    connect(resetViewAction, &QAction::triggered, m_toolpathView, &ToolpathView::resetView);
    m_viewMenu->addAction(resetViewAction);

    m_viewMenu->addSeparator();

    QAction *toggleGridAction = new QAction(tr("显示网格"), this);
    toggleGridAction->setCheckable(true);
    toggleGridAction->setChecked(true);
    connect(toggleGridAction, &QAction::toggled, m_toolpathView, &ToolpathView::setGridVisible);
    m_viewMenu->addAction(toggleGridAction);

    QAction *toggleLimitsAction = new QAction(tr("显示行程极限"), this);
    toggleLimitsAction->setCheckable(true);
    toggleLimitsAction->setChecked(true);
    connect(toggleLimitsAction, &QAction::toggled, m_toolpathView, &ToolpathView::setLimitsVisible);
    m_viewMenu->addAction(toggleLimitsAction);

    // ---- 帮助菜单 ----
    m_helpMenu = menuBar()->addMenu(tr("帮助(&H)"));
    QAction *aboutAction = new QAction(tr("关于 linuxQtCnc"), this);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
    m_helpMenu->addAction(aboutAction);
}

void MainWindow::setupToolBars()
{
    // ---- 机床控制工具栏 ----
    m_machineToolBar = addToolBar(tr("机床控制"));
    m_machineToolBar->setObjectName("MachineToolBar");
    m_machineToolBar->addAction(m_estopAction);
    m_machineToolBar->addSeparator();
    m_machineToolBar->addAction(m_machineOnAction);
    m_machineToolBar->addAction(m_machineOffAction);
    m_machineToolBar->addSeparator();
    m_machineToolBar->addAction(m_homeAllAction);

    // ---- 程序控制工具栏 ----
    m_programToolBar = addToolBar(tr("程序控制"));
    m_programToolBar->setObjectName("ProgramToolBar");
    m_programToolBar->addAction(m_programOpenAction);
    m_programToolBar->addSeparator();
    m_programToolBar->addAction(m_modeAutoAction);
    m_programToolBar->addAction(m_modeManualAction);
    m_programToolBar->addAction(m_modeMdiAction);
    m_programToolBar->addSeparator();
    m_programToolBar->addAction(m_programRunAction);
    m_programToolBar->addAction(m_programPauseAction);
    m_programToolBar->addAction(m_programStopAction);
}

void MainWindow::setupConnections()
{
    if (!m_core) return;

    // LcncCore 状态更新 -> 更新所有组件
    connect(m_core, &LcncCore::statusUpdated,
            this, &MainWindow::onStatusUpdated);
    connect(m_core, &LcncCore::connected,
            this, &MainWindow::onConnected);
    connect(m_core, &LcncCore::disconnected,
            this, &MainWindow::onDisconnected);
    connect(m_core, &LcncCore::errorOccurred,
            this, &MainWindow::onError);

    // G代码编辑器 -> 刀具路径视图
    connect(m_gcodeEditor, &GCodeEditor::fileLoaded,
            m_toolpathView, &ToolpathView::loadGCode);

    // G代码编辑器 -> 状态栏
    connect(m_gcodeEditor, &GCodeEditor::fileLoaded,
            this, [this](const QString &path) {
                statusBar()->showMessage(tr("已加载: %1").arg(path), 3000);
            });

    // 点动面板 -> LcncCore
    connect(m_jogPanel, &JogPanel::jogStopRequested,
            this, [this](int axis) {
                if (m_core) m_core->sendJogStop(axis);
            });
    connect(m_jogPanel, &JogPanel::jogRequested,
            this, [this](int axis, double velocity) {
                if (m_core) m_core->sendJog(axis, velocity);
            });

    // 回参考点请求 -> LcncCore
    connect(m_jogPanel, &JogPanel::homeRequested,
            this, [this](int axis) {
                if (m_core) m_core->sendHome(axis);
            });

    // 主轴控制 -> LcncCore
    connect(m_jogPanel, &JogPanel::spindleControl,
            this, [this](bool forward, bool stop) {
                if (!m_core) return;
                if (stop) {
                    m_core->sendSpindleStop();
                } else if (forward) {
                    m_core->sendSpindleForward(1000.0);  // 默认转速
                } else {
                    m_core->sendSpindleReverse(1000.0);  // 默认转速
                }
            });

    // 冷却液控制 -> LcncCore
    connect(m_jogPanel, &JogPanel::coolantFloodOn,
            this, [this]() {
                if (m_core) m_core->sendCoolantFloodOn();
            });
    connect(m_jogPanel, &JogPanel::coolantFloodOff,
            this, [this]() {
                if (m_core) m_core->sendCoolantFloodOff();
            });
    connect(m_jogPanel, &JogPanel::coolantMistOn,
            this, [this]() {
                if (m_core) m_core->sendCoolantMistOn();
            });
    connect(m_jogPanel, &JogPanel::coolantMistOff,
            this, [this]() {
                if (m_core) m_core->sendCoolantMistOff();
            });

    // DRO Widget -> 刀具路径视图（点击坐标跳转到位置）
    connect(m_droWidget, &DROWidget::positionClicked,
            this, [this](int axis, double position) {
                Q_UNUSED(axis)
                Q_UNUSED(position)
                // 点击 DRO 时聚焦到当前位置
                m_toolpathView->fitToView();
            });
}

// ============================================================================
// 公共方法
// ============================================================================

void MainWindow::startSimulation()
{
    if (!m_core) return;

    // 模拟模式：创建模拟 INI 并连接到仿真
    const QString simIni = ":/sim/lathe.ini";
    if (QFile::exists(simIni)) {
        connectToIni(simIni);
    } else {
        // 尝试从命令行参数查找
        QString iniPath = QCoreApplication::arguments().value(1);
        if (!iniPath.isEmpty() && QFile::exists(iniPath)) {
            connectToIni(iniPath);
        } else {
            QMessageBox::information(this, tr("模拟模式"),
                tr("未找到配置文件。请使用 --ini <path> 指定 INI 文件。\n"
                   "当前以演示模式启动。"));
            onConnected(); // 显示已连接状态
        }
    }
}

void MainWindow::connectToIni(const QString &iniFile)
{
    if (!m_core) return;

    statusBar()->showMessage(tr("正在连接: %1").arg(iniFile));
    bool ok = m_core->connectToServer(iniFile);
    if (!ok) {
        QMessageBox::warning(this, tr("连接失败"),
            tr("无法连接到 %1\n请检查 NML 服务器是否运行。").arg(iniFile));
    }
}

// ============================================================================
// 槽函数
// ============================================================================

void MainWindow::onStatusUpdated()
{
    if (!m_core) return;

    const LcncStatusData *st = m_core->statusData();
    if (!st) return;

    // 更新 DRO
    for (int i = 0; i < st->axisCount && i < MAX_DRO_AXES; ++i) {
        m_droWidget->setPosition(i, st->positions[i], st->dtg[i]);
        m_droWidget->setOffset(i, st->g5x_offset[i], st->g92_offset[i]);
        m_droWidget->setHomePosition(i, st->homePosition[i]);
        m_droWidget->setLimit(i, st->minLimit[i], st->maxLimit[i]);
        m_droWidget->setOverride(i, st->overridePercent[i]);
        m_droWidget->setHomed(i, st->homed[i]);
    }

    // 更新刀具路径视图中的当前位置
    m_toolpathView->updateToolPosition(QVector3D(
        static_cast<float>(st->positions[0]),
        static_cast<float>(st->positions[1]),
        static_cast<float>(st->positions[2])));

    // 更新状态栏
    m_statusBarWidget->updateFromStatus(st);

    // 更新模式
    m_modeAutoAction->setChecked(st->motionMode == MotionModeAuto);
    m_modeManualAction->setChecked(st->motionMode == MotionModeManual);
    m_modeMdiAction->setChecked(st->motionMode == MotionModeMdi);

    // 更新程序状态
    m_programRunAction->setEnabled(st->taskState == TaskStateOn
                                   && st->interpState == InterpIdle);
    m_programPauseAction->setEnabled(st->interpState == InterpRunning);
    m_programStopAction->setEnabled(st->interpState != InterpIdle);

    // 更新机床状态
    bool machineOn = st->taskState == TaskStateOn;
    m_machineOnAction->setEnabled(!machineOn);
    m_machineOffAction->setEnabled(machineOn);

    // 急停状态
    if (st->taskState == TaskStateEstop) {
        m_estopAction->setEnabled(false); // 急停中不可再次触发
    } else {
        m_estopAction->setEnabled(true);
    }
}

void MainWindow::onConnected()
{
    qInfo() << "[MainWindow] 已连接到 LinuxCNC";
    statusBar()->showMessage(tr("已连接到 LinuxCNC"), 3000);

    // 使能机床控制动作
    m_machineOnAction->setEnabled(true);
    m_machineOffAction->setEnabled(false);
    m_homeAllAction->setEnabled(true);
    m_modeAutoAction->setEnabled(true);
    m_modeManualAction->setEnabled(true);
    m_modeMdiAction->setEnabled(true);
}

void MainWindow::onDisconnected()
{
    qInfo() << "[MainWindow] 与 LinuxCNC 断开连接";
    statusBar()->showMessage(tr("未连接到 LinuxCNC"), 5000);

    // 禁用所有需要连接的动作
    m_machineOnAction->setEnabled(false);
    m_machineOffAction->setEnabled(false);
    m_homeAllAction->setEnabled(false);
    m_modeAutoAction->setEnabled(false);
    m_modeManualAction->setEnabled(false);
    m_modeMdiAction->setEnabled(false);
    m_programRunAction->setEnabled(false);
    m_programPauseAction->setEnabled(false);
    m_programStopAction->setEnabled(false);

    // 重置 DRO 到 0
    for (int i = 0; i < MAX_DRO_AXES; ++i) {
        m_droWidget->setPosition(i, 0.0, 0.0);
        m_droWidget->setHomed(i, false);
    }
}

void MainWindow::onError(const QString &message)
{
    qWarning().noquote() << "[MainWindow] 错误:" << message;
    QMessageBox::critical(this, tr("错误"), message);
}

// ============================================================================
// 动作槽
// ============================================================================

void MainWindow::onEstop()
{
    if (!m_core) return;
    m_core->estop();
    statusBar()->showMessage(tr("急停触发！"), 5000);
}

void MainWindow::onMachineOn()
{
    if (!m_core) return;
    bool ok = m_core->machineOn();
    if (!ok) {
        QMessageBox::warning(this, tr("操作失败"), tr("无法开启机床"));
    } else {
        m_machineOnAction->setEnabled(false);
        m_machineOffAction->setEnabled(true);
        statusBar()->showMessage(tr("机床已开启"), 3000);
    }
}

void MainWindow::onMachineOff()
{
    if (!m_core) return;
    bool ok = m_core->machineOff();
    if (!ok) {
        QMessageBox::warning(this, tr("操作失败"), tr("无法关闭机床"));
    } else {
        m_machineOnAction->setEnabled(true);
        m_machineOffAction->setEnabled(false);
        statusBar()->showMessage(tr("机床已关闭"), 3000);
    }
}

void MainWindow::onHomeAll()
{
    if (!m_core) return;
    statusBar()->showMessage(tr("正在回零..."), 2000);
    for (int i = 0; i < 6; ++i) {
        m_core->homeAxis(i);
    }
}

void MainWindow::onModeAuto()
{
    if (!m_core) return;
    m_core->setMode(LcncCore::ModeAuto);
    m_modeAutoAction->setChecked(true);
    m_modeManualAction->setChecked(false);
    m_modeMdiAction->setChecked(false);
    statusBar()->showMessage(tr("切换到自动模式"), 2000);
}

void MainWindow::onModeManual()
{
    if (!m_core) return;
    m_core->setMode(LcncCore::ModeManual);
    m_modeAutoAction->setChecked(false);
    m_modeManualAction->setChecked(true);
    m_modeMdiAction->setChecked(false);
    statusBar()->showMessage(tr("切换到手动模式"), 2000);
}

void MainWindow::onModeMdi()
{
    if (!m_core) return;
    m_core->setMode(LcncCore::ModeMdi);
    m_modeAutoAction->setChecked(false);
    m_modeManualAction->setChecked(false);
    m_modeMdiAction->setChecked(true);
    statusBar()->showMessage(tr("切换到 MDI 模式"), 2000);
}

void MainWindow::onProgramRun()
{
    if (!m_core) return;
    if (m_gcodeEditor->program().isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("请先打开 G代码文件"));
        onOpenProgram();
        return;
    }
    bool ok = m_core->runProgram(m_gcodeEditor->program());
    if (!ok) {
        QMessageBox::warning(this, tr("运行失败"), tr("无法运行程序"));
    } else {
        statusBar()->showMessage(tr("程序运行中..."), 2000);
    }
}

void MainWindow::onProgramPause()
{
    if (!m_core) return;
    m_core->pauseProgram();
    statusBar()->showMessage(tr("程序已暂停"), 2000);
}

void MainWindow::onProgramStop()
{
    if (!m_core) return;
    m_core->stopProgram();
    statusBar()->showMessage(tr("程序已停止"), 2000);
}

void MainWindow::onOpenProgram()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("打开 G代码文件"),
        QString(),
        tr("G代码文件 (*.ngc *.gcode *.nc);;所有文件 (*.*)")
    );
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("打开失败"),
            tr("无法打开文件: %1").arg(filePath));
        return;
    }
    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    m_gcodeEditor->setProgram(content);
    m_gcodeEditor->setFilePath(filePath);

    // 加载到刀具路径预览
    m_toolpathView->loadGCode(filePath);

    statusBar()->showMessage(tr("已加载: %1").arg(filePath), 3000);
}

void MainWindow::onOpenIni()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("打开 INI 配置文件"),
        QString(),
        tr("INI 配置文件 (*.ini);;所有文件 (*.*)")
    );
    if (filePath.isEmpty()) return;
    connectToIni(filePath);
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, tr("关于 linuxQtCnc"),
        tr("<h3>linuxQtCnc</h3>"
           "<p>版本: %1</p>"
           "<p>基于 LinuxCNC 架构的跨平台 CNC 控制器 Qt5 GUI。"
           "支持 G代码编辑、3D 刀具路径预览、实时状态显示和点动控制。</p>"
           "<p>Windows + Linux 双平台支持。</p>")
           .arg(QCoreApplication::applicationVersion()));
}

// ============================================================================
// 事件处理
// ============================================================================

void MainWindow::closeEvent(QCloseEvent *event)
{
    // 断开连接
    if (m_core && m_core->isConnected()) {
        m_core->disconnectFromServer();
    }
    event->accept();
}
