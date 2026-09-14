#include "wpe.hpp"
#include <QCoreApplication>

#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);
  QCoreApplication::setOrganizationName("saadiq");
  QCoreApplication::setApplicationName("kde-wallpaperengine");

  if (const auto contextOptional = makeWallpaperEngineContext()) {
    const auto &context = *contextOptional;

    WallpaperEngine wallpaperEngine(context);
    wallpaperEngine.init();

    const auto wallpapers = wallpaperEngine.getWallpapers();

    wallpaperEngine.openWallpaper(wallpapers[20].id, 2880, 1800);
    return app.exec();
  }
}
