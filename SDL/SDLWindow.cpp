#include "SDLWindow.h"
#include <signal.h>
#include <array>
#include <csignal>
#include "utils.h"

void signal_handler(int signal) {
  std::exit(-1);
}

SDLWindow::SDLWindow(const std::string& title, int width, int height)
    : window_(nullptr),
      renderer_(nullptr),
      texture_(nullptr),
      audio_device_(0),
      audio_stream_(nullptr),
      base_width_(width),
      base_height_(height) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) < 0) {
    FATAL("Can't init SDL");
    return;
  }

  std::signal(SIGINT, signal_handler);

  // Get monitor resolution to calculate maximum scale factor
  SDL_DisplayMode display_mode;
  if (SDL_GetCurrentDisplayMode(0, &display_mode) == 0) {
    // Calculate max scale based on monitor height (leave some space for taskbar and window borders)
    int available_height = display_mode.h - 100;  // Reserve 100px for taskbar/borders
    max_scale_factor_ = available_height / height;
    if (max_scale_factor_ < 1)
      max_scale_factor_ = 1;
    if (max_scale_factor_ > 20)
      max_scale_factor_ = 20;  // Reasonable upper limit

    // Set default scale to be just above middle of range
    scale_factor_ = (max_scale_factor_ + 2) / 2;
    if (scale_factor_ < 1)
      scale_factor_ = 1;
  }

  window_ = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             width * scale_factor_, height * scale_factor_, SDL_WINDOW_SHOWN);

  if (!window_) {
    FATAL("Can't create SDL window");
    SDL_Quit();
    return;
  }

  renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
  if (!renderer_) {
    SDL_DestroyWindow(window_);
    SDL_Quit();
    FATAL("Can't create SDL Renderer");
    return;
  }

  SDL_RenderSetLogicalSize(renderer_, width, height);
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

  texture_ =
      SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
  if (!texture_) {
    SDL_DestroyRenderer(renderer_);
    SDL_DestroyWindow(window_);
    SDL_Quit();
    FATAL("Can't create SDL Texture");
    return;
  }

  // Set up audio specification for queue-based audio
  SDL_AudioSpec desired_spec;
  SDL_zero(desired_spec);
  desired_spec.freq = 48000;
  desired_spec.format = AUDIO_S16SYS;  // 16-bit signed integer audio
  desired_spec.channels = 2;           // Stereo
  desired_spec.samples = 512;          // Buffer size in samples

  audio_device_ = SDL_OpenAudioDevice(nullptr, 0, &desired_spec, nullptr, 0);
  if (audio_device_ == 0) {
    std::string error_msg = std::string("Failed to open audio device: ") + SDL_GetError();
    FATAL(error_msg.c_str());
  }

  audio_stream_ = SDL_NewAudioStream(AUDIO_S16SYS, 2, 48000, desired_spec.format, desired_spec.channels,
                                     desired_spec.freq);
  if (!audio_stream_) {
    std::string error_msg = std::string("Failed to create audio stream: ") + SDL_GetError();
    FATAL(error_msg.c_str());
  }

  // Try to open the first available game controller
  const int num_joysticks = SDL_NumJoysticks();
  for (int i = 0; i < num_joysticks; ++i) {
    if (SDL_IsGameController(i)) {
      controller_ = SDL_GameControllerOpen(i);
      if (controller_) {
        break;
      }
    }
  }
  SDL_GameControllerEventState(SDL_ENABLE);

  start_audio();
}

SDLWindow::~SDLWindow() {
  if (controller_) {
    SDL_GameControllerClose(controller_);
    controller_ = nullptr;
    controller_lx_ = 0;
    controller_ly_ = 0;
  }
  if (audio_stream_) {
    SDL_FreeAudioStream(audio_stream_);
  }
  if (audio_device_ != 0) {
    SDL_CloseAudioDevice(audio_device_);
  }
  if (texture_)
    SDL_DestroyTexture(texture_);
  if (renderer_)
    SDL_DestroyRenderer(renderer_);
  if (window_)
    SDL_DestroyWindow(window_);
  SDL_Quit();
}

void SDLWindow::clear() {
  SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
  SDL_RenderClear(renderer_);
}

void SDLWindow::present() {
  SDL_RenderPresent(renderer_);
}

void SDLWindow::blit_screen(const uint32_t* pixels, size_t pitch) {
  SDL_UpdateTexture(texture_, nullptr, pixels, static_cast<int>(pitch));
  SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
}

