# GBEmu - Game Boy Emulator

A high-performance Game Boy (DMG-only) emulator written in C++20, featuring a modular architecture with independent PPU and APU libraries.

## Features

 - **Save states**
 - **Controller support**
   - Bluetooth, wired
 - **RAM saving**
 - **Boot ROM support**
 - **Modular: APU and PPU can be plugged into any emulator with no other dependencies**
 - **Accurate, passes every blargg and almost every mooneye test for DMG**
 - Windows only for now, but code structured to easily swap OS out.
 - **Fully automated Blargg & Mooneye testing (See below)**
 - **Fast**

## Architecture

### Independent Libraries

This emulator is built with a modular design, featuring two completely independent libraries that can be dropped into any other project:

#### **PPU (Picture Processing Unit) Library**
The PPU library (`ppu/`) is a self-contained module with zero external dependencies. It handles all video processing, rendering, and display logic.


#### **APU (Audio Processing Unit) Library**
The APU library (`apu/`) is similarly self-contained and handles all audio generation:

Both libraries are compiled as separate static libraries (`PPULib` and `APULib`) in the CMake build system, making them easy to extract and reuse.

## Automated Test Suite

The emulator uses two test suites to validate correctness:

### Blargg Test Suite

| Test | Status | Notes |
|------|--------|-------|
| **CGB Sound Tests** |
| `cgb_sound/rom_singles/01-registers.gb` | ✅ | |
| `cgb_sound/rom_singles/02-len ctr.gb` | ✅ | |
| `cgb_sound/rom_singles/03-trigger.gb` | ✅ | |
| `cgb_sound/rom_singles/04-sweep.gb` | ✅ | |
| `cgb_sound/rom_singles/05-sweep details.gb` | ✅ | |
| `cgb_sound/rom_singles/06-overflow on trigger.gb` | ✅ | |
| `cgb_sound/rom_singles/07-len sweep period sync.gb` | ✅ | |
| `cgb_sound/rom_singles/08-len ctr during power.gb` | ❌ | CGB Only |
| `cgb_sound/rom_singles/09-wave read while on.gb` | ❌ | CGB Only |
| `cgb_sound/rom_singles/10-wave trigger while on.gb` | ❌ | CGB Only |
| `cgb_sound/rom_singles/11-regs after power.gb` | ❌ | CGB Only |
| `cgb_sound/rom_singles/12-wave.gb` | ❌ | CGB Only |
| **CPU Instruction Tests** |
| `cpu_instrs/individual/01-special.gb` | ✅ | |
| `cpu_instrs/individual/02-interrupts.gb` | ✅ | |
| `cpu_instrs/individual/03-op sp,hl.gb` | ✅ | |
| `cpu_instrs/individual/04-op r,imm.gb` | ✅ | |
| `cpu_instrs/individual/05-op rp.gb` | ✅ | |
| `cpu_instrs/individual/06-ld r,r.gb` | ✅ | |
| `cpu_instrs/individual/07-jr,jp,call,ret,rst.gb` | ✅ | |
| `cpu_instrs/individual/08-misc instrs.gb` | ✅ | |
| `cpu_instrs/individual/09-op r,r.gb` | ✅ | |
| `cpu_instrs/individual/10-bit ops.gb` | ✅ | |
| `cpu_instrs/individual/11-op a,(hl).gb` | ✅ | |
| **DMG Sound Tests** |
| `dmg_sound/rom_singles/01-registers.gb` | ✅ | |
| `dmg_sound/rom_singles/02-len ctr.gb` | ✅ | |
| `dmg_sound/rom_singles/03-trigger.gb` | ✅ | |
| `dmg_sound/rom_singles/04-sweep.gb` | ✅ | |
| `dmg_sound/rom_singles/05-sweep details.gb` | ✅ | |
| `dmg_sound/rom_singles/06-overflow on trigger.gb` | ✅ | |
| `dmg_sound/rom_singles/07-len sweep period sync.gb` | ✅ | |
| `dmg_sound/rom_singles/08-len ctr during power.gb` | ✅ | |
| `dmg_sound/rom_singles/09-wave read while on.gb` | ✅ | |
| `dmg_sound/rom_singles/10-wave trigger while on.gb` | ✅ | |
| `dmg_sound/rom_singles/11-regs after power.gb` | ✅ | |
| `dmg_sound/rom_singles/12-wave write while on.gb` | ✅ | |
| **Other Tests** |
| `halt_bug.gb` | ✅ | |
| `instr_timing/instr_timing.gb` | ✅ | |
| `interrupt_time/interrupt_time.gb` | ✅❌ | The DMG portion of this test passes, the rest requires CGB |
| `mem_timing/individual/01-read_timing.gb` | ✅ | |
| `mem_timing/individual/02-write_timing.gb` | ✅ | |
| `mem_timing/individual/03-modify_timing.gb` | ✅ | |
| `mem_timing-2/rom_singles/01-read_timing.gb` | ✅ | |
| `mem_timing-2/rom_singles/02-write_timing.gb` | ✅ | |
| `mem_timing-2/rom_singles/03-modify_timing.gb` | ✅ | |
| `oam_bug/rom_singles/1-lcd_sync.gb` | ✅ | |
| `oam_bug/rom_singles/3-non_causes.gb` | ✅ | |
| `oam_bug/rom_singles/6-timing_no_bug.gb` | ✅ | |
| **These tests require writing a lot of code to corrupt the OAM which pollutes the code for no real purpose** |
| `oam_bug/rom_singles/2-causes.gb` | ❌ | Not implemented |
| `oam_bug/rom_singles/4-scanline_timing.gb` | ❌ | Not implemented |
| `oam_bug/rom_singles/5-timing_bug.gb` | ❌ | Not implemented |
| `oam_bug/rom_singles/7-timing_effect.gb` | ❌ | Not implemented |
| `oam_bug/rom_singles/8-instr_effect.gb` | ❌ | Not implemented |


