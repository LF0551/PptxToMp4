#include "PresentationBatchConverter.h"

#include "PdfConverter.h"
#include "PptxConverter.h"

namespace {


bool moveFile(const QString &from, const QString &to)
{
    if (QFile::exists(to)) {
        return false;
    }
    if (!QFile::copy(from, to)) {
        return false;
    }
    QFile::remove(from);
    return true;
}

void renameAndMoveAllFilesInDirectory(const QString &srcDirPath, const QString &dstDirPath)
{
    QDir().mkpath(dstDirPath);
    QDir srcDir(srcDirPath);
    QDir dstDir(dstDirPath);

    if (!srcDir.exists() || !dstDir.exists()) {
        return;
    }

    QFileInfoList fileList = srcDir.entryInfoList(QDir::Files);
    QCollator collator;
    collator.setNumericMode(true);

    std::sort(fileList.begin(), fileList.end(), [&collator](const QFileInfo &a, const QFileInfo &b) {
        return collator.compare(a.fileName(), b.fileName()) < 0;
    });

    int count = dstDir.entryInfoList(QDir::Files).size();

    for (const QFileInfo &fileInfo : fileList) {
        const QString oldFilePath = fileInfo.absoluteFilePath();
        const QString newFileName = QString::number(++count) + "." + fileInfo.suffix();
        const QString newFilePath = dstDir.absoluteFilePath(newFileName);

        moveFile(oldFilePath, newFilePath);
        srcDir.remove(oldFilePath);
    }
}

QStringList findFilesWithExtension(QDir dir, const QString &extension)
{
    if (!dir.exists()) {
        return {};
    }

    QString filterExt = extension;
    if (!filterExt.startsWith('.')) {
        filterExt.prepend('.');
    }

    dir.setNameFilters({"*" + filterExt});
    dir.setFilter(QDir::Files);

    return dir.entryList();
}

} // namespace

bool PresentationBatchConverter::convertFolder(const QString &inputDirPath, const QString &outputDirPath) const
{
    QDir inputDir(inputDirPath);
    if (!inputDir.exists()) {
        return false;
    }

    QDir outputDir(outputDirPath);
    if (!outputDir.exists() && !outputDir.mkpath(".")) {
        return false;
    }

    const auto pptxList = findFilesWithExtension(inputDir, "pptx");
    const auto pdfList = findFilesWithExtension(inputDir, "pdf");

    const QString tmpPath = QDir::toNativeSeparators(QDir::tempPath() + "/curConvertResult");
    if (!QDir().mkpath(tmpPath)) {
        return false;
    }

    const PptxConverter pptxConverter;
    const PdfConverter pdfConverter;

    for (const auto &item : pptxList) {
        const QString cleanPath = QDir::toNativeSeparators(inputDirPath + "/" + item);
        pptxConverter.convertToImages(cleanPath, tmpPath);
        renameAndMoveAllFilesInDirectory(tmpPath, outputDirPath);
    }

    for (const auto &item : pdfList) {
        const QString cleanPath = QDir::toNativeSeparators(inputDirPath + "/" + item);
        pdfConverter.convertToImages(cleanPath, tmpPath, 150);
        renameAndMoveAllFilesInDirectory(tmpPath, outputDirPath);
    }

    return true;
}
