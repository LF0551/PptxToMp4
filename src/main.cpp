#include "ImageRenamerWidget.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    ImageRenamerWidget widget;
    widget.show();
    return app.exec();
}