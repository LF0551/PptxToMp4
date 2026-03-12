#pragma once

#include <QString>

class PptxConverter
{
public:
    bool convertToImages(const QString &pptxPath, const QString &outputDir) const;
};
