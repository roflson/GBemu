#include "GBEmulator.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include "OSBridge.h"
#include "joypad_state.h"
#include "main_loop.h"
#include "rom_loader.h"
#include "save_state.h"

// ===== GBEmulator Display Constants =====
namespace {
constexpr int GAMEBOY_SCREEN_WIDTH = 160;   // Game Boy screen width in pixels
constexpr int GAMEBOY_SCREEN_HEIGHT = 144;  // Game Boy screen height in pixels
constexpr int IDLE_LOOP_SLEEP_MS = 10;      // Sleep duration when no ROM is loaded
}  // namespace

GBEmulator::GBEmulator(const char* rom_filename)
    : window_("GBEmu", GAMEBOY_SCREEN_WIDTH, GAMEBOY_SCREEN_HEIGHT) {

  // Initialize Windows UI with the SDL window
  windows_ui_.initialize(window_.get_sdl_window(), window_.get_max_scale_factor());

  // Set up Windows UI callbacks
  windows_ui_.set_on_open_rom([this](const std::string& path) { this->open_rom(path); });
  windows_ui_.set_on_restart_gameboy([this]() {
    if (!current_rom_path_.empty()) {
      open_rom(current_rom_path_);
    }
  });
  windows_ui_.set_on_save([this](const std::string& path) { this->save(path); });
  windows_ui_.set_on_quick_save([this]() { this->quick_save(); });
  windows_ui_.set_on_quick_load([this]() { this->quick_load(); });
  windows_ui_.set_on_select_boot_rom([this](const std::string& path) { this->set_boot_rom(path); });
  windows_ui_.set_on_scale_change([this](uint32_t factor) { window_.apply_scale_factor(factor); });
  windows_ui_.set_on_prepare_pause([this]() { window_.prepare_for_pause(); });
  windows_ui_.set_on_resume_pause([this]() { window_.resume_from_pause(); });

  // Set up SDL Window callbacks for keyboard shortcuts
  window_.set_on_quick_save([this]() { this->quick_save(); });
  window_.set_on_quick_load([this]() { this->quick_load(); });

  window_.clear();

  if (rom_filename && rom_filename[0] != '\0') {
    open_rom(std::string(rom_filename));
  }
}

std::string GBEmulator::get_rom_name_without_extension(const std::string& path) {
  size_t last_dot = path.find_last_of('.');
  if (last_dot != std::string::npos) {
    return path.substr(0, last_dot);
  }
  return path;
}

void GBEmulator::open_rom(const std::string& path) {
  // Convert relative path to absolute path
  std::string absolute_path;
  try {
    absolute_path = std::filesystem::absolute(path).string();
    std::cout << "Resolved path: " << absolute_path << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Failed to resolve path: " << e.what() << std::endl;
    absolute_path = path;  // Fall back to original path
  }

  // Check if file extension is .sav (case-insensitive)
  auto ext_pos = absolute_path.find_last_of('.');
  if (ext_pos != std::string::npos) {
    std::string ext = absolute_path.substr(ext_pos);
    for (auto& ch : ext)
      ch = static_cast<char>(tolower(ch));
    if (ext == ".sav") {
      load(absolute_path);
      return;
    }
  }

  current_rom_path_ = absolute_path;
  current_rom_name_ = get_rom_name_without_extension(absolute_path);

  // Create loader with optional boot ROM
  if (!boot_rom_path_.empty()) {
    loader_.emplace(absolute_path.c_str(), boot_rom_path_.c_str());
  } else {
    loader_.emplace(absolute_path.c_str());
  }

  if (!loader_->load()) {
    std::string error = loader_->get_load_error();
    if (error.empty()) {
      error = "Failed to load ROM:\n" + absolute_path;
    }
    show_error("ROM Load Error", error);
    loader_.reset();
    return;
  }
  loader_->header()->pretty_print();
  loader_->check_compatibility();
  OSBridge bridge = get_os_bridge();
  loop_.emplace(*loader_, bridge);
}

