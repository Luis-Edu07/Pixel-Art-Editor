#include "core/PixelDocument.h"
#include "editor/CanvasView.h"
#include "editor/MainWindow.h"
#include "formats/ProjectFormat.h"

#include <QMouseEvent>
#include <QTemporaryDir>
#include <QApplication>
#include <QDialog>
#include <QMenuBar>
#include <QSpinBox>
#include <QTimer>
#include <QtTest>

class PixelDocumentTest : public QObject
{
    Q_OBJECT
private slots:
    void createsCanvasAndFillsContiguousPixels()
    {
        PixelDocument document;
        QVERIFY(document.create(8, 8));
        document.setPixel(1, 1, Qt::red);
        document.setPixel(2, 1, Qt::red);
        document.floodFill(0, 0, Qt::blue);
        QCOMPARE(document.image().pixelColor(0, 0), QColor(Qt::blue));
        QCOMPARE(document.image().pixelColor(2, 1), QColor(Qt::red));
        QVERIFY(!document.create(0, 8));
    }

    void layersAndFramesKeepIndependentPixels()
    {
        PixelDocument document;
        QVERIFY(document.create(4, 4));
        document.setPixel(0, 0, Qt::red);
        QVERIFY(document.addLayer(QStringLiteral("Cima")));
        document.setPixel(1, 1, Qt::blue);
        QCOMPARE(document.compositeImage().pixelColor(0, 0), QColor(Qt::red));
        QCOMPARE(document.compositeImage().pixelColor(1, 1), QColor(Qt::blue));
        QVERIFY(document.addFrame(true));
        QCOMPARE(document.frameCount(), 2);
        document.setPixel(2, 2, Qt::green);
        document.setActiveFrame(0);
        QCOMPARE(document.image().pixelColor(2, 2).alpha(), 0);
    }

    void resizingPreservesTopLeftAndTileMetadata()
    {
        PixelDocument document;
        document.create(3, 3);
        document.setPixel(2, 2, Qt::yellow);
        QVERIFY(document.resizeCanvas(5, 4));
        QCOMPARE(document.image().size(), QSize(5, 4));
        QCOMPARE(document.image().pixelColor(2, 2), QColor(Qt::yellow));
        document.setTileMetadata(3, {QStringLiteral("grass"), true, true});
        QCOMPARE(document.tileMetadata(3).tag, QStringLiteral("grass"));
        QVERIFY(document.tileMetadata(3).collision);
        QVERIFY(document.tileMetadata(3).terrain);
        document.setTileMetadata(4, {QStringLiteral("grass"), false, true, 7});
        QCOMPARE(document.autoTileId(QStringLiteral("grass"), 7), 4);
    }

    void tileDefaultsToCanvasSizeAndTracksCanvasResize()
    {
        PixelDocument document;
        QVERIFY(document.create(20, 12));
        QCOMPARE(document.tileSize(), QSize(20, 12));

        QVERIFY(document.resizeCanvas(30, 18));
        QCOMPARE(document.tileSize(), QSize(30, 18));

        document.setTileSize(QSize(10, 9));
        QVERIFY(document.resizeCanvas(40, 24));
        QCOMPARE(document.tileSize(), QSize(10, 9));
    }

    void importedImageSetsTileToImageSize()
    {
        PixelDocument document;
        QImage image(13, 21, QImage::Format_ARGB32);
        image.fill(Qt::transparent);
        document.setImage(image);
        QCOMPARE(document.tileSize(), image.size());
    }

    void tilesetPanelDefaultsToCanvasSize()
    {
        MainWindow window;
        auto *width = window.findChild<QSpinBox *>(QStringLiteral("tilesetTileWidth"));
        auto *height = window.findChild<QSpinBox *>(QStringLiteral("tilesetTileHeight"));
        QVERIFY(width);
        QVERIFY(height);
        QCOMPARE(width->value(), 64);
        QCOMPARE(height->value(), 64);
    }

    void tilesetPanelTracksCanvasResizeFromImageMenu()
    {
        MainWindow window;
        auto *width = window.findChild<QSpinBox *>(QStringLiteral("tilesetTileWidth"));
        auto *height = window.findChild<QSpinBox *>(QStringLiteral("tilesetTileHeight"));
        QVERIFY(width);
        QVERIFY(height);

        QMenu *imageMenu = nullptr;
        for (QAction *action : window.menuBar()->actions()) {
            if (action->text() == QStringLiteral("Imagem")) imageMenu = action->menu();
        }
        QVERIFY(imageMenu);
        QAction *resizeAction = nullptr;
        for (QAction *action : imageMenu->actions()) {
            if (action->text().startsWith(QStringLiteral("Tamanho do canvas"))) resizeAction = action;
        }
        QVERIFY(resizeAction);

        QTimer::singleShot(0, &window, [&window] {
            auto *dialog = window.findChild<QDialog *>();
            if (!dialog) return;
            const auto dimensions = dialog->findChildren<QSpinBox *>();
            if (dimensions.size() != 2) { dialog->reject(); return; }
            dimensions[0]->setValue(20);
            dimensions[1]->setValue(12);
            dialog->accept();
        });
        resizeAction->trigger();

        QCOMPARE(width->value(), 20);
        QCOMPARE(height->value(), 12);
    }

    void pencilBrushControlDefaultsToOneAndUpdatesCanvas()
    {
        MainWindow window;
        auto *brushSize = window.findChild<QSpinBox *>(QStringLiteral("pencilBrushSize"));
        auto *canvas = window.findChild<CanvasView *>();
        QVERIFY(brushSize);
        QVERIFY(canvas);
        QCOMPARE(brushSize->value(), 1);
        QCOMPARE(canvas->brushSize(), 1);

        brushSize->setValue(3);
        QCOMPARE(canvas->brushSize(), 3);
    }

