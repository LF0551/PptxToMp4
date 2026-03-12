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
        return true; // Директория уже не существует

    // Получаем список всех файлов и папок
    QFileInfoList entries = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden |
                                              QDir::AllDirs | QDir::Files, QDir::DirsFirst);

    // Сначала удаляем поддиректории рекурсивно, потом файлы
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

    // Удаляем саму папку
    return dir.rmdir(dirPath);
}


ImageRenamerWidget::ImageRenamerWidget(QWidget *parent)
        : QWidget(parent)
{
    setWindowTitle("Переименовщик изображений");
    resize(600, 500);


    listWidget = new DraggableListWidget(this);

    listWidget->setDragEnabled(true);
    listWidget->setAcceptDrops(true);
    listWidget->setDragDropMode(QAbstractItemView::InternalMove); // ← ключевой режим
    listWidget->setDefaultDropAction(Qt::MoveAction);

    // Настройки IconMode
    listWidget->setViewMode(QListWidget::IconMode);
    listWidget->setFlow(QListView::LeftToRight);
    listWidget->setWrapping(true);
    listWidget->setSpacing(20);
    listWidget->setIconSize(QSize(128, 128));
    listWidget->setResizeMode(QListView::Adjust); // ← помогает с пересчётом позиций
    listWidget->setUniformItemSizes(true);


    btnOpen = new QPushButton("Открыть папку", this);
    btnRename = new QPushButton("Переименовать по порядку", this);
    btnCreateVideo = new QPushButton("Склеить в MP4", this);

    fpsSpinBox = new QSpinBox(this);
    fpsSpinBox->setRange(1, 120);
    fpsSpinBox->setValue(30);
    fpsSpinBox->setSuffix(" FPS");

    auto *fpsRow = new QHBoxLayout();
    fpsRow->addWidget(new QLabel("Частота кадров:", this));
    fpsRow->addWidget(fpsSpinBox);
    fpsRow->addStretch();

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(btnOpen);
    layout->addWidget(listWidget);
    layout->addWidget(btnRename); // убрали btnUp/btnDown — их нет в объявлении
    layout->addLayout(fpsRow);
    layout->addWidget(btnCreateVideo);

    connect(btnOpen, &QPushButton::clicked, this, &ImageRenamerWidget::onOpenFolder);
    connect(btnRename, &QPushButton::clicked, this, &ImageRenamerWidget::renameFiles);
    connect(btnCreateVideo, &QPushButton::clicked, this, &ImageRenamerWidget::createMp4Video);
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

void ImageRenamerWidget::onOpenFolder()
{
    const QString picturesPath = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).value(0);
    const QString folder = QFileDialog::getExistingDirectory(this, "Выберите папку с исходниками (PPTX/PDF)", picturesPath);
    if (folder.isEmpty()) return;

    const QString output = QFileDialog::getExistingDirectory(this, "Выберите папку для сохранения изображений", picturesPath);
    if (output.isEmpty()) return;

    if (QDir(output).entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs).count() > 0)
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

    // Первый проход: переименовываем во временные имена, чтобы избежать конфликтов
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

    // Второй проход: убираем суффикс .tmp
    QStringList finalFiles;
    finalFiles.reserve(tmpFiles.size());
    for (const QString &tmpPath : tmpFiles)
    {
        const QString finalPath = tmpPath.chopped(4); // убрать ".tmp"
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

    const QString outputPath = QFileDialog::getSaveFileName(
        this,
        "Сохранить видео",
        QFileInfo(currentFiles.first()).absolutePath() + "/result.mp4",
        "MP4 Video (*.mp4)");

    if (outputPath.isEmpty())
    {
        return;
    }

    const QStringList orderedFiles = getOrderedFilesFromListWidget();
    const cv::Mat firstFrame = cv::imread(orderedFiles.first().toStdString(), cv::IMREAD_COLOR);
    if (firstFrame.empty())
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось прочитать первое изображение.");
        return;
    }

    cv::VideoWriter writer(
        outputPath.toStdString(),
        cv::VideoWriter::fourcc('m', 'p', '4', 'v'),
        static_cast<double>(fpsSpinBox->value()),
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

        writer.write(frame);
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
