#ifndef SCOPECHANNEL_H
#define SCOPECHANNEL_H

#include <QObject>
#include <QColor>
#include <QVector>
#include <QString>

/**
 * @brief 示波器通道数据类
 *
 * 管理单个示波器通道的数据缓冲区、显示属性和采样。
 * 每个通道对应一个 HAL pin 数据源。
 */
class ScopeChannel : public QObject
{
    Q_OBJECT

public:
    explicit ScopeChannel(QObject *parent = nullptr);
    ~ScopeChannel() override = default;

    /**
     * @brief 获取通道名称
     */
    QString name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }

    /**
     * @brief 获取/设置 HAL pin 名称（数据源）
     */
    QString pinName() const { return m_pinName; }
    void setPinName(const QString &pin) { m_pinName = pin; }

    /**
     * @brief 获取/设置通道颜色
     */
    QColor color() const { return m_color; }
    void setColor(const QColor &color) { m_color = color; }

    /**
     * @brief 获取/设置垂直缩放（V/div）
     */
    double scale() const { return m_scale; }
    void setScale(double scale) { m_scale = scale; }

    /**
     * @brief 获取/设置垂直偏移
     */
    double offset() const { return m_offset; }
    void setOffset(double offset) { m_offset = offset; }

    /**
     * @brief 获取/设置通道使能状态
     */
    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled) { m_enabled = enabled; }

    /**
     * @brief 添加一个采样值到缓冲区
     * @param value 采样值
     */
    void addSample(double value);

    /**
     * @brief 清除所有采样数据
     */
    void clear();

    /**
     * @brief 获取指定索引的采样值
     * @param index 索引位置
     * @return 采样值
     */
    double valueAt(int index) const;

    /**
     * @brief 获取当前采样数量
     */
    int sampleCount() const { return m_sampleCount; }

    /**
     * @brief 获取缓冲区容量
     */
    int bufferSize() const { return m_bufferSize; }

    /**
     * @brief 设置缓冲区容量
     * @param size 缓冲区大小
     */
    void setBufferSize(int size);

    /**
     * @brief 获取缓冲区数据（用于绘制）
     * @return 采样值向量
     */
    const QVector<double>& data() const { return m_buffer; }

    /**
     * @brief 获取通道索引
     */
    int index() const { return m_index; }
    void setIndex(int idx) { m_index = idx; }

signals:
    /**
     * @brief 数据更新信号
     */
    void dataUpdated();

private:
    QString m_name;           // 通道名称
    QString m_pinName;         // HAL pin 名称
    QColor m_color;            // 显示颜色
    double m_scale;            // 垂直缩放（V/div）
    double m_offset;           // 垂直偏移
    bool m_enabled;           // 是否使能

    QVector<double> m_buffer; // 环形缓冲区
    int m_bufferSize;          // 缓冲区容量
    int m_writePos;            // 写入位置
    int m_sampleCount;         // 当前采样数量
    int m_index;              // 通道索引
};

#endif // SCOPECHANNEL_H
