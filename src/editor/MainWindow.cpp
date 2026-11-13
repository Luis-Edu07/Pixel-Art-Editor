#include "MainWindow.h"

#include "formats/ProjectFormat.h"
#include "formats/GifFormat.h"

#include <QAction>
#include <QColorDialog>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QCheckBox>
#include <QListWidget>
#include <QSlider>
#include <QTimer>
#include <QSpinBox>
#include <QFormLayout>
#include <QSignalBlocker>
#include <QSettings>
#include <QKeySequenceEdit>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSpinBox>
#include <QStatusBar>
#include <QTextStream>
#include <QToolBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDir>

namespace {
QSize requestDimensions(QWidget *parent, const QString &title, const QSize &initialSize)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    auto *layout = new QFormLayout(&dialog);
    auto *width = new QSpinBox(&dialog);
    auto *height = new QSpinBox(&dialog);
    width->setRange(1, 4096);
    height->setRange(1, 4096);
    width->setValue(qBound(1, initialSize.width(), 4096));
    height->setValue(qBound(1, initialSize.height(), 4096));
    layout->addRow(QObject::tr("Largura (px)"), width);
    layout->addRow(QObject::tr("Altura (px)"), height);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addRow(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return {};
    return QSize(width->value(), height->value());
}

void syncTileSizeControls(QSpinBox *width, QSpinBox *height, const QSize &size)
{
    const QSignalBlocker widthBlocker(width);
    const QSignalBlocker heightBlocker(height);
    width->setValue(size.width());
    height->setValue(size.height());
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_canvas = new CanvasView(&m_document, this);
    setCentralWidget(m_canvas);
    resize(1180, 760);
    setMinimumSize(820, 560);
    createActions();
    createPalettePanel();
    createTilesetPanel();
    createLayerPanel();
    createTimelinePanel();

    connect(m_canvas, &CanvasView::editStarted, this, [this] { m_beforeEdit = m_document; });
    connect(m_canvas, &CanvasView::editFinished, this, [this] {
        if (m_beforeEdit.image() != m_document.image()) {
            while (m_history.size() > m_historyIndex + 1) m_history.removeLast();
            m_history.push_back(m_document);
            m_historyIndex = m_history.size() - 1;
            m_dirty = true;
            updateStatus();
        }
    });
    connect(m_canvas, &CanvasView::colorPicked, this, [this](const QColor &color) { m_canvas->setColor(color); m_colorPreview->setStyleSheet(QString("background:%1").arg(color.name(QColor::HexArgb))); });
    connect(m_canvas, &CanvasView::cursorPixelChanged, this, [this](const QPoint &p) {
        statusBar()->showMessage(tr("X: %1  Y: %2   |   Canvas: %3 × %4 px   |   Zoom: %5%").arg(p.x()).arg(p.y()).arg(m_document.image().width()).arg(m_document.image().height()).arg(m_canvas->zoom()));
    });
    connect(m_canvas, &CanvasView::zoomChanged, this, [this](int) { updateStatus(); });
    m_playbackTimer = new QTimer(this);
    connect(m_playbackTimer, &QTimer::timeout, this, [this] {
        int next = m_document.activeFrame() + 1;
        if (next >= m_document.frameCount()) {
            if (!m_document.looping()) { m_playbackTimer->stop(); m_playAction->setText(tr("▶ Reproduzir")); return; }
            next = 0;
        }
        m_document.setActiveFrame(next); refreshTimeline(); m_canvas->update(); updateStatus();
    });
    resetHistory();
    setCurrentFile(QString());
    refreshPalette();
    refreshTilesetPreview();
    updateStatus();
}

void MainWindow::createActions()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("Arquivo"));
    QAction *newAction = fileMenu->addAction(tr("Novo..."), this, &MainWindow::newDocument, QKeySequence::New);
    QAction *openAction = fileMenu->addAction(tr("Abrir imagem..."), this, &MainWindow::openImage, QKeySequence::Open);
    QAction *openProjectAction = fileMenu->addAction(tr("Abrir projeto .pxl..."), this, [this] {
        if (!maybeSave()) return;
        const QString path = QFileDialog::getOpenFileName(this, tr("Abrir projeto"), {}, tr("Projeto Pixel Art (*.pxl)"));
        if (path.isEmpty()) return;
        QString error;
        if (!ProjectFormat::load(m_document, path, &error)) QMessageBox::critical(this, tr("Erro ao abrir"), error);
        else { m_projectFile = path; m_currentFile = path; syncTileSizeControls(m_tileWidth, m_tileHeight, m_document.tileSize()); m_dirty = false; m_canvas->update(); resetHistory(); refreshPalette(); refreshTilesetPreview(); refreshLayers(); refreshTimeline(); updateStatus(); }
    });
    Q_UNUSED(newAction); Q_UNUSED(openAction); Q_UNUSED(openProjectAction);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Salvar projeto"), this, [this] { saveProject(); }, QKeySequence::Save);
    fileMenu->addAction(tr("Salvar projeto como..."), this, [this] { saveProjectAs(); }, QKeySequence::SaveAs);
    fileMenu->addAction(tr("Exportar PNG..."), this, &MainWindow::exportPng);
    fileMenu->addAction(tr("Exportar Sprite Sheet..."), this, &MainWindow::exportSpriteSheet);
    fileMenu->addAction(tr("Exportar animação GIF..."), this, &MainWindow::exportGif);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Sair"), this, &QWidget::close, QKeySequence::Quit);

    QMenu *editMenu = menuBar()->addMenu(tr("Editar"));
    editMenu->addAction(tr("Desfazer"), this, &MainWindow::undo, QKeySequence::Undo);
    editMenu->addAction(tr("Refazer"), this, &MainWindow::redo, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z));
    editMenu->addAction(tr("Refazer (Ctrl+Y)"), this, &MainWindow::redo, QKeySequence(Qt::CTRL | Qt::Key_Y));
    editMenu->addSeparator();
    editMenu->addAction(tr("Copiar seleção"), m_canvas, &CanvasView::copySelection, QKeySequence::Copy);
    editMenu->addAction(tr("Recortar seleção"), m_canvas, &CanvasView::cutSelection, QKeySequence::Cut);
    editMenu->addAction(tr("Colar"), m_canvas, &CanvasView::pasteSelection, QKeySequence::Paste);
    editMenu->addAction(tr("Espelhar horizontalmente"), this, [this] { m_canvas->flipSelection(Qt::Horizontal); }, QKeySequence(Qt::CTRL | Qt::Key_H));
    editMenu->addAction(tr("Espelhar verticalmente"), this, [this] { m_canvas->flipSelection(Qt::Vertical); }, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_H));
    editMenu->addAction(tr("Rotacionar 90°"), m_canvas, &CanvasView::rotateSelection, QKeySequence(Qt::CTRL | Qt::Key_R));
    editMenu->addAction(tr("Redimensionar seleção..."), this, [this] {
        if (!m_canvas->hasSelection()) return;
        const QSize size = requestDimensions(this, tr("Redimensionar seleção"), m_canvas->selectionRect().size());
        if (size.isValid()) m_canvas->resizeSelection(size);
    });

    QMenu *viewMenu = menuBar()->addMenu(tr("Exibir"));
    QAction *grid = viewMenu->addAction(tr("Mostrar grade")); grid->setCheckable(true); grid->setChecked(true);
    connect(grid, &QAction::toggled, m_canvas, &CanvasView::setGridVisible);
    QMenu *imageMenu = menuBar()->addMenu(tr("Imagem"));
    imageMenu->addAction(tr("Tamanho do canvas..."), this, [this] {
        const QSize size = requestDimensions(this, tr("Tamanho do canvas"), m_document.image().size());
        if (!size.isValid()) return;
        emit m_canvas->editStarted();
        m_document.resizeCanvas(size.width(), size.height()); m_canvas->update(); emit m_canvas->editFinished(); updateStatus(); refreshTilesetPreview();
    });
    QMenu *animationMenu = menuBar()->addMenu(tr("Animação"));
    animationMenu->addAction(tr("Adicionar quadro"), this, [this] { m_document.addFrame(); recordHistory(); refreshTimeline(); m_canvas->update(); });
    animationMenu->addAction(tr("Duplicar quadro"), this, [this] { m_document.addFrame(true); recordHistory(); refreshTimeline(); m_canvas->update(); });
    animationMenu->addAction(tr("Excluir quadro"), this, [this] { if (m_document.removeFrame()) { recordHistory(); refreshTimeline(); m_canvas->update(); } });
    animationMenu->addAction(tr("Exportar Sprite Sheet..."), this, &MainWindow::exportSpriteSheet);
    animationMenu->addAction(tr("Exportar GIF..."), this, &MainWindow::exportGif);
    QMenu *tilesMenu = menuBar()->addMenu(tr("Tileset"));
    tilesMenu->addAction(tr("Exportar metadados de tiles..."), this, [this] {
        const QString path=QFileDialog::getSaveFileName(this,tr("Metadados do Tileset"),{},tr("JSON (*.json)")); if(path.isEmpty())return;
        QJsonArray tiles; const int across=m_document.image().width()/m_document.tileSize().width();
        for(int id=0;id<across*(m_document.image().height()/m_document.tileSize().height());++id){ const TileMetadata meta=m_document.tileMetadata(id); const int col=across>0?id%across:0,row=across>0?id/across:0; tiles.append(QJsonObject{{"id",id},{"x",col*m_document.tileSize().width()},{"y",row*m_document.tileSize().height()},{"width",m_document.tileSize().width()},{"height",m_document.tileSize().height()},{"tag",meta.tag},{"collision",meta.collision},{"terrain",meta.terrain},{"autoTileMask",meta.autoTileMask}}); }
        const QJsonObject json{{"tileWidth",m_document.tileSize().width()},{"tileHeight",m_document.tileSize().height()},{"imageWidth",m_document.image().width()},{"imageHeight",m_document.image().height()},{"tiles",tiles}};
        QFile file(path.endsWith(".json",Qt::CaseInsensitive)?path:path+".json"); if(file.open(QIODevice::WriteOnly))file.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
    });

    QToolBar *tools = addToolBar(tr("Ferramentas"));
    tools->setMovable(false);
    auto addTool = [tools, this](const QString &label, const QString &tip, CanvasView::Tool tool, bool checked = false) {
        QAction *action = tools->addAction(label); action->setToolTip(tip); action->setCheckable(true); action->setChecked(checked);
        const QString key=QStringLiteral("tool_%1").arg(int(tool));action->setObjectName(key);
        const QStringList defaults={"P","E","G","I","H","Z","L","R","O","M","N"};
        const QKeySequence defaultSequence(int(tool)<defaults.size()?defaults[int(tool)]:QString());
        QSettings settings; action->setShortcut(settings.value(QStringLiteral("shortcuts/")+key,defaultSequence).value<QKeySequence>());action->setShortcutContext(Qt::ApplicationShortcut);
        connect(action, &QAction::triggered, this, [this, tool, action, tools] {
            for (QAction *other : tools->actions()) if (other->isCheckable()) other->setChecked(other == action);
            m_canvas->setTool(tool);
        });
    };
    addTool(tr("Lápis"), tr("Desenhar pixels"), CanvasView::Tool::Pencil, true);
    addTool(tr("Borracha"), tr("Apagar pixels"), CanvasView::Tool::Eraser);
    addTool(tr("Balde"), tr("Preencher área contígua"), CanvasView::Tool::Fill);
    addTool(tr("Conta-gotas"), tr("Selecionar cor do canvas"), CanvasView::Tool::Eyedropper);
    addTool(tr("Linha"), tr("Desenhar linha"), CanvasView::Tool::Line);
    addTool(tr("Retângulo"), tr("Desenhar retângulo"), CanvasView::Tool::Rectangle);
    addTool(tr("Elipse"), tr("Desenhar elipse"), CanvasView::Tool::Ellipse);
    addTool(tr("Seleção retangular"), tr("Selecionar uma região retangular"), CanvasView::Tool::SelectRectangle);
    addTool(tr("Seleção livre"), tr("Selecionar uma região à mão livre"), CanvasView::Tool::SelectFree);
    addTool(tr("Pan"), tr("Mover a visualização"), CanvasView::Tool::Pan);
    addTool(tr("Zoom"), tr("Aumentar zoom"), CanvasView::Tool::Zoom);
    tools->addSeparator();
    tools->addAction(tr("−"), this, [this] { m_canvas->setZoom(m_canvas->zoom() / 2); });
    tools->addAction(tr("+"), this, [this] { m_canvas->setZoom(m_canvas->zoom() * 2); });
    tools->addSeparator();
    auto *brushSize = new QSpinBox(tools);
    brushSize->setObjectName("pencilBrushSize");
    brushSize->setRange(1, 64);
    brushSize->setValue(m_canvas->brushSize());
    brushSize->setPrefix(tr("Pincel "));
    brushSize->setSuffix(tr(" px"));
    brushSize->setToolTip(tr("Tamanho quadrado aplicado pela ferramenta Lápis"));
    tools->addWidget(brushSize);
    connect(brushSize, qOverload<int>(&QSpinBox::valueChanged), m_canvas, &CanvasView::setBrushSize);
    editMenu->addAction(tr("Configurar atalhos..."),this,[this,tools]{
        QDialog dialog(this);dialog.setWindowTitle(tr("Atalhos de ferramentas"));auto *layout=new QFormLayout(&dialog);QVector<QPair<QAction*,QKeySequenceEdit*>> editors;
        for(QAction *action:tools->actions())if(action->isCheckable()){auto *editor=new QKeySequenceEdit(action->shortcut(),&dialog);layout->addRow(action->text(),editor);editors.push_back({action,editor});}
        auto *buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel,&dialog);layout->addRow(buttons);connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
        if(dialog.exec()==QDialog::Accepted){QSettings settings;for(const auto &entry:editors){entry.first->setShortcut(entry.second->keySequence());settings.setValue(QStringLiteral("shortcuts/")+entry.first->objectName(),entry.second->keySequence());}}
    });
}

