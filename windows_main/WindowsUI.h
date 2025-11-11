#pragma once

#include <windows.h>
#include <cstdint>
#include <functional>
#include <string>

// Forward declarations
struct SDL_Window;

/**
 * WindowsUI - Handles Windows-specific UI elements (menus, file dialogs, message boxes)
 * This class wraps Win32 API functionality separate from SDL graphics/input handling
 */
class WindowsUI {
public:
  WindowsUI();
  ~WindowsUI();

  // Initialize Windows UI elements for the given SDL window
  // This attaches menus and sets up window message handling
  void initialize(SDL_Window* sdl_window, uint32_t max_scale_factor);

  // Cleanup Windows UI elements
  void cleanup();

  // Allow Win32 thunk to access private members
  friend LRESULT CALLBACK WindowsUI_WndProcThunk(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

  // Windows message handler - returns true if message was handled
  // Returns the LRESULT to use if message was handled
  bool handle_message(void* hwnd, unsigned int msg, unsigned long long wparam, long long lparam,
                      long long& result);

  // Show error message box
  void show_error(SDL_Window* sdl_window, const std::string& title, const std::string& message);

  // Show file open dialog
  bool show_open_file_dialog(SDL_Window* sdl_window, const char* filter, const char* title,
                             std::string& out_path);

  // Show file save dialog
  bool show_save_file_dialog(SDL_Window* sdl_window, const char* filter, const char* title,
                             std::string& out_path);

  // Callbacks for menu/UI actions
  void set_on_open_rom(std::function<void(const std::string&)> cb) { on_open_rom_ = std::move(cb); }
  void set_on_restart_gameboy(std::function<void()> cb) { on_restart_gameboy_ = std::move(cb); }
  void set_on_save(std::function<void(const std::string&)> cb) { on_save_ = std::move(cb); }
  void set_on_quick_save(std::function<void()> cb) { on_quick_save_ = std::move(cb); }
  void set_on_quick_load(std::function<void()> cb) { on_quick_load_ = std::move(cb); }
  void set_on_select_boot_rom(std::function<void(const std::string&)> cb) {
    on_select_boot_rom_ = std::move(cb);
  }
  void set_on_scale_change(std::function<void(uint32_t)> cb) { on_scale_change_ = std::move(cb); }
  void set_on_exit(std::function<void()> cb) { on_exit_ = std::move(cb); }
  void set_on_prepare_pause(std::function<void()> cb) { on_prepare_pause_ = std::move(cb); }
  void set_on_resume_pause(std::function<void()> cb) { on_resume_pause_ = std::move(cb); }

private:
  HWND hwnd_ = nullptr;
  uint32_t max_scale_factor_ = 10;

  // Callbacks
  std::function<void(const std::string&)> on_open_rom_;
  std::function<void()> on_restart_gameboy_;
  std::function<void(const std::string&)> on_save_;
  std::function<void()> on_quick_save_;
  std::function<void()> on_quick_load_;
  std::function<void(const std::string&)> on_select_boot_rom_;
  std::function<void(uint32_t)> on_scale_change_;
  std::function<void()> on_exit_;
  std::function<void()> on_prepare_pause_;
  std::function<void()> on_resume_pause_;

  HWND get_hwnd_from_sdl(SDL_Window* sdl_window);
};
