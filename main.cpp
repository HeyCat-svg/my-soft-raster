#include "widget.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    SoftRaster w;
    w.show();

    w.GenerateImage();

    return a.exec();
}
