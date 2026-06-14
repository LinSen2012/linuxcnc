/*
 * linuxQtCncGui - LinuxCNC Qt5 GUI 控制器
 * GCodeEditor - G 代码编辑器
 *
 * 基于 QPlainTextEdit 的 G 代码编辑器，支持：
 * - 行号显示
 * - 当前行高亮（程序运行时）
 * - G 代码语法高亮
 * - MDI 命令输入
 * - 文件加载/保存
 */
#include "GCodeEditor.h"

#include <QFile>
#include <QFileDialog>
#include <QTextStream>
#include <QPainter>
#include <QFontMetrics>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QLabel>
#include <QShortcut>
#include <QKeyEvent>
#include <QAction>
#include <QMenu>
#include <QRegularExpression>
#include <QTextDocument>

// ============================================================================
// GCodeHighlighter 完整实现
// ============================================================================

GCodeHighlighter::GCodeHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    // --- G 代码 (蓝色粗体) ---
    QTextCharFormat gcodeFormat;
    gcodeFormat.setForeground(QColor(0x66, 0xa0, 0xff));
    gcodeFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression(QStringLiteral("\\bG\\d+\\.?\\d*"),
                                       QRegularExpression::CaseInsensitiveOption);
    rule.format = gcodeFormat;
    m_rules.append(rule);

    // --- M 代码 (洋红色粗体) ---
    QTextCharFormat mcodeFormat;
    mcodeFormat.setForeground(QColor(0xff, 0x70, 0xff));
    mcodeFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression(QStringLiteral("\\bM\\d+\\.?\\d*"),
                                       QRegularExpression::CaseInsensitiveOption);
    rule.format = mcodeFormat;
    m_rules.append(rule);

    // --- T 代码 (橙色粗体) ---
    QTextCharFormat tcodeFormat;
    tcodeFormat.setForeground(QColor(0xff, 0xaa, 0x33));
    tcodeFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression(QStringLiteral("\\bT\\d+\\.?\\d*"),
                                       QRegularExpression::CaseInsensitiveOption);
    rule.format = tcodeFormat;
    m_rules.append(rule);

    // --- S/F/P/H/D/R/Q 等参数 (绿色) ---
    QTextCharFormat paramFormat;
    paramFormat.setForeground(QColor(0x7c, 0xfc, 0x00));
    QStringList paramPrefixes;
    paramPrefixes << QStringLiteral("\\bS") << QStringLiteral("\\bF")
                  << QStringLiteral("\\bP") << QStringLiteral("\\bH")
                  << QStringLiteral("\\bD") << QStringLiteral("\\bR")
                  << QStringLiteral("\\bQ") << QStringLiteral("\\bE")
                  << QStringLiteral("\\bK");
    for (const QString &prefix : qAsConst(paramPrefixes)) {
        rule.pattern = QRegularExpression(
            prefix + QStringLiteral("[-+]?\\d*\\.?\\d+"),
            QRegularExpression::CaseInsensitiveOption
        );
        rule.format = paramFormat;
        m_rules.append(rule);
    }

    // --- X/Y/Z/A/B/C/U/V/W 轴字 (青色) ---
    QTextCharFormat axisFormat;
    axisFormat.setForeground(QColor(0x40, 0xe0, 0xd0));
    QStringList axisPrefixes;
    axisPrefixes << QStringLiteral("\\bX") << QStringLiteral("\\bY")
                 << QStringLiteral("\\bZ") << QStringLiteral("\\bA")
                 << QStringLiteral("\\bB") << QStringLiteral("\\bC")
                 << QStringLiteral("\\bU") << QStringLiteral("\\bV")
                 << QStringLiteral("\\bW");
    for (const QString &prefix : qAsConst(axisPrefixes)) {
        rule.pattern = QRegularExpression(
            prefix + QStringLiteral("[-+]?\\d*\\.?\\d+"),
            QRegularExpression::CaseInsensitiveOption
        );
        rule.format = axisFormat;
        m_rules.append(rule);
    }

    // --- I/J/K 圆弧参数 (紫色) ---
    QTextCharFormat arcFormat;
    arcFormat.setForeground(QColor(0xb0, 0x80, 0xff));
    QStringList arcPrefixes;
    arcPrefixes << QStringLiteral("\\bI") << QStringLiteral("\\bJ")
                << QStringLiteral("\\bK");
    for (const QString &prefix : qAsConst(arcPrefixes)) {
        rule.pattern = QRegularExpression(
            prefix + QStringLiteral("[-+]?\\d*\\.?\\d+"),
            QRegularExpression::CaseInsensitiveOption
        );
        rule.format = arcFormat;
        m_rules.append(rule);
    }

    // --- 数字常量 (黄色，非轴字/参数字的数字) ---
    QTextCharFormat numberFormat;
    numberFormat.setForeground(QColor(0xff, 0xff, 0x99));
    rule.pattern = QRegularExpression(QStringLiteral("\\b\\d+\\.?\\d*\\b"));
    rule.format = numberFormat;
    m_rules.append(rule);

    // --- 分号注释 (灰色斜体) ---
    QTextCharFormat semicolonCommentFormat;
    semicolonCommentFormat.setForeground(QColor(0x80, 0x80, 0x80));
    semicolonCommentFormat.setFontItalic(true);
    rule.pattern = QRegularExpression(QStringLiteral(";[^\n]*"));
    rule.format = semicolonCommentFormat;
    m_rules.append(rule);

    // --- 圆括号注释 (灰色斜体) ---
    // 多行注释：( ... ) 可能跨越多行，这里在 highlightBlock 中特殊处理
}

