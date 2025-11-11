#pragma once

#include <cstdint>
#include <functional>
#include <string>

// Forward declarations
struct SDL_Window;

/**
 * MacUI - Handles macOS-specific UI elements (menus, file dialogs, message boxes)
 * This class wraps Cocoa functionality separate from SDL graphics/input handling
 */
class MacUI {
public:
  struct Impl;

  MacUI();
  ~MacUI();

  // Initialize macOS UI elements for the given SDL window
  void initialize(SDL_Window* sdl_window, uint32_t max_scale_factor);

  // Cleanup macOS UI elements
  void cleanup();

  // Show error message box
  void show_error(SDL_Window* sdl_window, const std::string& title, const std::string& message);

  // Show file open dialog
  bool show_open_file_dialog(SDL_Window* sdl_window, const char* filter, const char* title,
                             std::string& out_path);

  // Show file save dialog
  bool show_save_file_dialog(SDL_Window* sdl_window, const char* filter, const char* title,
                             std::string& out_path);

  // Callbacks for menu/UI actions
  void set_on_open_rom(std::function<void(const std::string&)> cb);
  void set_on_restart_gameboy(std::function<void()> cb);
  void set_on_save(std::function<void(const std::string&)> cb);
  void set_on_quick_save(std::function<void()> cb);
  void set_on_quick_load(std::function<void()> cb);
  void set_on_select_boot_rom(std::function<void(const std::string&)> cb);
  void set_on_scale_change(std::function<void(uint32_t)> cb);
  void set_on_prepare_pause(std::function<void()> cb);
  void set_on_resume_pause(std::function<void()> cb);

private:
  Impl* impl_;
};

