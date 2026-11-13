#include "CanvasView.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <QtMath>

CanvasView::CanvasView(PixelDocument *document, QWidget *parent)
    : QWidget(parent), m_document(document)
{
    setMinimumSize(420, 320);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setBackgroundRole(QPalette::Dark);
}

void CanvasView::setTool(Tool tool) { m_tool = tool; setCursor(tool == Tool::Pan ? Qt::OpenHandCursor : Qt::CrossCursor); }
void CanvasView::setGridVisible(bool visible) { m_grid = visible; update(); }
void CanvasView::setZoom(int percent) { m_zoom = qBound(25, percent, 3200); emit zoomChanged(m_zoom); update(); }
void CanvasView::setImage(const QImage &image) { m_document->setImage(image); update(); }

QRectF CanvasView::imageRect() const
{
    const qreal scale = m_zoom / 100.0;
    const QSizeF size(m_document->image().width() * scale, m_document->image().height() * scale);
    return QRectF((width() - size.width()) / 2 + m_panOffset.x(), (height() - size.height()) / 2 + m_panOffset.y(), size.width(), size.height());
}

QPoint CanvasView::pixelAt(const QPoint &position) const
{
    const QRectF rect = imageRect();
    if (!rect.contains(position)) return QPoint(-1, -1);
    const qreal scaleX = rect.width() / m_document->image().width();
    const qreal scaleY = rect.height() / m_document->image().height();
    return QPoint(qFloor((position.x() - rect.left()) / scaleX), qFloor((position.y() - rect.top()) / scaleY));
}

