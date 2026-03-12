#include "ImageRenamerWidget.h"
#include "PptxToJpgConverter.h"
#include "Mp4VideoCreator.h"
#include "DraggableListWidget.h"

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
    setWindowTitle("PptxToMp4 — подготовка слайдов и сборка видео");
    resize(980, 700);
    setMinimumSize(900, 620);

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
    listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

    btnOpen = new QPushButton("Конвертировать в изображения", this);
    btnRename = new QPushButton("Переименовать по порядку", this);
    btnDeleteSelected = new QPushButton("Удалить выбранные", this);
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

    sourceFolderEdit->setToolTip("Папка, в которой лежат PPTX или PDF файлы.");
    outputFolderEdit->setToolTip("Папка, куда будут сохранены изображения слайдов.");
    videoPathEdit->setToolTip("Полный путь к финальному MP4-файлу.");

    btnOpen->setToolTip("Шаг 1. Конвертировать презентацию в набор изображений.");
    btnRename->setToolTip("Шаг 2. Нормализовать порядок имён по текущему порядку в списке.");
    btnDeleteSelected->setToolTip("Удалить выделенные изображения из набора.");
    btnCreateVideo->setToolTip("Шаг 3. Собрать MP4 по текущему порядку изображений.");

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

    auto *convertBox = new QGroupBox("Шаг 1. Конвертация", this);
    auto *convertLayout = new QVBoxLayout(convertBox);
    convertLayout->addLayout(sourceRow);
    convertLayout->addLayout(outputRow);
    convertLayout->addWidget(btnOpen);

    hintLabel = new QLabel("Подсказка: перетаскивайте карточки мышью для изменения порядка. Двойной клик — предпросмотр.", this);
    hintLabel->setWordWrap(true);
    hintLabel->setStyleSheet("color: #555;");

    listSummaryLabel = new QLabel("Изображений: 0", this);

    auto *listActionsRow = new QHBoxLayout();
    listActionsRow->addWidget(btnRename);
    listActionsRow->addWidget(btnDeleteSelected);
    listActionsRow->addStretch();
    listActionsRow->addWidget(listSummaryLabel);

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

    auto *videoBox = new QGroupBox("Шаг 2. Сборка видео", this);
    auto *videoLayout = new QVBoxLayout(videoBox);
    videoLayout->addLayout(videoRow);
    videoLayout->addLayout(fpsRow);
    videoLayout->addWidget(btnCreateVideo);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(10);
    layout->addWidget(convertBox);
    layout->addWidget(hintLabel);
    layout->addWidget(listWidget, 1);
    layout->addLayout(listActionsRow);
    layout->addWidget(videoBox);

    connect(btnChooseSourceFolder, &QPushButton::clicked, this, &ImageRenamerWidget::chooseSourceFolder);
    connect(btnChooseOutputFolder, &QPushButton::clicked, this, &ImageRenamerWidget::chooseOutputFolder);
    connect(btnChooseVideoPath, &QPushButton::clicked, this, &ImageRenamerWidget::chooseVideoPath);
    connect(btnOpen, &QPushButton::clicked, this, &ImageRenamerWidget::onOpenFolder);
    connect(btnRename, &QPushButton::clicked, this, &ImageRenamerWidget::renameFiles);
    connect(btnDeleteSelected, &QPushButton::clicked, this, &ImageRenamerWidget::deleteSelectedImages);
    connect(btnCreateVideo, &QPushButton::clicked, this, &ImageRenamerWidget::createMp4Video);
    connect(listWidget, &QListWidget::itemDoubleClicked, this, &ImageRenamerWidget::openImagePreview);

    loadSettings();
}

void ImageRenamerWidget::closeEvent(QCloseEvent *event)
{
    saveSettings();
    QWidget::closeEvent(event);
}

