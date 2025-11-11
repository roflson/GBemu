cmake_minimum_required(VERSION 3.20)

project("GBEmu")

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL2
find_package(PkgConfig REQUIRED)
set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON)

# ============================================================================
# SDL2 LIBRARY - link statically on mac and windows
# ============================================================================
if(APPLE)
    # Force pkg-config to return static library information
    set(ENV{PKG_CONFIG_STATIC} "1")
    pkg_check_modules(SDL2 REQUIRED sdl2)
    
    # Manually find the static library path
    find_library(SDL2_STATIC_LIB NAMES libSDL2.a PATHS /opt/homebrew/lib /opt/homebrew/opt/sdl2/lib NO_DEFAULT_PATH)
    if(SDL2_STATIC_LIB)
        message(STATUS "Found static SDL2 library: ${SDL2_STATIC_LIB}")
        
        # Use the static library directly instead of -lSDL2
        set(SDL2_LIBRARIES ${SDL2_STATIC_LIB})
        set(SDL2_INCLUDE_DIRS ${SDL2_STATIC_INCLUDE_DIRS})
        
        # Extract only the framework link flags from LDFLAGS, excluding -lSDL2
        string(REGEX REPLACE "-lSDL2[^ ]*" "" SDL2_FRAMEWORKS "${SDL2_STATIC_LDFLAGS_OTHER}")
        set(SDL2_LDFLAGS ${SDL2_FRAMEWORKS})
        
        message(STATUS "SDL2_LIBRARIES: ${SDL2_LIBRARIES}")
        message(STATUS "SDL2_LDFLAGS: ${SDL2_LDFLAGS}")
    else()
        message(FATAL_ERROR "Could not find static SDL2 library")
    endif()
else()
    pkg_check_modules(SDL2 REQUIRED sdl2)
    if(SDL2_STATIC_LIBRARIES)
        set(SDL2_LIBRARIES ${SDL2_STATIC_LIBRARIES})
    endif()
    if(SDL2_STATIC_INCLUDE_DIRS)
        set(SDL2_INCLUDE_DIRS ${SDL2_STATIC_INCLUDE_DIRS})
    endif()
endif()


# Add compiler flags for all C++ compilation
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -Werror -fno-omit-frame-pointer")

# Enable fully static linking only for MinGW builds on Windows
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
endif()

# Optional: Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ============================================================================
# LIBRARY: Shared code for both main and test executables
# ============================================================================

# Find all source files in current directory (excluding main.cpp and test files)
file(GLOB LIB_SOURCES
    "*.cpp"
    "*.c"
)

# Add source files from subdirectories
file(GLOB INSTRUCTION_SOURCES
    "cpu/instructions/*.cpp"
    "memory/*.cpp"
    "cpu/*.cpp"
    "rom/*.cpp"
    "utils/*.cpp"
    "joypad/*.cpp"
)

# Combine all library sources
list(APPEND LIB_SOURCES ${INSTRUCTION_SOURCES})

# Remove main.cpp and test files from library sources
list(FILTER LIB_SOURCES EXCLUDE REGEX ".*main\\.cpp$")
list(FILTER LIB_SOURCES EXCLUDE REGEX ".*test.*")
list(FILTER LIB_SOURCES EXCLUDE REGEX ".*Test.*")

# Create library
if(LIB_SOURCES)
    add_library(${PROJECT_NAME}Lib STATIC ${LIB_SOURCES})

    # Include directories for library
    target_include_directories(${PROJECT_NAME}Lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/cpu
        ${CMAKE_CURRENT_SOURCE_DIR}/memory
        ${CMAKE_CURRENT_SOURCE_DIR}/cpu/instructions
        ${CMAKE_CURRENT_SOURCE_DIR}/rom
        ${CMAKE_CURRENT_SOURCE_DIR}/joypad
        ${CMAKE_CURRENT_SOURCE_DIR}/utils
    )

    # Link libraries used by the emulator core
    target_link_libraries(${PROJECT_NAME}Lib PUBLIC APULib PPULib)

    # Set compiler flags for library
    target_compile_options(${PROJECT_NAME}Lib PRIVATE
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
    )
