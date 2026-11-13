#include "PixelDocument.h"

#include <QPainter>
#include <QQueue>

PixelDocument::PixelDocument()
{
    create(64, 64);
    m_palette = {Qt::black, Qt::white, QColor("#e74c3c"), QColor("#f1c40f"),
                 QColor("#2ecc71"), QColor("#3498db"), QColor("#9b59b6"), QColor("#8b5a2b")};
}

const QImage &PixelDocument::image() const
{
    static const QImage empty;
    if (m_layers.isEmpty() || m_activeLayer < 0 || m_activeLayer >= m_layers.size()) return empty;
    const auto &frames = m_layers[m_activeLayer].frames;
    return frames.isEmpty() ? empty : frames[qBound(0, m_activeFrame, frames.size() - 1)];
}

QImage &PixelDocument::image()
{
    return m_layers[m_activeLayer].frames[m_activeFrame];
}

QImage PixelDocument::compositeImage(int frame) const
{
    if (m_layers.isEmpty()) return {};
    frame = frame < 0 ? m_activeFrame : qBound(0, frame, frameCount() - 1);
    QImage result(image().size(), QImage::Format_ARGB32);
    result.fill(Qt::transparent);
    QPainter painter(&result);
    for (const PixelLayer &layer : m_layers) {
        if (!layer.visible || layer.frames.isEmpty()) continue;
        painter.setOpacity(qBound(0.0, layer.opacity, 1.0));
        painter.drawImage(0, 0, layer.frames[qMin(frame, layer.frames.size() - 1)]);
    }
    return result;
}

void PixelDocument::setImage(const QImage &source)
{
    const QImage converted = source.convertToFormat(QImage::Format_ARGB32);
    if (converted.isNull()) return;
    m_layers = {PixelLayer{QStringLiteral("Camada 1"), true, false, 1.0, {converted}}};
    m_activeLayer = m_activeFrame = 0;
    m_tileSize = converted.size();
}

bool PixelDocument::create(int width, int height, const QColor &background)
{
    if (width < 1 || height < 1 || width > 4096 || height > 4096) return false;
    QImage canvas(width, height, QImage::Format_ARGB32);
    canvas.fill(background);
    setImage(canvas);
    return !canvas.isNull();
}

bool PixelDocument::setPixel(int x, int y, const QColor &color)
{
    if (m_layers[m_activeLayer].locked || x < 0 || y < 0 || x >= image().width() || y >= image().height()) return false;
    if (image().pixelColor(x, y) == color) return false;
    image().setPixelColor(x, y, color);
    return true;
}

void PixelDocument::floodFill(int x, int y, const QColor &color)
{
    if (m_layers[m_activeLayer].locked || x < 0 || y < 0 || x >= image().width() || y >= image().height()) return;
    const QRgb target = image().pixel(x, y), replacement = color.rgba();
    if (target == replacement) return;
    QQueue<QPoint> pending;
    pending.enqueue(QPoint(x, y));
    while (!pending.isEmpty()) {
        const QPoint p = pending.dequeue();
        if (p.x() < 0 || p.y() < 0 || p.x() >= image().width() || p.y() >= image().height() || image().pixel(p) != target) continue;
        image().setPixel(p, replacement);
        pending.enqueue(p + QPoint(1, 0)); pending.enqueue(p + QPoint(-1, 0));
        pending.enqueue(p + QPoint(0, 1)); pending.enqueue(p + QPoint(0, -1));
    }
}

bool PixelDocument::resizeCanvas(int width, int height)
{
    if (width < 1 || height < 1 || width > 4096 || height > 4096) return false;
    const bool tileFillsCanvas = m_tileSize == image().size();
    for (PixelLayer &layer : m_layers) for (QImage &frame : layer.frames) {
        QImage resized(width, height, QImage::Format_ARGB32); resized.fill(Qt::transparent);
        QPainter painter(&resized); painter.drawImage(0, 0, frame); frame = resized;
    }
    if (tileFillsCanvas) m_tileSize = QSize(width, height);
    return true;
}

bool PixelDocument::addLayer(const QString &name)
{
    if (m_layers.size() >= 256) return false;
    PixelLayer layer; layer.name = name.isEmpty() ? QStringLiteral("Camada %1").arg(m_layers.size() + 1) : name;
    for (int i = 0; i < frameCount(); ++i) { QImage frame(image().size(), QImage::Format_ARGB32); frame.fill(Qt::transparent); layer.frames.push_back(frame); }
    m_layers.insert(m_activeLayer + 1, layer); ++m_activeLayer; return true;
}

