/*
 * linuxQtCncGui - LinuxCNC Qt6 GUI 控制器
 * ToolpathView - 3D 刀具路径 OpenGL 渲染视图
 *
 * 基于 QOpenGLWidget 的 3D 刀具路径预览组件。
 * 支持：
 * - 3D 视角旋转/缩放/平移
 * - 刀具路径渲染（快移/直线/圆弧）
 * - 当前刀具位置指示
 * - 坐标轴显示
 * - 网格显示
 */

#ifndef TOOLPATHVIEW_H
#define TOOLPATHVIEW_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <QVector3D>
#include <QVector>
#include <QPointF>

#include "GCodeParser.h"

/**
 * @brief ToolpathView - 3D 刀具路径 OpenGL 渲染视图
 *
 * 使用 OpenGL 渲染 G 代码解析后的刀具路径。
 * 支持鼠标交互控制视角。
 */
class ToolpathView : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父组件
     */
    explicit ToolpathView(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~ToolpathView() override;

    /**
     * @brief 加载 G 代码文件并解析渲染
     * @param filename 文件路径
     */
    void loadGCode(const QString &filename);

    /**
     * @brief 设置路径数据（直接设置已解析的路径段）
     * @param segments 路径段列表
     */
    void setPathData(const QVector<PathSegment> &segments);

    /**
     * @brief 更新当前刀具位置
     * @param pos 刀具位置
     */
    void updateToolPosition(const QVector3D &pos);

    /**
     * @brief 清除路径数据
     */
    void clearPath();

    /**
     * @brief 设置视图模式
     * @param topView true=俯视图, false=3D 透视
     */
    void setTopView(bool topView);

    /**
     * @brief 适配视图到路径边界
     */
    void fitToView();

    /**
     * @brief 设置网格显示
     * @param show true=显示网格
     */
    void setShowGrid(bool show);

    /**
     * @brief 设置坐标轴显示
     * @param show true=显示坐标轴
     */
    void setShowAxes(bool show);

protected:
    /**
     * @brief OpenGL 初始化
     */
    void initializeGL() override;

    /**
     * @brief 窗口大小改变
     * @param w 宽度
     * @param h 高度
     */
    void resizeGL(int w, int h) override;

    /**
     * @brief OpenGL 渲染
     */
    void paintGL() override;

    /**
     * @brief 鼠标按下事件
     */
    void mousePressEvent(QMouseEvent *event) override;

    /**
     * @brief 鼠标移动事件
     */
    void mouseMoveEvent(QMouseEvent *event) override;

    /**
     * @brief 鼠标释放事件
     */
    void mouseReleaseEvent(QMouseEvent *event) override;

    /**
     * @brief 滚轮事件（缩放）
     */
    void wheelEvent(QWheelEvent *event) override;

private:
    /**
     * @brief 绘制网格
     */
    void drawGrid();

    /**
     * @brief 绘制坐标轴
     */
    void drawAxes();

    /**
     * @brief 绘制刀具路径
     */
    void drawToolpath();

    /**
     * @brief 绘制当前刀具位置
     */
    void drawToolPosition();

    /**
     * @brief 绘制一条线段
     * @param from 起点
     * @param to 终点
     * @param color 颜色
     * @param width 线宽
     */
    void drawLine3D(const QVector3D &from, const QVector3D &to,
                     const QVector3D &color, float width);

    /**
     * @brief 设置 2D 投影（用于绘制 HUD 元素）
     */
    void setup2DProjection();

    /**
     * @brief 设置 3D 投影（用于绘制 3D 场景）
     */
    void setup3DProjection();

    /**
     * @brief 更新视图矩阵
     */
    void updateViewMatrix();

private:
    // 路径数据
    QVector<PathSegment> m_segments;  ///< 路径段列表
    QVector3D m_toolPos;              ///< 当前刀具位置
    bool m_toolPosValid = false;      ///< 刀具位置是否有效

    // 视图参数
    float m_rotationX = 30.0f;        ///< X 轴旋转角度（度）
    float m_rotationZ = -45.0f;       ///< Z 轴旋转角度（度）
    float m_zoom = 1.0f;              ///< 缩放比例
    float m_panX = 0.0f;             ///< X 平移
    float m_panY = 0.0f;             ///< Y 平移
    float m_viewScale = 1.0f;         ///< 视图缩放（自动适配）

    // 鼠标交互
    QPointF m_lastMousePos;           ///< 上次鼠标位置
    bool m_isRotating = false;        ///< 正在旋转
    bool m_isPanning = false;         ///< 正在平移

    // 显示选项
    bool m_showGrid = true;           ///< 显示网格
    bool m_showAxes = true;           ///< 显示坐标轴
    bool m_topView = false;           ///< 俯视图模式

    // OpenGL 资源
    QMatrix4x4 m_projectionMatrix;    ///< 投影矩阵
    QMatrix4x4 m_viewMatrix;         ///< 视图矩阵
    QMatrix4x4 m_modelMatrix;        ///< 模型矩阵

    // G 代码解析器
    GCodeParser *m_parser = nullptr;
};

#endif // TOOLPATHVIEW_H