void MainWindow::createLayerPanel()
{
    auto *dock = new QDockWidget(tr("Camadas"), this); dock->setObjectName("LayersDock"); dock->setMinimumWidth(245);
    auto *content = new QWidget(dock); content->setStyleSheet("QPushButton { min-height: 30px; padding: 3px 7px; }"); auto *layout = new QVBoxLayout(content);
    m_layersList = new QListWidget(content); layout->addWidget(m_layersList);
    auto *name = new QLineEdit(content); name->setPlaceholderText(tr("Nome da camada")); layout->addWidget(name);
    auto *opacity = new QSlider(Qt::Horizontal, content); opacity->setRange(0, 100); layout->addWidget(opacity);
    auto *locked = new QCheckBox(tr("Bloqueada"), content); layout->addWidget(locked);
    auto *controls = new QHBoxLayout();
    auto *add = new QPushButton(tr("+"), content); auto *remove = new QPushButton(tr("−"), content);
    auto *up = new QPushButton(tr("↑"), content); auto *down = new QPushButton(tr("↓"), content);
    for (QPushButton *button : {add, remove, up, down}) button->setMinimumSize(38, 36);
    for (QPushButton *button : {add, remove, up, down}) controls->addWidget(button); layout->addLayout(controls);
    connect(m_layersList, &QListWidget::currentRowChanged, this, [this, name, opacity, locked](int row) {
        if (row < 0 || row >= m_document.layerCount()) return;
        m_document.setActiveLayer(row); const PixelLayer &layer = m_document.layers()[row];
        name->setText(layer.name); opacity->setValue(qRound(layer.opacity * 100)); locked->setChecked(layer.locked); m_canvas->update(); updateStatus();
    });
    connect(m_layersList, &QListWidget::itemChanged, this, [this](QListWidgetItem *item) {
        const int row = m_layersList->row(item); if (row < 0) return;
        const PixelLayer old = m_document.layers()[row]; m_document.updateLayer(row, old.name, item->checkState() == Qt::Checked, old.locked, old.opacity);
        m_dirty = true; recordHistory(); m_canvas->update(); updateStatus();
    });
    connect(name, &QLineEdit::editingFinished, this, [this, name, opacity, locked] {
        const int row = m_document.activeLayer(); const PixelLayer old = m_document.layers()[row];
        m_document.updateLayer(row, name->text(), old.visible, locked->isChecked(), opacity->value() / 100.0); recordHistory(); refreshLayers(); m_dirty = true;
    });
    connect(opacity, &QSlider::valueChanged, this, [this, name, opacity, locked](int value) {
        const int row = m_document.activeLayer(); const PixelLayer old = m_document.layers()[row];
        m_document.updateLayer(row, name->text(), old.visible, locked->isChecked(), value / 100.0); m_dirty = true; m_canvas->update();
    });
    connect(opacity, &QSlider::sliderReleased, this, [this] { recordHistory(); });
    connect(locked, &QCheckBox::toggled, this, [this, name, opacity, locked](bool value) {
        const int row = m_document.activeLayer(); const PixelLayer old = m_document.layers()[row];
        m_document.updateLayer(row, name->text(), old.visible, value, opacity->value() / 100.0); m_dirty = true; recordHistory(); m_canvas->update();
    });
    connect(add, &QPushButton::clicked, this, [this] { if (m_document.addLayer()) { recordHistory(); refreshLayers(); m_canvas->update(); } });
    connect(remove, &QPushButton::clicked, this, [this] { if (m_document.removeLayer()) { recordHistory(); refreshLayers(); m_canvas->update(); } });
    connect(up, &QPushButton::clicked, this, [this] { const int row=m_document.activeLayer(); if(m_document.moveLayer(row,qMax(0,row-1))){recordHistory();refreshLayers();m_canvas->update();} });
    connect(down, &QPushButton::clicked, this, [this] { const int row=m_document.activeLayer(); if(m_document.moveLayer(row,qMin(m_document.layerCount()-1,row+1))){recordHistory();refreshLayers();m_canvas->update();} });
    auto *scroll = new QScrollArea(dock); scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame); scroll->setWidget(content);
    dock->setWidget(scroll); addDockWidget(Qt::RightDockWidgetArea, dock); tabifyDockWidget(findChild<QDockWidget*>("PaletteDock"), dock);
}

