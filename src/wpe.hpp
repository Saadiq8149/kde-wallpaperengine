#pragma once

#include <QProcess>
#include <QProcessEnvironment>
#include <QSettings>
#include <QThread>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;
using std::optional, fs::path, std::string;

struct WallpaperEngineContext {
  path workshopPath;
  QProcessEnvironment env;
  QString winePath;
  QString wallpaperEnginePath;
};

inline optional<WallpaperEngineContext> makeWallpaperEngineContext() {
  QSettings settings;
  path workshopPath = settings.value("workshopPath").toString().toStdString();

  if (!workshopPath.empty()) {
    return WallpaperEngineContext{workshopPath};
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
    workshopPath = p;

    settings.setValue("workshopPath",
                      QString::fromStdString(workshopPath.string()));
    return WallpaperEngineContext{workshopPath};
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
  explicit WallpaperEngine(WallpaperEngineContext context)
      : context(std::move(context)) {}
  inline void init();
  inline void openWallpaper(string wallpaperId, int screenWidth,
                            int screenHeight);
  inline std::vector<Wallpaper> getWallpapers();
  WallpaperEngineContext context;

private:
};

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

inline void WallpaperEngine::init() {
  QProcess::startDetached("steam", {"steam://run/431960"});

  string pid;
  for (int w = 0; w < 1000; w++) {
    pid = getWallpaperEnginePid();
    if (!pid.empty()) {
      break;
    }
    QThread::msleep(100);
  }

  QString protonPath;
  QString winePath;

  for (int w = 0; w < 1000; w++) {
    context.env.clear();
    std::ifstream file("/proc/" + pid + "/environ");

    string envVariable;
    while (std::getline(file, envVariable, '\0')) {
      if (envVariable.empty())
        continue;

      const auto pos = envVariable.find('=');
      if (pos == string::npos)
        continue;

      const QString key = QString::fromStdString(envVariable.substr(0, pos));
      const QString value = QString::fromStdString(envVariable.substr(pos + 1));

      context.env.insert(key, value);
    }

    protonPath =
        context.env.value("STEAM_COMPAT_TOOL_PATHS").split(":").first();
    winePath = context.env.value("STEAM_COMPAT_DATA_PATH");

    if (!protonPath.isEmpty() && !winePath.isEmpty()) {
      break;
    }

    QThread::msleep(100);
  }

  context.env.remove("WINESERVERSOCKET");
  context.winePath = protonPath + "/files/bin/wine";

  for (const auto &entry : fs::directory_iterator(path(winePath.toStdString()) /
                                                  "pfx" / "dosdevices")) {
    if (!fs::is_symlink(entry.path()))
      continue;

    if (fs::read_symlink(entry.path()) == "/") {
      const QString drive =
          QString::fromStdString(entry.path().filename().string());

      context.wallpaperEnginePath =
          drive + context.env.value("STEAM_COMPAT_INSTALL_PATH");
      break;
    }
  }
}

inline void WallpaperEngine::openWallpaper(string wallpaperId, int screenWidth,
                                           int screenHeight) {
  const path wallpaperPath =
      context.workshopPath / wallpaperId / "project.json";
  if (!fs::exists(wallpaperPath))
    return;

  const QString wallpaperExecutable =
      context.wallpaperEnginePath + "/wallpaper64.exe";

  QProcess process;
  process.setProcessEnvironment(context.env);
  process.setProgram(context.winePath);
  process.setArguments({
      wallpaperExecutable,
      "-control",
      "openWallpaper",
      "-file",
      QString::fromStdString(wallpaperPath.string()),
      "-playInWindow",
      "kde-wallpaperengine",
      "-width",
      QString::number(screenWidth),
      "-height",
      QString::number(screenHeight),
      "-borderless",
      // "-x",
      // QString::number(screenWidth),
      // "-y",
      // QString::number(screenHeight),
  });
  process.startDetached();
}

inline std::vector<Wallpaper> WallpaperEngine::getWallpapers() {
  std::vector<Wallpaper> wallpapers;

  if (!fs::exists(context.workshopPath))
    return wallpapers;

  for (const auto dir : fs::directory_iterator(context.workshopPath)) {
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
