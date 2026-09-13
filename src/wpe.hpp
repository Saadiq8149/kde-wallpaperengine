#pragma once

#include <QProcess>
#include <QSettings>

#include <filesystem>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;
using std::optional, fs::path, std::string;

struct WallpaperEnginePaths {
  path steamDir;
  path wallpaperEngine;
  path workshop;
  path compatData;
  path steamRuntime;
  path proton;
  string windowsRootDrive;
};

inline optional<path> getSteamDirPath() {
  QSettings settings;
  path steamDir = settings.value("steamDir").toString().toStdString();

  if (!steamDir.empty() && fs::exists(steamDir / "steamapps" / "common" /
                                      "wallpaper_engine" / "wallpaper64.exe"))
    return steamDir;

  std::error_code ec;
  for (fs::recursive_directory_iterator it(
           "/", fs::directory_options::skip_permission_denied, ec);
       it != fs::recursive_directory_iterator();) {
    const path p = it->path();
    it.increment(ec);
    if (ec)
      ec.clear();

    if (p.filename() != "wallpaper_engine")
      continue;
    if (p.parent_path().filename() != "common")
      continue;
    if (p.parent_path().parent_path().filename() != "steamapps")
      continue;
    steamDir = p.parent_path().parent_path().parent_path();

    settings.setValue("steamDir", QString::fromStdString(steamDir.string()));
    return steamDir;
  }

  return std::nullopt;
}

inline optional<WallpaperEnginePaths> getWallpaperEnginePaths() {
  if (const auto optionalSteamDir = getSteamDirPath()) {
    const auto &steamDir = *optionalSteamDir;
    const path dosDevices =
        steamDir / "steamapps" / "compatdata" / "431960" / "pfx" / "dosdevices";

    string windowsRootDrive;
    for (const auto &entry : fs::directory_iterator(dosDevices)) {
      std::error_code ec;
      if (!fs::is_symlink(entry.path(), ec))
        continue;

      const path target = fs::read_symlink(entry.path(), ec);
      if (ec)
        continue;

      if (target == "/") {
        windowsRootDrive = entry.path().filename().string();
        break;
      }
    }

    return WallpaperEnginePaths{
        steamDir,
        steamDir / "steamapps" / "common" / "wallpaper_engine",
        steamDir / "steamapps" / "workshop" / "content" / "431960",
        steamDir / "steamapps" / "compatdata" / "431960",
        steamDir / "steamapps" / "common" / "SteamLinuxRuntime_4" /
            "_v2-entry-point",
        steamDir / "steamapps" / "common" / "Proton - Experimental" / "proton",
        windowsRootDrive,
    };
  }

  return std::nullopt;
}

enum WallpaperType {
  Video,
  Image,
  Scene,
  Unknown,
};

struct Wallpaper {
  string id;
  WallpaperType type;
};

class WallpaperEngine {
public:
  explicit WallpaperEngine(WallpaperEnginePaths paths)
      : paths(std::move(paths)) {}
  void openWallpaper(string wallpaperId, int screenWidth, int screenHeight);
  std::vector<Wallpaper> getWallpapers();

private:
  WallpaperEnginePaths paths;
  QProcess process;
};

inline std::vector<Wallpaper> WallpaperEngine::getWallpapers() {
  std::vector<Wallpaper> wallpapers;

  if (!fs::exists(paths.workshop))
    return wallpapers;

  for (const auto dir : fs::directory_iterator(paths.workshop)) {
    if (!fs::is_directory(dir))
      continue;
    const string wallpaperId = dir.path().filename().string();
    const path wallpaperJsonPath = dir.path() / "project.json";
    bool hasPkg = false;

    for (const auto &entry : std::filesystem::directory_iterator(dir.path())) {
      if (entry.is_regular_file() && entry.path().extension() == ".pkg") {
        hasPkg = true;
        break;
      }
    }

    WallpaperType type = WallpaperType::Unknown;
    if (hasPkg) {
      type = WallpaperType::Scene;
    } else {
      bool hasVideo = false;
      for (const auto &f :
           std::filesystem::recursive_directory_iterator(dir.path())) {
        if (f.path().extension() == ".mp4") {
          type = WallpaperType::Video;
          break;
        }
      }
    }
    wallpapers.push_back({wallpaperId, type});
  }

  return wallpapers;
}

// Video wallpapers not working when rendering using this way using Proton + WE,
// so we need to give video file directly as source to make it work

inline void WallpaperEngine::openWallpaper(string wallpaperId, int screenWidth,
                                           int screenHeight) {
  const path wallpaperPath = paths.workshop / wallpaperId / "project.json";

  if (!fs::exists(wallpaperPath))
    return;

  if (process.state() != QProcess::NotRunning) {
    process.kill();
    process.waitForFinished();
  }

  const string windowsWallpaperPath =
      paths.windowsRootDrive + wallpaperPath.string();

  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  env.insert("STEAM_COMPAT_CLIENT_INSTALL_PATH",
             QString::fromStdString(paths.steamDir.string()));
  env.insert("STEAM_COMPAT_DATA_PATH",
             QString::fromStdString(paths.compatData.string()));
  process.setProcessEnvironment(env);

  const QString runtime = QString::fromStdString(paths.steamRuntime.string());
  const QString proton = QString::fromStdString(paths.proton.string());
  const QString wallpaperExecutable = QString::fromStdString(
      paths.windowsRootDrive +
      (paths.wallpaperEngine / "wallpaper64.exe").string());

  QStringList arguments = {"--verb=waitforexitandrun",
                           "--",
                           proton,
                           "waitforexitandrun",
                           wallpaperExecutable,
                           "-control",
                           "openWallpaper",
                           "-file",
                           QString::fromStdString(windowsWallpaperPath),
                           "-playInWindow",
                           "LinuxTest",
                           "-width",
                           QString::fromStdString(std::to_string(screenWidth)),
                           "-height",
                           QString::fromStdString(std::to_string(screenHeight)),
                           "-borderless"};

  process.start(runtime, arguments);
}
