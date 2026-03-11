
#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QStringList>


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

    void refreshListWidget();


private:
    QStringList getImageExtensions();

    QStringList getImageNameFilters();

    QListWidget *listWidget;
    QPushButton *btnOpen;
    QPushButton *btnRename;

    QStringList currentFiles;

};
