#include <QStandardPaths>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include <QPixmap>
#include <QListWidgetItem>
#include <algorithm>
#include <QImageReader>
#include <QFile>
#include <QMimeData>
#include <QUrl>
#include <QAbstractItemModel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <cmath>
#include "ImageRenamerWidget.h"
#include "PptxToJpgConverter.h"
#include <QDragEnterEvent>
#include <QCollator>
#include "DraggableListWidget.h"
#include <opencv2/opencv.hpp>

const int iconSize = 128;


bool removeDir(const QString &dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists())
        return true;

    QFileInfoList entries = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden |
                                              QDir::AllDirs | QDir::Files, QDir::DirsFirst);

    for (const QFileInfo &entry: entries)
    {
        if (entry.isDir())
        {
            if (!removeDir(entry.absoluteFilePath()))
            {
                return false;
            }
        }
        else
        {
            if (!QFile::remove(entry.absoluteFilePath()))
            {
                return false;
            }
        }
    }

    return dir.rmdir(dirPath);
}


ImageRenamerWidget::ImageRenamerWidget(QWidget *parent)
        : QWidget(parent)
{
    setWindowTitle("Переименовщик изображений");
    resize(760, 580);

    listWidget = new DraggableListWidget(this);

    listWidget->setDragEnabled(true);
    listWidget->setAcceptDrops(true);
    listWidget->setDragDropMode(QAbstractItemView::InternalMove);
    listWidget->setDefaultDropAction(Qt::MoveAction);

    listWidget->setViewMode(QListWidget::IconMode);
    listWidget->setFlow(QListView::LeftToRight);
    listWidget->setWrapping(true);
    listWidget->setSpacing(20);
    listWidget->setIconSize(QSize(128, 128));
    listWidget->setResizeMode(QListView::Adjust);
    listWidget->setUniformItemSizes(true);

    btnOpen = new QPushButton("Конвертировать в изображения", this);
    btnRename = new QPushButton("Переименовать по порядку", this);
    btnCreateVideo = new QPushButton("Склеить в MP4", this);

    btnChooseSourceFolder = new QPushButton("...", this);
    btnChooseOutputFolder = new QPushButton("...", this);
    btnChooseVideoPath = new QPushButton("...", this);

    sourceFolderEdit = new QLineEdit(this);
    outputFolderEdit = new QLineEdit(this);
    videoPathEdit = new QLineEdit(this);

    sourceFolderEdit->setPlaceholderText("Папка с исходниками (PPTX/PDF)");
    outputFolderEdit->setPlaceholderText("Папка для изображений");
    videoPathEdit->setPlaceholderText("Путь к итоговому MP4");

    fpsSpinBox = new QSpinBox(this);
    fpsSpinBox->setRange(1, 120);
    fpsSpinBox->setValue(30);
    fpsSpinBox->setSuffix(" FPS");

    secondsPerImageSpinBox = new QDoubleSpinBox(this);
    secondsPerImageSpinBox->setRange(0.05, 60.0);
    secondsPerImageSpinBox->setSingleStep(0.1);
    secondsPerImageSpinBox->setDecimals(2);
    secondsPerImageSpinBox->setValue(1.0);
    secondsPerImageSpinBox->setSuffix(" сек");

    auto *sourceRow = new QHBoxLayout();
    sourceRow->addWidget(new QLabel("Исходники:", this));
    sourceRow->addWidget(sourceFolderEdit);
    sourceRow->addWidget(btnChooseSourceFolder);

    auto *outputRow = new QHBoxLayout();
    outputRow->addWidget(new QLabel("Изображения:", this));
    outputRow->addWidget(outputFolderEdit);
    outputRow->addWidget(btnChooseOutputFolder);

    auto *videoRow = new QHBoxLayout();
    videoRow->addWidget(new QLabel("Видео MP4:", this));
    videoRow->addWidget(videoPathEdit);
    videoRow->addWidget(btnChooseVideoPath);

    auto *fpsRow = new QHBoxLayout();
    fpsRow->addWidget(new QLabel("Частота кадров:", this));
    fpsRow->addWidget(fpsSpinBox);
    fpsRow->addSpacing(16);
    fpsRow->addWidget(new QLabel("Время на 1 изображение:", this));
    fpsRow->addWidget(secondsPerImageSpinBox);
    fpsRow->addStretch();

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addLayout(sourceRow);
    layout->addLayout(outputRow);
    layout->addWidget(btnOpen);
    layout->addWidget(listWidget);
    layout->addWidget(btnRename);
    layout->addLayout(videoRow);
    layout->addLayout(fpsRow);
    layout->addWidget(btnCreateVideo);

    connect(btnChooseSourceFolder, &QPushButton::clicked, this, &ImageRenamerWidget::chooseSourceFolder);
    connect(btnChooseOutputFolder, &QPushButton::clicked, this, &ImageRenamerWidget::chooseOutputFolder);
    connect(btnChooseVideoPath, &QPushButton::clicked, this, &ImageRenamerWidget::chooseVideoPath);
    connect(btnOpen, &QPushButton::clicked, this, &ImageRenamerWidget::onOpenFolder);
    connect(btnRename, &QPushButton::clicked, this, &ImageRenamerWidget::renameFiles);
    connect(btnCreateVideo, &QPushButton::clicked, this, &ImageRenamerWidget::createMp4Video);

    const QString picturesPath = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).value(0);
    sourceFolderEdit->setText(picturesPath);
    outputFolderEdit->setText(picturesPath + "/converted_images");
    videoPathEdit->setText(picturesPath + "/result.mp4");
}

