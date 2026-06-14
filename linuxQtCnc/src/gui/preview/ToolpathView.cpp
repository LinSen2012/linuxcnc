/*
 * linuxQtCncGui - LinuxCNC Qt6 GUI 控制器
 * ToolpathView 实现
 *
 * 3D 刀具路径 OpenGL 渲染视图的实现。
 * 使用兼容 OpenGL 2.x 的固定管线进行渲染。
 */

#include "ToolpathView.h"

#include <QMouseEvent>
#include <QWheelEvent>
#include <QOpenGLShaderProgram>
#include <QDebug>
#include <cmath>

// ============================================================================
// 构造 / 析构
// ============================================================================

ToolpathView::ToolpathView(QWidget *parent)
    : QOpenGLWidget(parent)
    , m_parser(new GCodeParser(this))
{
    setMinimumSize(200, 200);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(false);

    qInfo().noquote() << "[ToolpathView] 3D 刀具路径视图已创建";
}

ToolpathView::~ToolpathView() = default;

// ============================================================================
// 公共方法
// ============================================================================

void ToolpathView::loadGCode(const QString &filename)
{
    if (m_parser->parseFile(filename)) {
        m_segments = m_parser->pathSegments();
        fitToView();
        update();
        qInfo().noquote() << "[ToolpathView] 已加载 G 代码:"
                           << m_segments.size() << "段路径";
    } else {
        qWarning().noquote() << "[ToolpathView] G 代码解析失败:"
                               << m_parser->errors();
    }
}

void ToolpathView::setPathData(const QVector<PathSegment> &segments)
{
    m_segments = segments;
    if (!m_segments.isEmpty()) {
        fitToView();
    }
    update();
}

void ToolpathView::updateToolPosition(const QVector3D &pos)
{
    m_toolPos = pos;
    m_toolPosValid = true;
    update();
}

void ToolpathView::clearPath()
{
    m_segments.clear();
    m_toolPosValid = false;
    if (m_parser) {
        m_parser->clear();
    }
    update();
}

void ToolpathView::setTopView(bool topView)
{
    m_topView = topView;
    if (m_topView) {
        m_rotationX = 90.0f;
        m_rotationZ = 0.0f;
    } else {
        m_rotationX = 30.0f;
        m_rotationZ = -45.0f;
    }
    update();
}

void ToolpathView::fitToView()
{
    if (m_segments.isEmpty()) {
        m_viewScale = 1.0f;
        return;
    }

    auto bbox = m_parser->boundingBox();
    double minX = bbox.first.first;
    double minY = bbox.first.second;
    double maxX = bbox.second.first;
    double maxY = bbox.second.second;

    double rangeX = maxX - minX;
    double rangeY = maxY - minY;

    if (rangeX < 0.001 && rangeY < 0.001) {
        m_viewScale = 1.0f;
        return;
    }

    float maxRange = static_cast<float>(std::max(rangeX, rangeY));
    m_viewScale = 200.0f / maxRange;  // 适配到约 200 像素范围

    // 将视图中心移到路径中心
    m_panX = static_cast<float>(-(minX + maxX) / 2.0) * m_viewScale;
    m_panY = static_cast<float>(-(minY + maxY) / 2.0) * m_viewScale;

    m_zoom = 1.0f;
    update();
}

void ToolpathView::setShowGrid(bool show)
{
    m_showGrid = show;
    update();
}

void ToolpathView::setShowAxes(bool show)
{
    m_showAxes = show;
    update();
}

// ============================================================================
// OpenGL 方法
// ============================================================================

void ToolpathView::initializeGL()
{
    initializeOpenGLFunctions();

    // 设置背景色（深色工业风格）
    glClearColor(0.06f, 0.06f, 0.12f, 1.0f);  // #0f0f1e

    // 启用抗锯齿
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 启用深度测试
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    qInfo().noquote() << "[ToolpathView] OpenGL 初始化完成";
}

void ToolpathView::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    updateViewMatrix();
}

void ToolpathView::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    setup3DProjection();

    // 绘制网格
    if (m_showGrid) {
        drawGrid();
    }

    // 绘制坐标轴
    if (m_showAxes) {
        drawAxes();
    }

    // 绘制刀具路径
    drawToolpath();

    // 绘制当前刀具位置
    if (m_toolPosValid) {
        drawToolPosition();
    }
}

// ============================================================================
// 绘制方法
// ============================================================================