### Mooneye Test Suite

| Test | Status | Notes |
|------|--------|-------|
| **Acceptance Tests** |
| `acceptance/add_sp_e_timing.gb` | ✅ | |
| `acceptance/bits/mem_oam.gb` | ✅ | |
| `acceptance/bits/reg_f.gb` | ✅ | |
| `acceptance/bits/unused_hwio-GS.gb` | ✅ | |
| `acceptance/boot_regs-dmgABC.gb` | ✅ | |
| `acceptance/di_timing-GS.gb` | ✅ | |
| `acceptance/div_timing.gb` | ✅ | |
| `acceptance/ei_sequence.gb` | ✅ | |
| `acceptance/ei_timing.gb` | ✅ | |
| `acceptance/halt_ime0_ei.gb` | ✅ | |
| `acceptance/halt_ime0_nointr_timing.gb` | ✅ | |
| `acceptance/halt_ime1_timing.gb` | ✅ | |
| `acceptance/halt_ime1_timing2-GS.gb` | ✅ | |
| `acceptance/if_ie_registers.gb` | ✅ | |
| `acceptance/instr/daa.gb` | ✅ | |
| `acceptance/intr_timing.gb` | ✅ | |
| `acceptance/jp_cc_timing.gb` | ✅ | |
| `acceptance/jp_timing.gb` | ✅ | |
| `acceptance/ld_hl_sp_e_timing.gb` | ✅ | |
| `acceptance/oam_dma/basic.gb` | ✅ | |
| `acceptance/oam_dma/reg_read.gb` | ✅ | |
| `acceptance/oam_dma/sources-GS.gb` | ✅ | |
| `acceptance/oam_dma_restart.gb` | ✅ | |
| `acceptance/oam_dma_start.gb` | ✅ | |
| `acceptance/oam_dma_timing.gb` | ✅ | |
| `acceptance/pop_timing.gb` | ✅ | |
| `acceptance/push_timing.gb` | ✅ | |
| `acceptance/ppu/intr_1_2_timing-GS.gb` | ✅ | |
| `acceptance/ppu/intr_2_0_timing.gb` | ✅ | |
| `acceptance/ppu/intr_2_mode0_timing.gb` | ✅ | |
| `acceptance/ppu/intr_2_mode3_timing.gb` | ✅ | |
| `acceptance/ppu/intr_2_oam_ok_timing.gb` | ✅ | |
| `acceptance/ppu/stat_irq_blocking.gb` | ✅ | |
| `acceptance/ppu/stat_lyc_onoff.gb` | ✅ | |
| `acceptance/ppu/vblank_stat_intr-GS.gb` | ✅ | |
| `acceptance/ppu/hblank_ly_scx_timing-GS.gb` | ✅ | |
| `acceptance/ppu/intr_2_mode0_timing_sprites.gb` | ✅ | |
| `acceptance/rapid_di_ei.gb` | ✅ | |
| `acceptance/ret_cc_timing.gb` | ✅ | |
| `acceptance/ret_timing.gb` | ✅ | |
| `acceptance/reti_intr_timing.gb` | ✅ | |
| `acceptance/reti_timing.gb` | ✅ | |
| `acceptance/rst_timing.gb` | ✅ | |
| `acceptance/timer/div_write.gb` | ✅ | |
| `acceptance/timer/tim00.gb` | ✅ | |
| `acceptance/timer/tim00_div_trigger.gb` | ✅ | |
| `acceptance/timer/tim01.gb` | ✅ | |
| `acceptance/timer/tim01_div_trigger.gb` | ✅ | |
| `acceptance/timer/tim10.gb` | ✅ | |
| `acceptance/timer/tim10_div_trigger.gb` | ✅ | |
| `acceptance/timer/tim11.gb` | ✅ | |
| `acceptance/timer/tim11_div_trigger.gb` | ✅ | |
| `acceptance/timer/rapid_toggle.gb` | ✅ | |
| `acceptance/timer/tima_reload.gb` | ✅ | |
| `acceptance/timer/tima_write_reloading.gb` | ✅ | |
| `acceptance/timer/tma_write_reloading.gb` | ✅ | |
| `acceptance/boot_div-dmgABCmgb.gb` | ✅ | |
| `acceptance/interrupts/ie_push.gb` | ✅ | |
| `acceptance/boot_hwio-dmgABCmgb.gb` | ❌ | Not implemented |
| `acceptance/ppu/lcdon_timing-GS.gb` | ❌ | Not implemented |
| `acceptance/ppu/lcdon_write_timing-GS.gb` | ❌ | Not implemented |
| **MBC Tests** |
| `emulator-only/mbc1/bits_bank1.gb` | ✅ | |
| `emulator-only/mbc1/bits_bank2.gb` | ✅ | |
| `emulator-only/mbc1/bits_mode.gb` | ✅ | |
| `emulator-only/mbc1/bits_ramg.gb` | ✅ | |
| `emulator-only/mbc1/ram_256kb.gb` | ✅ | |
| `emulator-only/mbc1/ram_64kb.gb` | ✅ | |
| `emulator-only/mbc1/rom_1Mb.gb` | ✅ | |
| `emulator-only/mbc1/rom_16Mb.gb` | ✅ | |
| `emulator-only/mbc1/rom_2Mb.gb` | ✅ | |
| `emulator-only/mbc1/rom_4Mb.gb` | ✅ | |
| `emulator-only/mbc1/rom_512kb.gb` | ✅ | |
| `emulator-only/mbc1/rom_8Mb.gb` | ✅ | |
| `emulator-only/mbc1/multicart_rom_8Mb.gb` | ❌ | Not supporting multicart ROMs |
| `emulator-only/mbc2/bits_ramg.gb` | ✅ | |
| `emulator-only/mbc2/bits_romb.gb` | ✅ | |
| `emulator-only/mbc2/bits_unused.gb` | ✅ | |
| `emulator-only/mbc2/ram.gb` | ✅ | |
| `emulator-only/mbc2/rom_1Mb.gb` | ✅ | |
| `emulator-only/mbc2/rom_2Mb.gb` | ✅ | |
| `emulator-only/mbc2/rom_512kb.gb` | ✅ | |
| `emulator-only/mbc5/rom_16Mb.gb` | ✅ | |
| `emulator-only/mbc5/rom_1Mb.gb` | ✅ | |
| `emulator-only/mbc5/rom_2Mb.gb` | ✅ | |
| `emulator-only/mbc5/rom_32Mb.gb` | ✅ | |
| `emulator-only/mbc5/rom_4Mb.gb` | ✅ | |
| `emulator-only/mbc5/rom_512kb.gb` | ✅ | |
| `emulator-only/mbc5/rom_64Mb.gb` | ✅ | |
| `emulator-only/mbc5/rom_8Mb.gb` | ✅ | |
| **Manual PPU Tests** |
| `manual-only/sprite_priority.gb` | ✅ | |
| `dmg-acid2.gb` | ✅ | |
| **Platform-Specific Tests** |
| `misc/boot_div-A.gb` | ❌ | AGB only |
| `misc/boot_regs-A.gb` | ❌ | AGB only |
| `misc/boot_div-cgb0.gb` | ❌ | CGB0 only |
| `misc/boot_div-cgbABCDE.gb` | ❌ | CGB only |
| `misc/boot_regs-cgb.gb` | ❌ | CGB only |
| `misc/boot_hwio-C.gb` | ❌ | CGB only |
| `misc/ppu/vblank_stat_intr-C.gb` | ❌ | CGB only |
| `misc/bits/unused_hwio-C.gb` | ❌ | CGB only |
| `acceptance/boot_div-S.gb` | ❌ | SGB only |
| `acceptance/boot_div2-S.gb` | ❌ | SGB only |
| `acceptance/boot_div-dmg0.gb` | ❌ | DMG0 only |
| `acceptance/boot_hwio-dmg0.gb` | ❌ | DMG0 only |
| `acceptance/boot_regs-dmg0.gb` | ❌ | DMG0 only |
| `acceptance/boot_hwio-S.gb` | ❌ | SGB only |
| `acceptance/boot_regs-sgb.gb` | ❌ | SGB only |
| `acceptance/boot_regs-sgb2.gb` | ❌ | SGB only |
| `acceptance/boot_regs-mgb.gb` | ❌ | MGB only |
| `madness/mgb_oam_dma_halt_sprites.gb` | ❌ | MGB only |
| `acceptance/serial/boot_sclk_align-dmgABCmgb.gb` | ❌ | Serial port not supported |


## Building

### Windows setup 
Built on windows inside MSYS MINGW64 using GCC (Also used clangd for code completion so should be buildable with clang too). I have not tried to use Visual Studio and msvc as the compiler but it's unlikely to work.
I used cmake and ninja on windows to build

Packages required:
```bash
mingw-w64-x86_64-cmake
mingw-w64-x86_64-SDL2
mingw-w64-x86_64-gcc
mingw-w64-x86_64-ninja
mingw-w64-x86_64-pkgconf
```

### Mac setup
Built on mac using the version of clang that comes with xcode natively installed.
SDL2 should be installed via brew (or any other way you like)
Then cmake can be used with make to build and run tests.


```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Running Tests

```bash
cd build
ctest --output-on-failure
```

## Status Legend

- ✅ **Passing** - Test passes successfully
- ❌ **Failing** - Test is disabled or not yet implemented

## Future plans
- Adding full CGB support
- Adding RetroAchievements.org support
- Linux support

