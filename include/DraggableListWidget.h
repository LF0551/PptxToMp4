#pragma once
#include <QListWidget>

class DraggableListWidget : public QListWidget
{
public:
    using QListWidget::QListWidget;

protected:
    void dropEvent(QDropEvent *event) override;
};