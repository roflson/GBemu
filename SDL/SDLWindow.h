#pragma once

#include <SDL.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include "joypad_state.h"

class SDLWindow {
public:
  SDLWindow(const std::string& title, int width, int height);
  ~SDLWindow();

  void clear();
  void present();

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

  // Callbacks for keyboard shortcuts
  void set_on_quick_save(std::function<void()> cb) { on_quick_save_ = std::move(cb); }
  void set_on_quick_load(std::function<void()> cb) { on_quick_load_ = std::move(cb); }
  void set_on_open_rom(std::function<void()> cb) { on_open_rom_ = std::move(cb); }
  void set_on_save(std::function<void()> cb) { on_save_ = std::move(cb); }
  void set_on_exit(std::function<void()> cb) { on_exit_ = std::move(cb); }

  // Audio pause/resume methods (for blocking operations like file dialogs)
  void prepare_for_pause();  // Call before blocking operations
  void resume_from_pause();

  // Scale factor management
  void apply_scale_factor(uint32_t factor);
  uint32_t get_max_scale_factor() const { return max_scale_factor_; }

  // Get the underlying SDL_Window pointer (for platform-specific extensions)
  SDL_Window* get_sdl_window() { return window_; }

private:
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

  // Callbacks for keyboard shortcuts (not menu-triggered)
  std::function<void()> on_quick_save_;
  std::function<void()> on_quick_load_;
  std::function<void()> on_open_rom_;
  std::function<void()> on_save_;
  std::function<void()> on_exit_;

  std::vector<int16_t> last_audio_samples_;
};