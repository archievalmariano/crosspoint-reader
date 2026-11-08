#pragma once

// Fork: built-in NotoSerif for GOTO and DOGEAR only. CrossPoint 2.0 moved its
// serif to the SD card; these apps keep the 1.6.5 faces so they work without
// card fonts and render exactly as before. The reader does not use them.
// IDs are the 1.6.5 values (lib/EpdFont/scripts/build-font-ids.sh over the
// full families). Faces present, from an audit of every GOTO/DOGEAR call:
// Regular 12/14/16/18 (line height reads Regular), Bold 14/16/18, Italic 12.
// Any other style at these sizes falls back to Regular.
#define NOTOSERIF_12_FONT_ID (778531645)
#define NOTOSERIF_14_FONT_ID (-928381217)
#define NOTOSERIF_16_FONT_ID (17214534)
#define NOTOSERIF_18_FONT_ID (840051567)

class GfxRenderer;

// Registers the faces above. Does nothing in builds without GOTO or DOGEAR.
void registerSuiteSerifFonts(GfxRenderer& renderer);