bool PixelDocument::setLayers(const QVector<PixelLayer> &layers, int activeLayer, int activeFrame)
{
    if (layers.isEmpty() || layers.size() > 256 || activeLayer < 0 || activeLayer >= layers.size()) return false;
    const QSize dimensions = layers.first().frames.isEmpty() ? QSize() : layers.first().frames.first().size();
    if (dimensions.isEmpty() || dimensions.width() > 4096 || dimensions.height() > 4096) return false;
    const int frames = layers.first().frames.size();
    if (frames < 1 || frames > 512 || activeFrame < 0 || activeFrame >= frames) return false;
    for (const PixelLayer &layer : layers) {
        if (layer.frames.size() != frames) return false;
        for (const QImage &frame : layer.frames) if (frame.size() != dimensions) return false;
    }
    m_layers = layers;
    for (PixelLayer &layer : m_layers) for (QImage &frame : layer.frames) frame = frame.convertToFormat(QImage::Format_ARGB32);
    m_activeLayer = activeLayer; m_activeFrame = activeFrame;
    return true;
}

bool PixelDocument::removeLayer(int index)
{
    index = index < 0 ? m_activeLayer : index;
    if (m_layers.size() <= 1 || index < 0 || index >= m_layers.size()) return false;
    m_layers.removeAt(index); m_activeLayer = qBound(0, m_activeLayer - (index <= m_activeLayer ? 1 : 0), m_layers.size() - 1); return true;
}

bool PixelDocument::moveLayer(int from, int to)
{
    if (from < 0 || from >= m_layers.size() || to < 0 || to >= m_layers.size()) return false;
    m_layers.move(from, to);
    if (m_activeLayer == from) m_activeLayer = to;
    else if (from < m_activeLayer && to >= m_activeLayer) --m_activeLayer;
    else if (from > m_activeLayer && to <= m_activeLayer) ++m_activeLayer;
    return true;
}

void PixelDocument::setActiveLayer(int index) { if (index >= 0 && index < m_layers.size()) m_activeLayer = index; }
void PixelDocument::updateLayer(int index, const QString &name, bool visible, bool locked, qreal opacity)
{
    if (index < 0 || index >= m_layers.size()) return;
    PixelLayer &layer = m_layers[index]; layer.name = name; layer.visible = visible; layer.locked = locked; layer.opacity = qBound(0.0, opacity, 1.0);
}

int PixelDocument::frameCount() const { return m_layers.isEmpty() ? 0 : m_layers.first().frames.size(); }
void PixelDocument::setActiveFrame(int index) { if (index >= 0 && index < frameCount()) m_activeFrame = index; }
bool PixelDocument::addFrame(bool duplicate)
{
    if (frameCount() >= 512) return false;
    for (PixelLayer &layer : m_layers) {
        QImage frame(image().size(), QImage::Format_ARGB32); frame.fill(Qt::transparent);
        if (duplicate && !layer.frames.isEmpty()) frame = layer.frames[m_activeFrame].copy();
        layer.frames.insert(m_activeFrame + 1, frame);
    }
    ++m_activeFrame; return true;
}
bool PixelDocument::removeFrame(int index)
{
    index = index < 0 ? m_activeFrame : index;
    if (frameCount() <= 1 || index < 0 || index >= frameCount()) return false;
    for (PixelLayer &layer : m_layers) layer.frames.removeAt(index);
    m_activeFrame = qBound(0, m_activeFrame - (index <= m_activeFrame ? 1 : 0), frameCount() - 1); return true;
}
bool PixelDocument::moveFrame(int from, int to)
{
    if (from < 0 || from >= frameCount() || to < 0 || to >= frameCount()) return false;
    for (PixelLayer &layer : m_layers) layer.frames.move(from, to);
    if (m_activeFrame == from) m_activeFrame = to;
    else if (from < m_activeFrame && to >= m_activeFrame) --m_activeFrame;
    else if (from > m_activeFrame && to <= m_activeFrame) ++m_activeFrame;
    return true;
}

void PixelDocument::addPaletteColor(const QColor &color) { if (color.isValid() && !m_palette.contains(color)) m_palette.push_back(color); }
void PixelDocument::setTileMetadata(int tileId, const TileMetadata &metadata)
{
    if (tileId < 0 || tileId >= 65536) return;
    while (m_tileMetadata.size() <= tileId) m_tileMetadata.push_back({});
    m_tileMetadata[tileId] = metadata;
    m_tileMetadata[tileId].autoTileMask = qBound(0, metadata.autoTileMask, 15);
}
TileMetadata PixelDocument::tileMetadata(int tileId) const { return tileId >= 0 && tileId < m_tileMetadata.size() ? m_tileMetadata[tileId] : TileMetadata{}; }
int PixelDocument::autoTileId(const QString &terrainTag, int neighborMask) const
{
    const int mask=qBound(0,neighborMask,15);
    for(int id=0;id<m_tileMetadata.size();++id){const TileMetadata &metadata=m_tileMetadata[id];if(metadata.terrain&&metadata.tag==terrainTag&&metadata.autoTileMask==mask)return id;}
    return -1;
}
