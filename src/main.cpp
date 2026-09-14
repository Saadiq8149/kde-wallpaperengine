#include "wpe.hpp"
#include <QCoreApplication>

using namespace std;

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);
  QCoreApplication::setOrganizationName("saadiq");
  QCoreApplication::setApplicationName("kde-wallpaperengine");

  if (const auto configOptional = getWallpaperEngineConfig()) {
    const auto &config = *configOptional;

    WallpaperEngine wallpaperEngine(config);
    wallpaperEngine.launch();

    const auto wallpapers = wallpaperEngine.getWallpapers();

    // wallpaperEngine.openWallpaper(wallpapers[1].id, 2880, 1800);
    return app.exec();
  }
}
