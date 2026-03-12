#pragma once

#include <QString>

class PresentationBatchConverter
{
public:
    bool convertFolder(const QString &inputDir, const QString &outputDir) const;
};