bool SDLWindow::handleEvents(JoypadState& joypad_state) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      return true;
    } else if (event.type == SDL_KEYDOWN) {
      // Check for function keys (no modifier needed)
      switch (event.key.keysym.sym) {
        case SDLK_F5:
          // Trigger Quick Save
          if (on_quick_save_) {
            on_quick_save_();
          }
          break;
        case SDLK_F8:
          // Trigger Quick Load
          if (on_quick_load_) {
            on_quick_load_();
          }
          break;
        default:
          // Handle gamepad keys
          switch (event.key.keysym.sym) {
            case SDLK_z:
              keyboard_state_.a_pressed = true;
              break;
            case SDLK_x:
              keyboard_state_.b_pressed = true;
              break;
            case SDLK_a:
              keyboard_state_.select_pressed = true;
              break;
            case SDLK_s:
              keyboard_state_.start_pressed = true;
              break;
            case SDLK_UP:
              keyboard_state_.up_pressed = true;
              break;
            case SDLK_DOWN:
              keyboard_state_.down_pressed = true;
              break;
            case SDLK_LEFT:
              keyboard_state_.left_pressed = true;
              break;
            case SDLK_RIGHT:
              keyboard_state_.right_pressed = true;
              break;
            default:
              break;
          }
          break;
      }
    } else if (event.type == SDL_KEYUP) {
      switch (event.key.keysym.sym) {
        case SDLK_z:
          keyboard_state_.a_pressed = false;
          break;
        case SDLK_x:
          keyboard_state_.b_pressed = false;
          break;
        case SDLK_a:
          keyboard_state_.select_pressed = false;
          break;
        case SDLK_s:
          keyboard_state_.start_pressed = false;
          break;
        case SDLK_UP:
          keyboard_state_.up_pressed = false;
          break;
        case SDLK_DOWN:
          keyboard_state_.down_pressed = false;
          break;
        case SDLK_LEFT:
          keyboard_state_.left_pressed = false;
          break;
        case SDLK_RIGHT:
          keyboard_state_.right_pressed = false;
          break;
      }
    } else if (event.type == SDL_CONTROLLERDEVICEADDED) {
      if (!controller_ && SDL_IsGameController(event.cdevice.which)) {
        controller_ = SDL_GameControllerOpen(event.cdevice.which);
      }
    } else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
      if (controller_ && SDL_GameControllerGetJoystick(controller_) &&
          SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller_)) == event.cdevice.which) {
        SDL_GameControllerClose(controller_);
        controller_ = nullptr;
        // Clear controller states to prevent stuck inputs
        controller_button_state_.a_pressed = false;
        controller_button_state_.b_pressed = false;
        controller_button_state_.select_pressed = false;
        controller_button_state_.start_pressed = false;
        controller_button_state_.up_pressed = false;
        controller_button_state_.down_pressed = false;
        controller_button_state_.left_pressed = false;
        controller_button_state_.right_pressed = false;
        controller_lx_ = 0;
        controller_ly_ = 0;
      }
    } else if (event.type == SDL_CONTROLLERBUTTONDOWN) {
      switch (event.cbutton.button) {
        case SDL_CONTROLLER_BUTTON_A:
          controller_button_state_.a_pressed = true;
          break;
        case SDL_CONTROLLER_BUTTON_B:
          controller_button_state_.b_pressed = true;
          break;
        case SDL_CONTROLLER_BUTTON_BACK:
          controller_button_state_.select_pressed = true;
          break;
        case SDL_CONTROLLER_BUTTON_START:
          controller_button_state_.start_pressed = true;
          break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP:
          controller_button_state_.up_pressed = true;
          break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
          controller_button_state_.down_pressed = true;
          break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
          controller_button_state_.left_pressed = true;
          break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
          controller_button_state_.right_pressed = true;
          break;
        default:
          break;
      }
    } else if (event.type == SDL_CONTROLLERBUTTONUP) {
      switch (event.cbutton.button) {
        case SDL_CONTROLLER_BUTTON_A:
          controller_button_state_.a_pressed = false;
          break;
        case SDL_CONTROLLER_BUTTON_B:
          controller_button_state_.b_pressed = false;
          break;
        case SDL_CONTROLLER_BUTTON_BACK:
          controller_button_state_.select_pressed = false;
          break;
        case SDL_CONTROLLER_BUTTON_START:
          controller_button_state_.start_pressed = false;
          break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP:
          controller_button_state_.up_pressed = false;
          break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
          controller_button_state_.down_pressed = false;
          break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
          controller_button_state_.left_pressed = false;
          break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
          controller_button_state_.right_pressed = false;
          break;
        default:
          break;
      }
    } else if (event.type == SDL_CONTROLLERAXISMOTION) {
      if (event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) {
        controller_lx_ = event.caxis.value;
      } else if (event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
        controller_ly_ = event.caxis.value;
      }
    }
  }
  const int16_t DEADZONE = 8000;
  bool axis_left = controller_lx_ < -DEADZONE;
  bool axis_right = controller_lx_ > DEADZONE;
  bool axis_up = controller_ly_ < -DEADZONE;
  bool axis_down = controller_ly_ > DEADZONE;

  bool ctrl_up = controller_button_state_.up_pressed || axis_up;
  bool ctrl_down = controller_button_state_.down_pressed || axis_down;
  bool ctrl_left = controller_button_state_.left_pressed || axis_left;
  bool ctrl_right = controller_button_state_.right_pressed || axis_right;

  joypad_state.a_pressed = keyboard_state_.a_pressed || controller_button_state_.a_pressed;
  joypad_state.b_pressed = keyboard_state_.b_pressed || controller_button_state_.b_pressed;
  joypad_state.select_pressed = keyboard_state_.select_pressed || controller_button_state_.select_pressed;
  joypad_state.start_pressed = keyboard_state_.start_pressed || controller_button_state_.start_pressed;
  joypad_state.up_pressed = keyboard_state_.up_pressed || ctrl_up;
  joypad_state.down_pressed = keyboard_state_.down_pressed || ctrl_down;
  joypad_state.left_pressed = keyboard_state_.left_pressed || ctrl_left;
  joypad_state.right_pressed = keyboard_state_.right_pressed || ctrl_right;

  return false;
}