void GBEmulator::save(const std::string& path) {
  if (!loop_ || !loader_) {
    std::cerr << "Cannot save: No ROM loaded" << std::endl;
    return;
  }

  try {
    SaveStateSerializer serializer(path, false);

    // Save ROM verification data and ROM name
    const ROMHeader* header = loader_->header();
    std::cout << "Saving serializer version: " << SERIALIZER_VERSION << std::endl;
    serializer << (uint32_t)SERIALIZER_VERSION;
    serializer << *header;
    serializer << current_rom_name_;
    serializer << *loop_;

    std::cout << "Save state written to: " << path << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Failed to save state: " << e.what() << std::endl;
  }
}

bool GBEmulator::load(const std::string& path) {
  try {
    SaveStateSerializer serializer(path, true);
    std::cout << "Save state loaded from: " << path << std::endl;

    uint32_t serializer_version;
    serializer >> serializer_version;
    if (serializer_version != SERIALIZER_VERSION) {
      std::cerr << "Save state version mismatch: " << serializer_version << " != " << SERIALIZER_VERSION
                << std::endl;
      return false;
    }

    ROMHeader header;
    serializer >> header;
    loader_.emplace(header);

    // Load the ROM name from the save state
    serializer >> current_rom_name_;

    OSBridge bridge = get_os_bridge();
    loop_.emplace(*loader_, bridge);

    serializer >> *loop_;

    return true;
  } catch (const std::exception& e) {
    std::cerr << "Failed to load save state: " << e.what() << std::endl;
    return false;
  }
}

void GBEmulator::quick_save() {
  if (current_rom_name_.empty()) {
    std::cerr << "Cannot quick save: No ROM loaded" << std::endl;
    return;
  }

  std::string quicksave_path = current_rom_name_ + "-quicksave.sav";
  save(quicksave_path);
}

void GBEmulator::quick_load() {
  if (current_rom_name_.empty()) {
    show_error("Quick Load Error", "Cannot quick load: No ROM loaded");
    return;
  }

  std::string quicksave_path = current_rom_name_ + "-quicksave.sav";

  // Check if file exists
  std::ifstream file(quicksave_path);
  if (!file.good()) {
    show_error("Quick Load Error", "Quick save file not found:\n" + quicksave_path);
    return;
  }
  file.close();

  load(quicksave_path);
}

void GBEmulator::set_boot_rom(const std::string& path) {
  boot_rom_path_ = path;
  std::cout << "Boot ROM set to: " << path << std::endl;

  // Restart the current ROM if one is loaded
  if (!current_rom_path_.empty()) {
    std::cout << "Restarting with boot ROM..." << std::endl;
    open_rom(current_rom_path_);
  }
}

void GBEmulator::show_error(const std::string& title, const std::string& message) {
  windows_ui_.show_error(window_.get_sdl_window(), title, message);
}

OSBridge GBEmulator::get_os_bridge() {
  OSBridge bridge;
  bridge.on_audio_generated = [this](const int16_t* samples, int num_samples) {
    window_.queue_audio(samples, num_samples);
  };
  bridge.present_frame = [this]() {
    window_.present();
  };
  bridge.blit_screen = [this](const uint32_t* pixels, size_t pitch) {
    window_.blit_screen(pixels, pitch);
  };
  bridge.handle_events = [this](JoypadState& joypad_state) {
    return window_.handleEvents(joypad_state);
  };
  return bridge;
}

void GBEmulator::run() {
  JoypadState joypad_state = {false, false, false, false, false, false, false, false};
  while (true) {
    if (loop_) {
      if (loop_->run(joypad_state)) {
        if (window_.handleEvents(joypad_state)) {
          return;
        }
      }
    } else {
      // Idle loop: process events until user opens a ROM
      if (window_.handleEvents(joypad_state)) {
        return;  // Quit requested
      }
      window_.clear();
      window_.present();
      std::this_thread::sleep_for(std::chrono::milliseconds(IDLE_LOOP_SLEEP_MS));
    }
  }
}