void MainWindow::createTimelinePanel()
{
    auto *dock = new QDockWidget(tr("Timeline"), this); dock->setObjectName("TimelineDock");
    auto *content = new QWidget(dock); auto *layout = new QHBoxLayout(content);
    m_timelineList = new QListWidget(content); m_timelineList->setFlow(QListView::LeftToRight); m_timelineList->setMaximumHeight(90); layout->addWidget(m_timelineList, 1);
    auto *controls = new QVBoxLayout(); auto *buttons = new QHBoxLayout();
    auto *add = new QPushButton(tr("+"), content); auto *duplicate = new QPushButton(tr("Duplicar"), content); auto *remove = new QPushButton(tr("−"), content);
    add->setMinimumSize(38, 34); duplicate->setMinimumHeight(34); remove->setMinimumSize(38, 34);
    buttons->addWidget(add); buttons->addWidget(duplicate); buttons->addWidget(remove); controls->addLayout(buttons);
    auto *settings = new QHBoxLayout(); m_fpsSpin = new QSpinBox(content); m_fpsSpin->setRange(1, 60); m_fpsSpin->setPrefix(tr("FPS ")); m_fpsSpin->setValue(12);
    auto *loop = new QCheckBox(tr("Loop"), content); loop->setChecked(true); auto *onion = new QCheckBox(tr("Onion skin"), content);
    settings->addWidget(m_fpsSpin); settings->addWidget(loop); settings->addWidget(onion); controls->addLayout(settings);
    m_playAction = new QAction(tr("▶ Reproduzir"), this); m_playButton = new QPushButton(content); m_playButton->setDefault(false); m_playButton->setText(m_playAction->text()); controls->addWidget(m_playButton);
    m_playButton->setMinimumHeight(34);
    layout->addLayout(controls); dock->setWidget(content); addDockWidget(Qt::BottomDockWidgetArea, dock);
    connect(m_timelineList, &QListWidget::currentRowChanged, this, [this](int frame) { m_document.setActiveFrame(frame); m_canvas->update(); updateStatus(); });
    connect(add, &QPushButton::clicked, this, [this] { if(m_document.addFrame()) { recordHistory(); refreshTimeline(); m_canvas->update(); } });
    connect(duplicate, &QPushButton::clicked, this, [this] { if(m_document.addFrame(true)) { recordHistory(); refreshTimeline(); m_canvas->update(); } });
    connect(remove, &QPushButton::clicked, this, [this] { if(m_document.removeFrame()) { recordHistory(); refreshTimeline(); m_canvas->update(); } });
    connect(m_fpsSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) { m_document.setFps(value); if(m_playbackTimer->isActive()) m_playbackTimer->setInterval(1000 / value); m_dirty=true; });
    connect(m_fpsSpin, &QSpinBox::editingFinished, this, [this] { recordHistory(); });
    connect(loop, &QCheckBox::toggled, this, [this](bool value) { m_document.setLooping(value); m_dirty=true; recordHistory(); });
    connect(onion, &QCheckBox::toggled, m_canvas, &CanvasView::setOnionSkin);
    connect(m_playButton, &QPushButton::clicked, this, &MainWindow::togglePlayback);
    connect(m_playAction, &QAction::triggered, this, &MainWindow::togglePlayback);
}

