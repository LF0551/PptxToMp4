#pragma once

#include <functional>

#include <QString>
#include <QStringList>

class Mp4VideoCreator
{
public:
    bool createVideo(const QStringList &orderedImages,
                     const QString &outputPath,
                     int fps,
                     double secondsPerImage,
                     const std::function<void(int, int)> &progressCallback = {},
                     QString *error = nullptr) const;
};
