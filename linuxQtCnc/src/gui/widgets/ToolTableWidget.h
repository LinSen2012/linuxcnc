/*
 * linuxQtCncGui - LinuxCNC Qt5 GUI 控制器
 * ToolTableWidget - 刀具表编辑器
 *
 * 显示和编辑 LinuxCNC 刀具表。
 * 支持刀具参数修改、添加、删除操作。
 */

#ifndef TOOLTABLEWIDGET_H
#define TOOLTABLEWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QHeaderView>

// ============================================================================
// 刀具数据结构
// ============================================================================

struct ToolData {
    int pocketNo = 0;       // 刀位号
    int toolNo = 0;         // 刀具号
    double xOffset = 0.0;   // X 偏置
    double yOffset = 0.0;   // Y 偏置
    double zOffset = 0.0;   // Z 偏置
    double diameter = 0.0;  // 直径
    double frontAngle = 0.0;// 前角
    double backAngle = 0.0; // 后角
    int orientation = 0;    // 方向
};

class ToolTableWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父组件
     */
    explicit ToolTableWidget(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~ToolTableWidget() override;

    // =========================================================================
    // 数据操作
    // =========================================================================

    /**
     * @brief 加载刀具表
     * @param filename 刀具表文件路径
     * @return 是否成功
     */
    bool loadFromFile(const QString &filename);

    /**
     * @brief 保存刀具表
     * @param filename 刀具表文件路径
     * @return 是否成功
     */
    bool saveToFile(const QString &filename) const;

    /**
     * @brief 获取刀具数据
     * @param pocket 刀位号
     * @return 刀具数据
     */
    ToolData toolAt(int pocket) const;

    /**
     * @brief 设置刀具数据
     * @param pocket 刀位号
     * @param data 刀具数据
     */
    void setToolAt(int pocket, const ToolData &data);

    /**
     * @brief 获取当前选中的刀具
     * @return 刀具号，未选中返回 0
     */
    int currentTool() const;

    /**
     * @brief 清除所有刀具数据
     */
    void clear();

signals:
    /**
     * @brief 刀具改变信号
     * @param toolNo 刀具号
     */
    void toolChanged(int toolNo);

    /**
     * @brief 数据修改信号
     */
    void dataModified();

public slots:
    /**
     * @brief 添加新刀具
     */
    void addTool();

    /**
     * @brief 删除选中刀具
     */
    void deleteTool();

    /**
     * @brief 刷新刀具表显示
     */
    void refresh();

private:
    /**
     * @brief 初始化 UI
     */
    void setupUi();

    /**
     * @brief 创建刀具表格
     */
    void createToolTable();

    /**
     * @brief 更新刀具详情面板
     */
    void updateToolDetails();

    /**
     * @brief 获取选中行
     * @return 行号，-1 表示无选中
     */
    int selectedRow() const;

private slots:
    /**
     * @brief 表格选择改变
     */
    void onSelectionChanged();

    /**
     * @brief 单元格修改
     */
    void onCellChanged(int row, int column);

    /**
     * @brief 应用修改到刀具
     */
    void applyChanges();

private:
    // UI 组件
    QTableWidget *m_toolTable = nullptr;
    QGroupBox *m_detailsGroup = nullptr;

    // 刀具参数编辑
    QSpinBox *m_pocketSpin = nullptr;
    QSpinBox *m_toolNoSpin = nullptr;
    QDoubleSpinBox *m_xOffsetSpin = nullptr;
    QDoubleSpinBox *m_yOffsetSpin = nullptr;
    QDoubleSpinBox *m_zOffsetSpin = nullptr;
    QDoubleSpinBox *m_diameterSpin = nullptr;
    QDoubleSpinBox *m_frontAngleSpin = nullptr;
    QDoubleSpinBox *m_backAngleSpin = nullptr;
    QComboBox *m_orientationCombo = nullptr;

    QPushButton *m_addBtn = nullptr;
    QPushButton *m_deleteBtn = nullptr;
    QPushButton *m_applyBtn = nullptr;

    // 数据
    QVector<ToolData> m_tools;
    bool m_modified = false;
};

#endif // TOOLTABLEWIDGET_H