void MainWindow::createPalettePanel()
{
    auto *dock = new QDockWidget(tr("Cores"), this);
    dock->setObjectName("PaletteDock"); dock->setMinimumWidth(245);
    auto *content = new QWidget(dock); content->setStyleSheet("QPushButton { min-height: 30px; padding: 3px 7px; }"); auto *layout = new QVBoxLayout(content);
    auto *currentRow = new QHBoxLayout(); currentRow->addWidget(new QLabel(tr("Cor ativa:"), content));
    m_colorPreview = new QLabel(content); m_colorPreview->setFixedSize(34, 26); m_colorPreview->setFrameShape(QFrame::Box); m_colorPreview->setStyleSheet("background:#000000");
    auto *pick = new QPushButton(tr("Escolher..."), content); connect(pick, &QPushButton::clicked, this, &MainWindow::chooseColor);
    currentRow->addWidget(m_colorPreview); currentRow->addWidget(pick); layout->addLayout(currentRow);
    m_paletteGrid = new QWidget(content); m_paletteLayout = new QVBoxLayout(m_paletteGrid); m_paletteLayout->setContentsMargins(0, 0, 0, 0); layout->addWidget(m_paletteGrid); layout->addStretch();
    auto *add = new QPushButton(tr("Adicionar cor à paleta"), content);
    connect(add, &QPushButton::clicked, this, [this] { m_document.addPaletteColor(m_canvas->color()); refreshPalette(); m_dirty = true; updateStatus(); });
    layout->addWidget(add);
    auto *savePalette = new QPushButton(tr("Salvar paleta .gpl"), content);
    connect(savePalette, &QPushButton::clicked, this, [this] {
        const QString path = QFileDialog::getSaveFileName(this, tr("Salvar paleta"), {}, tr("Paleta GIMP (*.gpl)")); if (path.isEmpty()) return;
        QFile file(path); if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) { QMessageBox::warning(this, tr("Erro"), file.errorString()); return; }
        QTextStream out(&file); out << "GIMP Palette\nName: Pixel Art Editor\nColumns: 8\n#\n";
        for (const QColor &c : m_document.palette()) out << c.red() << ' ' << c.green() << ' ' << c.blue() << "\t" << c.name() << '\n';
    });
    auto *saveJsonPalette = new QPushButton(tr("Salvar paleta .json"), content);
    connect(saveJsonPalette, &QPushButton::clicked, this, [this] {
        const QString path=QFileDialog::getSaveFileName(this,tr("Salvar paleta"),{},tr("Paleta JSON (*.json)"));if(path.isEmpty())return;
        QJsonArray colors;for(const QColor &color:m_document.palette())colors.append(color.name(QColor::HexArgb));
        QFile file(path.endsWith(".json",Qt::CaseInsensitive)?path:path+".json");if(file.open(QIODevice::WriteOnly))file.write(QJsonDocument(QJsonObject{{"format","pixel-art-palette"},{"colors",colors}}).toJson(QJsonDocument::Indented));
    });
    auto *loadPalette = new QPushButton(tr("Carregar paleta"), content);
    connect(loadPalette, &QPushButton::clicked, this, [this] {
        const QString path = QFileDialog::getOpenFileName(this, tr("Carregar paleta"), {}, tr("Paletas (*.gpl *.json)")); if (path.isEmpty()) return;
        QFile file(path); if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) { QMessageBox::warning(this, tr("Erro"), file.errorString()); return; }
        QVector<QColor> colors;
        if(path.endsWith(".json",Qt::CaseInsensitive)){
            const QJsonDocument json=QJsonDocument::fromJson(file.readAll());QJsonArray values=json.isArray()?json.array():json.object().value("colors").toArray();
            for(const QJsonValue &value:values){const QColor color(value.toString());if(color.isValid())colors.push_back(color);}
        }else{
            QTextStream in(&file); while (!in.atEnd()) { const QString line = in.readLine().trimmed(); if (line.isEmpty() || line.startsWith('#') || line.startsWith("GIMP") || line.startsWith("Name:") || line.startsWith("Columns:")) continue; const QStringList cols = line.split(QRegularExpression("\\s+")); if (cols.size() >= 3) { bool a=false,b=false,c=false; const int r=cols[0].toInt(&a),g=cols[1].toInt(&b),bl=cols[2].toInt(&c); if(a&&b&&c) colors.push_back(QColor(r,g,bl)); } }
        }
        if (!colors.isEmpty()) { m_document.setPalette(colors); refreshPalette(); m_dirty = true; recordHistory(); }
    });
    layout->addWidget(savePalette); layout->addWidget(saveJsonPalette); layout->addWidget(loadPalette);
    auto *scroll = new QScrollArea(dock); scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame); scroll->setWidget(content);
    dock->setWidget(scroll); addDockWidget(Qt::RightDockWidgetArea, dock);
}