QStringList ImageRenamerWidget::getOrderedFilesFromListWidget() const
{
    QStringList orderedFiles;
    orderedFiles.reserve(listWidget->count());

    for (int i = 0; i < listWidget->count(); ++i)
    {
        const QString label = listWidget->item(i)->text();
        for (const QString &path : currentFiles)
        {
            if (QFileInfo(path).fileName() == label)
            {
                orderedFiles << path;
                break;
            }
        }
    }

    if (orderedFiles.size() != currentFiles.size())
    {
        return currentFiles;
    }

    return orderedFiles;
}

QStringList ImageRenamerWidget::getImageExtensions()
{
    QStringList extensions;
    extensions.reserve(QImageReader::supportedImageFormats().size());
    for (const QByteArray &fmt: QImageReader::supportedImageFormats())
    {
        extensions << "*." + QString::fromLatin1(fmt).toLower();
    }
    return extensions;
}

QStringList ImageRenamerWidget::getImageNameFilters()
{
    return getImageExtensions();
}

void ImageRenamerWidget::chooseSourceFolder()
{
    const QString currentPath = sourceFolderEdit->text().trimmed();
    const QString startPath = currentPath.isEmpty()
            ? QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).value(0)
            : currentPath;

    const QString folder = QFileDialog::getExistingDirectory(this, "Выберите папку с исходниками (PPTX/PDF)", startPath);
    if (!folder.isEmpty())
    {
        sourceFolderEdit->setText(folder);
    }
}

void ImageRenamerWidget::chooseOutputFolder()
{
    const QString currentPath = outputFolderEdit->text().trimmed();
    const QString startPath = currentPath.isEmpty()
            ? QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).value(0)
            : currentPath;

    const QString folder = QFileDialog::getExistingDirectory(this, "Выберите папку для сохранения изображений", startPath);
    if (!folder.isEmpty())
    {
        outputFolderEdit->setText(folder);
    }
}

void ImageRenamerWidget::chooseVideoPath()
{
    const QString startPath = videoPathEdit->text().trimmed().isEmpty()
            ? QStandardPaths::standardLocations(QStandardPaths::MoviesLocation).value(0) + "/result.mp4"
            : videoPathEdit->text().trimmed();

    const QString outputPath = QFileDialog::getSaveFileName(this, "Сохранить видео", startPath, "MP4 Video (*.mp4)");
    if (!outputPath.isEmpty())
    {
        videoPathEdit->setText(outputPath);
    }
}

void ImageRenamerWidget::onOpenFolder()
{
    const QString folder = sourceFolderEdit->text().trimmed();
    const QString output = outputFolderEdit->text().trimmed();

    if (folder.isEmpty() || output.isEmpty())
    {
        QMessageBox::warning(this, "Ошибка", "Заполните папку исходников и папку изображений.");
        return;
    }

    if (!QDir(folder).exists())
    {
        QMessageBox::warning(this, "Ошибка", "Папка с исходниками не существует: " + folder);
        return;
    }

    QDir outputDir(output);
    if (outputDir.exists() && outputDir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs).count() > 0)
    {
        const auto btn = QMessageBox::warning(this, "Папка не пуста",
            "Папка \"" + output + "\" не пуста. Всё содержимое будет удалено. Продолжить?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (btn != QMessageBox::Yes)
            return;
    }

    removeDir(output);

    convertPptxToJpg(folder, output);

    QDir dir(output);
    const QStringList filters = getImageNameFilters();
    const QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);

    currentFiles.clear();
    for (const QFileInfo &file: files)
    {
        currentFiles << file.absoluteFilePath();
    }

    if (!currentFiles.isEmpty() && videoPathEdit->text().trimmed().isEmpty())
    {
        videoPathEdit->setText(QFileInfo(currentFiles.first()).absolutePath() + "/result.mp4");
    }

    refreshListWidget();
}

