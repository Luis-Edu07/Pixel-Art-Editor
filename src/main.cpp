#include "editor/MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("Pixel Art Editor");
    QApplication::setOrganizationName("PixelArtEditor");
#ifdef PIXEL_ART_EDITOR_VERSION
    QApplication::setApplicationVersion(PIXEL_ART_EDITOR_VERSION);
#endif

    MainWindow window;
    window.show();
    return app.exec();
}
