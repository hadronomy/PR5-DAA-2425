#if defined(_WIN32)
// To avoid conflicting windows.h symbols with raylib, some flags are defined
// WARNING: Those flags avoid inclusion of some Win32 headers that could be required
// by user at some point and won't be included...
//-------------------------------------------------------------------------------------

// If defined, the following flags inhibit definition of the indicated items.
#define NOGDICAPMASKS      // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
#define NOVIRTUALKEYCODES  // VK_*
#define NOWINMESSAGES      // WM_*, EM_*, LB_*, CB_*
#define NOWINSTYLES        // WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
#define NOSYSMETRICS       // SM_*
#define NOMENUS            // MF_*
#define NOICONS            // IDI_*
#define NOKEYSTATES        // MK_*
#define NOSYSCOMMANDS      // SC_*
#define NORASTEROPS        // Binary and Tertiary raster ops
#define NOSHOWWINDOW       // SW_*
#define OEMRESOURCE        // OEM Resource values
#define NOATOM             // Atom Manager routines
#define NOCLIPBOARD        // Clipboard routines
#define NOCOLOR            // Screen colors
#define NOCTLMGR           // Control and Dialog routines
#define NODRAWTEXT         // DrawText() and DT_*
#define NOGDI              // All GDI defines and routines
#define NOKERNEL           // All KERNEL defines and routines
#define NOUSER             // All USER defines and routines
// #define NONLS             // All NLS defines and routines
#define NOMB              // MB_* and MessageBox()
#define NOMEMMGR          // GMEM_*, LMEM_*, GHND, LHND, associated routines
#define NOMETAFILE        // typedef METAFILEPICT
#define NOMINMAX          // Macros min(a,b) and max(a,b)
#define NOMSG             // typedef MSG and associated routines
#define NOOPENFILE        // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#define NOSCROLL          // SB_* and scrolling routines
#define NOSERVICE         // All Service Controller routines, SERVICE_ equates, etc.
#define NOSOUND           // Sound driver routines
#define NOTEXTMETRIC      // typedef TEXTMETRIC and associated routines
#define NOWH              // SetWindowsHook and WH_*
#define NOWINOFFSETS      // GWL_*, GCL_*, associated routines
#define NOCOMM            // COMM driver routines
#define NOKANJI           // Kanji support stuff.
#define NOHELP            // Help engine interface.
#define NOPROFILER        // Profiler interface.
#define NODEFERWINDOWPOS  // DeferWindowPos routines
#define NOMCX             // Modem Configuration Extensions
#define MMNOSOUND

// Type required before windows.h inclusion
typedef struct tagMSG* LPMSG;

#include <windows.h>

// Type required by some unused function...
#ifndef TAG_BITMAPINFOHEADER
#define TAG_BITMAPINFOHEADER
typedef struct tagBITMAPINFOHEADER {
  DWORD biSize;
  LONG biWidth;
  LONG biHeight;
  WORD biPlanes;
  WORD biBitCount;
  DWORD biCompression;
  DWORD biSizeImage;
  LONG biXPelsPerMeter;
  LONG biYPelsPerMeter;
  DWORD biClrUsed;
  DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;
#endif

#include <mmreg.h>
#include <mmsystem.h>
#include <objbase.h>

// Some required types defined for MSVC/TinyC compiler
#if defined(_MSC_VER) || defined(__TINYC__)
#include "propidl.h"
#endif
#endif

#include "commands/visualize.h"

#include <fmt/format.h>

#include "ui.h"
#include "visualization/application.h"

namespace daa {

bool VisualizeCommand::execute() {
  try {
    visualization::VisApplication gui;

    // Create configuration for the application
    visualization::VisApplication::Config config;
    config.width = 1024;
    config.height = 768;
    config.title = "DAA Visualizer";

    // Initialize the application with the configuration
    auto result = gui.initialize(config);
    if (!result) {
      if (verbose_) {
        UI::error("Failed to initialize visualization GUI");
      }
      return false;
    }

    // Run the application
    gui.run();
    return true;
  } catch (const std::exception& e) {
    UI::error(fmt::format("Visualization failed: {}", e.what()));
    return false;
  }
}

void VisualizeCommand::registerCommand(CommandRegistry& registry) {
  registry.registerCommandType<VisualizeCommand>(
    "visualize",
    "Visualize solutions",
    [](CLI::App* cmd) {
      // Add any visualize-specific options here
      return cmd;
    },
    [](bool verbose) { return std::make_unique<VisualizeCommand>(verbose); }
  );
}

}  // namespace daa