void CanvasView::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor("#303238"));
    const QRectF target = imageRect();
    painter.fillRect(target, QColor("#e8e8e8"));
    const int checker = 4;
    for (int y = 0; y < m_document->image().height(); y += checker) {
        for (int x = 0; x < m_document->image().width(); x += checker) {
            const int cellWidth = qMin(checker, m_document->image().width() - x);
            const int cellHeight = qMin(checker, m_document->image().height() - y);
            const QRectF cell(target.left() + x * target.width() / m_document->image().width(),
                              target.top() + y * target.height() / m_document->image().height(),
                              cellWidth * target.width() / m_document->image().width(),
                              cellHeight * target.height() / m_document->image().height());
            painter.fillRect(cell, ((x / checker + y / checker) % 2) ? QColor("#d4d4d4") : QColor("#f0f0f0"));
        }
    }
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    if (m_onionSkin && m_document->frameCount() > 1) {
        const int previous = m_document->activeFrame() > 0 ? m_document->activeFrame() - 1 : (m_document->looping() ? m_document->frameCount() - 1 : -1);
        const int next = m_document->activeFrame() + 1 < m_document->frameCount() ? m_document->activeFrame() + 1 : (m_document->looping() ? 0 : -1);
        if (previous >= 0) { painter.setOpacity(0.22); painter.drawImage(target, m_document->compositeImage(previous)); }
        if (next >= 0) { painter.setOpacity(0.12); painter.drawImage(target, m_document->compositeImage(next)); }
        painter.setOpacity(1.0);
    }
    painter.drawImage(target, m_document->compositeImage());
    if (m_movingSelection && !m_movingImage.isNull()) {
        const qreal sx = target.width() / m_document->image().width(), sy = target.height() / m_document->image().height();
        painter.drawImage(QRectF(target.left()+m_moveCurrent.x()*sx,target.top()+m_moveCurrent.y()*sy,m_movingImage.width()*sx,m_movingImage.height()*sy),m_movingImage);
    }
    if (m_grid && m_zoom >= 400) {
        QPen pen(QColor(30, 30, 30, 65));
        pen.setWidth(0);
        painter.setPen(pen);
        const qreal sx = target.width() / m_document->image().width();
        const qreal sy = target.height() / m_document->image().height();
        for (int x = 0; x <= m_document->image().width(); ++x)
            painter.drawLine(QPointF(target.left() + x * sx, target.top()), QPointF(target.left() + x * sx, target.bottom()));
        for (int y = 0; y <= m_document->image().height(); ++y)
            painter.drawLine(QPointF(target.left(), target.top() + y * sy), QPointF(target.right(), target.top() + y * sy));
    }
    const QSize tile = m_document->tileSize();
    if (tile.width() > 0 && tile.height() > 0) {
        QPen tilePen(QColor(0, 115, 255, 255));
        tilePen.setWidth(2);
        painter.setPen(tilePen);
        const qreal sx = target.width() / m_document->image().width();
        const qreal sy = target.height() / m_document->image().height();
        for (int x = tile.width(); x < m_document->image().width(); x += tile.width())
            painter.drawLine(QPointF(target.left() + x * sx, target.top()), QPointF(target.left() + x * sx, target.bottom()));
        for (int y = tile.height(); y < m_document->image().height(); y += tile.height())
            painter.drawLine(QPointF(target.left(), target.top() + y * sy), QPointF(target.right(), target.top() + y * sy));
    }
    painter.setPen(QPen(QColor(20, 20, 24, 210), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(target.adjusted(0.5, 0.5, -0.5, -0.5));
    QPen selectionPen(Qt::white); selectionPen.setStyle(Qt::DashLine); selectionPen.setWidth(0);
    painter.setPen(selectionPen); painter.setBrush(Qt::NoBrush);
    QPolygon boundary = m_selecting && m_freeSelecting ? m_previewPolygon : (m_selectionActive ? m_selectionPolygon : QPolygon{});
    if (boundary.size() >= 2) {
        QPolygonF screenPolygon;
        for (const QPoint &point : boundary) screenPolygon << QPointF(target.left() + (point.x()+0.5)*target.width()/m_document->image().width(), target.top() + (point.y()+0.5)*target.height()/m_document->image().height());
        painter.drawPolygon(screenPolygon);
    }
    if (m_selecting && !m_freeSelecting && (m_tool == Tool::SelectRectangle || m_tool == Tool::Rectangle || m_tool == Tool::Ellipse || m_tool == Tool::Line)) {
        const QPoint a = m_startPixel, b = pixelAt(m_lastMouse);
        const QRectF shape(target.left()+qMin(a.x(),b.x())*target.width()/m_document->image().width(), target.top()+qMin(a.y(),b.y())*target.height()/m_document->image().height(), (qAbs(a.x()-b.x())+1)*target.width()/m_document->image().width(), (qAbs(a.y()-b.y())+1)*target.height()/m_document->image().height());
        if (m_tool == Tool::SelectRectangle) { painter.setPen(QPen(Qt::white,0,Qt::DashLine)); painter.drawRect(shape); }
        else { painter.setPen(QPen(m_color, qMax<qreal>(1,target.width()/m_document->image().width()))); if(m_tool==Tool::Line)painter.drawLine(target.topLeft()+QPointF((a.x()+0.5)*target.width()/m_document->image().width(),(a.y()+0.5)*target.height()/m_document->image().height()),target.topLeft()+QPointF((b.x()+0.5)*target.width()/m_document->image().width(),(b.y()+0.5)*target.height()/m_document->image().height())); else if(m_tool==Tool::Rectangle)painter.drawRect(shape); else painter.drawEllipse(shape); }
    }
}

void CanvasView::applyAt(const QPoint &position)
{
    const QPoint pixel = pixelAt(position);
    if (pixel.x() < 0) return;
    if (m_tool == Tool::Pencil) m_document->setPixel(pixel.x(), pixel.y(), m_color);
    else if (m_tool == Tool::Eraser) m_document->setPixel(pixel.x(), pixel.y(), Qt::transparent);
    update();
}

void CanvasView::drawLine(const QPoint &from, const QPoint &to)
{
    int x = from.x();
    int y = from.y();
    const int dx = qAbs(to.x() - x);
    const int sx = x < to.x() ? 1 : -1;
    const int dy = -qAbs(to.y() - y);
    const int sy = y < to.y() ? 1 : -1;
    int error = dx + dy;
    const QColor color = m_tool == Tool::Pencil ? m_color : QColor(Qt::transparent);
    const int brushSize = m_tool == Tool::Pencil ? m_brushSize : 1;
    const int brushOffset = brushSize / 2;

    while (true) {
        for (int brushY = 0; brushY < brushSize; ++brushY)
            for (int brushX = 0; brushX < brushSize; ++brushX)
                m_document->setPixel(x + brushX - brushOffset, y + brushY - brushOffset, color);
        if (x == to.x() && y == to.y()) break;
        const int twiceError = 2 * error;
        if (twiceError >= dy) { error += dy; x += sx; }
        if (twiceError <= dx) { error += dx; y += sy; }
    }
}

void CanvasView::mousePressEvent(QMouseEvent *event)
{
    m_lastMouse = event->pos();
    if (event->button() != Qt::LeftButton) return;
    const QPoint p = pixelAt(event->pos());
    if (p.x() < 0) return;
    if (m_tool == Tool::Eyedropper) {
        if (p.x() >= 0) emit colorPicked(m_document->compositeImage().pixelColor(p));
    } else if (m_tool == Tool::Fill) {
        const QPoint p = pixelAt(event->pos());
        if (p.x() >= 0) { emit editStarted(); m_document->floodFill(p.x(), p.y(), m_color); emit editFinished(); update(); }
    } else if (m_tool == Tool::Pan) {
        m_panning = true; setCursor(Qt::ClosedHandCursor);
    } else if (m_tool == Tool::Zoom) {
        setZoom(m_zoom < 3200 ? m_zoom * 2 : 25);
    } else if (m_tool == Tool::SelectRectangle || m_tool == Tool::SelectFree) {
        if (m_selectionActive && m_selectionRect.contains(p)) {
            m_startPixel = p;
            emit editStarted(); m_movingImage = selectedImage(); m_moveOrigin = m_selectionRect.topLeft(); m_moveCurrent = m_moveOrigin;
            clearSelectionPixels(); m_movingSelection = true; m_dragging = true;
        } else {
            m_selecting = true; m_freeSelecting = m_tool == Tool::SelectFree; m_startPixel = p;
            m_previewPolygon.clear(); m_previewPolygon << p;
        }
    } else if (m_tool == Tool::Line || m_tool == Tool::Rectangle || m_tool == Tool::Ellipse) {
        m_startPixel = p; m_selecting = true; emit editStarted();
    } else {
        emit editStarted(); m_dragging = true; m_lastDrawPixel = p; drawLine(p, p); update();
    }
}

void CanvasView::mouseMoveEvent(QMouseEvent *event)
{
    const QPoint p = pixelAt(event->pos());
    if (p.x() >= 0) emit cursorPixelChanged(p);
    if (m_selecting && m_freeSelecting && (event->buttons() & Qt::LeftButton)) { const QPoint p=pixelAt(event->pos()); if(p.x()>=0 && (m_previewPolygon.isEmpty() || (m_previewPolygon.last()-p).manhattanLength()>=1))m_previewPolygon<<p; }
    if (m_dragging && m_movingSelection && (event->buttons() & Qt::LeftButton)) { const QPoint p=pixelAt(event->pos()); if(p.x()>=0)m_moveCurrent=m_moveOrigin+(p-m_startPixel); }
    else if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        if ((m_tool == Tool::Pencil || m_tool == Tool::Eraser) && p.x() >= 0 && m_lastDrawPixel.x() >= 0) {
            drawLine(m_lastDrawPixel, p);
            m_lastDrawPixel = p;
        }
        update();
    }
    if (m_panning && (event->buttons() & Qt::LeftButton)) {
        m_panOffset += event->pos() - m_lastMouse; update();
    }
    m_lastMouse = event->pos(); update();
}

