#pragma once

#include <SDL.h>
#include <windows.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include "joypad_state.h"
#include "rgb.h"

class SDLWindow {
public:
  SDLWindow(const std::string& title, int width, int height);
  ~SDLWindow();

  void clear();
  void present();

  void draw_pixel(int x, int y, RGBValue color);
  void draw_line(int x1, int y1, int x2, int y2, RGBValue color);
  void blit_screen(const uint32_t* pixels, size_t pitch);
  // Handle events and return true if window should close
  bool handleEvents(JoypadState& joypad_state);

  // Audio methods
  void start_audio();
  void stop_audio();

  // Queue audio samples (stereo int16 format)
  void queue_audio(const int16_t* samples, int num_samples);

  // Get current queued audio sample count
  int get_queued_audio_samples() const;

  // UI callbacks
  void set_on_open_rom(std::function<void(const std::string&)> cb) { on_open_rom_ = std::move(cb); }
  void set_on_restart_gameboy(std::function<void()> cb) { on_restart_gameboy_ = std::move(cb); }
  void set_on_save(std::function<void(const std::string&)> cb) { on_save_ = std::move(cb); }
  void set_on_quick_save(std::function<void()> cb) { on_quick_save_ = std::move(cb); }
  void set_on_quick_load(std::function<void()> cb) { on_quick_load_ = std::move(cb); }
  void set_on_select_boot_rom(std::function<void(const std::string&)> cb) {
    on_select_boot_rom_ = std::move(cb);
  }

  // Error dialog
  void show_error(const std::string& title, const std::string& message);

  // Allow Win32 thunk to invoke private handler
  friend LRESULT CALLBACK SDLWindow_WndProcThunk(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

private:
  // Win32 message handler (called from static thunk)
  long long handle_window_message(void* hwnd, unsigned int msg, unsigned long long wparam, long long lparam);
  void apply_scale_factor(uint32_t factor);

  SDL_Window* window_;
  SDL_Renderer* renderer_;
  SDL_Texture* texture_ = nullptr;

  // Audio members
  SDL_AudioDeviceID audio_device_;
  SDL_AudioStream* audio_stream_;

  // Controller members
  SDL_GameController* controller_ = nullptr;

  // Keep keyboard and controller states separate; combine into JoypadState each frame
  JoypadState keyboard_state_;
  JoypadState controller_button_state_;  // buttons + dpad buttons

  // Track left stick axes for controller directional mapping
  int16_t controller_lx_ = 0;
  int16_t controller_ly_ = 0;
  uint32_t scale_factor_ = 4;
  uint32_t max_scale_factor_ = 10;  // Will be calculated based on monitor resolution
  int base_width_ = 0;
  int base_height_ = 0;

  // Callbacks
  std::function<void(const std::string&)> on_open_rom_;
  std::function<void()> on_restart_gameboy_;
  std::function<void(const std::string&)> on_save_;
  std::function<void()> on_quick_save_;
  std::function<void()> on_quick_load_;
  std::function<void(const std::string&)> on_select_boot_rom_;

  void prepare_for_pause();  // Call before blocking operations
  void resume_from_pause();

  std::vector<int16_t> last_audio_samples_;
};