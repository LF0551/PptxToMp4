
#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QStringList>
#include <QSpinBox>


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

    void renameFiles();

    void createMp4Video();

    void refreshListWidget();


private:
    QStringList getImageExtensions();

    QStringList getImageNameFilters();

    QStringList getOrderedFilesFromListWidget() const;

    QListWidget *listWidget;
    QPushButton *btnOpen;
    QPushButton *btnRename;
    QPushButton *btnCreateVideo;
    QSpinBox *fpsSpinBox;

    QStringList currentFiles;

};
