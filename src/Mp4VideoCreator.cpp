#include "Mp4VideoCreator.h"

bool Mp4VideoCreator::createVideo(const QStringList &orderedImages,
                                  const QString &outputPath,
                                  int fps,
                                  double secondsPerImage,
                                  const std::function<void(int, int)> &progressCallback,
                                  QString *error) const
{
    if (orderedImages.isEmpty()) {
        if (error) {
            *error = "Нет изображений для создания видео.";
        }
        return false;
    }

    const cv::Mat firstFrame = cv::imread(orderedImages.first().toStdString(), cv::IMREAD_COLOR);
    if (firstFrame.empty()) {
        if (error) {
            *error = "Не удалось прочитать первое изображение.";
        }
        return false;
    }

    const int framesPerImage = std::max(1, static_cast<int>(std::lround(secondsPerImage * fps)));

    cv::VideoWriter writer(
        outputPath.toStdString(),
        cv::VideoWriter::fourcc('m', 'p', '4', 'v'),
        static_cast<double>(fps),
        firstFrame.size());

    if (!writer.isOpened()) {
        if (error) {
            *error = "Не удалось открыть MP4-файл для записи.";
        }
        return false;
    }

    const int totalImages = orderedImages.size();
    int processedImages = 0;

    if (progressCallback) {
        progressCallback(processedImages, totalImages);
    }

    for (const QString &filePath : orderedImages) {
        cv::Mat frame = cv::imread(filePath.toStdString(), cv::IMREAD_COLOR);
        if (frame.empty()) {
            writer.release();
            if (error) {
                *error = "Не удалось прочитать изображение: " + filePath;
            }
            return false;
        }

        if (frame.size() != firstFrame.size()) {
            cv::resize(frame, frame, firstFrame.size(), 0.0, 0.0, cv::INTER_AREA);
        }

        for (int i = 0; i < framesPerImage; ++i) {
            writer.write(frame);
        }

        ++processedImages;
        if (progressCallback) {
            progressCallback(processedImages, totalImages);
        }
    }

    writer.release();
    return true;
}
