#include "DraggableListWidget.h"
#include <QDropEvent>
#include <QMimeData>
#include <QListWidgetItem>
#include <QDebug>

void DraggableListWidget::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist"))
    {
        // Получаем позицию курсора
        QPoint pos = event->position().toPoint();

        // Находим элемент под курсором
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


        if (overItem)
        {
            int rowOver = row(overItem);
            int oldRow = currentRow();

            // Если перетаскиваем не на самого себя
            if (oldRow != rowOver)
            {
                int t = count();
                // Извлекаем перетаскиваемый элемент
                QListWidgetItem *draggedItem = takeItem(oldRow);

                for (int i = 0; i < t; ++i)
                {
                    if (i == rowOver)
                    {
                        qDebug() << draggedItem;
                        addItem(draggedItem);
                    }
                    else
                    {
                        auto item = takeItem(0);
                        qDebug() << item;
                        addItem(item);
                    }
                }
            }
        }
        else
        {
            qDebug() << count();
            int oldRow = currentRow();
            QListWidgetItem *draggedItem = takeItem(oldRow);
            qDebug() << count();
            addItem(draggedItem);
            qDebug() << count();
            //setCurrentItem(draggedItem);

        }

        event->accept();
        return;
    }
    QListWidget::dropEvent(event);
}