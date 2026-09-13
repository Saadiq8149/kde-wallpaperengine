#include <QCoreApplication>
#include <QSettings>

#include <filesystem>
#include <iostream>
#include <system_error>

namespace fs = std::filesystem;

struct WallpaperEnginePaths {
  fs::path wallpaperEnginePath;
  fs::path workshopPath;
};

WallpaperEnginePaths getWallpaperEnginePaths() {
  QSettings settings;

  fs::path wallpaperEnginePath =
      settings.value("wallpaperEnginePath").toString().toStdString();
  fs::path workshopPath =
      settings.value("workshopPath").toString().toStdString();

  if (fs::exists(wallpaperEnginePath / "wallpaper64.exe"))
    return {wallpaperEnginePath, workshopPath};

  std::error_code ec;

  for (fs::recursive_directory_iterator it(
           "/", fs::directory_options::skip_permission_denied, ec);
       it != fs::recursive_directory_iterator(); it.increment(ec)) {
    const fs::path p = it->path();

    if (p.filename() != "wallpaper_engine")
      continue;
    if (p.parent_path().filename() != "common")
      continue;
    if (p.parent_path().parent_path().filename() != "steamapps")
      continue;

    wallpaperEnginePath = p;

    const fs::path steamAppsPath = p.parent_path().parent_path();
    workshopPath = steamAppsPath / "workshop" / "content" / "431960";

    settings.setValue("wallpaperEnginePath",
                      QString::fromStdString(wallpaperEnginePath.string()));
    settings.setValue("workshopPath",
                      QString::fromStdString(workshopPath.string()));

    return {wallpaperEnginePath, workshopPath};
  }

  return {};
}

class WallpaperEngine {
public:
  explicit WallpaperEngine(WallpaperEnginePaths paths);

  void launch(

  );
  void openWallpaper();

private:
  WallpaperEnginePaths paths;
};

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);

  QCoreApplication::setOrganizationName("saadiq");
  QCoreApplication::setApplicationName("kde-wallpaperengine");

  const auto paths = getWallpaperEnginePaths();

  std::cout << "Wallpaper Engine: " << paths.wallpaperEnginePath << '\n';
  std::cout << "Workshop: " << paths.workshopPath << '\n';
}
