#pragma once

#include <cstdint>

// Audio register addresses
constexpr uint16_t AUDIO_REG_START = 0xFF10;
constexpr uint16_t AUDIO_REG_END = 0xFF26;
constexpr uint16_t WAVE_RAM_START = 0xFF30;
constexpr uint16_t WAVE_RAM_END = 0xFF3F;

// Specific register addresses
constexpr uint16_t NR10_ADDR = 0xFF10;
constexpr uint16_t NR11_ADDR = 0xFF11;
constexpr uint16_t NR12_ADDR = 0xFF12;
constexpr uint16_t NR13_ADDR = 0xFF13;
constexpr uint16_t NR14_ADDR = 0xFF14;
constexpr uint16_t NR15_ADDR = 0xFF15;
constexpr uint16_t NR16_ADDR = 0xFF16;
constexpr uint16_t NR17_ADDR = 0xFF17;
constexpr uint16_t NR18_ADDR = 0xFF18;
constexpr uint16_t NR19_ADDR = 0xFF19;
constexpr uint16_t NR1A_ADDR = 0xFF1A;
constexpr uint16_t NR1B_ADDR = 0xFF1B;
constexpr uint16_t NR1C_ADDR = 0xFF1C;
constexpr uint16_t NR1D_ADDR = 0xFF1D;
constexpr uint16_t NR1E_ADDR = 0xFF1E;
constexpr uint16_t NR1F_ADDR = 0xFF1F;
constexpr uint16_t NR20_ADDR = 0xFF20;
constexpr uint16_t NR21_ADDR = 0xFF21;
constexpr uint16_t NR22_ADDR = 0xFF22;
constexpr uint16_t NR23_ADDR = 0xFF23;
constexpr uint16_t NR24_ADDR = 0xFF24;  // NR50 - Master Volume
constexpr uint16_t NR25_ADDR = 0xFF25;  // NR51 - Sound Panning
constexpr uint16_t NR26_ADDR = 0xFF26;  // NR52 - Master Control

// Audio channel panning multiplier (0.25 = 1/4 channels)
constexpr float PANNING_MULTIPLIER = 0.25f;

// Channel enable/disable values
constexpr float CHANNEL_ENABLED = 1.0f;
constexpr float CHANNEL_DISABLED = 0.0f;

// Noise channel clock shift maximum (values above this disable the channel)
constexpr uint8_t MAX_NOISE_CLOCK_SHIFT = 14;

// Wave RAM size
constexpr uint8_t WAVE_RAM_SIZE = 16;

// Sample buffer size (must be power of 2 for efficiency)
constexpr uint32_t SAMPLE_BUFFER_SIZE = 128;

// Bit masks for extracting register values
constexpr uint8_t LENGTH_COUNTER_MASK = 0x3F;  // 6 bits for most length counters
constexpr uint8_t WAVE_LENGTH_MASK = 0xFF;     // 8 bits for wave channel length
constexpr uint8_t MASTER_ENABLE_BIT = 0x80;    // Bit 7