void ToolpathView::drawGrid()
{
    glLineWidth(1.0f);
    glColor4f(0.15f, 0.15f, 0.25f, 0.5f);

    float gridSize = 100.0f * m_viewScale * m_zoom;
    float gridStep = 10.0f * m_viewScale * m_zoom;
    float offsetX = m_panX * m_zoom;
    float offsetY = m_panY * m_zoom;

    // 确保网格步进合理
    if (gridStep < 5.0f) gridStep = 5.0f;
    if (gridStep > 200.0f) gridStep = 200.0f;

    glBegin(GL_LINES);
    for (float x = -gridSize; x <= gridSize; x += gridStep) {
        glVertex3f(x + offsetX, -gridSize + offsetY, 0.0f);
        glVertex3f(x + offsetX, gridSize + offsetY, 0.0f);
    }
    for (float y = -gridSize; y <= gridSize; y += gridStep) {
        glVertex3f(-gridSize + offsetX, y + offsetY, 0.0f);
        glVertex3f(gridSize + offsetX, y + offsetY, 0.0f);
    }
    glEnd();
}

void ToolpathView::drawAxes()
{
    float axisLen = 50.0f;
    float ox = m_panX * m_zoom;
    float oy = m_panY * m_zoom;

    glLineWidth(2.0f);

    glBegin(GL_LINES);
    // X 轴 - 红色
    glColor3f(1.0f, 0.3f, 0.3f);
    glVertex3f(ox, oy, 0.0f);
    glVertex3f(ox + axisLen, oy, 0.0f);

    // Y 轴 - 绿色
    glColor3f(0.3f, 1.0f, 0.3f);
    glVertex3f(ox, oy, 0.0f);
    glVertex3f(ox, oy + axisLen, 0.0f);

    // Z 轴 - 蓝色
    glColor3f(0.3f, 0.3f, 1.0f);
    glVertex3f(ox, oy, 0.0f);
    glVertex3f(ox, oy, axisLen);
    glEnd();
}

void ToolpathView::drawToolpath()
{
    if (m_segments.isEmpty()) return;

    float scale = m_viewScale * m_zoom;
    float ox = m_panX * m_zoom;
    float oy = m_panY * m_zoom;

    for (const auto &seg : m_segments) {
        QVector3D from(seg.x0 * scale + ox, seg.y0 * scale + oy, seg.z0 * scale);
        QVector3D to(seg.x1 * scale + ox, seg.y1 * scale + oy, seg.z1 * scale);

        switch (seg.type) {
        case PathType::Rapid:
            // 快速移动 - 深灰色虚线
            glLineWidth(1.0f);
            glColor4f(0.4f, 0.4f, 0.4f, 0.6f);
            glEnable(GL_LINE_STIPPLE);
            glLineStipple(2, 0xAAAA);
            glBegin(GL_LINES);
            glVertex3f(from.x(), from.y(), from.z());
            glVertex3f(to.x(), to.y(), to.z());
            glEnd();
            glDisable(GL_LINE_STIPPLE);
            break;

        case PathType::Linear:
            // 直线插补 - 青色
            glLineWidth(2.0f);
            glColor3f(0.0f, 0.82f, 1.0f);
            glBegin(GL_LINES);
            glVertex3f(from.x(), from.y(), from.z());
            glVertex3f(to.x(), to.y(), to.z());
            glEnd();
            break;

        case PathType::ArcCW:
        case PathType::ArcCCW:
            // 圆弧 - 黄色
            glLineWidth(2.0f);
            glColor3f(1.0f, 0.85f, 0.0f);

            // 简化圆弧绘制（用直线段近似）
            {
                float cx = from.x() + seg.i * scale;
                float cy = from.y() + seg.j * scale;
                float radius = std::sqrt(seg.i * seg.i + seg.j * seg.j) * scale;
                if (radius < 0.001f) radius = 0.001f;

                float startAngle = std::atan2(from.y() - cy, from.x() - cx);
                float endAngle = std::atan2(to.y() - cy, to.x() - cx);

                int segments = std::max(8, static_cast<int>(radius * 0.5f));
                float angleStep = (endAngle - startAngle) / segments;

                // 确保正确的圆弧方向
                if (seg.type == PathType::ArcCW) {
                    if (angleStep > 0) angleStep -= 2.0f * M_PI;
                } else {
                    if (angleStep < 0) angleStep += 2.0f * M_PI;
                }

                glBegin(GL_LINE_STRIP);
                for (int i = 0; i <= segments; ++i) {
                    float angle = startAngle + angleStep * i;
                    float x = cx + radius * std::cos(angle);
                    float y = cy + radius * std::sin(angle);
                    glVertex3f(x, y, from.z());
                }
                glEnd();
            }
            break;
        }
    }
}