endif()

# ============================================================================
# APU LIBRARY
# ============================================================================

file(GLOB APU_SOURCES
    "apu/*.cpp"
    "apu/components/*.cpp"
)

add_library(APULib STATIC ${APU_SOURCES})

# Include directories for APU library
target_include_directories(APULib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/apu
    ${CMAKE_CURRENT_SOURCE_DIR}/apu/components
    ${CMAKE_CURRENT_SOURCE_DIR}/data_structures
)

# Set compiler flags for APU library
target_compile_options(APULib PRIVATE
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)

# ============================================================================
# PPU LIBRARY
# ============================================================================

file(GLOB PPU_SOURCES
    "ppu/*.cpp"
)

add_library(PPULib STATIC ${PPU_SOURCES})

# Include directories for PPU library
target_include_directories(PPULib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/ppu
    ${CMAKE_CURRENT_SOURCE_DIR}/data_structures
)

# Set compiler flags for PPU library
target_compile_options(PPULib PRIVATE
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)

# ============================================================================
# SDL WINDOW LIBRARY
# ============================================================================


add_library(SDLWindowLib STATIC SDL/SDLWindow.cpp)

# Include directories for SDLWindow library
target_include_directories(SDLWindowLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/SDL
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/joypad
    ${SDL2_INCLUDE_DIRS}
)

# Link SDL2 to SDLWindow library
target_link_libraries(SDLWindowLib PUBLIC ${SDL2_LIBRARIES})

if(APPLE)
    # On macOS, use static SDL2 libraries and link flags    
    if(SDL2_LDFLAGS)
        target_link_options(SDLWindowLib PUBLIC ${SDL2_LDFLAGS})
    endif()
endif()

# Set compiler flags for SDLWindow library
target_compile_options(SDLWindowLib PRIVATE
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)


# ============================================================================
# MAIN EXECUTABLE (Windows-only)
# ============================================================================

if(WIN32 AND LIB_SOURCES)
    set(WINDOWS_MAIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/windows_main)
    set(WINDOWS_MAIN_SOURCES
        ${WINDOWS_MAIN_DIR}/main.cpp
        ${WINDOWS_MAIN_DIR}/WindowsUI.cpp
    )

    set(RESOURCE_FILE ${WINDOWS_MAIN_DIR}/resource.rc)
    add_executable(${PROJECT_NAME} WIN32 ${WINDOWS_MAIN_SOURCES} ${RESOURCE_FILE})

    target_include_directories(${PROJECT_NAME} PRIVATE ${WINDOWS_MAIN_DIR})

    # Link with library, APULib, PPULib, SDLWindow library, and Windows libraries
    target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}Lib APULib PPULib SDLWindowLib comdlg32)

    # Set compiler flags for main executable
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
    )
endif()

# ============================================================================
# MAIN EXECUTABLE (macOS-only)
# ============================================================================

