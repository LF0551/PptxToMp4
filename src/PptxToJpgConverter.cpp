#include "PptxToJpgConverter.h"
#include <iostream>
#include <QAxObject>
#include <QDir>
#include <QApplication>
#include <QFileInfo>
#include <QProcess>
#include <QCollator>
#include <QImage>
#include <QPainter>

namespace {

constexpr int kLandscapeWidth = 1920;
constexpr int kLandscapeHeight = 1080;

bool isPortrait(const QImage &image)
{
    return image.height() > image.width();
}

void convertPortraitImageToLandscape(const QString &filePath)
{
    QImage source(filePath);
    if (source.isNull() || !isPortrait(source)) {
        return;
    }

    const QSize targetSize(kLandscapeWidth, kLandscapeHeight);
    const QImage scaled = source.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QImage canvas(targetSize, QImage::Format_RGB32);
    canvas.fill(Qt::black);

    QPainter painter(&canvas);
    const int x = (targetSize.width() - scaled.width()) / 2;
    const int y = (targetSize.height() - scaled.height()) / 2;
    painter.drawImage(x, y, scaled);
    painter.end();

    canvas.save(filePath);
}

void normalizePortraitImagesInDirectory(const QString &dirPath)
{
    QDir directory(dirPath);
    if (!directory.exists()) {
        return;
    }

    const QFileInfoList imageFiles = directory.entryInfoList(QDir::Files, QDir::Name);
    for (const QFileInfo &imageFile : imageFiles) {
        convertPortraitImageToLandscape(imageFile.absoluteFilePath());
    }
}

}


bool moveFile(const QString& from, const QString& to) {
    if (QFile::exists(to)) {
        return false;
    }
    if (!QFile::copy(from, to)) {
        return false;
    }
    QFile::remove(from);
    return true;
}

void renameAndMoveAllFilesInDirectory(const QString& srcDirPath, const QString& dstDirPath) {
    QDir().mkpath(dstDirPath);
    QDir srcDir(srcDirPath);
    QDir dstDir(dstDirPath);

    if (!srcDir.exists() || ! dstDir.exists()) {
        return;
    }


    QFileInfoList fileList = srcDir.entryInfoList(QDir::Files);
    QCollator collator;
    collator.setNumericMode(true);

    std::sort(fileList.begin(), fileList.end(),
              [&collator](const QFileInfo &a, const QFileInfo &b) {
                  return collator.compare(a.fileName(), b.fileName()) < 0;
              });

    int count = dstDir.entryInfoList(QDir::Files).size();

    for (const QFileInfo& fileInfo : fileList) {
        QString oldFilePath = fileInfo.absoluteFilePath();
        QString newFileName = QString::number(++count) + "." + fileInfo.suffix();
        QString newFilePath = dstDir.absoluteFilePath(newFileName);

        moveFile(oldFilePath, newFilePath);
        srcDir.remove(oldFilePath);
    }


}


QStringList findFilesWithExtension(QDir dir,  const QString& extension)
{
    if (!dir.exists()) {
        return QStringList();
    }

    // Убедитесь, что расширение начинается с точки
    QString filterExt = extension;
    if (!filterExt.startsWith('.'))
        filterExt.prepend('.');

    QStringList filters;
    filters << ("*" + filterExt);

    dir.setNameFilters(filters);
    dir.setFilter(QDir::Files);

    return dir.entryList();
}

void convertPptx(const QString& pptxPath, const QString& outputDir) {
    QAxObject* powerpoint = new QAxObject("PowerPoint.Application");
    if (!powerpoint || powerpoint->isNull()) {
        delete powerpoint;
        return;
    }
    powerpoint->setProperty("Visible", false);

    QAxObject* presentations = powerpoint->querySubObject("Presentations");
    if (!presentations || presentations->isNull()) {
        powerpoint->dynamicCall("Quit()");
        delete presentations;
        delete powerpoint;
        return;
    }

    QAxObject* presentation = presentations->querySubObject("Open(const QString&)", pptxPath);
    if (!presentation || presentation->isNull()) {
        powerpoint->dynamicCall("Quit()");
        delete presentation;
        delete presentations;
        delete powerpoint;
        return;
    }

    // Экспорт всех слайдов как изображений
    presentation->dynamicCall("Export(const QString&, const QString&, int, int)",
                              outputDir, "JPG", 1920, 1080);

    presentation->dynamicCall("Close()");
    powerpoint->dynamicCall("Quit()");
    delete presentation;
    delete presentations;
    delete powerpoint;
}


bool convertPdfToJpg(const QString &pdfPath, const QString &outputDir, int dpi)
{
    QFileInfo fi(pdfPath);
    if(!fi.exists())
        return false;

    QDir().mkpath(outputDir);


    QString outPrefix = outputDir + '/' + fi.fileName();

    QProcess proc;
    proc.setProgram("pdftocairo");
    proc.setArguments({
                              "-jpeg",
                              "-r", QString::number(dpi),
                              pdfPath,
                              outPrefix
                      });

    proc.start();
    proc.waitForFinished(-1);

    if (proc.exitCode() != 0) {

        return false;
    }

    return true;
}

void convertPptxToJpg(const QString& pptxPath, const QString& outputDir)
{
    QDir inputDir(pptxPath);
    if (!inputDir.exists()) {
        return;
    }

    QDir dir(outputDir);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            return;
        }
    }

    auto pptxList = findFilesWithExtension(inputDir, "pptx");
    auto pdfList = findFilesWithExtension(inputDir, "pdf");


    QString tmpPath =QDir::toNativeSeparators( QDir::tempPath() +  "/curConvertResult");
    if (!QDir().mkpath(tmpPath)) {
        return;
    }

    for (const auto& item : pptxList)
    {
        QString cleanPath = QDir::toNativeSeparators(pptxPath + "/" + item);
        convertPptx(cleanPath, tmpPath);
        renameAndMoveAllFilesInDirectory(tmpPath,outputDir);
    }

    for (const auto& item : pdfList)
    {
        QString cleanPath = QDir::toNativeSeparators(pptxPath + "/" + item);
        convertPdfToJpg(cleanPath, tmpPath, 150);
        renameAndMoveAllFilesInDirectory(tmpPath,outputDir);
    }

    normalizePortraitImagesInDirectory(outputDir);
}
