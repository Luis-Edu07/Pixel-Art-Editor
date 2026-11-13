#pragma once

#include "core/PixelDocument.h"

#include <QWidget>
#include <QPolygon>
#include <QImage>

class CanvasView : public QWidget
{
    Q_OBJECT
public:
    enum class Tool { Pencil, Eraser, Fill, Eyedropper, Pan, Zoom, Line, Rectangle, Ellipse, SelectRectangle, SelectFree };
    explicit CanvasView(PixelDocument *document, QWidget *parent = nullptr);
    void setTool(Tool tool);
    Tool tool() const { return m_tool; }
    void setColor(const QColor &color) { m_color = color; }
    QColor color() const { return m_color; }
    void setBrushSize(int size) { m_brushSize = qBound(1, size, 64); }
    int brushSize() const { return m_brushSize; }
    void setGridVisible(bool visible);
    void setZoom(int percent);
    int zoom() const { return m_zoom; }
    void setImage(const QImage &image);
    void setOnionSkin(bool enabled) { m_onionSkin = enabled; update(); }
    QPoint pixelAt(const QPoint &position) const;
    bool hasSelection() const { return m_selectionActive; }
    QRect selectionRect() const { return m_selectionRect; }
    void copySelection();
    void cutSelection();
    void pasteSelection();
    void flipSelection(Qt::Orientation orientation);
    void rotateSelection();
    void resizeSelection(const QSize &size);

signals:
    void editStarted();
    void editFinished();
    void colorPicked(const QColor &color);
    void cursorPixelChanged(const QPoint &point);
    void zoomChanged(int percent);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    QRectF imageRect() const;
    void applyAt(const QPoint &position);
    void drawLine(const QPoint &from, const QPoint &to);
    QPolygon selectionInPixels() const;
    QImage selectedImage() const;
    void clearSelectionPixels();
    void paintSelectionImage();
    PixelDocument *m_document;
    Tool m_tool{Tool::Pencil};
    QColor m_color{Qt::black};
    int m_brushSize{1};
    bool m_grid{true};
    bool m_onionSkin{false};
    bool m_dragging{false};
    bool m_panning{false};
    int m_zoom{800};
    QPoint m_lastMouse;
    QPoint m_lastDrawPixel{-1, -1};
    QPoint m_startPixel;
    QPoint m_panOffset{0, 0};
    bool m_selecting{false};
    bool m_freeSelecting{false};
    bool m_movingSelection{false};
    bool m_selectionActive{false};
    QRect m_selectionRect;
    QPolygon m_selectionPolygon;
    QPolygon m_previewPolygon;
    QImage m_movingImage;
    QPoint m_moveOrigin;
    QPoint m_moveCurrent;
};
