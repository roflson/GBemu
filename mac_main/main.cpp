
#include "GBEmulator.h"
#include "MacUI.h"

int main(int argc, char** argv) {
  const char* filename = argc > 1 ? argv[1] : nullptr;
  GBEmulator<MacUI> emu(filename);

  const char* boot_rom_filename = argc > 2 ? argv[2] : nullptr;
  if (boot_rom_filename) {
    emu.set_boot_rom(boot_rom_filename);
  }

  emu.run();
  return 0;
}

