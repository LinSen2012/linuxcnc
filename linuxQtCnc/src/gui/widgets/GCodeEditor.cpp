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
