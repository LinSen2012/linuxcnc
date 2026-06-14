/*
 * linuxQtCncGui - LinuxCNC Qt6 GUI 控制器
 * LcncApplication - 应用程序类，继承 QApplication
 * 管理全局状态、主题样式、核心单例引用
 */

#ifndef LCNC_APPLICATION_H
#define LCNC_APPLICATION_H

#include <QApplication>

class LcncCore;

/**
 * @brief LcncApplication - CNC 控制器 GUI 应用程序类
 *
 * 继承自 QApplication，负责：
 * - 持有 LcncCore 单例引用
 * - 管理应用生命周期
 * - 提供全局深色工业风格主题
 */
class LcncApplication : public QApplication
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param argc 命令行参数数量
     * @param argv 命令行参数数组
     */
    explicit LcncApplication(int &argc, char **argv);

    /**
     * @brief 析构函数
     */
    ~LcncApplication() override;

    /**
     * @brief 获取 LcncCore 单例引用
     * @return LcncCore 指针
     */
    LcncCore *core() const;

    /**
     * @brief 应用深色工业风格主题样式表
     *
     * 该样式表定义了 CNC 控制器界面的全局外观：
     * - 深色背景（#1a1a2e, #16213e）
     * - 绿色/青色强调色（工业仪表风格）
     * - 高对比度数字显示
     * - 紧凑布局适合工业触摸屏
     */
    void applyIndustrialTheme();

    /**
     * @brief 获取当前主题样式表字符串
     * @return CSS 样式表
     */
    static QString industrialStyleSheet();

private:
    /// LcncCore 单例指针（不拥有所有权）
    LcncCore *m_core = nullptr;
};

#endif // LCNC_APPLICATION_H
