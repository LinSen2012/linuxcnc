#include "ScopeChannel.h"

// ============================================================
// 构造
// ============================================================

ScopeChannel::ScopeChannel(QObject *parent)
    : QObject(parent)
    , m_color(QColor(0, 255, 0))       // 默认绿色
    , m_scale(1.0)                       // 默认 1V/div
    , m_offset(0.0)
    , m_enabled(true)
    , m_bufferSize(4096)                 // 默认 4096 点
    , m_writePos(0)
    , m_sampleCount(0)
    , m_index(0)
{
    m_buffer.resize(m_bufferSize);
    m_buffer.fill(0.0);
}

// ============================================================
// 数据操作
// ============================================================

void ScopeChannel::addSample(double value)
{
    if (m_bufferSize <= 0) return;

    // 环形缓冲区写入
    m_buffer[m_writePos] = value;
    m_writePos = (m_writePos + 1) % m_bufferSize;

    // 更新采样计数（不超过缓冲区大小）
    if (m_sampleCount < m_bufferSize) {
        m_sampleCount++;
    }

    emit dataUpdated();
}

void ScopeChannel::clear()
{
    m_buffer.fill(0.0);
    m_writePos = 0;
    m_sampleCount = 0;
}

double ScopeChannel::valueAt(int index) const
{
    if (index < 0 || index >= m_sampleCount) {
        return 0.0;
    }

    // 从环形缓冲区读取（从最早的数据开始）
    int readPos = (m_writePos - m_sampleCount + index + m_bufferSize) % m_bufferSize;
    return m_buffer[readPos];
}

void ScopeChannel::setBufferSize(int size)
{
    if (size <= 0) return;

    m_bufferSize = size;
    m_buffer.resize(size);
    m_buffer.fill(0.0);
    m_writePos = 0;
    m_sampleCount = 0;
}