void GCodeHighlighter::highlightBlock(const QString &text)
{
    const int ParenCommentState = 1;  // 自定义状态

    // ---- 多行圆括号注释处理 ----
    int startIndex = 0;
    int state = previousBlockState();

    if (state != ParenCommentState) {
        // 当前块未在括号注释内，查找第一个 '('
        startIndex = text.indexOf(QLatin1Char('('));
    }

    while (startIndex >= 0) {
        // 查找匹配的 ')'
        int endIndex = text.indexOf(QLatin1Char(')'), startIndex);
        int commentLength;
        if (endIndex == -1) {
            // 本行未闭合，延续到下一行
            setCurrentBlockState(ParenCommentState);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + 1;
        }

        QTextCharFormat commentFormat;
        commentFormat.setForeground(QColor(0x80, 0x80, 0x80));
        commentFormat.setFontItalic(true);
        setFormat(startIndex, commentLength, commentFormat);

        // 查找下一个 '('
        startIndex = text.indexOf(QLatin1Char('('), startIndex + commentLength);
    }

    // ---- 单行规则匹配（不覆盖已有括号注释）----
    for (const HighlightingRule &rule : m_rules) {
        QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            int pos = match.capturedStart();
            int len = match.capturedLength();
            // 跳过已被括号注释标记的文本
            if (format(pos) != QTextCharFormat()) {
                continue;
            }
            setFormat(pos, len, rule.format);
        }
    }
}

// ============================================================================
// GCodeLineNumberArea 完整实现
// ============================================================================

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

// ============================================================================
// 构造 / 析构
// ============================================================================

GCodeEditor::GCodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
    , m_lineNumberArea(new GCodeLineNumberArea(this))
    , m_mdiInput(new QLineEdit(this))
    , m_highlighter(new GCodeHighlighter(document()))
    , m_currentFile()
    , m_highlightedLine(-1)
{
    setupUi();

    // 行号更新
    connect(this, &QPlainTextEdit::blockCountChanged,
            this, &GCodeEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest,
            this, &GCodeEditor::updateLineNumberArea);

    // 当前行高亮
    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, &GCodeEditor::highlightCurrentLine);

    // Ctrl+G: 跳转到行
    QShortcut *gotoLineShortcut = new QShortcut(QKeySequence(tr("Ctrl+G")), this);
    connect(gotoLineShortcut, &QShortcut::activated, this, [this]() {
        bool ok = false;
        int line = QInputDialog::getInt(this, tr("跳转到行"),
            tr("行号:"), 1, 1, blockCount(), 1, &ok);
        if (ok) {
            QTextCursor cursor(document()->findBlockByLineNumber(line - 1));
            setTextCursor(cursor);
        }
    });
}

GCodeEditor::~GCodeEditor() = default;

// ============================================================================
// 公共方法
// ============================================================================

bool GCodeEditor::loadFile(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    QTextStream in(&file);
    setPlainText(in.readAll());
    file.close();

    m_currentFile = filename;
    setWindowTitle(tr("G代码编辑器 - %1").arg(filename));
    emit fileLoaded(filename);
    return true;
}

void GCodeEditor::highlightLine(int line)
{
    m_highlightedLine = line;
    viewport()->update();
}

QString GCodeEditor::currentFile() const
{
    return m_currentFile;
}

void GCodeEditor::setProgram(const QString &program)
{
    setPlainText(program);
    m_currentFile.clear();
}

QString GCodeEditor::program() const
{
    return toPlainText();
}

void GCodeEditor::setFilePath(const QString &path)
{
    m_currentFile = path;
    if (!path.isEmpty()) {
        setWindowTitle(tr("G代码 - %1").arg(QFileInfo(path).fileName()));
    }
}

// ============================================================================
// 私有方法
// ============================================================================

