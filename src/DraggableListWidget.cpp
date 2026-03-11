#include "DraggableListWidget.h"
#include <QDropEvent>
#include <QMimeData>
#include <QListWidgetItem>
#include <QDebug>

void DraggableListWidget::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist"))
    {
        QPoint pos = event->position().toPoint();

        QListWidgetItem *overItem = itemAt(pos);
        if (!overItem)
            overItem = itemAt({pos.x() + 20, pos.y()});

        if (!overItem)
        {
            auto col1 = itemAt({pos.x(), 20});
            auto row1 = itemAt({20, pos.y() + 128});
            if (!col1 && row1)
                overItem = item(row(row1) - 1);
        }

        int oldRow = currentRow();

        if (overItem)
        {
            int rowOver = row(overItem);

            if (oldRow != rowOver)
            {
                QListWidgetItem *draggedItem = takeItem(oldRow);
                insertItem(rowOver, draggedItem);
                setCurrentItem(draggedItem);
            }
        }
        else
        {
            QListWidgetItem *draggedItem = takeItem(oldRow);
            addItem(draggedItem);
            setCurrentItem(draggedItem);
        }

        // Предотвращаем повторное удаление элемента через Qt::startDrag → clearOrRemove():
        // при MoveAction Qt удаляет исходный элемент сам, но мы уже сделали это вручную.
        event->setDropAction(Qt::IgnoreAction);
        event->accept();
        return;
    }
    QListWidget::dropEvent(event);
}