#include "pch.h"
#include "PdfConverter.h"

bool PdfConverter::convertToImages(const QString &pdfPath, const QString &outputDir, int dpi) const
{
    QFileInfo fi(pdfPath);
    if (!fi.exists()) {
        return false;
    }

    QDir().mkpath(outputDir);

    const QString outPrefix = outputDir + '/' + fi.fileName();

    QProcess proc;
    proc.setProgram("pdftocairo");
    proc.setArguments({"-jpeg", "-r", QString::number(dpi), pdfPath, outPrefix});
    proc.start();
    proc.waitForFinished(-1);

    return proc.exitCode() == 0;
}
