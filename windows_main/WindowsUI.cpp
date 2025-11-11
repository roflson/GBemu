#include "WindowsUI.h"
#include <SDL_syswm.h>
#include <commdlg.h>
#include <iostream>

// Global original window procedure pointer
static WNDPROC g_original_wndproc = nullptr;

// Global thunk function to forward messages to WindowsUI instance
LRESULT CALLBACK WindowsUI_WndProcThunk(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  auto* self = reinterpret_cast<WindowsUI*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
  if (self) {
    long long result = 0;
    if (self->handle_message(reinterpret_cast<void*>(hwnd), msg, static_cast<unsigned long long>(wparam),
                             static_cast<long long>(lparam), result)) {
      return static_cast<LRESULT>(result);
    }
  }
  return CallWindowProc(g_original_wndproc, hwnd, msg, wparam, lparam);
}

WindowsUI::WindowsUI() {}

WindowsUI::~WindowsUI() {
  cleanup();
}

HWND WindowsUI::get_hwnd_from_sdl(SDL_Window* sdl_window) {
  if (!sdl_window) {
    return nullptr;
  }

  SDL_SysWMinfo wmInfo;
  SDL_VERSION(&wmInfo.version);
  if (!SDL_GetWindowWMInfo(sdl_window, &wmInfo)) {
    return nullptr;
  }

  return wmInfo.info.win.window;
}

void WindowsUI::initialize(SDL_Window* sdl_window, uint32_t max_scale_factor) {
  hwnd_ = get_hwnd_from_sdl(sdl_window);
  if (!hwnd_) {
    std::cerr << "Failed to get HWND from SDL window" << std::endl;
    return;
  }

  max_scale_factor_ = max_scale_factor;

  // Create and attach a menu
  HMENU hMenuBar = CreateMenu();
  HMENU hFileMenu = CreateMenu();
  HMENU hVideoMenu = CreateMenu();

  AppendMenu(hFileMenu, MF_STRING, 1, "&Open ROM\tCtrl+O / Ctrl+L");
  AppendMenu(hFileMenu, MF_STRING, 8, "Select &Boot ROM");
  AppendMenu(hFileMenu, MF_STRING, 2, "&Save\tCtrl+S");
  AppendMenu(hFileMenu, MF_SEPARATOR, 0, NULL);
  AppendMenu(hFileMenu, MF_STRING, 5, "&Quick Save\tF5");
  AppendMenu(hFileMenu, MF_STRING, 6, "Quick &Load\tF8");
  AppendMenu(hFileMenu, MF_SEPARATOR, 0, NULL);
  AppendMenu(hFileMenu, MF_STRING, 4, "&Restart Gameboy");
  AppendMenu(hFileMenu, MF_SEPARATOR, 0, NULL);
  AppendMenu(hFileMenu, MF_STRING, 3, "E&xit\tCtrl+X");

  // Dynamically create scale factor menu items based on max_scale_factor_
  for (uint32_t scale = 1; scale <= max_scale_factor_; ++scale) {
    std::string menu_text = "Scale Factor x" + std::to_string(scale);
    AppendMenu(hVideoMenu, MF_STRING, 10 + scale, menu_text.c_str());
  }

  AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, "File");
  AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hVideoMenu, "Video");

  SetMenu(hwnd_, hMenuBar);

  // Subclass the window to handle menu messages
  // Store this pointer in window user data
  SetLastError(0);
  SetWindowLongPtr(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

  // Replace the window procedure with our thunk
  g_original_wndproc = (WNDPROC)SetWindowLongPtr(hwnd_, GWLP_WNDPROC, (LONG_PTR)WindowsUI_WndProcThunk);
}

void WindowsUI::cleanup() {
  hwnd_ = nullptr;
}

