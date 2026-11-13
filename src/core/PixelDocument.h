#pragma once

#include <QColor>
#include <QImage>
#include <QPoint>
#include <QRect>
#include <QStringList>
#include <QVector>

struct PixelLayer {
    QString name;
    bool visible{true};
    bool locked{false};
    qreal opacity{1.0};
    QVector<QImage> frames;
};

struct TileMetadata {
    QString tag;
    bool collision{false};
    bool terrain{false};
    int autoTileMask{0};
};

class PixelDocument
{
public:
    PixelDocument();
    const QImage &image() const;
    QImage &image();
    QImage compositeImage(int frame = -1) const;
    void setImage(const QImage &image);
    bool create(int width, int height, const QColor &background = Qt::transparent);
    bool setPixel(int x, int y, const QColor &color);
    void floodFill(int x, int y, const QColor &color);
    bool resizeCanvas(int width, int height);

    int layerCount() const { return m_layers.size(); }
    int activeLayer() const { return m_activeLayer; }
    const QVector<PixelLayer> &layers() const { return m_layers; }
    bool setLayers(const QVector<PixelLayer> &layers, int activeLayer = 0, int activeFrame = 0);
    bool addLayer(const QString &name = QString());
    bool removeLayer(int index = -1);
    bool moveLayer(int from, int to);
    void setActiveLayer(int index);
    void updateLayer(int index, const QString &name, bool visible, bool locked, qreal opacity);

    int frameCount() const;
    int activeFrame() const { return m_activeFrame; }
    void setActiveFrame(int index);
    bool addFrame(bool duplicate = false);
    bool removeFrame(int index = -1);
    bool moveFrame(int from, int to);
    int fps() const { return m_fps; }
    void setFps(int fps) { m_fps = qBound(1, fps, 60); }
    bool looping() const { return m_looping; }
    void setLooping(bool looping) { m_looping = looping; }

    const QVector<QColor> &palette() const { return m_palette; }
    void setPalette(const QVector<QColor> &colors) { m_palette = colors; }
    void addPaletteColor(const QColor &color);
    QSize tileSize() const { return m_tileSize; }
    void setTileSize(const QSize &size) { m_tileSize = size; }
    const QVector<TileMetadata> &tileMetadata() const { return m_tileMetadata; }
    void setTileMetadata(int tileId, const TileMetadata &metadata);
    TileMetadata tileMetadata(int tileId) const;
    int autoTileId(const QString &terrainTag, int neighborMask) const;

private:
    QVector<PixelLayer> m_layers;
    int m_activeLayer{0};
    int m_activeFrame{0};
    int m_fps{12};
    bool m_looping{true};
    QVector<QColor> m_palette;
    QSize m_tileSize{16, 16};
    QVector<TileMetadata> m_tileMetadata;
};
