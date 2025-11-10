#include "bank_registers.h"
#include "utils.h"

// ===== MBC Register Constants =====
namespace MBCConstants {
// MBC1/MBC5 Register Address Ranges
constexpr uint16_t RAM_ENABLE_START = 0x0000;
constexpr uint16_t RAM_ENABLE_END = 0x1FFF;
constexpr uint16_t ROM_BANK_LOW_START = 0x2000;
constexpr uint16_t ROM_BANK_LOW_END = 0x2FFF;
constexpr uint16_t ROM_BANK_HIGH_START = 0x3000;
constexpr uint16_t ROM_BANK_HIGH_END = 0x3FFF;
constexpr uint16_t RAM_BANK_START = 0x4000;
constexpr uint16_t RAM_BANK_END = 0x5FFF;
constexpr uint16_t BANK_MODE_START = 0x6000;
constexpr uint16_t BANK_MODE_END = 0x7FFF;

// MBC Register Values/Masks
constexpr uint8_t RAM_ENABLE_VALUE = 0x0A;
constexpr uint8_t RAM_ENABLE_MASK = 0x0F;
constexpr uint8_t ROM_BANK_LOW_MASK_MBC1 = 0x1F;
constexpr uint8_t ROM_BANK_HIGH_MASK = 0x03;
constexpr uint8_t ROM_BANK_LOW_MASK_MBC5 = 0xFF;
constexpr uint8_t ROM_BANK_HIGH_MASK_MBC5 = 0x01;
constexpr uint8_t RAM_BANK_MASK = 0x0F;
constexpr uint8_t BANK_MODE_MASK = 0x01;

// MBC2 Specific
constexpr uint16_t MBC2_ADDRESS_MASK = 0x4100;
constexpr uint16_t MBC2_RAM_ENABLE = 0x0000;
constexpr uint16_t MBC2_ROM_BANK_SELECT = 0x0100;
constexpr uint8_t MBC2_ROM_BANK_MASK = 0x0F;
constexpr uint8_t MBC2_RAM_ENABLE_MASK = 0x0F;

// Bit shifts
constexpr uint8_t BANK2_SHIFT_MBC1 = 5;
constexpr uint8_t BANK2_SHIFT_MBC5 = 8;
}  // namespace MBCConstants

BankRegisters::BankRegisters(uint32_t rom_bank_count, uint32_t ram_bank_count, ROMType rom_type)
    : rom_bank_mask_(static_cast<uint8_t>(rom_bank_count - 1)),
      ram_bank_mask_(static_cast<uint8_t>(ram_bank_count - 1)),
      rom_type_(rom_type) {}

void BankRegisters::write(uint16_t address, uint8_t value) {
  if (rom_type_ == ROMType::MBC5) {
    if (address >= MBCConstants::RAM_ENABLE_START && address <= MBCConstants::RAM_ENABLE_END) {
      ramEnabled_ = value == MBCConstants::RAM_ENABLE_VALUE;
    } else if (address >= MBCConstants::ROM_BANK_LOW_START && address <= MBCConstants::ROM_BANK_LOW_END) {
      bank1_ = value & MBCConstants::ROM_BANK_LOW_MASK_MBC5;
    } else if (address >= MBCConstants::ROM_BANK_HIGH_START && address <= MBCConstants::ROM_BANK_HIGH_END) {
      bank2_ = value & MBCConstants::ROM_BANK_HIGH_MASK_MBC5;
    } else if (address >= MBCConstants::RAM_BANK_START && address <= MBCConstants::RAM_BANK_END) {
      bankRAM_ = value & MBCConstants::RAM_BANK_MASK;
    } else {
      FATAL("Writing to bank registers with an invalid address");
    }
  } else if (rom_type_ == ROMType::MBC1) {
    if (address >= MBCConstants::ROM_BANK_LOW_START && address <= MBCConstants::ROM_BANK_HIGH_END) {
      bank1_ = value & MBCConstants::ROM_BANK_LOW_MASK_MBC1;
      if (bank1_ == 0)
        bank1_ = 1;
    } else if (address >= MBCConstants::RAM_BANK_START && address <= MBCConstants::RAM_BANK_END) {
      bank2_ = value & MBCConstants::ROM_BANK_HIGH_MASK;
    } else if (address >= MBCConstants::BANK_MODE_START && address <= MBCConstants::BANK_MODE_END) {
      bankMode_ = value & MBCConstants::BANK_MODE_MASK;
    } else if (address >= MBCConstants::RAM_ENABLE_START && address <= MBCConstants::RAM_ENABLE_END) {
      ramEnabled_ = (value & MBCConstants::RAM_ENABLE_MASK) == MBCConstants::RAM_ENABLE_VALUE;
    } else {
      FATAL("Writing to bank registers with an invalid address");
    }
  } else if (rom_type_ == ROMType::MBC2) {
    switch (address & MBCConstants::MBC2_ADDRESS_MASK) {
      case MBCConstants::MBC2_RAM_ENABLE:
        ramEnabled_ = (value & MBCConstants::MBC2_RAM_ENABLE_MASK) == MBCConstants::RAM_ENABLE_VALUE;
        break;
      case MBCConstants::MBC2_ROM_BANK_SELECT:
        bank1_ = value & MBCConstants::MBC2_ROM_BANK_MASK;
        if (bank1_ == 0)
          bank1_ = 1;
        break;
    }
  }
}

uint32_t BankRegisters::get_rom0() {
  if (!bankMode_) {
    return 0;
  } else {
    return (bank2_ << MBCConstants::BANK2_SHIFT_MBC1) & rom_bank_mask_;
  }
}

uint32_t BankRegisters::get_rom1() {
  if (rom_type_ == ROMType::MBC5) {
    return ((bank2_ << MBCConstants::BANK2_SHIFT_MBC5) | bank1_) & rom_bank_mask_;
  } else {
    return ((bank2_ << MBCConstants::BANK2_SHIFT_MBC1) | bank1_) & rom_bank_mask_;
  }
}

uint32_t BankRegisters::get_ram0() {
  if (rom_type_ == ROMType::MBC5) {
    return bankRAM_ & ram_bank_mask_;
  } else {
    if (!bankMode_)
      return 0;
    else
      return bank2_ & ram_bank_mask_;
  }
}

bool BankRegisters::get_ram_enabled() const {
  return ramEnabled_;
}

bool BankRegisters::get_bankMode() const {
  return bankMode_;
}

ROMType BankRegisters::rom_type() const {
  return rom_type_;
}