bool WindowsUI::handle_message(void* hwnd, unsigned int msg, unsigned long long wparam, long long lparam,
                               long long& result) {
  if (msg == WM_COMMAND) {
    int menu_id = LOWORD(wparam);
    switch (menu_id) {
      case 1: {  // File -> Open ROM
        std::string file_path;
        if (show_open_file_dialog(
                nullptr, "Game Boy ROM / Save State (*.gb;*.sav)\0*.gb;*.sav\0All Files (*.*)\0*.*\0",
                "Open ROM", file_path)) {
          if (on_open_rom_) {
            on_open_rom_(file_path);
          }
        }
      } break;
      case 2: {  // File -> Save
        std::string file_path;
        if (show_save_file_dialog(nullptr, "Save State (*.sav)\0*.sav\0All Files (*.*)\0*.*\0", "Save State",
                                  file_path)) {
          if (on_save_) {
            on_save_(file_path);
          }
        }
      } break;
      case 3:  // File -> Exit
        if (on_exit_) {
          on_exit_();
        }
        PostQuitMessage(0);
        std::exit(0);
        break;
      case 4:  // File -> Restart Gameboy
        if (on_restart_gameboy_) {
          on_restart_gameboy_();
        }
        break;
      case 5:  // File -> Quick Save
        if (on_quick_save_) {
          on_quick_save_();
        }
        break;
      case 6:  // File -> Quick Load
        if (on_quick_load_) {
          on_quick_load_();
        }
        break;
      case 8: {  // File -> Select Boot ROM
        std::string file_path;
        if (show_open_file_dialog(nullptr, "Boot ROM (*.bin)\0*.bin\0All Files (*.*)\0*.*\0",
                                  "Select Boot ROM", file_path)) {
          if (on_select_boot_rom_) {
            on_select_boot_rom_(file_path);
          }
        }
      } break;
      default:
        // Handle scale factor menu items (IDs 11 and above)
        if (menu_id >= 11 && menu_id <= static_cast<int>(10 + max_scale_factor_)) {
          uint32_t requested_scale = static_cast<uint32_t>(menu_id - 10);
          if (on_scale_change_) {
            on_scale_change_(requested_scale);
          }
        }
        break;
    }

    result = 0;
    return true;
  }

  if (msg == WM_ENTERSIZEMOVE) {
    if (on_prepare_pause_) {
      on_prepare_pause_();
    }
    result = 0;
    return false;  // Let default processing continue
  }

  if (msg == WM_EXITSIZEMOVE) {
    if (on_resume_pause_) {
      on_resume_pause_();
    }
    result = 0;
    return false;  // Let default processing continue
  }

  return false;  // Message not handled
}

void WindowsUI::show_error(SDL_Window* sdl_window, const std::string& title, const std::string& message) {
  HWND hwnd = get_hwnd_from_sdl(sdl_window);
  MessageBoxA(hwnd, message.c_str(), title.c_str(), MB_OK | MB_ICONERROR);
}

bool WindowsUI::show_open_file_dialog(SDL_Window* sdl_window, const char* filter, const char* title,
                                      std::string& out_path) {
  HWND hwnd = sdl_window ? get_hwnd_from_sdl(sdl_window) : hwnd_;

  CHAR file_path[MAX_PATH] = {0};
  OPENFILENAMEA ofn = {};
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = hwnd;
  ofn.lpstrFile = file_path;
  ofn.nMaxFile = MAX_PATH;
  ofn.lpstrFilter = filter;
  ofn.lpstrTitle = title;
  ofn.nFilterIndex = 1;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER | OFN_NOCHANGEDIR;

  if (GetOpenFileNameA(&ofn)) {
    out_path = std::string(file_path);
    return true;
  }

  return false;
}

bool WindowsUI::show_save_file_dialog(SDL_Window* sdl_window, const char* filter, const char* title,
                                      std::string& out_path) {
  HWND hwnd = sdl_window ? get_hwnd_from_sdl(sdl_window) : hwnd_;

  CHAR file_path[MAX_PATH] = {0};
  OPENFILENAMEA ofn = {};
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = hwnd;
  ofn.lpstrFile = file_path;
  ofn.nMaxFile = MAX_PATH;
  ofn.lpstrFilter = filter;
  ofn.lpstrTitle = title;
  ofn.nFilterIndex = 1;
  ofn.Flags = OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_NOCHANGEDIR;

  if (GetSaveFileNameA(&ofn)) {
    out_path = std::string(file_path);
    return true;
  }

  return false;
}
