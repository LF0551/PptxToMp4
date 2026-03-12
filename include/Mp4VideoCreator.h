#pragma once

#include <QString>
#include <QStringList>

class Mp4VideoCreator
{
public:
    bool createVideo(const QStringList &orderedImages,
                     const QString &outputPath,
                     int fps,
                     double secondsPerImage,
                     QString *error = nullptr) const;
};