void CanvasView::mouseReleaseEvent(QMouseEvent *event)
{
    const QPoint p = pixelAt(event->pos());
    if (event->button() == Qt::LeftButton && m_dragging && !m_movingSelection
        && (m_tool == Tool::Pencil || m_tool == Tool::Eraser) && p.x() >= 0 && m_lastDrawPixel.x() >= 0) {
        drawLine(m_lastDrawPixel, p);
    }
    if (event->button() == Qt::LeftButton && m_selecting) {
        m_selecting = false;
        if (m_tool == Tool::SelectRectangle) {
            if (p.x() >= 0) m_selectionRect = QRect(m_startPixel,p).normalized();
            m_selectionPolygon = {m_selectionRect.topLeft(),m_selectionRect.topRight(),m_selectionRect.bottomRight(),m_selectionRect.bottomLeft(),m_selectionRect.topLeft()};
        } else if (m_tool == Tool::SelectFree) {
            m_selectionPolygon = m_previewPolygon; if(m_selectionPolygon.size()>2 && m_selectionPolygon.first()!=m_selectionPolygon.last())m_selectionPolygon<<m_selectionPolygon.first();
            m_selectionRect = m_selectionPolygon.boundingRect();
        } else {
            if (!m_document->layers()[m_document->activeLayer()].locked) {
                QPainter painter(&m_document->image()); painter.setRenderHint(QPainter::Antialiasing,false); painter.setPen(QPen(m_color,1,Qt::SolidLine,Qt::SquareCap,Qt::MiterJoin));
                if(m_tool==Tool::Line)painter.drawLine(m_startPixel,p); else if(m_tool==Tool::Rectangle)painter.drawRect(QRect(m_startPixel,p).normalized()); else painter.drawEllipse(QRect(m_startPixel,p).normalized());
            }
            emit editFinished();
        }
        m_selectionActive = (m_tool == Tool::SelectRectangle || m_tool == Tool::SelectFree) && m_selectionRect.isValid();
        update();
    }
    if (event->button() == Qt::LeftButton && m_movingSelection) {
        paintSelectionImage(); m_movingSelection=false; m_dragging=false; emit editFinished(); update();
    }
    if (event->button() == Qt::LeftButton && m_dragging) { m_dragging = false; m_lastDrawPixel = QPoint(-1, -1); emit editFinished(); update(); }
    if (event->button() == Qt::LeftButton && m_panning) { m_panning = false; setCursor(Qt::OpenHandCursor); }
}

