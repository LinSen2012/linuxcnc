/*
 * linuxQtCncGui - LinuxCNC Qt6 GUI 控制器
 * LcncApplication 实现
 */

#include "LcncApplication.h"
#include "core/LcncCore.h"

#include <QDebug>

// ============================================================================
// 构造 / 析构
// ============================================================================

LcncApplication::LcncApplication(int &argc, char **argv)
    : QApplication(argc, argv)
    , m_core(LcncCore::instance())
{
    qInfo().noquote() << "[LcncApplication] 应用程序初始化完成";
}

LcncApplication::~LcncApplication()
{
    // LcncCore 是单例，由自身管理生命周期
    m_core = nullptr;
}

// ============================================================================
// 核心单例访问
// ============================================================================

LcncCore *LcncApplication::core() const
{
    return m_core;
}

// ============================================================================
// 主题样式
// ============================================================================

void LcncApplication::applyIndustrialTheme()
{
    setStyleSheet(industrialStyleSheet());
}

QString LcncApplication::industrialStyleSheet()
{
    // 深色工业风格主题 - 适合 CNC 控制器界面
    // 颜色方案：深蓝灰背景 + 绿色/青色强调 + 高对比度数字
    return QStringLiteral(
        /* ===== 全局基础样式 ===== */
        "QWidget {"
        "    background-color: #1a1a2e;"
        "    color: #e0e0e0;"
        "    font-family: 'Segoe UI', 'Microsoft YaHei', sans-serif;"
        "    font-size: 13px;"
        "}"

        /* ===== 主窗口 ===== */
        "QMainWindow {"
        "    background-color: #16213e;"
        "}"

        /* ===== 分组框 ===== */
        "QGroupBox {"
        "    border: 1px solid #0f3460;"
        "    border-radius: 4px;"
        "    margin-top: 12px;"
        "    padding-top: 12px;"
        "    font-weight: bold;"
        "    color: #00d2ff;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 5px;"
        "}"

        /* ===== 按钮 - 通用 ===== */
        "QPushButton {"
        "    background-color: #0f3460;"
        "    border: 1px solid #1a1a5e;"
        "    border-radius: 4px;"
        "    padding: 6px 16px;"
        "    color: #e0e0e0;"
        "    min-height: 24px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #1a4a8a;"
        "    border-color: #00d2ff;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #00d2ff;"
        "    color: #1a1a2e;"
        "}"

        /* ===== 紧急停止按钮 - 红色大按钮 ===== */
        "QPushButton[emergency=\"true\"] {"
        "    background-color: #cc0000;"
        "    border: 2px solid #ff3333;"
        "    border-radius: 8px;"
        "    padding: 10px 20px;"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "    color: #ffffff;"
        "    min-height: 40px;"
        "}"
        "QPushButton[emergency=\"true\"]:hover {"
        "    background-color: #ff0000;"
        "}"
        "QPushButton[emergency=\"true\"]:pressed {"
        "    background-color: #990000;"
        "}"

        /* ===== 机床开启按钮 - 绿色 ===== */
        "QPushButton[machineon=\"true\"] {"
        "    background-color: #006600;"
        "    border: 2px solid #00cc00;"
        "    border-radius: 8px;"
        "    padding: 10px 20px;"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "    color: #ffffff;"
        "    min-height: 40px;"
        "}"
        "QPushButton[machineon=\"true\"]:hover {"
        "    background-color: #00aa00;"
        "}"
        "QPushButton[machineon=\"true\"]:pressed {"
        "    background-color: #004400;"
        "}"

        /* ===== DRO 数字读数显示 ===== */
        "QLabel[dro=\"true\"] {"
        "    background-color: #0a0a1a;"
        "    border: 1px solid #00ff88;"
        "    border-radius: 3px;"
        "    padding: 4px 8px;"
        "    font-family: 'Consolas', 'Courier New', monospace;"
        "    font-size: 20px;"
        "    font-weight: bold;"
        "    color: #00ff88;"
        "    min-width: 120px;"
        "    qproperty-alignment: AlignRight | AlignVCenter;"
        "}"

        /* ===== 行编辑框 ===== */
        "QLineEdit {"
        "    background-color: #0a0a1a;"
        "    border: 1px solid #0f3460;"
        "    border-radius: 3px;"
        "    padding: 4px 8px;"
        "    color: #00ff88;"
        "    font-family: 'Consolas', 'Courier New', monospace;"
        "    selection-background-color: #0f3460;"
        "}"
        "QLineEdit:focus {"
        "    border-color: #00d2ff;"
        "}"

        /* ===== 文本编辑框（G代码编辑器） ===== */
        "QPlainTextEdit, QTextEdit {"
        "    background-color: #0a0a1a;"
        "    border: 1px solid #0f3460;"
        "    color: #00ff88;"
        "    font-family: 'Consolas', 'Courier New', monospace;"
        "    font-size: 13px;"
        "    selection-background-color: #0f3460;"
        "}"
        "QPlainTextEdit:focus, QTextEdit:focus {"
        "    border-color: #00d2ff;"
        "}"

        /* ===== 下拉框 ===== */
        "QComboBox {"
        "    background-color: #0f3460;"
        "    border: 1px solid #1a1a5e;"
        "    border-radius: 3px;"
        "    padding: 4px 8px;"
        "    color: #e0e0e0;"
        "    min-height: 24px;"
        "}"
        "QComboBox:hover {"
        "    border-color: #00d2ff;"
        "}"
        "QComboBox::drop-down {"
        "    border: none;"
        "    width: 20px;"
        "}"
        "QComboBox QAbstractItemView {"
        "    background-color: #16213e;"
        "    border: 1px solid #0f3460;"
        "    color: #e0e0e0;"
        "    selection-background-color: #0f3460;"
        "}"

        /* ===== 滚动条 ===== */
        "QScrollBar:vertical {"
        "    background: #0a0a1a;"
        "    width: 12px;"
        "    margin: 0;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background: #0f3460;"
        "    border-radius: 4px;"
        "    min-height: 30px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "    background: #1a4a8a;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "    height: 0;"
        "}"
        "QScrollBar:horizontal {"
        "    background: #0a0a1a;"
        "    height: 12px;"
        "}"
        "QScrollBar::handle:horizontal {"
        "    background: #0f3460;"
        "    border-radius: 4px;"
        "    min-width: 30px;"
        "}"
        "QScrollBar::handle:horizontal:hover {"
        "    background: #1a4a8a;"
        "}"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
        "    width: 0;"
        "}"

        /* ===== 标签页 ===== */
        "QTabWidget::pane {"
        "    border: 1px solid #0f3460;"
        "    background-color: #16213e;"
        "}"
        "QTabBar::tab {"
        "    background-color: #0f3460;"
        "    border: 1px solid #1a1a5e;"
        "    padding: 6px 12px;"
        "    color: #a0a0a0;"
        "    border-top-left-radius: 4px;"
        "    border-top-right-radius: 4px;"
        "}"
        "QTabBar::tab:selected {"
        "    background-color: #16213e;"
        "    border-bottom-color: #16213e;"
        "    color: #00d2ff;"
        "}"
        "QTabBar::tab:hover {"
        "    color: #00d2ff;"
        "}"

        /* ===== 表格 ===== */
        "QHeaderView::section {"
        "    background-color: #0f3460;"
        "    border: 1px solid #1a1a5e;"
        "    padding: 4px;"
        "    font-weight: bold;"
        "    color: #00d2ff;"
        "}"
        "QTableView {"
        "    background-color: #0a0a1a;"
        "    alternate-background-color: #0d0d20;"
        "    border: 1px solid #0f3460;"
        "    gridline-color: #0f3460;"
        "}"
        "QTableView::item:selected {"
        "    background-color: #0f3460;"
        "}"

        /* ===== 树形视图 ===== */
        "QTreeView {"
        "    background-color: #0a0a1a;"
        "    border: 1px solid #0f3460;"
        "    alternate-background-color: #0d0d20;"
        "}"
        "QTreeView::item:selected {"
        "    background-color: #0f3460;"
        "}"

        /* ===== 分割器 ===== */
        "QSplitter::handle {"
        "    background-color: #0f3460;"
        "    height: 3px;"
        "    width: 3px;"
        "}"
        "QSplitter::handle:hover {"
        "    background-color: #00d2ff;"
        "}"

        /* ===== 工具栏 ===== */
        "QToolBar {"
        "    background-color: #0f3460;"
        "    border-bottom: 1px solid #1a1a5e;"
        "    spacing: 4px;"
        "    padding: 2px;"
        "}"

        /* ===== 状态栏 ===== */
        "QStatusBar {"
        "    background-color: #0a0a1a;"
        "    border-top: 1px solid #0f3460;"
        "    color: #00ff88;"
        "    font-family: 'Consolas', 'Courier New', monospace;"
        "    font-size: 12px;"
        "}"

        /* ===== 菜单栏 ===== */
        "QMenuBar {"
        "    background-color: #0f3460;"
        "    border-bottom: 1px solid #1a1a5e;"
        "}"
        "QMenuBar::item:selected {"
        "    background-color: #1a4a8a;"
        "}"
        "QMenu {"
        "    background-color: #16213e;"
        "    border: 1px solid #0f3460;"
        "}"
        "QMenu::item:selected {"
        "    background-color: #0f3460;"
        "}"

        /* ===== 进度条 ===== */
        "QProgressBar {"
        "    background-color: #0a0a1a;"
        "    border: 1px solid #0f3460;"
        "    border-radius: 3px;"
        "    text-align: center;"
        "    color: #00ff88;"
        "}"
        "QProgressBar::chunk {"
        "    background-color: #00d2ff;"
        "    border-radius: 2px;"
        "}"

        /* ===== 滑块 ===== */
        "QSlider::groove:horizontal {"
        "    height: 6px;"
        "    background: #0a0a1a;"
        "    border-radius: 3px;"
        "}"
        "QSlider::handle:horizontal {"
        "    background: #00d2ff;"
        "    width: 16px;"
        "    height: 16px;"
        "    margin: -5px 0;"
        "    border-radius: 8px;"
        "}"
        "QSlider::sub-page:horizontal {"
        "    background: #0f3460;"
        "    border-radius: 3px;"
        "}"

        /* ===== 旋转框 ===== */
        "QSpinBox, QDoubleSpinBox {"
        "    background-color: #0a0a1a;"
        "    border: 1px solid #0f3460;"
        "    border-radius: 3px;"
        "    padding: 4px 8px;"
        "    color: #00ff88;"
        "    font-family: 'Consolas', 'Courier New', monospace;"
        "}"
        "QSpinBox:focus, QDoubleSpinBox:focus {"
        "    border-color: #00d2ff;"
        "}"

        /* ===== 复选框 ===== */
        "QCheckBox {"
        "    spacing: 8px;"
        "    color: #e0e0e0;"
        "}"
        "QCheckBox::indicator {"
        "    width: 16px;"
        "    height: 16px;"
        "    border: 1px solid #0f3460;"
        "    border-radius: 3px;"
        "    background-color: #0a0a1a;"
        "}"
        "QCheckBox::indicator:checked {"
        "    background-color: #00d2ff;"
        "    border-color: #00d2ff;"
        "}"

        /* ===== 单选按钮 ===== */
        "QRadioButton {"
        "    spacing: 8px;"
        "    color: #e0e0e0;"
        "}"
        "QRadioButton::indicator {"
        "    width: 16px;"
        "    height: 16px;"
        "    border: 1px solid #0f3460;"
        "    border-radius: 8px;"
        "    background-color: #0a0a1a;"
        "}"
        "QRadioButton::indicator:checked {"
        "    background-color: #00d2ff;"
        "    border-color: #00d2ff;"
        "}"

        /* ===== 工具提示 ===== */
        "QToolTip {"
        "    background-color: #16213e;"
        "    border: 1px solid #00d2ff;"
        "    color: #e0e0e0;"
        "    padding: 4px;"
        "}"
    );
}
