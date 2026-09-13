#include "wpe.hpp"
#include <QCoreApplication>

using namespace std;

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);
  QCoreApplication::setOrganizationName("saadiq");
  QCoreApplication::setApplicationName("kde-wallpaperengine");

  if (const auto optionalPaths = getWallpaperEnginePaths()) {
    const auto &paths = *optionalPaths;

    WallpaperEngine wallpaperEngine(paths);
    const auto wallpapers = wallpaperEngine.getWallpapers();

    wallpaperEngine.openWallpaper(wallpapers[1].id, 2880, 1800);
    return app.exec();
  }
}