QPolygon CanvasView::selectionInPixels() const
{
    if (!m_selectionPolygon.isEmpty()) return m_selectionPolygon;
    return {m_selectionRect.topLeft(),m_selectionRect.topRight(),m_selectionRect.bottomRight(),m_selectionRect.bottomLeft(),m_selectionRect.topLeft()};
}

QImage CanvasView::selectedImage() const
{
    if (!m_selectionActive || m_selectionRect.isEmpty()) return {};
    QImage result(m_selectionRect.size(),QImage::Format_ARGB32); result.fill(Qt::transparent);
    QPainter painter(&result); QPainterPath path; QPolygon relative;
    for(const QPoint &point:selectionInPixels())relative<<point-m_selectionRect.topLeft();
    path.addPolygon(relative); painter.setClipPath(path); painter.drawImage(-m_selectionRect.topLeft(),m_document->image()); return result;
}

void CanvasView::clearSelectionPixels()
{
    if (m_document->layers()[m_document->activeLayer()].locked) return;
    QPainter painter(&m_document->image()); QPainterPath path; QPolygon relative;
    for(const QPoint &point:selectionInPixels())relative<<point;
    path.addPolygon(relative); painter.setClipPath(path); painter.setCompositionMode(QPainter::CompositionMode_Source); painter.fillRect(m_selectionRect,Qt::transparent);
}

