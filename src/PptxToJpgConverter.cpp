#include "PptxToJpgConverter.h"
#include "PresentationBatchConverter.h"

void convertPptxToJpg(const QString &pptxPath, const QString &outputDir)
{
    PresentationBatchConverter().convertFolder(pptxPath, outputDir);
}