void ImageRenamerWidget::loadSettings()
{
    const QString picturesPath = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).value(0);
    const QString defaultSource = picturesPath;
    const QString defaultOutput = picturesPath + "/converted_images";
    const QString defaultVideo = picturesPath + "/result.mp4";

    QSettings settings("PptxToMp4", "PptxToMp4");
    sourceFolderEdit->setText(settings.value("paths/sourceFolder", defaultSource).toString());
    outputFolderEdit->setText(settings.value("paths/outputFolder", defaultOutput).toString());
    videoPathEdit->setText(settings.value("paths/videoPath", defaultVideo).toString());
    fpsSpinBox->setValue(settings.value("video/fps", 30).toInt());
    secondsPerImageSpinBox->setValue(settings.value("video/secondsPerImage", 1.0).toDouble());
}

void ImageRenamerWidget::saveSettings() const
{
    QSettings settings("PptxToMp4", "PptxToMp4");
    settings.setValue("paths/sourceFolder", sourceFolderEdit->text().trimmed());
    settings.setValue("paths/outputFolder", outputFolderEdit->text().trimmed());
    settings.setValue("paths/videoPath", videoPathEdit->text().trimmed());
    settings.setValue("video/fps", fpsSpinBox->value());
    settings.setValue("video/secondsPerImage", secondsPerImageSpinBox->value());
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

void ImageRenamerWidget::deleteSelectedImages()
{
    const QList<QListWidgetItem *> selectedItems = listWidget->selectedItems();
    if (selectedItems.isEmpty())
    {
        QMessageBox::warning(this, "Ошибка", "Выберите изображения для удаления.");
        return;
    }

    const auto btn = QMessageBox::question(
        this,
        "Подтверждение",
        "Удалить выбранные изображения?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (btn != QMessageBox::Yes)
    {
        return;
    }

    int deletedCount = 0;

    for (QListWidgetItem *item : selectedItems)
    {
        const QString itemFileName = item->text();
        for (int i = currentFiles.size() - 1; i >= 0; --i)
        {
            if (QFileInfo(currentFiles[i]).fileName() != itemFileName)
            {
                continue;
            }

            if (QFile::remove(currentFiles[i]))
            {
                currentFiles.removeAt(i);
                ++deletedCount;
            }
            break;
        }
    }

    refreshListWidget();

    if (deletedCount == 0)
    {
        QMessageBox::warning(this, "Ошибка", "Не удалось удалить выбранные изображения.");
        return;
    }

    QMessageBox::information(this, "Готово", "Удалено изображений: " + QString::number(deletedCount));
}


void ImageRenamerWidget::openImagePreview(QListWidgetItem *item)
{
    if (item == nullptr)
    {
        return;
    }

    const QString fileName = item->text();
    QString filePath;

    for (const QString &path : currentFiles)
    {
        if (QFileInfo(path).fileName() == fileName)
        {
            filePath = path;
            break;
        }
    }

    if (filePath.isEmpty())
    {
        QMessageBox::warning(this, "Ошибка", "Не удалось найти выбранное изображение.");
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(fileName);
    dialog.resize(1000, 700);

    auto *layout = new QVBoxLayout(&dialog);
    auto *imageLabel = new QLabel(&dialog);
    imageLabel->setAlignment(Qt::AlignCenter);

    QPixmap pixmap(filePath);
    if (pixmap.isNull())
    {
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть изображение: " + filePath);
        return;
    }

    imageLabel->setPixmap(pixmap.scaled(dialog.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    layout->addWidget(imageLabel);

    dialog.exec();
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

    QString error;
    const Mp4VideoCreator creator;
    if (!creator.createVideo(orderedFiles,
                             outputPath,
                             fpsSpinBox->value(),
                             secondsPerImageSpinBox->value(),
                             &error))
    {
        QMessageBox::critical(this, "Ошибка", error);
        return;
    }

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

    listSummaryLabel->setText("Изображений: " + QString::number(currentFiles.size()));
}
