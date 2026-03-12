
#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QStringList>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>


class ImageRenamerWidget : public QWidget
{
Q_OBJECT

public:
    explicit ImageRenamerWidget(QWidget *parent = nullptr);

protected:
//    void dragEnterEvent(QDragEnterEvent *event) override;
//    void dropEvent(QDropEvent *event) override;

private slots:

    void onOpenFolder();
    void chooseSourceFolder();
    void chooseOutputFolder();
    void chooseVideoPath();

    void renameFiles();

    void createMp4Video();
    void deleteSelectedImages();

    void refreshListWidget();
    void openImagePreview(QListWidgetItem *item);


private:
    QStringList getImageExtensions();

    QStringList getImageNameFilters();

    QStringList getOrderedFilesFromListWidget() const;

    QListWidget *listWidget;
    QPushButton *btnOpen;
    QPushButton *btnRename;
    QPushButton *btnDeleteSelected;
    QPushButton *btnCreateVideo;
    QPushButton *btnChooseSourceFolder;
    QPushButton *btnChooseOutputFolder;
    QPushButton *btnChooseVideoPath;
    QSpinBox *fpsSpinBox;
    QDoubleSpinBox *secondsPerImageSpinBox;
    QLineEdit *sourceFolderEdit;
    QLineEdit *outputFolderEdit;
    QLineEdit *videoPathEdit;

    QStringList currentFiles;

};
