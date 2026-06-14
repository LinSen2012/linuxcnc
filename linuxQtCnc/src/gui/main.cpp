/*
 * linuxQtCncGui - LinuxCNC Qt6 GUI 控制器
 * 主入口文件
 */

#include "LcncApplication.h"
#include "mainwindow/MainWindow.h"

#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QFileInfo>

int main(int argc, char *argv[])
{
    // 初始化 LcncApplication（继承自 QApplication）
    LcncApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("linuxQtCncGui"));
    app.setApplicationVersion(QStringLiteral("0.1.0"));
    app.setOrganizationName(QStringLiteral("LinuxCNC"));

    // 应用全局深色工业风格主题
    app.applyIndustrialTheme();

    // 命令行参数解析
    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main",
        "LinuxCNC Qt6 图形界面控制器"));
    parser.addHelpOption();
    parser.addVersionOption();

    // -i, --ini  指定 INI 配置文件路径
    QCommandLineOption iniOption(QStringList() << QStringLiteral("i") << QStringLiteral("ini"),
        QCoreApplication::translate("main", "指定 LinuxCNC INI 配置文件路径"),
        QCoreApplication::translate("main", "ini文件"));
    parser.addOption(iniOption);

    // -s, --sim  模拟模式（不连接真实 NML 服务器）
    QCommandLineOption simOption(QStringList() << QStringLiteral("s") << QStringLiteral("sim"),
        QCoreApplication::translate("main", "模拟模式，使用模拟数据而不连接 NML 服务器"));
    parser.addOption(simOption);

    parser.process(app);

    // 获取参数
    QString iniFile = parser.value(iniOption);
    bool simulationMode = parser.isSet(simOption);

    qInfo().noquote() << "[linuxQtCncGui]" << "版本:" << app.applicationVersion();
    qInfo().noquote() << "[linuxQtCncGui]" << "模式:"
                       << (simulationMode ? "模拟" : "实际连接");

    if (!iniFile.isEmpty()) {
        QFileInfo fi(iniFile);
        if (!fi.exists()) {
            qWarning().noquote() << "[linuxQtCncGui] INI 文件不存在:" << iniFile;
            return 1;
        }
        qInfo().noquote() << "[linuxQtCncGui] INI 文件:" << fi.absoluteFilePath();
    }

    // 创建主窗口
    MainWindow mainWindow;
    mainWindow.show();

    // 如果指定了 INI 文件或模拟模式，启动时自动连接
    if (simulationMode) {
        mainWindow.startSimulation();
    } else if (!iniFile.isEmpty()) {
        mainWindow.connectToIni(iniFile);
    }

    return app.exec();
}
