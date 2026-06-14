/*
 * linuxQtCncGui - LinuxCNC Qt6 GUI 控制器
 * GCodeEditor - G 代码编辑器
 *
 * 基于 QPlainTextEdit 的 G 代码编辑器，支持：
 * - 行号显示
 * - 当前行高亮（程序运行时）
 * - G 代码语法高亮
 * - MDI 命令输入
 * - 文件加载/保存
 */

#ifndef GCODEEDITOR_H
#define GCODEEDITOR_H

#include <QPlainTextEdit>
#include <QWidget>
#include <QLineEdit>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

class GCodeLineNumberArea;

// ============================================================================
// G/M/T 代码语法高亮器
// ============================================================================

/**
 * @brief GCodeHighlighter - G 代码语法高亮器
 *
 * 基于 QSyntaxHighlighter 的 G 代码语法高亮实现，支持：
 * - G 代码（蓝色粗体）
 * - M 代码（洋红色粗体）
 * - T 代码（橙色粗体）
 * - S/F/P 等参数字（绿色）
 * - X/Y/Z/A/B/C/U/V/W 轴字（青色）
 * - 圆括号注释 ; 注释 （灰色斜体）
 * - 数值常量（黄色）
 */
class GCodeHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit GCodeHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> m_rules;
};

/**
 * @brief GCodeEditor - G 代码编辑器组件
 *
 * 提供带行号的 G 代码编辑功能，支持语法高亮和当前行标记。
 */
class GCodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父组件
     */
    explicit GCodeEditor(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~GCodeEditor() override;

    /**
     * @brief 加载 G 代码文件
     * @param filename 文件路径
     * @return 是否加载成功
     */
    bool loadFile(const QString &filename);

    /**
     * @brief 设置 G 代码程序内容
     * @param program G 代码文本
     */
    void setProgram(const QString &program);

    /**
     * @brief 获取 G 代码程序内容
     * @return G 代码文本
     */
    QString program() const;

    /**
     * @brief 设置当前文件路径
     * @param path 文件路径
     */
    void setFilePath(const QString &path);

    /**
     * @brief 高亮指定行（程序运行时指示当前执行行）
     * @param line 行号（从 0 开始，-1 取消高亮）
     */
    void highlightLine(int line);

    /**
     * @brief 获取当前文件名
     * @return 文件路径
     */
    QString currentFile() const;

signals:
    /**
     * @brief 文件加载完成信号
     * @param filename 文件路径
     */
    void fileLoaded(const QString &filename);

    /**
     * @brief MDI 命令请求信号
     * @param command G 代码命令字符串
     */
    void mdiCommandRequested(const QString &command);

protected:
    /**
     * @brief 行号区域宽度计算
     * @return 宽度像素
     */
    int lineNumberAreaWidth() const;

    /**
     * @brief 绘制事件
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * @brief 调整大小事件
     */
    void resizeEvent(QResizeEvent *event) override;

    /**
     * @brief 绘制行号区域（由 GCodeLineNumberArea 调用）
     * @param event 绘制事件
     */
    void lineNumberAreaPaintEvent(QPaintEvent *event);

private slots:
    /**
     * @brief 更新行号区域宽度
     */
    void updateLineNumberAreaWidth(int newBlockCount);

    /**
     * @brief 高亮当前行
     * @param block 当前文本块
     */
    void highlightCurrentLine();

    /**
     * @brief 更新行号区域
     * @param rect 更新区域
     * @param dy 垂直滚动偏移
     */
    void updateLineNumberArea(const QRect &rect, int dy);

    /**
     * @brief MDI 命令提交
     */
    void onMdiSubmitted();

private:
    /**
     * @brief 初始化 UI
     */
    void setupUi();

    GCodeLineNumberArea *m_lineNumberArea = nullptr; ///< 行号区域
    QLineEdit *m_mdiInput = nullptr;                  ///< MDI 命令输入框
    GCodeHighlighter *m_highlighter = nullptr;       ///< 语法高亮器
    QString m_currentFile;                             ///< 当前文件路径
    int m_highlightedLine = -1;                        ///< 高亮行号
};

// ============================================================================
// 行号区域
// ============================================================================

/**
 * @brief GCodeLineNumberArea - 行号显示区域
 *
 * 作为 QPlainTextEdit 的侧边栏显示行号。
 */
class GCodeLineNumberArea : public QWidget
{
public:
    explicit GCodeLineNumberArea(GCodeEditor *editor)
        : QWidget(editor)
        , m_codeEditor(editor)
    {}

    QSize sizeHint() const override
    {
        return QSize(m_codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        m_codeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    GCodeEditor *m_codeEditor;
};

#endif // GCODEEDITOR_H
