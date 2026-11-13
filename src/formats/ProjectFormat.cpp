#include "ProjectFormat.h"

#include <QBuffer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
QString encodeImage(const QImage &image)
{
    QByteArray data; QBuffer buffer(&data);
    if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG")) return {};
    return QString::fromLatin1(data.toBase64());
}

bool decodeImage(const QJsonValue &value, QImage *image)
{
    return image && image->loadFromData(QByteArray::fromBase64(value.toString().toLatin1()), "PNG")
           && image->width() <= 4096 && image->height() <= 4096;
}
}

namespace ProjectFormat {
bool save(const PixelDocument &document, const QString &path, QString *error)
{
    QJsonArray palette;
    for (const QColor &color : document.palette()) palette.append(color.name(QColor::HexArgb));
    QJsonArray layers;
    for (const PixelLayer &layer : document.layers()) {
        QJsonArray frames;
        for (const QImage &frame : layer.frames) frames.append(encodeImage(frame));
        layers.append(QJsonObject{{"name", layer.name}, {"visible", layer.visible}, {"locked", layer.locked},
                                  {"opacity", layer.opacity}, {"frames", frames}});
    }
    QJsonArray tiles;
    for (int i = 0; i < document.tileMetadata().size(); ++i) {
        const TileMetadata metadata = document.tileMetadata()[i];
        tiles.append(QJsonObject{{"id", i}, {"tag", metadata.tag}, {"collision", metadata.collision}, {"terrain", metadata.terrain}, {"autoTileMask", metadata.autoTileMask}});
    }
    const QSize tile = document.tileSize();
    const QJsonObject root{{"format", "pixel-art-editor"}, {"version", 2},
        {"width", document.image().width()}, {"height", document.image().height()},
        {"layers", layers}, {"activeLayer", document.activeLayer()}, {"activeFrame", document.activeFrame()},
        {"fps", document.fps()}, {"looping", document.looping()}, {"palette", palette},
        {"tileWidth", tile.width()}, {"tileHeight", tile.height()}, {"tiles", tiles}};
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) { if (error) *error = file.errorString(); return false; }
    const QByteArray contents = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (file.write(contents) != contents.size()) { if (error) *error = file.errorString(); return false; }
    return true;
}

bool load(PixelDocument &document, const QString &path, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) { if (error) *error = file.errorString(); return false; }
    QJsonParseError parseError;
    const QJsonDocument json = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !json.isObject()) {
        if (error) *error = QObject::tr("Arquivo de projeto inválido: %1").arg(parseError.errorString()); return false;
    }
    const QJsonObject root = json.object();
    const int version = root.value("version").toInt();
    if (root.value("format").toString() != "pixel-art-editor" || (version != 1 && version != 2)) {
        if (error) *error = QObject::tr("Formato ou versão de projeto não suportado."); return false;
    }

    PixelDocument loaded;
    if (version == 1) {
        QImage image;
        if (!decodeImage(root.value("image"), &image)) { if (error) *error = QObject::tr("Imagem de projeto inválida."); return false; }
        loaded.setImage(image);
    } else {
        const QJsonArray layerJson = root.value("layers").toArray();
        if (layerJson.isEmpty() || layerJson.size() > 256) { if (error) *error = QObject::tr("O projeto não contém camadas válidas."); return false; }
        QVector<PixelLayer> layers;
        for (const QJsonValue &layerValue : layerJson) {
            const QJsonObject object = layerValue.toObject();
            const QJsonArray framesJson = object.value("frames").toArray();
            if (framesJson.isEmpty() || framesJson.size() > 512) { if (error) *error = QObject::tr("Quantidade de quadros inválida."); return false; }
            PixelLayer layer; layer.name = object.value("name").toString(QObject::tr("Camada"));
            layer.visible = object.value("visible").toBool(true); layer.locked = object.value("locked").toBool(false);
            layer.opacity = qBound(0.0, object.value("opacity").toDouble(1.0), 1.0);
            for (const QJsonValue &frameValue : framesJson) {
                QImage image;
                if (!decodeImage(frameValue, &image)) { if (error) *error = QObject::tr("Quadro de imagem inválido."); return false; }
                layer.frames.push_back(image.convertToFormat(QImage::Format_ARGB32));
            }
            layers.push_back(layer);
        }
        if (!loaded.setLayers(layers, root.value("activeLayer").toInt(), root.value("activeFrame").toInt())) {
            if (error) *error = QObject::tr("As dimensões dos quadros do projeto são inconsistentes."); return false;
        }
        loaded.setFps(root.value("fps").toInt(12)); loaded.setLooping(root.value("looping").toBool(true));
        for (const QJsonValue &tileValue : root.value("tiles").toArray()) {
            const QJsonObject tile = tileValue.toObject(); const int id = tile.value("id").toInt(-1);
            loaded.setTileMetadata(id, {tile.value("tag").toString(), tile.value("collision").toBool(), tile.value("terrain").toBool(), tile.value("autoTileMask").toInt()});
        }
    }

    QVector<QColor> palette;
    for (const QJsonValue &value : root.value("palette").toArray()) { const QColor color(value.toString()); if (color.isValid()) palette.push_back(color); }
    if (!palette.isEmpty()) loaded.setPalette(palette);
    loaded.setTileSize(QSize(qBound(1, root.value("tileWidth").toInt(loaded.image().width()), 4096),
                             qBound(1, root.value("tileHeight").toInt(loaded.image().height()), 4096)));
    document = loaded;
    return true;
}
}
