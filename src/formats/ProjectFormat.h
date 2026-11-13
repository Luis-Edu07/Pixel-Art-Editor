#pragma once

#include "core/PixelDocument.h"

#include <QString>

namespace ProjectFormat {
bool save(const PixelDocument &document, const QString &path, QString *error = nullptr);
bool load(PixelDocument &document, const QString &path, QString *error = nullptr);
}
