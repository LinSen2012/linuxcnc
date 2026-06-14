/*
 * linuxQtCncGui - LinuxCNC Qt6 GUI 控制器
 * GCodeEditor 实现
 */

#include "GCodeEditor.h"

#include <QPainter>
#include <QTextBlock>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QDebug>

// ============================================================================
// 构造 / 析构
// ============================================================================

GCodeEditor::GCodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
{
    setupUi();
}

GCodeEditor::~GCodeEditor() = default;

// ============================================================================
// 公共方法
// ============================================================================

bool GCodeEditor::loadFile(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning().noquote() << "[GCodeEditor] 无法打开文件:" << filename;
        return false;
    }

    QTextStream in(&file);
    setPlainText(in.readAll());
    file.close();

    m_currentFile = filename;
    setReadOnly(true);  // 程序文件默认只读

    qInfo().noquote() << "[GCodeEditor] 已加载:" << filename
                       << "行数:" << document()->blockCount();
    return true;
}

void GCodeEditor::highlightLine(int line)
{
    m_highlightedLine = line;

    // 滚动到高亮行
    if (line >= 0) {
        QTextCursor cursor(document()->findBlockByNumber(line));
        setTextCursor(cursor);
        centerCursor();
    }

    viewport()->update();
}

QString GCodeEditor::currentFile() const
{
    return m_currentFile;
}

// ============================================================================
// 行号区域
// ============================================================================

int GCodeEditor::lineNumberAreaWidth() const
{
    int digits = 1;
    int max = qMax(1, document()->blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    // 行号宽度 = 边距 + 数字位数 * 字体宽度 + 间距
    int space = 8 + digits * fontMetrics().horizontalAdvance(QLatin1Char('9')) + 8;
    return space;
}

void GCodeEditor::paintEvent(QPaintEvent *event)
{
    QPlainTextEdit::paintEvent(event);

    // 绘制程序执行行高亮
    if (m_highlightedLine >= 0) {
        QPainter painter(viewport());
        QTextBlock block = document()->findBlockByNumber(m_highlightedLine);
        if (block.isValid()) {
            QRectF blockRect = blockBoundingGeometry(block).translated(contentOffset());
            // 高亮背景
            painter.fillRect(QRectF(blockRect.x(), blockRect.y(),
                                    viewport()->width(), blockRect.height()),
                            QColor(0, 210, 255, 40));  // 半透明青色
            // 左侧指示条
            painter.fillRect(QRectF(0, blockRect.y(), 3, blockRect.height()),
                            QColor(0, 210, 255));  // 青色指示条
        }
    }
}

void GCodeEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(),
                                        lineNumberAreaWidth(), cr.height()));
}

void GCodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), QColor(10, 10, 26));  // 深色背景

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);

            // 当前行高亮
            if (blockNumber == m_highlightedLine) {
                painter.fillRect(0, top, m_lineNumberArea->width(),
                                fontMetrics().height(),
                                QColor(0, 210, 255, 60));
                painter.setPen(QColor(0, 210, 255));
            } else {
                painter.setPen(QColor(100, 100, 120));
            }

            painter.drawText(0, top, m_lineNumberArea->width() - 4,
                            fontMetrics().height(),
                            Qt::AlignRight | Qt::AlignVCenter, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

// ============================================================================
// 私有槽
// ============================================================================

void GCodeEditor::updateLineNumberAreaWidth(int newBlockCount)
{
    Q_UNUSED(newBlockCount)
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void GCodeEditor::highlightCurrentLine()
{
    // 由光标位置触发的当前行高亮（编辑模式）
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(QColor(15, 52, 96, 100));
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void GCodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy) {
        m_lineNumberArea->scroll(0, dy);
    } else {
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void GCodeEditor::onMdiSubmitted()
{
    QString cmd = m_mdiInput->text().trimmed();
    if (!cmd.isEmpty()) {
        emit mdiCommandRequested(cmd);
        m_mdiInput->clear();
    }
}

// ============================================================================
// UI 初始化
// ============================================================================

void GCodeEditor::setupUi()
{
    // 创建行号区域
    m_lineNumberArea = new GCodeLineNumberArea(this);

    // 编辑器设置
    setFont(QFont(QStringLiteral("Consolas"), 11));
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setReadOnly(true);
    setPlaceholderText(QStringLiteral("在此加载 G 代码文件..."));

    // 行号区域更新
    connect(this, &GCodeEditor::blockCountChanged,
            this, &GCodeEditor::updateLineNumberAreaWidth);
    connect(this, &GCodeEditor::updateRequest,
            this, &GCodeEditor::updateLineNumberArea);
    connect(this, &GCodeEditor::cursorPositionChanged,
            this, &GCodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);

    // MDI 输入框（在编辑器底部）
    // 注意：MDI 输入框由父组件的布局管理
    // 这里我们创建一个独立的 MDI 输入区域
    // 实际集成时，MDI 输入框放在主窗口的底部或工具栏中
}
