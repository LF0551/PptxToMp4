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
#include "ImageRenamerWidget.h"
#include "PptxToJpgConverter.h"
#include <QDragEnterEvent>
#include <QCollator>
#include "DraggableListWidget.h"

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

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(btnOpen);
    layout->addWidget(listWidget);
    layout->addWidget(btnRename); // убрали btnUp/btnDown — их нет в объявлении

    connect(btnOpen, &QPushButton::clicked, this, &ImageRenamerWidget::onOpenFolder);
    connect(btnRename, &QPushButton::clicked, this, &ImageRenamerWidget::renameFiles);
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
    const QString folder = QFileDialog::getExistingDirectory(this, "Выберите папку с изображениями", picturesPath);
    if (folder.isEmpty()) return;


//    const QString folder = "C:\\Users\\Apostol\\Downloads\\np";
    const QString output = "C:\\Users\\Apostol\\Downloads\\Pic";

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

    const QFileInfo first(currentFiles.first());
    const QString dirPath = first.path();
    const QString baseName = "image";

    bool success = true;
    QStringList renamedFiles;
    renamedFiles.reserve(currentFiles.size());

    for (int i = 0; i < currentFiles.size(); ++i)
    {
        const QFileInfo oldFile(currentFiles[i]);
        const QString newFileName = QString("%1/%2%3.%4")
                .arg(dirPath)
                        //.arg(baseName)
                .arg(i + 1)
                .arg("." + oldFile.suffix())
                .arg("tmp");

        if (QFile::exists(newFileName))
        {
            // Защита от перезаписи (например, если уже есть image001.jpg)
            success = false;
            QMessageBox::critical(this, "Ошибка", "Файл уже существует: " + newFileName);
            break;
        }

        if (!QFile::rename(currentFiles[i], newFileName))
        {
            success = false;
            break;
        }
        renamedFiles << newFileName;
    }

    if (success)
    {
        currentFiles = renamedFiles;
        QMessageBox::information(this, "Готово", "Файлы успешно переименованы!");
        refreshListWidget();
    }
    else
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось переименовать некоторые файлы.");
    }
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
