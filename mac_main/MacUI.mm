#import "MacUI.h"
#include <SDL.h>
#include <SDL_syswm.h>
#include <filesystem>
#include <optional>
#include <sstream>
#include <vector>
#import <Cocoa/Cocoa.h>

#if __has_include(<UniformTypeIdentifiers/UniformTypeIdentifiers.h>)
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#define MACUI_HAS_UNIFORM_TYPE_IDENTIFIERS 1
#else
#define MACUI_HAS_UNIFORM_TYPE_IDENTIFIERS 0
#endif

namespace {
constexpr const char* kRomFilter =
    "Game Boy ROM / Save State (*.gb;*.sav)\0*.gb;*.sav\0All Files (*.*)\0*.*\0";
constexpr const char* kBootRomFilter = "Boot ROM (*.bin)\0*.bin\0All Files (*.*)\0*.*\0";
constexpr const char* kSaveFilter = "Save State (*.sav)\0*.sav\0All Files (*.*)\0*.*\0";

std::vector<std::string> parse_extensions_from_filter(const char* filter) {
  std::vector<std::string> extensions;
  if (!filter) {
    return extensions;
  }

  const char* ptr = filter;
  while (*ptr) {
    // Skip label
    while (*ptr) {
      ++ptr;
    }
    ++ptr;  // Skip null terminator
    if (!*ptr) {
      break;
    }

    std::string pattern(ptr);
    ptr += pattern.size() + 1;

    std::stringstream ss(pattern);
    std::string token;
    while (std::getline(ss, token, ';')) {
      size_t start = token.find("*.");
      if (start == std::string::npos) {
        continue;
      }
      start += 2;
      size_t end = token.find_first_of(");", start);
      std::string ext = token.substr(start, end == std::string::npos ? std::string::npos : end - start);
      if (ext == "*" || ext.empty()) {
        continue;
      }
      // Trim whitespace
      size_t first = ext.find_first_not_of(" \t\r\n");
      size_t last = ext.find_last_not_of(" \t\r\n");
      if (first != std::string::npos) {
        ext = ext.substr(first, last - first + 1);
      }
      for (char& ch : ext) {
        ch = static_cast<char>(::tolower(ch));
      }
      extensions.push_back(ext);
    }
  }
  return extensions;
}

NSWindow* get_ns_window_from_sdl(SDL_Window* sdl_window) {
  if (!sdl_window) {
    return nil;
  }

  SDL_SysWMinfo wmInfo;
  SDL_VERSION(&wmInfo.version);
  if (!SDL_GetWindowWMInfo(sdl_window, &wmInfo)) {
    return nil;
  }
  if (wmInfo.subsystem != SDL_SYSWM_COCOA) {
    return nil;
  }
  return wmInfo.info.cocoa.window;
}

std::filesystem::path executable_directory() {
  @autoreleasepool {
    NSString* executablePath = [[NSBundle mainBundle] executablePath];
    if (!executablePath) {
      return std::filesystem::current_path();
    }
    return std::filesystem::path([executablePath UTF8String]).parent_path();
  }
}

inline NSString* NSStringFromUTF8(const std::string& value) {
  if (value.empty()) {
    return @"";
  }
  NSString* str = [[NSString alloc] initWithBytes:value.data()
                                           length:value.size()
                                         encoding:NSUTF8StringEncoding];
  if (!str) {
    str = [[NSString alloc] initWithCString:value.c_str() encoding:NSUTF8StringEncoding];
  }
  return str;
}

inline NSString* NSStringFromUTF8(const char* value) {
  if (!value) {
    return @"";
  }
  return NSStringFromUTF8(std::string(value));
}

NSArray<NSString*>* NSArrayFromExtensions(const std::vector<std::string>& extensions) {
  if (extensions.empty()) {
    return nil;
  }
  NSMutableArray<NSString*>* types = [[NSMutableArray alloc] initWithCapacity:extensions.size()];
  for (const auto& ext : extensions) {
    NSString* nsExt = NSStringFromUTF8(ext);
    if (nsExt && nsExt.length > 0) {
      [types addObject:nsExt];
    }
  }
  return types.count > 0 ? types : nil;
}

#if MACUI_HAS_UNIFORM_TYPE_IDENTIFIERS
NSArray<UTType*>* ContentTypesFromExtensions(const std::vector<std::string>& extensions) API_AVAILABLE(macos(11.0)) {
  if (extensions.empty()) {
    return nil;
  }
  NSMutableArray<UTType*>* contentTypes = [[NSMutableArray alloc] initWithCapacity:extensions.size()];
  for (const auto& ext : extensions) {
    NSString* nsExt = NSStringFromUTF8(ext);
    if (!nsExt || nsExt.length == 0) {
      continue;
    }
    UTType* type = [UTType typeWithFilenameExtension:nsExt];
    if (type) {
      [contentTypes addObject:type];
    }
  }
  return contentTypes.count > 0 ? contentTypes : nil;
}
#endif
}  // namespace