void GCodeEditor::setupUi()
{
    // 基本属性
    setFont(QFont("Consolas", 11));
    setTabStopDistance(40); // 4 空格
    setLineWrapMode(NoWrap);

    // MDI 输入框
    m_mdiInput->setPlaceholderText(tr("输入 MDI 命令... (回车发送)"));
    m_mdiInput->setMaximumHeight(28);
    connect(m_mdiInput, &QLineEdit::returnPressed, this, &GCodeEditor::onMdiSubmitted);

    // 右键菜单增强
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        QMenu menu(this);

        QAction *copyAct = menu.addAction(tr("复制"));
        copyAct->setShortcut(QKeySequence::Copy);
        connect(copyAct, &QAction::triggered, this, &QPlainTextEdit::copy);

        QAction *pasteAct = menu.addAction(tr("粘贴"));
        pasteAct->setShortcut(QKeySequence::Paste);
        connect(pasteAct, &QAction::triggered, this, &QPlainTextEdit::paste);

        QAction *cutAct = menu.addAction(tr("剪切"));
        cutAct->setShortcut(QKeySequence::Cut);
        connect(cutAct, &QAction::triggered, this, &QPlainTextEdit::cut);

        menu.addSeparator();

        QAction *selectAllAct = menu.addAction(tr("全选"));
        selectAllAct->setShortcut(QKeySequence::SelectAll);
        connect(selectAllAct, &QAction::triggered, this, &QPlainTextEdit::selectAll);

        menu.addSeparator();

        QAction *openAct = menu.addAction(tr("打开文件..."));
        connect(openAct, &QAction::triggered, this, [this]() {
            QString path = QFileDialog::getOpenFileName(this,
                tr("打开 G代码"), QString(),
                tr("G代码 (*.ngc *.gcode *.nc);;所有文件 (*.*)"));
            if (!path.isEmpty()) loadFile(path);
        });

        QAction *saveAct = menu.addAction(tr("另存为..."));
        connect(saveAct, &QAction::triggered, this, [this]() {
            QString path = QFileDialog::getSaveFileName(this,
                tr("保存 G代码"), m_currentFile,
                tr("G代码 (*.ngc);;所有文件 (*.*)"));
            if (!path.isEmpty()) {
                QFile file(path);
                if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QTextStream out(&file);
                    out << toPlainText();
                    file.close();
                    setFilePath(path);
                }
            }
        });

        menu.exec(mapToGlobal(pos));
    });

    // 初始行号宽度
    updateLineNumberAreaWidth(0);
}

// ============================================================================
// 行号区域
// ============================================================================

int GCodeEditor::lineNumberAreaWidth() const
{
    int digits = 1;
    int maxLines = qMax(1, blockCount());
    while (maxLines >= 10) {
        maxLines /= 10;
        ++digits;
    }
    const QFontMetrics fm(font());
    return fm.horizontalAdvance(QLatin1Char('9')) * digits + 16;
}

void GCodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void GCodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect().topLeft())) {
        updateLineNumberAreaWidth(0);
    }
}

void GCodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), QColor(0x1a, 0x1a, 0x2e));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    qreal top = blockBoundingGeometry(block).translated(contentOffset()).top();
    qreal bottom = top + blockBoundingRect(block).height();

    const QFontMetrics fm(font());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            // 当前执行行高亮
            if (blockNumber == m_highlightedLine) {
                painter.fillRect(0, static_cast<int>(top),
                                 m_lineNumberArea->width(), static_cast<int>(bottom - top),
                                 QColor(0x00, 0x66, 0x00));
                painter.setPen(QColor(0xff, 0xff, 0xff));
            } else {
                painter.setPen(QColor(0x60, 0x60, 0x80));
            }
            painter.drawText(0, static_cast<int>(top),
                             m_lineNumberArea->width() - 6,
                             fm.height(),
                             Qt::AlignRight | Qt::AlignVCenter,
                             number);
        }

        block = block.next();
        top = bottom;
        bottom = top + blockBoundingRect(block).height();
        ++blockNumber;
    }
}

// ============================================================================
// 高亮当前行
// ============================================================================

void GCodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(QColor(0x1a, 0x4a, 0x8a, 60));
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    // 当前执行行高亮（绿色）
    if (m_highlightedLine >= 0) {
        QTextEdit::ExtraSelection execLine;
        execLine.format.setBackground(QColor(0x00, 0x80, 0x00, 80));
        execLine.format.setProperty(QTextFormat::FullWidthSelection, true);
        execLine.cursor = document()->findBlockByLineNumber(m_highlightedLine);
        extraSelections.append(execLine);
    }

    setExtraSelections(extraSelections);
}

// ============================================================================
// 事件处理
// ============================================================================

void GCodeEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(),
                                         lineNumberAreaWidth(), cr.height()));
}

// ============================================================================
// MDI 命令
// ============================================================================

void GCodeEditor::onMdiSubmitted()
{
    QString cmd = m_mdiInput->text().trimmed();
    if (!cmd.isEmpty()) {
        emit mdiCommandRequested(cmd);
        m_mdiInput->clear();
    }
}
