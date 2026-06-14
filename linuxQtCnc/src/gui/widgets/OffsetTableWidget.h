/*
 * linuxQtCncGui - LinuxCNC Qt5 GUI 控制器
 * OffsetTableWidget - 坐标系偏置表
 *
 * 显示和编辑 G54-G59.3 工件坐标系偏置。
 * 支持查看和修改各坐标系的 X/Y/Z/A/B/C 轴偏置。
 */

#ifndef OFFSETTABLEWIDGET_H
#define OFFSETTABLEWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QStringList>

// ============================================================================
// 坐标系数据结构
// ============================================================================

struct CoordinateSystem {
    QString name;          // 坐标系名称 (G54, G55, ...)
    QString description;   // 描述
    double offsets[6];      // X, Y, Z, A, B, C 轴偏置
    bool selected = false; // 是否选中
};

class OffsetTableWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父组件
     */
    explicit OffsetTableWidget(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~OffsetTableWidget() override;

    // =========================================================================
    // 数据操作
    // =========================================================================

    /**
     * @brief 从 INI 文件加载偏置数据
     * @param filename INI 文件路径
     * @return 是否成功
     */
    bool loadFromIni(const QString &filename);

    /**
     * @brief 保存偏置数据到 INI 文件
     * @param filename INI 文件路径
     * @return 是否成功
     */
    bool saveToIni(const QString &filename) const;

    /**
     * @brief 获取坐标系数据
     * @param index 坐标系索引 (0=G54, 1=G55, ...)
     * @return 坐标系数据
     */
    CoordinateSystem coordinateSystem(int index) const;

    /**
     * @brief 设置坐标系偏置
     * @param index 坐标系索引
     * @param offsets 偏置数组 [X, Y, Z, A, B, C]
     */
    void setCoordinateSystem(int index, const double offsets[6]);

    /**
     * @brief 获取当前选中的坐标系
     * @return 坐标系索引，-1 表示无选中
     */
    int currentSystem() const;

    /**
     * @brief 清除所有偏置数据
     */
    void clear();

signals:
    /**
     * @brief 偏置改变信号
     * @param index 坐标系索引
     * @param axis 轴索引 (0-5)
     * @param value 新的偏置值
     */
    void offsetChanged(int index, int axis, double value);

    /**
     * @brief 坐标系选中信号
     * @param index 坐标系索引
     */
    void systemSelected(int index);

public slots:
    /**
     * @brief 刷新显示
     */
    void refresh();

    /**
     * @brief 应用当前修改
     */
    void applyChanges();

    /**
     * @brief 复位当前坐标系
     */
    void resetCurrent();

    /**
     * @brief 复制选中坐标系到目标
     * @param target 目标坐标系索引
     */
    void copyToSystem(int target);

private:
    /**
     * @brief 初始化 UI
     */
    void setupUi();

    /**
     * @brief 创建偏置表格
     */
    void createOffsetTable();

    /**
     * @brief 更新详情面板
     */
    void updateDetails();

private slots:
    /**
     * @brief 表格选择改变
     */
    void onSelectionChanged();

    /**
     * @brief 单元格修改
     */
    void onCellChanged(int row, int column);

private:
    // UI 组件
    QTableWidget *m_offsetTable = nullptr;
    QGroupBox *m_detailsGroup = nullptr;

    // 偏置编辑
    QVector<QDoubleSpinBox *> m_offsetSpins;  // 6 个轴的偏置输入

    // 操作按钮
    QPushButton *m_applyBtn = nullptr;
    QPushButton *m_resetBtn = nullptr;
    QPushButton *m_copyBtn = nullptr;

    // 数据
    QVector<CoordinateSystem> m_systems;
    int m_currentRow = -1;
};

#endif // OFFSETTABLEWIDGET_H