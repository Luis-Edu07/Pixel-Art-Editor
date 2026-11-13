#pragma once

#include <QImage>
#include <QString>
#include <QVector>

namespace GifFormat {
bool save(const QVector<QImage> &frames, int fps, bool loop, const QString &path, QString *error = nullptr);
}
