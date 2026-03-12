#include "PptxConverter.h"

bool PptxConverter::convertToImages(const QString &pptxPath, const QString &outputDir) const
{
    QAxObject *powerpoint = new QAxObject("PowerPoint.Application");
    if (!powerpoint || powerpoint->isNull()) {
        delete powerpoint;
        return false;
    }

    powerpoint->setProperty("Visible", false);

    QAxObject *presentations = powerpoint->querySubObject("Presentations");
    if (!presentations || presentations->isNull()) {
        powerpoint->dynamicCall("Quit()");
        delete presentations;
        delete powerpoint;
        return false;
    }

    QAxObject *presentation = presentations->querySubObject("Open(const QString&)", pptxPath);
    if (!presentation || presentation->isNull()) {
        powerpoint->dynamicCall("Quit()");
        delete presentation;
        delete presentations;
        delete powerpoint;
        return false;
    }

    presentation->dynamicCall("Export(const QString&, const QString&, int, int)",
                              outputDir,
                              "JPG",
                              1920,
                              1080);

    presentation->dynamicCall("Close()");
    powerpoint->dynamicCall("Quit()");
    delete presentation;
    delete presentations;
    delete powerpoint;

    return true;
}