void SDLWindow::start_audio() {
  if (audio_device_ != 0) {
    SDL_PauseAudioDevice(audio_device_, 0);  // 0 = unpause (start playback)
  }
}

void SDLWindow::stop_audio() {
  if (audio_device_ != 0) {
    SDL_PauseAudioDevice(audio_device_, 1);  // 1 = pause (stop playback)
  }
}

// Queue audio samples to be played
void SDLWindow::queue_audio(const int16_t* samples, int num_samples) {

  if (!audio_stream_ || audio_device_ == 0) {
    return;
  }

  last_audio_samples_.assign(samples, samples + num_samples);
  Uint32 queued_bytes = SDL_GetQueuedAudioSize(audio_device_);
  int queued_samples = queued_bytes / sizeof(int16_t);

  const int MIN = 960;   // 10ms at 48kHz stereo - increased to prevent underruns
  const int MAX = 4800;  // 50ms at 48kHz stereo

  if (queued_samples > (MAX * 2)) {
  } else if (queued_samples < (MIN / 2)) {
    SDL_AudioStreamPut(audio_stream_, samples, num_samples * sizeof(int16_t));
    const uint32_t extra_samples = std::min(num_samples, 64);
    SDL_AudioStreamPut(audio_stream_, samples + num_samples - extra_samples, extra_samples * sizeof(int16_t));
  } else {
    SDL_AudioStreamPut(audio_stream_, samples, num_samples * sizeof(int16_t));
  }

  int available = SDL_AudioStreamAvailable(audio_stream_);
  if (available > 0 && queued_samples < MAX) {
    std::array<uint8_t, 20480> buffer;
    int received = SDL_AudioStreamGet(audio_stream_, buffer.data(), buffer.size());
    if (received > 0) {
      SDL_QueueAudio(audio_device_, buffer.data(), received);
    }
  }
}

int SDLWindow::get_queued_audio_samples() const {
  if (audio_device_ == 0) {
    return 0;
  }
  Uint32 queued_bytes = SDL_GetQueuedAudioSize(audio_device_);
  return queued_bytes / sizeof(int16_t);
}

void SDLWindow::apply_scale_factor(uint32_t factor) {
  if (factor == 0 || scale_factor_ == factor || !window_) {
    return;
  }

  scale_factor_ = factor;

  int scaled_width = base_width_ * static_cast<int>(scale_factor_);
  int scaled_height = base_height_ * static_cast<int>(scale_factor_);

  SDL_SetWindowSize(window_, scaled_width, scaled_height);
}

void SDLWindow::prepare_for_pause() {
  if (audio_device_ == 0)
    return;

  // We can't read the queue, but we have the last samples we queued
  // Apply a fade-out to those samples and re-queue them

  if (!last_audio_samples_.empty()) {
    // Create a fade-out version of the last audio samples
    std::vector<int16_t> faded_samples = last_audio_samples_;
    int num_samples = faded_samples.size();

    for (int i = 0; i < num_samples; i++) {
      float fade_multiplier = 1.0f - (static_cast<float>(i) / num_samples);
      faded_samples[i] = static_cast<int16_t>(faded_samples[i] * fade_multiplier);
    }

    // Clear the queue and re-queue with fade
    SDL_ClearQueuedAudio(audio_device_);
    SDL_QueueAudio(audio_device_, faded_samples.data(), num_samples * sizeof(int16_t));

    // Wait for the faded audio to play
    int wait_ms = (num_samples / 2 / 48) + 5;  // stereo samples to ms + margin
    SDL_Delay(wait_ms);
  }

  // Now pause cleanly at near-zero amplitude
  SDL_ClearQueuedAudio(audio_device_);
  SDL_PauseAudioDevice(audio_device_, 1);
}

void SDLWindow::resume_from_pause() {
  if (audio_device_ == 0)
    return;

  SDL_PauseAudioDevice(audio_device_, 0);
}