void MainWindow::createTilesetPanel()
{
    auto *dock = new QDockWidget(tr("Tileset"), this); dock->setObjectName("TilesetDock"); dock->setMinimumWidth(245);
    auto *content = new QWidget(dock); content->setStyleSheet("QPushButton { min-height: 30px; padding: 3px 7px; }"); auto *layout = new QVBoxLayout(content);
    auto *form = new QFormLayout();
    const QSize initialTileSize = m_document.tileSize();
    m_tileWidth = new QSpinBox(content); m_tileWidth->setObjectName("tilesetTileWidth"); m_tileWidth->setRange(1, 4096); m_tileWidth->setValue(initialTileSize.width());
    m_tileHeight = new QSpinBox(content); m_tileHeight->setObjectName("tilesetTileHeight"); m_tileHeight->setRange(1, 4096); m_tileHeight->setValue(initialTileSize.height());
    form->addRow(tr("Largura do tile"), m_tileWidth); form->addRow(tr("Altura do tile"), m_tileHeight); layout->addLayout(form);
    m_tilePreview = new QLabel(content); m_tilePreview->setMinimumSize(160, 130); m_tilePreview->setAlignment(Qt::AlignCenter); m_tilePreview->setFrameShape(QFrame::StyledPanel); layout->addWidget(m_tilePreview);
    auto *info = new QLabel(tr("Divisão automática em tiles. A grade do canvas mostra os limites conforme o tamanho configurado."), content); info->setWordWrap(true); layout->addWidget(info); layout->addStretch();
    m_tilesList = new QListWidget(content); m_tilesList->setMaximumHeight(135); layout->addWidget(m_tilesList);
    auto *exportTile = new QPushButton(tr("Exportar tile selecionado..."), content); layout->addWidget(exportTile);
    connect(exportTile, &QPushButton::clicked, this, [this] {
        const int tileId = m_tilesList->currentRow();
        const QSize tileSize = m_document.tileSize();
        const int tilesAcross = tileSize.width() > 0 ? m_document.image().width() / tileSize.width() : 0;
        if (tileId < 0 || tilesAcross < 1) return;
        const QRect tileRect((tileId % tilesAcross) * tileSize.width(),
                             (tileId / tilesAcross) * tileSize.height(),
                             tileSize.width(), tileSize.height());
        const QImage tileImage = m_document.compositeImage().copy(tileRect);
        const QString path = QFileDialog::getSaveFileName(this, tr("Exportar tile"), {}, tr("PNG (*.png)"));
        if (path.isEmpty()) return;
        const QString output = path.endsWith(".png", Qt::CaseInsensitive) ? path : path + ".png";
        if (!tileImage.save(output, "PNG"))
            QMessageBox::critical(this, tr("Erro ao exportar"), tr("Não foi possível salvar o tile."));
    });
    m_tileTag = new QLineEdit(content); m_tileTag->setPlaceholderText(tr("Tag do tile")); layout->addWidget(m_tileTag);
    m_tileCollision = new QCheckBox(tr("Colisão"), content); m_tileTerrain = new QCheckBox(tr("Terrain / autotiling"), content);
    layout->addWidget(m_tileCollision); layout->addWidget(m_tileTerrain);
    m_tileMask = new QSpinBox(content); m_tileMask->setRange(0,15); m_tileMask->setPrefix(tr("Máscara autotile ")); m_tileMask->setToolTip(tr("Quatro bits: norte, leste, sul e oeste conectados ao terreno.")); layout->addWidget(m_tileMask);
    connect(m_tilesList, &QListWidget::currentRowChanged, this, [this](int id) {
        const TileMetadata metadata=m_document.tileMetadata(id);
        const QSignalBlocker a(m_tileTag),b(m_tileCollision),c(m_tileTerrain),d(m_tileMask);
        m_tileTag->setText(metadata.tag); m_tileCollision->setChecked(metadata.collision); m_tileTerrain->setChecked(metadata.terrain);m_tileMask->setValue(metadata.autoTileMask);
    });
    auto saveMetadata = [this] {
        const int id=m_tilesList->currentRow(); if(id<0)return;
        m_document.setTileMetadata(id,{m_tileTag->text(),m_tileCollision->isChecked(),m_tileTerrain->isChecked(),m_tileMask->value()});
        m_dirty=true; recordHistory();
        if(auto *item=m_tilesList->item(id))item->setText(tr("Tile %1%2%3").arg(id).arg(m_tileCollision->isChecked()?"  ◆":"").arg(m_tileTerrain->isChecked()?"  ◈":""));
    };
    connect(m_tileTag,&QLineEdit::editingFinished,this,saveMetadata);
    connect(m_tileCollision,&QCheckBox::toggled,this,[saveMetadata](bool){saveMetadata();});
    connect(m_tileTerrain,&QCheckBox::toggled,this,[saveMetadata](bool){saveMetadata();});
    connect(m_tileMask,qOverload<int>(&QSpinBox::valueChanged),this,[saveMetadata](int){saveMetadata();});
    connect(m_tileWidth, qOverload<int>(&QSpinBox::valueChanged), this, [this](int) { m_document.setTileSize(QSize(m_tileWidth->value(), m_tileHeight->value())); m_dirty = true; refreshTilesetPreview(); m_canvas->update(); updateStatus(); });
    connect(m_tileHeight, qOverload<int>(&QSpinBox::valueChanged), this, [this](int) { m_document.setTileSize(QSize(m_tileWidth->value(), m_tileHeight->value())); m_dirty = true; refreshTilesetPreview(); m_canvas->update(); updateStatus(); });
    auto *scroll = new QScrollArea(dock); scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame); scroll->setWidget(content);
    dock->setWidget(scroll); addDockWidget(Qt::RightDockWidgetArea, dock);
}

