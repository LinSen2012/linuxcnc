/*
 * linuxQtCncGui - LinuxCNC Qt5 GUI 控制器
 * main.cpp - 程序入口
 *
 * 支持的命令行参数：
 *   --ini <file>     指定 INI 配置文件（默认：sim/lathe.ini）
 *   --sim            以模拟模式启动（不连接真实 NML 服务器）
 *   --debug          启用调试输出
 */
#include "LcncApplication.h"
#include "LcncCore.h"
#include "mainwindow/MainWindow.h"

#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDir>
#include <QFileInfo>

int main(int argc, char *argv[])
{
    // ---- QApplication ----
    LcncApplication app(argc, argv);
    app.setApplicationName("linuxQtCnc");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("linuxQtCnc");

    // ---- 命令行参数解析 ----
    QCommandLineParser parser;
    parser.setApplicationDescription(
        "linuxQtCnc - 跨平台 CNC 控制器 Qt5 GUI\n"
        "基于 LinuxCNC 架构，支持 Windows 和 Linux"
    );
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption iniFileOption(
        QStringList() << "i" << "ini",
        "指定 INI 配置文件路径。",
        "file"
    );
    parser.addOption(iniFileOption);

    QCommandLineOption simOption(
        "s" << "sim",
        "以模拟模式启动（不连接真实 NML 服务器）。"
    );
    parser.addOption(simOption);

    QCommandLineOption debugOption(
        "d" << "debug",
        "启用调试输出（QT_LOG_LEVEL=debug）。"
    );
    parser.addOption(debugOption);

    QCommandLineOption scaleOption(
        "scale",
        "设置界面的 DPI 缩放因子（默认 1.0）。",
        "factor",
        "1.0"
    );
    parser.addOption(scaleOption);

    parser.process(app);

    // 设置日志级别
    if (parser.isSet(debugOption)) {
        qputenv("QT_LOGGING_RULES", "qt.*=true");
    }

    // ---- 应用主题 ----
    app.applyIndustrialTheme();

    // ---- 获取 LcncCore 单例 ----
    LcncCore *core = app.core();

    // ---- 创建主窗口 ----
    MainWindow mainWin(core);
    mainWin.setWindowIcon(QIcon(":/icons/linuxcnc.png"));
    mainWin.show();

    // ---- 连接模式 ----
    bool simMode = parser.isSet(simOption);
    QString iniFile = parser.value(iniFileOption);

    if (simMode) {
        // 模拟模式：启动 GUI 但不连接 NML
        qInfo() << "[main] 模拟模式启动";
        mainWin.show();
    } else if (!iniFile.isEmpty()) {
        // 指定 INI 文件连接
        QFileInfo fi(iniFile);
        if (!fi.exists()) {
            qWarning() << "[main] INI 文件不存在:" << iniFile;
            mainWin.show();
        } else {
            qInfo() << "[main] 连接 INI:" << fi.absoluteFilePath();
            mainWin.connectToIni(fi.absoluteFilePath());
            mainWin.show();
        }
    } else {
        // 默认：尝试连接 sim/lathe.ini
        QString defaultIni = "sim/lathe.ini";
        if (QFileInfo::exists(defaultIni)) {
            qInfo() << "[main] 使用默认 INI:" << defaultIni;
            mainWin.connectToIni(QFileInfo(defaultIni).absoluteFilePath());
        } else {
            qInfo() << "[main] 无 INI 参数，以演示模式启动";
        }
        mainWin.show();
    }

    return app.exec();
}