void CanvasView::paintSelectionImage()
{
    if(m_document->layers()[m_document->activeLayer()].locked)return;
    QPainter painter(&m_document->image()); painter.drawImage(m_moveCurrent,m_movingImage);
    m_selectionRect=QRect(m_moveCurrent,m_movingImage.size()); m_selectionPolygon={m_selectionRect.topLeft(),m_selectionRect.topRight(),m_selectionRect.bottomRight(),m_selectionRect.bottomLeft(),m_selectionRect.topLeft()};
}

void CanvasView::copySelection() { m_movingImage=selectedImage(); }
void CanvasView::cutSelection() { if(!m_selectionActive)return; emit editStarted(); m_movingImage=selectedImage(); clearSelectionPixels(); emit editFinished(); update(); }
void CanvasView::pasteSelection()
{
    if(m_movingImage.isNull() || m_document->layers()[m_document->activeLayer()].locked)return;
    emit editStarted(); const QPoint origin=m_selectionActive?m_selectionRect.topLeft():QPoint(0,0);
    QPainter painter(&m_document->image()); painter.drawImage(origin,m_movingImage);
    m_selectionRect=QRect(origin,m_movingImage.size()); m_selectionPolygon={m_selectionRect.topLeft(),m_selectionRect.topRight(),m_selectionRect.bottomRight(),m_selectionRect.bottomLeft(),m_selectionRect.topLeft()}; m_selectionActive=true;
    emit editFinished(); update();
}
void CanvasView::flipSelection(Qt::Orientation orientation)
{
    if(!m_selectionActive || m_document->layers()[m_document->activeLayer()].locked)return;
    emit editStarted(); QImage selected=selectedImage().mirrored(orientation==Qt::Horizontal,orientation==Qt::Vertical); clearSelectionPixels();
    QPainter painter(&m_document->image()); painter.drawImage(m_selectionRect.topLeft(),selected); m_selectionPolygon={m_selectionRect.topLeft(),m_selectionRect.topRight(),m_selectionRect.bottomRight(),m_selectionRect.bottomLeft(),m_selectionRect.topLeft()};m_movingImage=selected; emit editFinished(); update();
}
void CanvasView::rotateSelection()
{
    if(!m_selectionActive || m_document->layers()[m_document->activeLayer()].locked)return;
    emit editStarted(); QTransform transform; transform.rotate(90); QImage rotated=selectedImage().transformed(transform,Qt::FastTransformation); clearSelectionPixels();
    QPainter painter(&m_document->image()); painter.drawImage(m_selectionRect.topLeft(),rotated); m_selectionRect.setSize(rotated.size());m_selectionPolygon={m_selectionRect.topLeft(),m_selectionRect.topRight(),m_selectionRect.bottomRight(),m_selectionRect.bottomLeft(),m_selectionRect.topLeft()};m_movingImage=rotated; emit editFinished(); update();
}

void CanvasView::resizeSelection(const QSize &size)
{
    if(!m_selectionActive||m_document->layers()[m_document->activeLayer()].locked||size.width()<1||size.height()<1||size.width()>4096||size.height()>4096)return;
    emit editStarted();QImage scaled=selectedImage().scaled(size,Qt::IgnoreAspectRatio,Qt::FastTransformation);clearSelectionPixels();
    m_selectionRect.setSize(size);QPainter painter(&m_document->image());painter.drawImage(m_selectionRect.topLeft(),scaled);
    m_selectionPolygon={m_selectionRect.topLeft(),m_selectionRect.topRight(),m_selectionRect.bottomRight(),m_selectionRect.bottomLeft(),m_selectionRect.topLeft()};m_movingImage=scaled;emit editFinished();update();
}

void CanvasView::wheelEvent(QWheelEvent *event)
{
    const int next = event->angleDelta().y() > 0 ? qMin(3200, m_zoom * 2) : qMax(25, m_zoom / 2);
    setZoom(next);
}