void MainWindow::refreshPalette()
{
    QLayoutItem *item;
    while ((item = m_paletteLayout->takeAt(0))) { delete item->widget(); delete item; }
    for (int i = 0; i < m_document.palette().size(); i += 8) {
        auto *row = new QHBoxLayout();
        for (int j = i; j < qMin(i + 8, m_document.palette().size()); ++j) {
            const QColor color = m_document.palette()[j]; auto *button = new QPushButton(m_paletteGrid); button->setFixedSize(28, 28); button->setToolTip(color.name(QColor::HexArgb)); button->setStyleSheet(QString("background:%1; border:1px solid #555").arg(color.name(QColor::HexArgb)));
            connect(button, &QPushButton::clicked, this, [this, color] { m_canvas->setColor(color); m_colorPreview->setStyleSheet(QString("background:%1").arg(color.name(QColor::HexArgb))); }); row->addWidget(button);
        }
        row->addStretch(); m_paletteLayout->addLayout(row);
    }
}

void MainWindow::refreshLayers()
{
    if (!m_layersList) return;
    const QSignalBlocker blocker(m_layersList);
    m_layersList->clear();
    for (const PixelLayer &layer : m_document.layers()) {
        auto *item = new QListWidgetItem(layer.name, m_layersList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        item->setCheckState(layer.visible ? Qt::Checked : Qt::Unchecked);
        if (layer.locked) item->setText(layer.name + tr(" 🔒"));
    }
    m_layersList->setCurrentRow(m_document.activeLayer());
}

void MainWindow::refreshTimeline()
{
    if (!m_timelineList) return;
    const QSignalBlocker blocker(m_timelineList);
    m_timelineList->clear();
    for (int i = 0; i < m_document.frameCount(); ++i)
        m_timelineList->addItem(tr("Quadro %1").arg(i + 1));
    m_timelineList->setCurrentRow(m_document.activeFrame());
    if (m_fpsSpin) { const QSignalBlocker fpsBlocker(m_fpsSpin); m_fpsSpin->setValue(m_document.fps()); }
}

void MainWindow::refreshTilesetPreview()
{
    if (!m_tilePreview) return;
    const QSize tile = m_document.tileSize();
    syncTileSizeControls(m_tileWidth, m_tileHeight, tile);
    const QImage composite = m_document.compositeImage();
    QImage preview(qMax(1, composite.width() / tile.width() * 16), qMax(1, composite.height() / tile.height() * 16), QImage::Format_ARGB32); preview.fill(QColor("#eeeeee"));
    QPainter p(&preview); p.setRenderHint(QPainter::SmoothPixmapTransform, false);
    for (int y = 0; y < composite.height() / tile.height(); ++y) for (int x = 0; x < composite.width() / tile.width(); ++x) {
        const QRect source(x * tile.width(), y * tile.height(), tile.width(), tile.height()); const QRect target(x * 16, y * 16, 16, 16); p.drawImage(target, composite, source); p.setPen(QColor(40,40,40,100)); p.drawRect(target);
    }
    m_tilePreview->setPixmap(QPixmap::fromImage(preview).scaled(m_tilePreview->size() - QSize(12,12), Qt::KeepAspectRatio, Qt::FastTransformation));
    if (m_tilesList) {
        const QSignalBlocker blocker(m_tilesList); m_tilesList->clear();
        const int across=composite.width()/tile.width(), down=composite.height()/tile.height();
        for(int id=0;id<across*down;++id){const TileMetadata metadata=m_document.tileMetadata(id);m_tilesList->addItem(tr("Tile %1%2%3").arg(id).arg(metadata.collision?"  ◆":"").arg(metadata.terrain?"  ◈":""));}
        if(m_tilesList->count()>0)m_tilesList->setCurrentRow(0);
    }
}

void MainWindow::setCurrentFile(const QString &path)
{
    m_currentFile = path;
    const QString name = path.isEmpty() ? tr("Sem título") : QFileInfo(path).fileName();
    setWindowTitle(QString("%1%2 — Pixel Art Editor").arg(name, m_dirty ? " *" : ""));
}

void MainWindow::resetHistory() { m_history = {m_document}; m_historyIndex = 0; m_beforeEdit = m_document; }

void MainWindow::recordHistory()
{
    while (m_history.size() > m_historyIndex + 1) m_history.removeLast();
    m_history.push_back(m_document); m_historyIndex = m_history.size() - 1;
    m_dirty = true; updateStatus();
}

void MainWindow::undo()
{
    if (m_historyIndex <= 0) return;
    --m_historyIndex; m_document = m_history[m_historyIndex]; m_canvas->update(); m_dirty = true; refreshLayers(); refreshTimeline(); refreshTilesetPreview(); updateStatus();
}
void MainWindow::redo()
{
    if (m_historyIndex + 1 >= m_history.size()) return;
    ++m_historyIndex; m_document = m_history[m_historyIndex]; m_canvas->update(); m_dirty = true; refreshLayers(); refreshTimeline(); refreshTilesetPreview(); updateStatus();
}

bool MainWindow::saveProject()
{
    if (m_projectFile.isEmpty()) return saveProjectAs();
    QString error; if (!ProjectFormat::save(m_document, m_projectFile, &error)) { QMessageBox::critical(this, tr("Erro ao salvar"), error); return false; }
    m_dirty = false; setCurrentFile(m_projectFile); return true;
}
bool MainWindow::saveProjectAs()
{
    const QString path = QFileDialog::getSaveFileName(this, tr("Salvar projeto"), {}, tr("Projeto Pixel Art (*.pxl)")); if (path.isEmpty()) return false;
    m_projectFile = path.endsWith(".pxl", Qt::CaseInsensitive) ? path : path + ".pxl";
    return saveProject();
}
void MainWindow::newDocument()
{
    if(!maybeSave())return;
    const QSize size = requestDimensions(this, tr("Novo canvas"), QSize(64, 64));
    if (!size.isValid()) return;
    if(!m_document.create(size.width(), size.height())){QMessageBox::warning(this,tr("Dimensões inválidas"),tr("Informe valores entre 1 e 4096."));return;}
    syncTileSizeControls(m_tileWidth, m_tileHeight, m_document.tileSize());
    m_projectFile.clear(); m_dirty=false; setCurrentFile({}); m_canvas->update(); resetHistory(); refreshLayers(); refreshTimeline(); updateStatus(); refreshTilesetPreview();
}
void MainWindow::openImage()
{
    if(!maybeSave())return;
    const QString path=QFileDialog::getOpenFileName(this,tr("Abrir imagem"),{},tr("Imagens (*.png *.bmp *.jpg *.jpeg *.gif);;Todos os arquivos (*)")); if(path.isEmpty())return;
    QImage image(path); if(image.isNull()){QMessageBox::critical(this,tr("Erro ao abrir"),tr("Não foi possível ler a imagem."));return;}
    if(image.width()>4096||image.height()>4096){QMessageBox::warning(this,tr("Imagem muito grande"),tr("O limite do editor é 4096 × 4096 pixels."));return;}
    m_document.setImage(image); syncTileSizeControls(m_tileWidth, m_tileHeight, m_document.tileSize()); m_projectFile.clear(); m_dirty=false; setCurrentFile(path); resetHistory(); m_canvas->update(); refreshLayers(); refreshTimeline(); updateStatus(); refreshTilesetPreview();
}
void MainWindow::exportSpriteSheet()
{
    if (m_document.frameCount() < 1) return;
    QDialog dialog(this); dialog.setWindowTitle(tr("Exportar Sprite Sheet"));
    auto *layout = new QFormLayout(&dialog);
    auto *columns = new QSpinBox(&dialog); columns->setRange(1, m_document.frameCount()); columns->setValue(m_document.frameCount());
    auto *padding = new QSpinBox(&dialog); padding->setRange(0, 256); padding->setValue(0);
    auto *margin = new QSpinBox(&dialog); margin->setRange(0, 256); margin->setValue(0);
    auto *metadata = new QCheckBox(tr("Gerar arquivo JSON de metadados"), &dialog); metadata->setChecked(true);
    layout->addRow(tr("Colunas"), columns); layout->addRow(tr("Padding (px)"), padding); layout->addRow(tr("Margem (px)"), margin); layout->addRow(metadata);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog); layout->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept); connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    const QString path = QFileDialog::getSaveFileName(this, tr("Salvar Sprite Sheet"), {}, tr("PNG (*.png)")); if(path.isEmpty())return;
    const int count=m_document.frameCount(), cols=columns->value(), rows=(count+cols-1)/cols, pad=padding->value(), mar=margin->value();
    const QSize frameSize=m_document.image().size();
    QImage sheet(mar*2+cols*frameSize.width()+(cols-1)*pad, mar*2+rows*frameSize.height()+(rows-1)*pad, QImage::Format_ARGB32); sheet.fill(Qt::transparent);
    QPainter painter(&sheet); QJsonArray frames;
    for(int i=0;i<count;++i){ const int x=mar+(i%cols)*(frameSize.width()+pad), y=mar+(i/cols)*(frameSize.height()+pad); painter.drawImage(x,y,m_document.compositeImage(i)); frames.append(QJsonObject{{"x",x},{"y",y},{"width",frameSize.width()},{"height",frameSize.height()},{"duration",1000/m_document.fps()}}); }
    painter.end(); const QString pngPath=path.endsWith(".png",Qt::CaseInsensitive)?path:path+".png";
    if(!sheet.save(pngPath,"PNG")){QMessageBox::critical(this,tr("Erro"),tr("Não foi possível salvar a sprite sheet."));return;}
    if(metadata){ const QFileInfo info(pngPath); const QJsonObject json{{"sprite",info.completeBaseName()},{"width",sheet.width()},{"height",sheet.height()},{"frameWidth",frameSize.width()},{"frameHeight",frameSize.height()},{"columns",cols},{"rows",rows},{"padding",pad},{"margin",mar},{"fps",m_document.fps()},{"frames",frames}}; QFile file(info.path()+"/"+info.completeBaseName()+".json"); if(file.open(QIODevice::WriteOnly))file.write(QJsonDocument(json).toJson(QJsonDocument::Indented)); }
}

