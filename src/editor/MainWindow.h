#pragma once

#include "core/PixelDocument.h"
#include "editor/CanvasView.h"

#include <QMainWindow>
#include <QVector>
#include <QCloseEvent>

class QAction;
class QLabel;
class QSpinBox;
class QVBoxLayout;
class QListWidget;
class QTimer;
class QDoubleSpinBox;
class QCheckBox;
class QPushButton;
class QLineEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void createActions();
    void createPalettePanel();
    void createTilesetPanel();
    void createLayerPanel();
    void createTimelinePanel();
    void refreshPalette();
    void refreshTilesetPreview();
    void refreshLayers();
    void refreshTimeline();
    void setCurrentFile(const QString &path);
    void resetHistory();
    void undo();
    void redo();
    bool saveProject();
    bool saveProjectAs();
    void newDocument();
    void openImage();
    void exportPng();
    void exportSpriteSheet();
    void togglePlayback();
    void recordHistory();
    void chooseColor();
    void updateStatus();
    bool maybeSave();
    void exportGif();

protected:
    void closeEvent(QCloseEvent *event) override;

    PixelDocument m_document;
    CanvasView *m_canvas{nullptr};
    QLabel *m_status{nullptr};
    QLabel *m_colorPreview{nullptr};
    QWidget *m_paletteGrid{nullptr};
    QVBoxLayout *m_paletteLayout{nullptr};
    QLabel *m_tilePreview{nullptr};
    QSpinBox *m_tileWidth{nullptr};
    QSpinBox *m_tileHeight{nullptr};
    QListWidget *m_tilesList{nullptr};
    QLineEdit *m_tileTag{nullptr};
    QCheckBox *m_tileCollision{nullptr};
    QCheckBox *m_tileTerrain{nullptr};
    QSpinBox *m_tileMask{nullptr};
    QListWidget *m_layersList{nullptr};
    QListWidget *m_timelineList{nullptr};
    QSpinBox *m_fpsSpin{nullptr};
    QTimer *m_playbackTimer{nullptr};
    QAction *m_playAction{nullptr};
    QPushButton *m_playButton{nullptr};
    QVector<PixelDocument> m_history;
    int m_historyIndex{0};
    PixelDocument m_beforeEdit;
    QString m_currentFile;
    QString m_projectFile;
    bool m_dirty{false};
    bool m_refreshingLayers{false};
};
