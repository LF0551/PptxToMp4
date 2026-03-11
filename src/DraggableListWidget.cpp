#include "DraggableListWidget.h"
#include <QDropEvent>
#include <QMimeData>
#include <QListWidgetItem>

void DraggableListWidget::dropEvent(QDropEvent *event)
{
    if (!event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist"))
    {
        QListWidget::dropEvent(event);
        return;
    }

    int fromRow = currentRow();
    if (fromRow < 0)
    {
        event->ignore();
        return;
    }

    QListWidgetItem *overItem = itemAt(event->position().toPoint());
    int toRow = overItem ? row(overItem) : count();

    if (fromRow != toRow)
    {
        QListWidgetItem *draggedItem = takeItem(fromRow);
        insertItem(toRow, draggedItem);
        setCurrentItem(draggedItem);
    }

    // CopyAction предотвращает повторное удаление источника через startDrag()
    event->setDropAction(Qt::CopyAction);
    event->accept();
}
