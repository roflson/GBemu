#pragma once

#include <optional>
#include <string>
#include "SDLWindow.h"
#include "main_loop.h"
#include "rom_loader.h"

template <typename UI>
class GBEmulator {
public:
  // Construct with optional ROM filename; if provided, attempt to load and start loop
  explicit GBEmulator(const char* rom_filename = nullptr);

  // Run loop if constructed
  void run();
  void set_boot_rom(const std::string& path);

private:
  void open_rom(const std::string& path);
  void save(const std::string& path);
  bool load(const std::string& path);
  void quick_save();
  void quick_load();
  void show_error(const std::string& title, const std::string& message);
  OSBridge get_os_bridge();
  std::string get_rom_name_without_extension(const std::string& path);

  SDLWindow window_;
  UI windows_ui_;
  std::string current_rom_path_;
  std::string current_rom_name_;  // ROM filename without path and extension
  std::string boot_rom_path_;     // Path to boot ROM file
  std::optional<ROMLoader> loader_;
  std::optional<MainLoop> loop_;
};


#include "GBEmulator.inc"