struct MacUI::Impl {
  SDL_Window* sdl_window = nullptr;
  NSWindow* ns_window = nil;
  uint32_t max_scale_factor = 10;

  std::function<void(const std::string&)> on_open_rom_;
  std::function<void()> on_restart_gameboy_;
  std::function<void(const std::string&)> on_save_;
  std::function<void()> on_quick_save_;
  std::function<void()> on_quick_load_;
  std::function<void(const std::string&)> on_select_boot_rom_;
  std::function<void(uint32_t)> on_scale_change_;
  std::function<void()> on_prepare_pause_;
  std::function<void()> on_resume_pause_;

  NSMenu* file_menu = nil;
  NSMenu* video_menu = nil;
  std::vector<NSMenuItem*> scale_items;
  NSMenuItem* restart_item = nil;

  id controller = nil;

  void begin_blocking_operation() {
    if (on_prepare_pause_) {
      on_prepare_pause_();
    }
  }

  void end_blocking_operation() {
    if (on_resume_pause_) {
      on_resume_pause_();
    }
  }

  bool show_open_file_dialog(SDL_Window* window, const char* filter, const char* title,
                             std::string& out_path) {
    begin_blocking_operation();
    @autoreleasepool {
      NSOpenPanel* panel = [NSOpenPanel openPanel];
      panel.canChooseDirectories = NO;
      panel.canChooseFiles = YES;
      panel.canCreateDirectories = NO;
      panel.allowsMultipleSelection = NO;
      panel.prompt = NSLocalizedString(@"Open", @"Prompt button title for opening a file");
      if (title) {
        NSString* titleString = NSStringFromUTF8(title);
        if (titleString.length > 0) {
          panel.title = titleString;
        }
      }

      auto extensions = parse_extensions_from_filter(filter);
      NSArray<NSString*>* nsExtensions = NSArrayFromExtensions(extensions);

#if MACUI_HAS_UNIFORM_TYPE_IDENTIFIERS
      if (@available(macOS 11.0, *)) {
        NSArray<UTType*>* contentTypes = ContentTypesFromExtensions(extensions);
        if (contentTypes.count > 0) {
          panel.allowedContentTypes = contentTypes;
        }
      } else
#endif
      if (nsExtensions) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        panel.allowedFileTypes = nsExtensions;
#pragma clang diagnostic pop
      }

      (void)window;
      NSInteger result = [panel runModal];
      if (result == NSModalResponseOK) {
        NSURL* url = panel.URL;
        if (url) {
          out_path = std::string([[url path] UTF8String]);
          end_blocking_operation();
          return true;
        }
      }
    }
    end_blocking_operation();
    return false;
  }

  bool show_save_file_dialog(SDL_Window* window, const char* filter, const char* title,
                             std::string& out_path) {
    begin_blocking_operation();
    @autoreleasepool {
      NSSavePanel* panel = [NSSavePanel savePanel];
      panel.canCreateDirectories = YES;
      if (title) {
        NSString* titleString = NSStringFromUTF8(title);
        if (titleString.length > 0) {
          panel.title = titleString;
        }
      }
      panel.prompt = NSLocalizedString(@"Save", @"Prompt button title for saving a file");

      auto extensions = parse_extensions_from_filter(filter);
      NSArray<NSString*>* nsExtensions = NSArrayFromExtensions(extensions);

#if MACUI_HAS_UNIFORM_TYPE_IDENTIFIERS
      if (@available(macOS 11.0, *)) {
        NSArray<UTType*>* contentTypes = ContentTypesFromExtensions(extensions);
        if (contentTypes.count > 0) {
          panel.allowedContentTypes = contentTypes;
        }
      } else
#endif
      if (nsExtensions) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        panel.allowedFileTypes = nsExtensions;
#pragma clang diagnostic pop
      }

      (void)window;
      NSInteger result = [panel runModal];
      if (result == NSModalResponseOK) {
        NSURL* url = panel.URL;
        if (url) {
          out_path = std::string([[url path] UTF8String]);
          end_blocking_operation();
          return true;
        }
      }
    }
    end_blocking_operation();
    return false;
  }

  void update_scale_menu_state(uint32_t requested_scale) {
    for (NSMenuItem* item : scale_items) {
      item.state = (item.tag == static_cast<NSInteger>(requested_scale)) ? NSControlStateValueOn
                                                                         : NSControlStateValueOff;
    }
  }

  void request_scale(uint32_t requested_scale) {
    update_scale_menu_state(requested_scale);
    if (on_scale_change_) {
      on_scale_change_(requested_scale);
    }
  }

  void set_application_icon() {
    namespace fs = std::filesystem;
    fs::path exe_dir = executable_directory();
    std::vector<fs::path> candidates = {
        exe_dir / "../mac_main/favicon.ico",
        exe_dir / "mac_main/favicon.ico",
        exe_dir / "../windows_main/favicon.ico"};

    for (const auto& candidate : candidates) {
      if (!fs::exists(candidate)) {
        continue;
      }
      @autoreleasepool {
        std::string candidateString = candidate.string();
        NSString* path = NSStringFromUTF8(candidateString);
        if (!path || path.length == 0) {
          continue;
        }
        NSImage* icon = [[NSImage alloc] initWithContentsOfFile:path];
        if (icon) {
          [NSApp setApplicationIconImage:icon];
          return;
        }
      }
    }
  }
};