if(APPLE AND LIB_SOURCES)
    set(MAC_MAIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mac_main)
    set(MAC_MAIN_SOURCES
        ${MAC_MAIN_DIR}/main.cpp
        ${MAC_MAIN_DIR}/MacUI.mm
    )

    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${MAC_MAIN_SOURCES})
    set_source_files_properties(${MAC_MAIN_DIR}/MacUI.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")
    
    # Configure macOS app bundle properties
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.gbemu.GBEmu"
        MACOSX_BUNDLE_BUNDLE_NAME "GBEmu"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
        MACOSX_BUNDLE_ICON_FILE "GBEmu.icns"
    )
    
    # Copy icon file into the app bundle
    set(ICON_FILE "${MAC_MAIN_DIR}/GBEmu.icns")
    if(EXISTS ${ICON_FILE})
        set_source_files_properties(${ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
        target_sources(${PROJECT_NAME} PRIVATE ${ICON_FILE})
    else()
        message(WARNING "Icon file not found: ${ICON_FILE}. App bundle will not have an icon.")
    endif()

    target_include_directories(${PROJECT_NAME} PRIVATE ${MAC_MAIN_DIR})

    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${PROJECT_NAME}Lib
        APULib
        PPULib
        SDLWindowLib
        ${SDL2_LIBRARIES}
        "-framework AppKit"
        "-framework Cocoa"
        "-framework Foundation"
        "-framework UniformTypeIdentifiers"
    )

    # Add SDL2 link flags for static linking (includes required frameworks)
    if(SDL2_LDFLAGS)
        target_link_options(${PROJECT_NAME} PRIVATE ${SDL2_LDFLAGS})
    endif()

    target_include_directories(${PROJECT_NAME} PRIVATE ${MAC_MAIN_DIR} ${SDL2_INCLUDE_DIRS})
    if(SDL2_LIBRARY_DIRS)
        target_link_directories(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARY_DIRS})
    endif()

    # Add -ObjC flag to ensure Objective-C categories are linked properly with static libs
    target_link_options(${PROJECT_NAME} PRIVATE "-ObjC")

    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
    )
endif()

# ============================================================================
# TEST CONFIGURATION
# ============================================================================

enable_testing()