void ImageRenamerWidget::renameFiles()
{
    if (currentFiles.isEmpty())
    {
        QMessageBox::warning(this, "Ошибка", "Нет файлов для переименования.");
        return;
    }

    const QStringList orderedFiles = getOrderedFilesFromListWidget();

    const QString dirPath = QFileInfo(orderedFiles.first()).path();

    QStringList tmpFiles;
    tmpFiles.reserve(orderedFiles.size());
    for (int i = 0; i < orderedFiles.size(); ++i)
    {
        const QFileInfo oldFile(orderedFiles[i]);
        const QString tmpName = dirPath + "/" + QString::number(i + 1) + "." + oldFile.suffix() + ".tmp";
        if (!QFile::rename(orderedFiles[i], tmpName))
        {
            QMessageBox::critical(this, "Ошибка", "Не удалось переименовать: " + orderedFiles[i]);
            return;
        }
        tmpFiles << tmpName;
    }

    QStringList finalFiles;
    finalFiles.reserve(tmpFiles.size());
    for (const QString &tmpPath : tmpFiles)
    {
        const QString finalPath = tmpPath.chopped(4);
        if (!QFile::rename(tmpPath, finalPath))
        {
            QMessageBox::critical(this, "Ошибка", "Не удалось финализировать: " + tmpPath);
            return;
        }
        finalFiles << finalPath;
    }

    currentFiles = finalFiles;
    QMessageBox::information(this, "Готово", "Файлы успешно переименованы!");
    refreshListWidget();
}

void ImageRenamerWidget::createMp4Video()
{
    if (currentFiles.isEmpty())
    {
        QMessageBox::warning(this, "Ошибка", "Нет изображений для создания видео.");
        return;
    }

    QString outputPath = videoPathEdit->text().trimmed();
    if (outputPath.isEmpty())
    {
        outputPath = QFileInfo(currentFiles.first()).absolutePath() + "/result.mp4";
        videoPathEdit->setText(outputPath);
    }

    const QStringList orderedFiles = getOrderedFilesFromListWidget();
    const cv::Mat firstFrame = cv::imread(orderedFiles.first().toStdString(), cv::IMREAD_COLOR);
    if (firstFrame.empty())
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось прочитать первое изображение.");
        return;
    }

    const int fps = fpsSpinBox->value();
    const int framesPerImage = std::max(1, static_cast<int>(std::lround(secondsPerImageSpinBox->value() * fps)));

    cv::VideoWriter writer(
        outputPath.toStdString(),
        cv::VideoWriter::fourcc('m', 'p', '4', 'v'),
        static_cast<double>(fps),
        firstFrame.size());

    if (!writer.isOpened())
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось открыть MP4-файл для записи.");
        return;
    }

    for (const QString &filePath : orderedFiles)
    {
        cv::Mat frame = cv::imread(filePath.toStdString(), cv::IMREAD_COLOR);
        if (frame.empty())
        {
            writer.release();
            QMessageBox::critical(this, "Ошибка", "Не удалось прочитать изображение: " + filePath);
            return;
        }

        if (frame.size() != firstFrame.size())
        {
            cv::resize(frame, frame, firstFrame.size(), 0.0, 0.0, cv::INTER_AREA);
        }

        for (int i = 0; i < framesPerImage; ++i)
        {
            writer.write(frame);
        }
    }

    writer.release();
    QMessageBox::information(this, "Готово", "Видео успешно создано: " + outputPath);
}

void ImageRenamerWidget::refreshListWidget()
{
    listWidget->clear();

    QCollator collator;
    collator.setNumericMode(true);
    std::sort(currentFiles.begin(), currentFiles.end(), collator);

    for (int i = 0; i < currentFiles.size(); ++i)
    {
        const auto &filePath = currentFiles[i];
        QFileInfo fi(filePath);
        QPixmap pixmap(filePath);
        if (pixmap.isNull())
        {
            pixmap = QPixmap(iconSize, iconSize);
            pixmap.fill(Qt::gray);
        }
        else
        {
            pixmap = pixmap.scaled(iconSize, iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        listWidget->addItem(new QListWidgetItem(QIcon(pixmap), fi.fileName()));
    }
}