    void projectRoundTripPreservesLayersFramesAndSettings()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        PixelDocument source;
        source.create(8, 8);
        source.setPixel(1, 2, QColor(20, 30, 40, 200));
        source.addLayer(QStringLiteral("Details"));
        source.setPixel(4, 5, Qt::magenta);
        source.addFrame(true);
        source.setFps(9);
        source.setLooping(false);
        source.setTileMetadata(2, {QStringLiteral("rock"), true, true, 13});
        const QString path = directory.filePath(QStringLiteral("roundtrip.pxl"));
        QString error;
        QVERIFY2(ProjectFormat::save(source, path, &error), qPrintable(error));

        PixelDocument loaded;
        QVERIFY2(ProjectFormat::load(loaded, path, &error), qPrintable(error));
        QCOMPARE(loaded.layerCount(), source.layerCount());
        QCOMPARE(loaded.frameCount(), source.frameCount());
        QCOMPARE(loaded.fps(), 9);
        QVERIFY(!loaded.looping());
        QCOMPARE(loaded.layers()[1].name, QStringLiteral("Details"));
        QCOMPARE(loaded.compositeImage(0).pixelColor(1, 2), QColor(20, 30, 40, 200));
        QCOMPARE(loaded.compositeImage(1).pixelColor(4, 5), QColor(Qt::magenta));
        QCOMPARE(loaded.tileMetadata(2).tag, QStringLiteral("rock"));
        QCOMPARE(loaded.tileMetadata(2).autoTileMask, 13);
    }

    void pencilDrawsOnePixelAndConnectsDraggedPixels()
    {
        PixelDocument document;
        QVERIFY(document.create(4, 4));
        CanvasView canvas(&document);
        canvas.resize(240, 240);
        canvas.setZoom(800);
        canvas.setColor(Qt::red);
        canvas.show();
        QTest::qWait(1);

        const int originX = (canvas.width() - document.image().width() * 8) / 2;
        const int originY = (canvas.height() - document.image().height() * 8) / 2;
        const QPoint firstPixel(originX + 4, originY + 4);
        QTest::mouseClick(&canvas, Qt::LeftButton, Qt::NoModifier, firstPixel);
        QCOMPARE(document.image().pixelColor(0, 0), QColor(Qt::red));
        QCOMPARE(document.image().pixelColor(1, 0).alpha(), 0);
        QCOMPARE(document.image().pixelColor(0, 1).alpha(), 0);

        const QPoint start(originX + 12, originY + 12);
        const QPoint finish(originX + 28, originY + 12);
        QCOMPARE(canvas.pixelAt(start), QPoint(1, 1));
        QTest::mousePress(&canvas, Qt::LeftButton, Qt::NoModifier, start);
        QCOMPARE(document.image().pixelColor(1, 1), QColor(Qt::red));
        QMouseEvent moveEvent(QEvent::MouseMove, QPointF(finish), Qt::NoButton,
                              Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(&canvas, &moveEvent);
        QTest::mouseRelease(&canvas, Qt::LeftButton, Qt::NoModifier, finish);
        for (int x = 1; x <= 3; ++x)
            QCOMPARE(document.image().pixelColor(x, 1), QColor(Qt::red));
    }

    void largerPencilBrushDrawsCenteredSquare()
    {
        PixelDocument document;
        QVERIFY(document.create(8, 8));
        CanvasView canvas(&document);
        canvas.resize(320, 320);
        canvas.setZoom(800);
        canvas.setColor(Qt::red);
        canvas.setBrushSize(3);
        canvas.show();
        QTest::qWait(1);

        const int originX = (canvas.width() - document.image().width() * 8) / 2;
        const int originY = (canvas.height() - document.image().height() * 8) / 2;
        const QPoint center(originX + 3 * 8 + 4, originY + 3 * 8 + 4);
        QTest::mouseClick(&canvas, Qt::LeftButton, Qt::NoModifier, center);
        for (int y = 2; y <= 4; ++y)
            for (int x = 2; x <= 4; ++x)
                QCOMPARE(document.image().pixelColor(x, y), QColor(Qt::red));
        QCOMPARE(document.image().pixelColor(1, 3).alpha(), 0);
        QCOMPARE(document.image().pixelColor(5, 3).alpha(), 0);
    }

    void transparencyCheckerDoesNotExceedCanvasBounds()
    {
        PixelDocument document;
        QVERIFY(document.create(20, 20));
        CanvasView canvas(&document);
        canvas.resize(420, 320);
        canvas.setZoom(800);
        canvas.show();
        QTest::qWait(1);

        const QImage rendered = canvas.grab().toImage();
        QCOMPARE(rendered.pixelColor(300, 100), QColor("#303238"));
    }

    void tilesetGuideLinesUseHighContrastBlue()
    {
        PixelDocument document;
        QVERIFY(document.create(20, 20));
        document.setTileSize(QSize(10, 10));
        CanvasView canvas(&document);
        canvas.resize(420, 320);
        canvas.setZoom(800);
        canvas.show();
        QTest::qWait(1);

        const QColor lineColor = canvas.grab().toImage().pixelColor(210, 150);
        QVERIFY(lineColor.blue() > 220);
        QVERIFY(lineColor.blue() > lineColor.green());
    }
};

QTEST_MAIN(PixelDocumentTest)
#include "PixelDocumentTest.moc"