@interface MacMenuController : NSObject <NSWindowDelegate>
@property(nonatomic, assign) MacUI::Impl* impl;
- (instancetype)initWithImpl:(MacUI::Impl*)impl;
- (void)setupMenus;
- (void)openRom:(id)sender;
- (void)saveState:(id)sender;
- (void)quickSave:(id)sender;
- (void)quickLoad:(id)sender;
- (void)restartGameboy:(id)sender;
- (void)selectBootRom:(id)sender;
- (void)exitApplication:(id)sender;
- (void)changeScale:(id)sender;
@end

@implementation MacMenuController

- (instancetype)initWithImpl:(MacUI::Impl*)impl {
  self = [super init];
  if (self) {
    _impl = impl;
  }
  return self;
}

- (void)setupMenus {
  @autoreleasepool {
    NSApplication* app = [NSApplication sharedApplication];
    [app setActivationPolicy:NSApplicationActivationPolicyRegular];

    NSMenu* mainMenu =
        [[NSMenu alloc] initWithTitle:NSLocalizedString(@"MainMenu", @"Main menu title")];

    // App menu with Quit
    NSMenuItem* appMenuItem = [[NSMenuItem alloc]
        initWithTitle:NSLocalizedString(@"GBEmu", @"App menu title")
               action:nil
        keyEquivalent:@""];
    NSMenu* appMenu = [[NSMenu alloc] initWithTitle:NSLocalizedString(@"GBEmu", @"App menu title")];
    NSMenuItem* quitItem = [[NSMenuItem alloc]
        initWithTitle:NSLocalizedString(@"Quit GBEmu", @"Quit menu item")
              action:@selector(exitApplication:)
       keyEquivalent:@"q"];
    quitItem.target = self;
    quitItem.keyEquivalentModifierMask = NSEventModifierFlagCommand;
    [appMenu addItem:quitItem];
    appMenuItem.submenu = appMenu;
    [mainMenu addItem:appMenuItem];

    // File menu
    NSMenuItem* fileMenuItem = [[NSMenuItem alloc]
        initWithTitle:NSLocalizedString(@"File", @"File menu title")
               action:nil
        keyEquivalent:@""];
    NSMenu* fileMenu = [[NSMenu alloc] initWithTitle:NSLocalizedString(@"File", @"File menu title")];

    NSMenuItem* openItem = [[NSMenuItem alloc]
        initWithTitle:NSLocalizedString(@"Open ROM", @"Open ROM menu item")
              action:@selector(openRom:)
       keyEquivalent:@"o"];
    openItem.target = self;
    openItem.keyEquivalentModifierMask = NSEventModifierFlagCommand;
    [fileMenu addItem:openItem];

    NSMenuItem* selectBootItem = [[NSMenuItem alloc]
        initWithTitle:NSLocalizedString(@"Select Boot ROM", @"Select Boot ROM menu item")
              action:@selector(selectBootRom:)
       keyEquivalent:@"b"];
    selectBootItem.target = self;
    selectBootItem.keyEquivalentModifierMask = NSEventModifierFlagCommand;
    [fileMenu addItem:selectBootItem];

    NSMenuItem* saveItem = [[NSMenuItem alloc]
        initWithTitle:NSLocalizedString(@"Save", @"Save menu item")
              action:@selector(saveState:)
       keyEquivalent:@"s"];
    saveItem.target = self;
    saveItem.keyEquivalentModifierMask = NSEventModifierFlagCommand;
    [fileMenu addItem:saveItem];

    [fileMenu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* quickSaveItem = [[NSMenuItem alloc]
        initWithTitle:NSLocalizedString(@"Quick Save", @"Quick Save menu item")
              action:@selector(quickSave:)
       keyEquivalent:@""];
    quickSaveItem.target = self;
    [fileMenu addItem:quickSaveItem];

    NSMenuItem* quickLoadItem = [[NSMenuItem alloc]
        initWithTitle:NSLocalizedString(@"Quick Load", @"Quick Load menu item")
              action:@selector(quickLoad:)
       keyEquivalent:@""];
    quickLoadItem.target = self;
    [fileMenu addItem:quickLoadItem];

    [fileMenu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* restartItem = [[NSMenuItem alloc]
        initWithTitle:NSLocalizedString(@"Restart Gameboy", @"Restart Gameboy menu item")
              action:@selector(restartGameboy:)
       keyEquivalent:@"r"];
    restartItem.target = self;
    restartItem.keyEquivalentModifierMask = NSEventModifierFlagCommand | NSEventModifierFlagShift;
    [fileMenu addItem:restartItem];
    _impl->restart_item = restartItem;

    [fileMenu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* exitItem = [[NSMenuItem alloc]
        initWithTitle:NSLocalizedString(@"Exit", @"Exit menu item")
              action:@selector(exitApplication:)
       keyEquivalent:@"w"];
    exitItem.target = self;
    exitItem.keyEquivalentModifierMask = NSEventModifierFlagCommand;
    [fileMenu addItem:exitItem];

    fileMenuItem.submenu = fileMenu;
    [mainMenu addItem:fileMenuItem];
    _impl->file_menu = fileMenu;

    // Video menu
    NSMenuItem* videoMenuItem = [[NSMenuItem alloc]
        initWithTitle:NSLocalizedString(@"Video", @"Video menu title")
               action:nil
        keyEquivalent:@""];
    NSMenu* videoMenu =
        [[NSMenu alloc] initWithTitle:NSLocalizedString(@"Video", @"Video menu title")];

    for (uint32_t scale = 1; scale <= _impl->max_scale_factor; ++scale) {
      NSString* title = [NSString stringWithFormat:NSLocalizedString(@"Scale Factor x%u",
                                                                     @"Scale factor menu item"),
                                                   scale];
      NSMenuItem* scaleItem = [[NSMenuItem alloc] initWithTitle:title
                                                         action:@selector(changeScale:)
                                                  keyEquivalent:@""];
      scaleItem.target = self;
      scaleItem.tag = static_cast<NSInteger>(scale);
      [videoMenu addItem:scaleItem];
      _impl->scale_items.push_back(scaleItem);
    }

    videoMenuItem.submenu = videoMenu;
    [mainMenu addItem:videoMenuItem];
    _impl->video_menu = videoMenu;

    app.mainMenu = mainMenu;
    [app activateIgnoringOtherApps:YES];
  }
}

- (void)openRom:(id)sender {
  if (!_impl) {
    return;
  }
  std::string path;
  if (_impl->show_open_file_dialog(_impl->sdl_window, kRomFilter,
                                   NSLocalizedString(@"Open ROM", @"Open ROM dialog title").UTF8String, path) &&
      _impl->on_open_rom_) {
    _impl->on_open_rom_(path);
  }
}

- (void)saveState:(id)sender {
  if (!_impl) {
    return;
  }
  std::string path;
  if (_impl->show_save_file_dialog(_impl->sdl_window, kSaveFilter,
                                   NSLocalizedString(@"Save State", @"Save state dialog title").UTF8String, path) &&
      _impl->on_save_) {
    _impl->on_save_(path);
  }
}

- (void)quickSave:(id)sender {
  if (_impl && _impl->on_quick_save_) {
    _impl->on_quick_save_();
  }
}

- (void)quickLoad:(id)sender {
  if (_impl && _impl->on_quick_load_) {
    _impl->on_quick_load_();
  }
}

- (void)restartGameboy:(id)sender {
  if (_impl && _impl->on_restart_gameboy_) {
    _impl->on_restart_gameboy_();
  }
}

- (void)selectBootRom:(id)sender {
  if (!_impl) {
    return;
  }
  std::string path;
  if (_impl->show_open_file_dialog(
          _impl->sdl_window, kBootRomFilter,
          NSLocalizedString(@"Select Boot ROM", @"Select boot ROM dialog title").UTF8String, path) &&
      _impl->on_select_boot_rom_) {
    _impl->on_select_boot_rom_(path);
  }
}

- (void)exitApplication:(id)sender {
   std::exit(0);
  [NSApp terminate:nil];
}

- (void)changeScale:(id)sender {
  if (!_impl) {
    return;
  }
  NSMenuItem* item = static_cast<NSMenuItem*>(sender);
  uint32_t requested_scale = static_cast<uint32_t>(item.tag);
  _impl->request_scale(requested_scale);
}

#pragma mark - NSWindowDelegate

- (void)windowWillStartLiveResize:(NSNotification*)notification {
  if (_impl && _impl->on_prepare_pause_) {
    _impl->on_prepare_pause_();
  }
}

- (void)windowDidEndLiveResize:(NSNotification*)notification {
  if (_impl && _impl->on_resume_pause_) {
    _impl->on_resume_pause_();
  }
}

@end

MacUI::MacUI() : impl_(new Impl()) {}

MacUI::~MacUI() {
  cleanup();
  delete impl_;
  impl_ = nullptr;
}

void MacUI::initialize(SDL_Window* sdl_window, uint32_t max_scale_factor) {
  impl_->sdl_window = sdl_window;
  impl_->ns_window = get_ns_window_from_sdl(sdl_window);
  impl_->max_scale_factor = max_scale_factor;

  MacMenuController* controller = [[MacMenuController alloc] initWithImpl:impl_];
  impl_->controller = controller;
  [controller setupMenus];

  if (impl_->ns_window) {
    impl_->ns_window.delegate = controller;
  }

  impl_->set_application_icon();
}

void MacUI::cleanup() {
  if (!impl_) {
    return;
  }
  @autoreleasepool {
    if (impl_->ns_window && impl_->controller) {
      impl_->ns_window.delegate = nil;
    }
    impl_->controller = nil;
    impl_->scale_items.clear();
    impl_->file_menu = nil;
    impl_->video_menu = nil;
    impl_->ns_window = nil;
    impl_->sdl_window = nullptr;
  }
}

void MacUI::show_error(SDL_Window* sdl_window, const std::string& title, const std::string& message) {
  @autoreleasepool {
    NSAlert* alert = [[NSAlert alloc] init];
    alert.alertStyle = NSAlertStyleCritical;
    NSString* titleString = NSStringFromUTF8(title);
    NSString* messageString = NSStringFromUTF8(message);
    if (titleString.length > 0) {
      alert.messageText = titleString;
    }
    if (messageString.length > 0) {
      alert.informativeText = messageString;
    }

    NSWindow* parent = sdl_window ? get_ns_window_from_sdl(sdl_window) : impl_->ns_window;
    if (parent) {
      [alert beginSheetModalForWindow:parent completionHandler:nil];
    } else {
      [alert runModal];
    }
  }
}

bool MacUI::show_open_file_dialog(SDL_Window* sdl_window, const char* filter, const char* title,
                                  std::string& out_path) {
  return impl_->show_open_file_dialog(sdl_window, filter, title, out_path);
}

bool MacUI::show_save_file_dialog(SDL_Window* sdl_window, const char* filter, const char* title,
                                  std::string& out_path) {
  return impl_->show_save_file_dialog(sdl_window, filter, title, out_path);
}

void MacUI::set_on_open_rom(std::function<void(const std::string&)> cb) {
  impl_->on_open_rom_ = std::move(cb);
}

void MacUI::set_on_restart_gameboy(std::function<void()> cb) {
  impl_->on_restart_gameboy_ = std::move(cb);
}

void MacUI::set_on_save(std::function<void(const std::string&)> cb) {
  impl_->on_save_ = std::move(cb);
}

void MacUI::set_on_quick_save(std::function<void()> cb) {
  impl_->on_quick_save_ = std::move(cb);
}

void MacUI::set_on_quick_load(std::function<void()> cb) {
  impl_->on_quick_load_ = std::move(cb);
}

void MacUI::set_on_select_boot_rom(std::function<void(const std::string&)> cb) {
  impl_->on_select_boot_rom_ = std::move(cb);
}

void MacUI::set_on_scale_change(std::function<void(uint32_t)> cb) {
  impl_->on_scale_change_ = std::move(cb);
}

void MacUI::set_on_prepare_pause(std::function<void()> cb) {
  impl_->on_prepare_pause_ = std::move(cb);
}

void MacUI::set_on_resume_pause(std::function<void()> cb) {
  impl_->on_resume_pause_ = std::move(cb);
}

