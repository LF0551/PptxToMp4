#pragma once

#include <QString>

class PdfConverter
{
public:
    bool convertToImages(const QString &pdfPath, const QString &outputDir, int dpi = 150) const;
};