void MainWindow::togglePlayback()
{
    if (!m_playbackTimer) return;
    if (m_playbackTimer->isActive()) { m_playbackTimer->stop(); m_playAction->setText(tr("▶ Reproduzir")); }
    else { m_playbackTimer->setInterval(qMax(1, 1000 / m_document.fps())); m_playbackTimer->start(); m_playAction->setText(tr("⏸ Pausar")); }
    if (m_playButton) m_playButton->setText(m_playAction->text());
}
void MainWindow::exportPng()
{
    QString selectedFilter; const QString path=QFileDialog::getSaveFileName(this,tr("Exportar imagem"),{},tr("PNG (*.png);;BMP (*.bmp)"),&selectedFilter); if(path.isEmpty())return;
    const bool bmp=selectedFilter.startsWith("BMP");const QString suffix=bmp?".bmp":".png";const QByteArray format=bmp?"BMP":"PNG";
    const QString output=path.endsWith(suffix,Qt::CaseInsensitive)?path:path+suffix;
    if(!m_document.compositeImage().save(output,format.constData()))QMessageBox::critical(this,tr("Erro ao exportar"),tr("Não foi possível salvar a imagem."));
}
void MainWindow::chooseColor()
{
    const QColor color=QColorDialog::getColor(m_canvas->color(),this,tr("Escolher cor"),QColorDialog::ShowAlphaChannel); if(!color.isValid())return;
    m_canvas->setColor(color); const int oldSize=m_document.palette().size(); m_document.addPaletteColor(color); if(m_document.palette().size()!=oldSize){m_dirty=true;recordHistory();} m_colorPreview->setStyleSheet(QString("background:%1").arg(color.name(QColor::HexArgb))); refreshPalette();
}