# Build test executable from test/test_blargg.cpp
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test/test_blargg.cpp AND LIB_SOURCES)
    add_executable(test_blargg test/test_blargg.cpp)
    add_executable(test_mooneye test/test_mooneye.cpp)

    # Link with core libraries (and SDL window support on Windows)
    target_link_libraries(test_blargg PRIVATE ${PROJECT_NAME}Lib APULib PPULib)
    target_link_libraries(test_mooneye PRIVATE ${PROJECT_NAME}Lib APULib PPULib)
    if(WIN32)
        target_link_libraries(test_blargg PRIVATE SDLWindowLib)
        target_link_libraries(test_mooneye PRIVATE SDLWindowLib)
    endif()

    # Set compiler flags for test executable
    target_compile_options(test_blargg PRIVATE
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
    )
    target_compile_options(test_mooneye PRIVATE
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
    )

    set(BLARGG_ROM_LIST
        test/blargg_roms/cgb_sound/rom_singles/01-registers.gb
        test/blargg_roms/cgb_sound/rom_singles/02-len\ ctr.gb
        test/blargg_roms/cgb_sound/rom_singles/03-trigger.gb
        test/blargg_roms/cgb_sound/rom_singles/04-sweep.gb
        test/blargg_roms/cgb_sound/rom_singles/05-sweep\ details.gb
        test/blargg_roms/cgb_sound/rom_singles/06-overflow\ on\ trigger.gb
        test/blargg_roms/cgb_sound/rom_singles/07-len\ sweep\ period\ sync.gb
        test/blargg_roms/cpu_instrs/individual/01-special.gb
        test/blargg_roms/cpu_instrs/individual/02-interrupts.gb
        test/blargg_roms/cpu_instrs/individual/03-op\ sp,hl.gb
        test/blargg_roms/cpu_instrs/individual/04-op\ r,imm.gb
        test/blargg_roms/cpu_instrs/individual/05-op\ rp.gb
        test/blargg_roms/cpu_instrs/individual/06-ld\ r,r.gb
        test/blargg_roms/cpu_instrs/individual/07-jr,jp,call,ret,rst.gb
        test/blargg_roms/cpu_instrs/individual/08-misc\ instrs.gb
        test/blargg_roms/cpu_instrs/individual/09-op\ r,r.gb
        test/blargg_roms/cpu_instrs/individual/10-bit\ ops.gb
        test/blargg_roms/cpu_instrs/individual/11-op\ a,\(hl\).gb
        test/blargg_roms/dmg_sound/rom_singles/01-registers.gb
        test/blargg_roms/dmg_sound/rom_singles/02-len\ ctr.gb
        test/blargg_roms/dmg_sound/rom_singles/03-trigger.gb
        test/blargg_roms/dmg_sound/rom_singles/04-sweep.gb
        test/blargg_roms/dmg_sound/rom_singles/05-sweep\ details.gb
        test/blargg_roms/dmg_sound/rom_singles/06-overflow\ on\ trigger.gb
        test/blargg_roms/dmg_sound/rom_singles/07-len\ sweep\ period\ sync.gb
        test/blargg_roms/dmg_sound/rom_singles/08-len\ ctr\ during\ power.gb
        test/blargg_roms/dmg_sound/rom_singles/09-wave\ read\ while\ on.gb
        test/blargg_roms/dmg_sound/rom_singles/10-wave\ trigger\ while\ on.gb
        test/blargg_roms/dmg_sound/rom_singles/11-regs\ after\ power.gb
        test/blargg_roms/dmg_sound/rom_singles/12-wave\ write\ while\ on.gb
        test/blargg_roms/halt_bug.gb
        test/blargg_roms/instr_timing/instr_timing.gb
        test/blargg_roms/mem_timing/individual/01-read_timing.gb
        test/blargg_roms/mem_timing/individual/02-write_timing.gb
        test/blargg_roms/mem_timing/individual/03-modify_timing.gb
        test/blargg_roms/mem_timing-2/rom_singles/01-read_timing.gb
        test/blargg_roms/mem_timing-2/rom_singles/02-write_timing.gb
        test/blargg_roms/mem_timing-2/rom_singles/03-modify_timing.gb
        test/blargg_roms/oam_bug/rom_singles/1-lcd_sync.gb
        test/blargg_roms/oam_bug/rom_singles/3-non_causes.gb
        test/blargg_roms/oam_bug/rom_singles/6-timing_no_bug.gb


        #test/blargg_roms/cgb_sound/rom_singles/08-len\ ctr\ during\ power.gb #Needs CGB
        #test/blargg_roms/cgb_sound/rom_singles/09-wave\ read\ while\ on.gb #Needs CGB
        #test/blargg_roms/cgb_sound/rom_singles/10-wave\ trigger\ while\ on.gb #Needs CGB
        #test/blargg_roms/cgb_sound/rom_singles/11-regs\ after\ power.gb #Needs CGB
        #test/blargg_roms/cgb_sound/rom_singles/12-wave.gb #Needs CGB
        #test/blargg_roms/interrupt_time/interrupt_time.gb #Needs CGB
        #test/blargg_roms/oam_bug/rom_singles/2-causes.gb
        #test/blargg_roms/oam_bug/rom_singles/4-scanline_timing.gb
        #test/blargg_roms/oam_bug/rom_singles/5-timing_bug.gb        
        #test/blargg_roms/oam_bug/rom_singles/7-timing_effect.gb
        #test/blargg_roms/oam_bug/rom_singles/8-instr_effect.gb
    )

    set(MOONEYE_ROM_LIST
        test/mooneye_roms/acceptance/add_sp_e_timing.gb
        test/mooneye_roms/acceptance/bits/mem_oam.gb
        test/mooneye_roms/acceptance/bits/reg_f.gb
        test/mooneye_roms/acceptance/bits/unused_hwio-GS.gb
        test/mooneye_roms/acceptance/boot_regs-dmgABC.gb
        test/mooneye_roms/acceptance/di_timing-GS.gb
        test/mooneye_roms/acceptance/div_timing.gb
        test/mooneye_roms/acceptance/ei_sequence.gb
        test/mooneye_roms/acceptance/ei_timing.gb
        test/mooneye_roms/acceptance/halt_ime0_ei.gb
        test/mooneye_roms/acceptance/halt_ime0_nointr_timing.gb
        test/mooneye_roms/acceptance/halt_ime1_timing.gb
        test/mooneye_roms/acceptance/halt_ime1_timing2-GS.gb
        test/mooneye_roms/acceptance/if_ie_registers.gb
        test/mooneye_roms/acceptance/instr/daa.gb
        test/mooneye_roms/acceptance/intr_timing.gb
        test/mooneye_roms/acceptance/jp_cc_timing.gb
        test/mooneye_roms/acceptance/jp_timing.gb
        test/mooneye_roms/acceptance/ld_hl_sp_e_timing.gb
        test/mooneye_roms/acceptance/oam_dma/basic.gb
        test/mooneye_roms/acceptance/oam_dma/reg_read.gb
        test/mooneye_roms/acceptance/oam_dma/sources-GS.gb
        test/mooneye_roms/acceptance/oam_dma_restart.gb
        test/mooneye_roms/acceptance/oam_dma_start.gb
        test/mooneye_roms/acceptance/oam_dma_timing.gb
        test/mooneye_roms/acceptance/pop_timing.gb
        test/mooneye_roms/acceptance/push_timing.gb
        test/mooneye_roms/acceptance/ppu/intr_1_2_timing-GS.gb
        test/mooneye_roms/acceptance/ppu/intr_2_0_timing.gb
        test/mooneye_roms/acceptance/ppu/intr_2_mode0_timing.gb
        test/mooneye_roms/acceptance/ppu/intr_2_mode3_timing.gb
        test/mooneye_roms/acceptance/ppu/intr_2_oam_ok_timing.gb
        test/mooneye_roms/acceptance/ppu/stat_irq_blocking.gb
        test/mooneye_roms/acceptance/ppu/stat_lyc_onoff.gb
        test/mooneye_roms/acceptance/rapid_di_ei.gb
        test/mooneye_roms/acceptance/ret_cc_timing.gb
        test/mooneye_roms/acceptance/ret_timing.gb
        test/mooneye_roms/acceptance/reti_intr_timing.gb
        test/mooneye_roms/acceptance/reti_timing.gb
        test/mooneye_roms/acceptance/rst_timing.gb
        test/mooneye_roms/acceptance/timer/div_write.gb
        test/mooneye_roms/acceptance/timer/tim00.gb
        test/mooneye_roms/acceptance/timer/tim00_div_trigger.gb
        test/mooneye_roms/acceptance/timer/tim01.gb
        test/mooneye_roms/acceptance/timer/tim01_div_trigger.gb
        test/mooneye_roms/acceptance/timer/tim10.gb
        test/mooneye_roms/acceptance/timer/tim10_div_trigger.gb
        test/mooneye_roms/acceptance/timer/tim11.gb
        test/mooneye_roms/acceptance/timer/tim11_div_trigger.gb
        test/mooneye_roms/emulator-only/mbc1/bits_bank1.gb
        test/mooneye_roms/emulator-only/mbc1/bits_bank2.gb
        test/mooneye_roms/emulator-only/mbc1/bits_mode.gb
        test/mooneye_roms/emulator-only/mbc1/bits_ramg.gb
        test/mooneye_roms/emulator-only/mbc1/ram_256kb.gb
        test/mooneye_roms/emulator-only/mbc1/ram_64kb.gb
        test/mooneye_roms/emulator-only/mbc1/rom_1Mb.gb
        test/mooneye_roms/emulator-only/mbc1/rom_16Mb.gb
        test/mooneye_roms/emulator-only/mbc1/rom_2Mb.gb
        test/mooneye_roms/emulator-only/mbc1/rom_4Mb.gb
        test/mooneye_roms/emulator-only/mbc1/rom_512kb.gb
        test/mooneye_roms/emulator-only/mbc1/rom_8Mb.gb
        test/mooneye_roms/emulator-only/mbc2/bits_ramg.gb
        test/mooneye_roms/emulator-only/mbc2/bits_romb.gb
        test/mooneye_roms/emulator-only/mbc2/bits_unused.gb
        test/mooneye_roms/emulator-only/mbc2/ram.gb
        test/mooneye_roms/emulator-only/mbc2/rom_1Mb.gb
        test/mooneye_roms/emulator-only/mbc2/rom_2Mb.gb
        test/mooneye_roms/emulator-only/mbc2/rom_512kb.gb
        test/mooneye_roms/emulator-only/mbc5/rom_16Mb.gb
        test/mooneye_roms/emulator-only/mbc5/rom_1Mb.gb
        test/mooneye_roms/emulator-only/mbc5/rom_2Mb.gb
        test/mooneye_roms/emulator-only/mbc5/rom_32Mb.gb
        test/mooneye_roms/emulator-only/mbc5/rom_4Mb.gb
        test/mooneye_roms/emulator-only/mbc5/rom_512kb.gb
        test/mooneye_roms/emulator-only/mbc5/rom_64Mb.gb
        test/mooneye_roms/emulator-only/mbc5/rom_8Mb.gb
        test/mooneye_roms/acceptance/boot_div-dmgABCmgb.gb
        test/mooneye_roms/acceptance/interrupts/ie_push.gb
        test/mooneye_roms/acceptance/ppu/vblank_stat_intr-GS.gb
        test/mooneye_roms/acceptance/timer/tma_write_reloading.gb
        test/mooneye_roms/acceptance/timer/tima_write_reloading.gb
        test/mooneye_roms/acceptance/timer/rapid_toggle.gb
        test/mooneye_roms/acceptance/timer/tima_reload.gb
        test/mooneye_roms/acceptance/ppu/intr_2_mode0_timing_sprites.gb
        test/mooneye_roms/acceptance/ppu/hblank_ly_scx_timing-GS.gb
        # test/mooneye_roms/manual-only/sprite_priority.gb - This works, it's just manual.

        # test/mooneye_roms/acceptance/boot_hwio-dmgABCmgb.gb #This one relies on startup timing of STAT being in the correct mode
        # test/mooneye_roms/acceptance/ppu/lcdon_timing-GS.gb #This is DMG only so I'm not sure I care.
        # test/mooneye_roms/acceptance/ppu/lcdon_write_timing-GS.gb


        #test/mooneye_roms/misc/boot_div-A.gb #AGB only
        #test/mooneye_roms/misc/boot_div-cgb0.gb #CGB0 Only
        #test/mooneye_roms/misc/boot_div-cgbABCDE.gb #CGBABCDE Only
        #test/mooneye_roms/misc/boot_regs-A.gb #AGB only
        #test/mooneye_roms/misc/boot_regs-cgb.gb #CGB Only
        #test/mooneye_roms/misc/boot_hwio-C.gb #CGB Only
        #test/mooneye_roms/misc/ppu/vblank_stat_intr-C.gb #CGB Only
        #test/mooneye_roms/misc/bits/unused_hwio-C.gb #CGB Only
        #test/mooneye_roms/acceptance/serial/boot_sclk_align-dmgABCmgb.gb #Serial port is not supported.
        #test/mooneye_roms/emulator-only/mbc1/multicart_rom_8Mb.gb #Not going to support these multi cart roms, it ruins the code
        #test/mooneye_roms/madness/mgb_oam_dma_halt_sprites.gb #Don't care about this.
        #test/mooneye_roms/acceptance/boot_div-dmg0.gb #DMG0 Only
        #test/mooneye_roms/acceptance/boot_div-S.gb #SGB Only
        #test/mooneye_roms/acceptance/boot_div2-S.gb #SGB Only
        #test/mooneye_roms/acceptance/boot_hwio-dmg0.gb #DMG0 Only
        #test/mooneye_roms/acceptance/boot_hwio-S.gb #SGB Only
        #test/mooneye_roms/acceptance/boot_regs-dmg0.gb #DMG0 Only
        #test/mooneye_roms/acceptance/boot_regs-mgb.gb #MGB Only
        #test/mooneye_roms/acceptance/boot_regs-sgb.gb #SGB Only
        #test/mooneye_roms/acceptance/boot_regs-sgb2.gb #SGB Only
    )

    # Tests to exclude from boot ROM testing
    set(MOONEYE_BOOT_ROM_EXCLUDE_LIST
        test/mooneye_roms/acceptance/boot_div-dmgABCmgb.gb
    )

    # Create a test for each explicit ROM file
    foreach(ROM_FILE ${BLARGG_ROM_LIST})
        # Remove test/ prefix and replace slashes and spaces with underscores
        string(REGEX REPLACE "^test/" "" TEST_NAME ${ROM_FILE})
        string(REGEX REPLACE "\\." "_" TEST_NAME ${TEST_NAME})
        string(REGEX REPLACE "/" "_" TEST_NAME ${TEST_NAME})
        string(REGEX REPLACE "\\\\ " "_" TEST_NAME ${TEST_NAME})
        string(REGEX REPLACE " " "_" TEST_NAME ${TEST_NAME})
        add_test(
            NAME blargg_${TEST_NAME}
            COMMAND test_blargg ${CMAKE_CURRENT_SOURCE_DIR}/${ROM_FILE}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
        set_tests_properties(blargg_${TEST_NAME} PROPERTIES TIMEOUT 30)
    endforeach()

    foreach(ROM_FILE ${BLARGG_ROM_LIST})
        # Remove test/ prefix and replace slashes and spaces with underscores
        string(REGEX REPLACE "^test/" "" TEST_NAME ${ROM_FILE})
        string(REGEX REPLACE "\\." "_" TEST_NAME ${TEST_NAME})
        string(REGEX REPLACE "/" "_" TEST_NAME ${TEST_NAME})
        string(REGEX REPLACE "\\\\ " "_" TEST_NAME ${TEST_NAME})
        string(REGEX REPLACE " " "_" TEST_NAME ${TEST_NAME})
        add_test(
            NAME blargg_boot_rom_${TEST_NAME}
            COMMAND test_blargg ${CMAKE_CURRENT_SOURCE_DIR}/${ROM_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/test/dmg_boot.bin
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
        set_tests_properties(blargg_boot_rom_${TEST_NAME} PROPERTIES TIMEOUT 30)
    endforeach()

    foreach(ROM_FILE ${MOONEYE_ROM_LIST})
        # Remove test/ prefix and replace slashes and spaces with underscores
        string(REGEX REPLACE "^test/" "" TEST_NAME ${ROM_FILE})
        string(REGEX REPLACE "\\." "_" TEST_NAME ${TEST_NAME})
        string(REGEX REPLACE "/" "_" TEST_NAME ${TEST_NAME})
        string(REGEX REPLACE "\\\\ " "_" TEST_NAME ${TEST_NAME})
        string(REGEX REPLACE " " "_" TEST_NAME ${TEST_NAME})
        add_test(
            NAME mooneye_${TEST_NAME}
            COMMAND test_mooneye ${CMAKE_CURRENT_SOURCE_DIR}/${ROM_FILE}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
        set_tests_properties(mooneye_${TEST_NAME} PROPERTIES TIMEOUT 30)
    endforeach()



    foreach(ROM_FILE ${MOONEYE_ROM_LIST})
        # Skip excluded tests
        list(FIND MOONEYE_BOOT_ROM_EXCLUDE_LIST ${ROM_FILE} EXCLUDE_INDEX)
        if(EXCLUDE_INDEX EQUAL -1)
            # Remove test/ prefix and replace slashes and spaces with underscores
            string(REGEX REPLACE "^test/" "" TEST_NAME ${ROM_FILE})
            string(REGEX REPLACE "\\." "_" TEST_NAME ${TEST_NAME})
            string(REGEX REPLACE "/" "_" TEST_NAME ${TEST_NAME})
            string(REGEX REPLACE "\\\\ " "_" TEST_NAME ${TEST_NAME})
            string(REGEX REPLACE " " "_" TEST_NAME ${TEST_NAME})
            add_test(
                NAME mooneye_boot_rom_${TEST_NAME}
                COMMAND test_mooneye ${CMAKE_CURRENT_SOURCE_DIR}/${ROM_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/test/dmg_boot.bin
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            )
            set_tests_properties(mooneye_boot_rom_${TEST_NAME} PROPERTIES TIMEOUT 30)
        endif()
    endforeach()

    # Add a custom target for running all tests
    add_custom_target(run_tests
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        DEPENDS test_blargg test_mooneye
        COMMENT "Running all tests..."
    )
endif()