void ToolpathView::drawToolPosition()
{
    if (!m_toolPosValid) return;

    float scale = m_viewScale * m_zoom;
    float ox = m_panX * m_zoom;
    float oy = m_panY * m_zoom;

    float x = m_toolPos.x() * scale + ox;
    float y = m_toolPos.y() * scale + oy;
    float z = m_toolPos.z() * scale;

    // 绘制十字标记
    float crossSize = 8.0f;
    glLineWidth(3.0f);
    glColor3f(1.0f, 0.0f, 0.0f);  // 红色

    glBegin(GL_LINES);
    glVertex3f(x - crossSize, y, z);
    glVertex3f(x + crossSize, y, z);
    glVertex3f(x, y - crossSize, z);
    glVertex3f(x, y + crossSize, z);
    glEnd();

    // 绘制到 Z=0 的投影线
    glLineWidth(1.0f);
    glColor4f(1.0f, 0.0f, 0.0f, 0.3f);
    glBegin(GL_LINES);
    glVertex3f(x, y, z);
    glVertex3f(x, y, 0.0f);
    glEnd();
}

void ToolpathView::drawLine3D(const QVector3D &from, const QVector3D &to,
                               const QVector3D &color, float width)
{
    glLineWidth(width);
    glColor3f(color.x(), color.y(), color.z());
    glBegin(GL_LINES);
    glVertex3f(from.x(), from.y(), from.z());
    glVertex3f(to.x(), to.y(), to.z());
    glEnd();
}

void ToolpathView::setup3DProjection()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float aspect = static_cast<float>(width()) / static_cast<float>(height());
    float size = 500.0f;

    if (m_topView) {
        // 正交投影（俯视图）
        glOrtho(-size * aspect, size * aspect, -size, size, -10000.0f, 10000.0f);
    } else {
        // 透视投影（3D 视图）
        float nearPlane = 1.0f;
        float farPlane = 10000.0f;
        float fov = 45.0f;
        float top = nearPlane * std::tan(fov * M_PI / 360.0f);
        float bottom = -top;
        float left = bottom * aspect;
        float right = top * aspect;
        glFrustum(left, right, bottom, top, nearPlane, farPlane);
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // 应用视图变换
    glTranslatef(0.0f, 0.0f, -500.0f);

    if (!m_topView) {
        glRotatef(m_rotationX, 1.0f, 0.0f, 0.0f);
        glRotatef(m_rotationZ, 0.0f, 0.0f, 1.0f);
    }
}

void ToolpathView::setup2DProjection()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width(), height(), 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void ToolpathView::updateViewMatrix()
{
    // 视图矩阵在 paintGL 中设置
    update();
}

// ============================================================================
// 鼠标交互
// ============================================================================

void ToolpathView::mousePressEvent(QMouseEvent *event)
{
    m_lastMousePos = event->pos();

    if (event->button() == Qt::LeftButton) {
        m_isRotating = true;
    } else if (event->button() == Qt::MiddleButton || event->button() == Qt::RightButton) {
        m_isPanning = true;
    }
}

void ToolpathView::mouseMoveEvent(QMouseEvent *event)
{
    QPointF delta = event->pos() - m_lastMousePos;

    if (m_isRotating && !m_topView) {
        m_rotationZ += static_cast<float>(delta.x()) * 0.5f;
        m_rotationX += static_cast<float>(delta.y()) * 0.5f;
        m_rotationX = std::clamp(m_rotationX, -90.0f, 90.0f);
        update();
    } else if (m_isPanning) {
        m_panX += static_cast<float>(delta.x()) / (m_viewScale * m_zoom);
        m_panY -= static_cast<float>(delta.y()) / (m_viewScale * m_zoom);
        update();
    }

    m_lastMousePos = event->pos();
}

void ToolpathView::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    m_isRotating = false;
    m_isPanning = false;
}

void ToolpathView::wheelEvent(QWheelEvent *event)
{
    float delta = event->angleDelta().y() / 120.0f;
    m_zoom *= std::pow(1.1f, delta);
    m_zoom = std::clamp(m_zoom, 0.01f, 100.0f);
    update();
}