bool MainWindow::maybeSave()
{
    if(!m_dirty)return true;
    const auto choice=QMessageBox::warning(this,tr("Alterações não salvas"),tr("Salvar as alterações antes de continuar?"),QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel,QMessageBox::Save);
    if(choice==QMessageBox::Save)return saveProject();
    return choice==QMessageBox::Discard;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(maybeSave())event->accept();else event->ignore();
}

void MainWindow::exportGif()
{
    const QString path=QFileDialog::getSaveFileName(this,tr("Exportar animação GIF"),{},tr("GIF animado (*.gif)"));if(path.isEmpty())return;
    QVector<QImage> frames;for(int i=0;i<m_document.frameCount();++i)frames.push_back(m_document.compositeImage(i));
    QString error;const QString output=path.endsWith(".gif",Qt::CaseInsensitive)?path:path+".gif";
    if(!GifFormat::save(frames,m_document.fps(),m_document.looping(),output,&error))QMessageBox::critical(this,tr("Erro ao exportar GIF"),error);
}
void MainWindow::updateStatus()
{
    if (!m_status) { m_status = new QLabel(this); statusBar()->addPermanentWidget(m_status); }
    m_status->setText(tr("Canvas: %1 × %2 px   |   Quadro: %3/%4   |   Camada: %5/%6   |   Zoom: %7%   |   Tile: %8 × %9")
        .arg(m_document.image().width()).arg(m_document.image().height()).arg(m_document.activeFrame() + 1).arg(m_document.frameCount())
        .arg(m_document.activeLayer() + 1).arg(m_document.layerCount()).arg(m_canvas->zoom()).arg(m_document.tileSize().width()).arg(m_document.tileSize().height()));
    const QString name = m_currentFile.isEmpty() ? tr("Sem título") : QFileInfo(m_currentFile).fileName(); setWindowTitle(QString("%1%2 — Pixel Art Editor").arg(name, m_dirty ? " *" : ""));
}
