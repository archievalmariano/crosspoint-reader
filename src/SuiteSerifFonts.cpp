#include "SuiteSerifFonts.h"

#include <GfxRenderer.h>

#if defined(GOTO_ENABLED) || defined(DOGEAR_ENABLED)
#include <builtinFonts/notoserif_12_italic.h>
#include <builtinFonts/notoserif_12_regular.h>
#include <builtinFonts/notoserif_14_bold.h>
#include <builtinFonts/notoserif_14_regular.h>
#include <builtinFonts/notoserif_16_bold.h>
#include <builtinFonts/notoserif_16_regular.h>
#include <builtinFonts/notoserif_18_bold.h>
#include <builtinFonts/notoserif_18_regular.h>

namespace {
EpdFont serif12Regular(&notoserif_12_regular);
EpdFont serif12Italic(&notoserif_12_italic);
EpdFont serif14Regular(&notoserif_14_regular);
EpdFont serif14Bold(&notoserif_14_bold);
EpdFont serif16Regular(&notoserif_16_regular);
EpdFont serif16Bold(&notoserif_16_bold);
EpdFont serif18Regular(&notoserif_18_regular);
EpdFont serif18Bold(&notoserif_18_bold);

EpdFontFamily serif12(12, &serif12Regular, nullptr, &serif12Italic);
EpdFontFamily serif14(14, &serif14Regular, &serif14Bold);
EpdFontFamily serif16(16, &serif16Regular, &serif16Bold);
EpdFontFamily serif18(18, &serif18Regular, &serif18Bold);
}  // namespace

void registerSuiteSerifFonts(GfxRenderer& renderer) {
  renderer.insertFont(NOTOSERIF_12_FONT_ID, serif12);
  renderer.insertFont(NOTOSERIF_14_FONT_ID, serif14);
  renderer.insertFont(NOTOSERIF_16_FONT_ID, serif16);
  renderer.insertFont(NOTOSERIF_18_FONT_ID, serif18);
}
#else
void registerSuiteSerifFonts(GfxRenderer&) {}
#endif
