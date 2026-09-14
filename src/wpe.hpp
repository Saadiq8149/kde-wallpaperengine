#pragma once

#include "qprocess.h"
#include <QProcess>
#include <QSettings>
#include <QThread>

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;
using std::optional, fs::path, std::string;

struct WallpaperEngineConfig {
  path workshop;
};

inline optional<WallpaperEngineConfig> getWallpaperEngineConfig() {
  QSettings settings;
  path workshop = settings.value("workshop").toString().toStdString();

  if (!workshop.empty()) {
    return WallpaperEngineConfig{workshop};
  }

  std::error_code ec;
  for (fs::recursive_directory_iterator it(
           "/", fs::directory_options::skip_permission_denied, ec);
       it != fs::recursive_directory_iterator();) {
    const path p = it->path();
    it.increment(ec);
    if (ec)
      ec.clear();

    if (p.filename() != "431960")
      continue;
    if (p.parent_path().filename() != "content")
      continue;
    if (p.parent_path().parent_path().filename() != "workshop")
      continue;
    workshop = p;

    settings.setValue("workshop", QString::fromStdString(workshop.string()));
    return WallpaperEngineConfig{workshop};
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
  explicit WallpaperEngine(WallpaperEngineConfig paths)
      : paths(std::move(paths)) {}
  inline void launch();
  inline void openWallpaper(string wallpaperId, int screenWidth,
                            int screenHeight);
  inline std::vector<Wallpaper> getWallpapers();

private:
  WallpaperEngineConfig paths;
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

inline string getWallpaperEnginePid() {
  for (const auto &entry : std::filesystem::directory_iterator("/proc")) {
    if (!entry.is_directory())
      continue;

    const path process = entry.path();
    const string name = process.filename().string();
    if (name.empty() || !std::isdigit(name[0]))
      continue;

    std::ifstream file(entry.path() / "cmdline");
    string executable;
    std::getline(file, executable, '\0');

    if (executable == "wallpaper64.exe") {
      return name;
    }
  }
  return "";
}

inline void WallpaperEngine::launch() {
  QProcess::startDetached("steam", {"steam://run/431960"});

  string pid;
  for (int w = 0; w < 100; w++) {
    pid = getWallpaperEnginePid();
    if (!pid.empty()) {
      break;
    }
    QThread::msleep(100);
  }
}

// Video wallpapers not working when rendering using this way using Proton +
// WE, so we need to give video file directly as source to make it work

inline void WallpaperEngine::openWallpaper(string wallpaperId, int screenWidth,
                                           int screenHeight) {
  const path wallpaperPath = paths.workshop / wallpaperId / "project.json";

  if (!fs::exists(wallpaperPath))
    return;
}
