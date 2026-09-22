#ifdef GATE_ENABLED

#include "GateActivity.h"

#include <GfxRenderer.h>
#include <HalDisplay.h>
#include <Logging.h>

#include <string>

#include "GateContent.generated.h"
#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"
#include "gate/GateLayout.h"  // shared layout constants (also used by the desktop preview)
#include "gate/Loader.h"
#include "gate/TextWrap.h"  // shared word-wrap policy (same as the desktop preview)

namespace {

// Map a shared Gate font role to a NATIVE CrossPoint font FAMILY id. Sizes come
// from gate::layout::fontSpec (the single source of truth shared with the preview)
// and are the point sizes CrossPoint itself ships (12/14/16/18); weight is applied
// as a render-time STYLE (gateDeviceStyle) rather than a custom bold-baked face,
// so Gate uses only native device fonts. This is the ONLY place CrossPoint font
// ids appear for Gate; the portable engine stays free of them. A role whose
// (serif,px) has no native family resolves to 0 and trips the static_assert below.
constexpr int gateDeviceFontId(gate::layout::Font role) {
  const gate::layout::FontSpec s = gate::layout::fontSpec(role);
  if (s.serif && s.px == 18) return NOTOSERIF_18_FONT_ID;  // Title / Beat (bold via style)
  if (s.serif && s.px == 16) return NOTOSERIF_16_FONT_ID;  // (headroom)
  if (s.serif && s.px == 14) return NOTOSERIF_14_FONT_ID;  // Body
  if (s.serif && s.px == 12) return NOTOSERIF_12_FONT_ID;  // (headroom)
  if (!s.serif && s.px == 18) return NOTOSANS_18_FONT_ID;  // (headroom)
  if (!s.serif && s.px == 16) return NOTOSANS_16_FONT_ID;  // Menu
  if (!s.serif && s.px == 14) return NOTOSANS_14_FONT_ID;  // (headroom)
  if (!s.serif && s.px == 12) return NOTOSANS_12_FONT_ID;  // Small
  return 0;
}

// Bold roles render with the family's BOLD face (native), keeping Gate's header
// personality without a custom bold-baked font.
constexpr EpdFontFamily::Style gateDeviceStyle(gate::layout::Font role) {
  return gate::layout::fontSpec(role).bold ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;
}

constexpr int TITLE_FONT = gateDeviceFontId(gate::layout::Font::Title);
constexpr int BODY_FONT = gateDeviceFontId(gate::layout::Font::Body);
constexpr int MENU_FONT = gateDeviceFontId(gate::layout::Font::Menu);
constexpr int SMALL_FONT = gateDeviceFontId(gate::layout::Font::Small);
constexpr int BEAT_FONT = gateDeviceFontId(gate::layout::Font::Beat);
constexpr EpdFontFamily::Style TITLE_STYLE = gateDeviceStyle(gate::layout::Font::Title);
constexpr EpdFontFamily::Style BEAT_STYLE = gateDeviceStyle(gate::layout::Font::Beat);
static_assert(TITLE_FONT && BODY_FONT && MENU_FONT && SMALL_FONT && BEAT_FONT,
              "A Gate typography role has no matching native device family. Extend "
              "gateDeviceFontId() to keep preview/device in sync.");

// Layout numbers are sourced from the shared gate::layout spec so the on-device
// renderer and the desktop preview cannot drift.
constexpr int MARGIN = gate::layout::kMargin;

std::string joinLines(const std::vector<std::string>& lines, const char* sep) {
  std::string out;
  for (size_t i = 0; i < lines.size(); ++i) {
    if (i) out += sep;
    out += lines[i];
  }
  return out;
}

const char* kindName(gate::Screen::Kind k) {
  switch (k) {
    case gate::Screen::Kind::Title:
      return "title";
    case gate::Screen::Kind::CharacterSelect:
      return "select";
    case gate::Screen::Kind::Intro:
      return "intro";
    case gate::Screen::Kind::Dialogue:
      return "dialogue";
    case gate::Screen::Kind::Beat:
      return "beat";
    case gate::Screen::Kind::Ending:
      return "ending";
    case gate::Screen::Kind::Home:
      return "home";
    case gate::Screen::Kind::Debug:
      return "debug";
  }
  return "?";
}

}  // namespace

GateActivity::GateActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("Gate", renderer, mappedInput) {
  // Load the SAME JSON content the desktop build uses (embedded, validated at
  // runtime). No narrative logic is constructed here.
  std::string err;
  if (!gate::loadCast(gate_content::kGateCharactersJson, gate_content::kGateRelationshipsJson, cast_, err)) {
    loadError_ = "cast: " + err;
    LOG_ERR("GATE", "cast load failed: %s", err.c_str());
    return;
  }
  gate::Story story;
  if (!gate::loadStory(gate_content::kGateChachaStoryJson, story, err)) {
    loadError_ = "story: " + err;
    LOG_ERR("GATE", "story load failed: %s", err.c_str());
    return;
  }
  stories_.push_back(std::move(story));
  // platform_ is all-null: on device we drive Game via handle()/buildScreen(),
  // so display/input/clock/storage/sleep are unused. Save/load is deferred for
  // this first physical pass (documented); the engine is unchanged.
  game_ = std::make_unique<gate::Game>(platform_, std::move(cast_), std::move(stories_));
  loadedOk_ = true;
}

GateActivity::~GateActivity() = default;

void GateActivity::onEnter() {
  Activity::onEnter();
  requestUpdate();
}

void GateActivity::loop() {
  using B = MappedInputManager::Button;

  if (!loadedOk_) {
    if (mappedInput.wasReleased(B::Back) || mappedInput.wasReleased(B::Confirm)) finish();
    return;
  }

  bool acted = false;
  // NavPrevious/NavNext are the SDK's device-correct list-navigation buttons;
  // accept the plain directional/page keys too for robustness across units.
  if (mappedInput.wasReleased(B::NavPrevious) || mappedInput.wasReleased(B::Up) ||
      mappedInput.wasReleased(B::PageBack)) {
    game_->handle(gate::InputEvent::Up);
    acted = true;
  } else if (mappedInput.wasReleased(B::NavNext) || mappedInput.wasReleased(B::Down) ||
             mappedInput.wasReleased(B::PageForward)) {
    game_->handle(gate::InputEvent::Down);
    acted = true;
  } else if (mappedInput.wasReleased(B::Confirm)) {
    game_->handle(gate::InputEvent::Select);
    acted = true;
  } else if (mappedInput.wasReleased(B::Back)) {
    // At the Gate title, Back leaves the game and returns to CrossPoint home.
    if (game_->phase() == gate::Game::Phase::Title) {
      finish();
      return;
    }
    game_->handle(gate::InputEvent::Back);
    acted = true;
  }

  // Touch seam (touch-capable devices only; inert on the button-only X4, where
  // wasScreenTapped() is always false). For now a tap confirms the highlighted
  // choice, proving taps reach the portable engine. Per-choice hit-testing
  // (moving the cursor to the tapped row) is the documented next step and needs
  // no engine change. See src/activities/gate/README.md.
  if (profile_.hasTouch) {
    int tx = 0;
    int ty = 0;
    if (mappedInput.wasScreenTapped(tx, ty)) {
      game_->handle(gate::InputEvent::Select);
      acted = true;
    }
  }

  if (acted) requestUpdate();
}

// Lay out one menu row's wrapped label + description lines (shared wrap policy).
static void layoutGateRow(GfxRenderer& renderer, const gate::MenuItem& item, int innerW,
                          std::vector<std::string>& labelLines, std::vector<std::string>& subLines) {
  std::string label = item.label;
  if (!item.enabled) label += "  (n/a)";
  labelLines = gate::text::wrap(label, innerW, gate::layout::kMenuLabelMaxLines,
                                [&](const std::string& str) { return renderer.getTextWidth(MENU_FONT, str.c_str()); });
  if (labelLines.empty()) labelLines.push_back("");
  subLines.clear();
  if (!item.sublabel.empty())
    subLines = gate::text::wrap(item.sublabel, innerW, gate::layout::kCharDescMaxLines,
                                [&](const std::string& str) { return renderer.getTextWidth(SMALL_FONT, str.c_str()); });
}

// Total wrapped height of the whole menu block (for centering / bottom-anchoring).
int GateActivity::menuBlockHeight(const gate::Screen& s) {
  const int W = renderer.getScreenWidth();
  const int innerW = W - 2 * MARGIN - 2 * gate::layout::kMenuTextInset;
  const int labelH = renderer.getLineHeight(MENU_FONT);
  const int subH = renderer.getLineHeight(SMALL_FONT);
  int h = 0;
  std::vector<std::string> labelLines, subLines;
  for (const auto& item : s.menu) {
    layoutGateRow(renderer, item, innerW, labelLines, subLines);
    h += static_cast<int>(labelLines.size()) * labelH + static_cast<int>(subLines.size()) * subH +
         gate::layout::kMenuRowPad + gate::layout::kMenuRowGap;
  }
  return h;
}

// Draw wrapped, variable-height rows. The selection fill covers the FULL wrapped
// row height, so a 2-line choice is one selectable item. Buttons navigate by item
// (cursor index) and on-device tap confirms the highlighted item, so no per-line
// hit geometry is needed here.
int GateActivity::drawMenu(const gate::Screen& s, int top, int bottomLimit) {
  const int W = renderer.getScreenWidth();
  const int barW = W - 2 * MARGIN;
  const int innerW = barW - 2 * gate::layout::kMenuTextInset;
  const int labelH = renderer.getLineHeight(MENU_FONT);
  const int subH = renderer.getLineHeight(SMALL_FONT);
  int y = top;
  std::vector<std::string> labelLines, subLines;
  for (size_t i = 0; i < s.menu.size(); ++i) {
    layoutGateRow(renderer, s.menu[i], innerW, labelLines, subLines);
    const int rowH = static_cast<int>(labelLines.size()) * labelH + static_cast<int>(subLines.size()) * subH +
                     gate::layout::kMenuRowPad;
    if (y + rowH > bottomLimit) break;  // never overflow into the footer
    const bool selected = static_cast<int>(i) == s.cursor;
    if (selected) renderer.fillRect(MARGIN, y, barW, rowH, true);
    const bool black = !selected;  // white text on the black selection bar

    int ty = y + gate::layout::kMenuRowPad / 2;
    for (const auto& ln : labelLines) {
      renderer.drawText(MENU_FONT, MARGIN + gate::layout::kMenuTextInset, ty, ln.c_str(), black);
      ty += labelH;
    }
    for (const auto& ln : subLines) {
      renderer.drawText(SMALL_FONT, MARGIN + gate::layout::kMenuTextInset, ty, ln.c_str(), black);
      ty += subH;
    }
    y += rowH + gate::layout::kMenuRowGap;
  }
  return y;
}

void GateActivity::drawError() {
  renderer.clearScreen();
  const int W = renderer.getScreenWidth();
  renderer.drawText(TITLE_FONT, MARGIN, MARGIN, "The Gate Is Open!", true, TITLE_STYLE);
  renderer.drawText(BODY_FONT, MARGIN, MARGIN + 40, "Content failed to load:", true);
  auto lines = renderer.wrappedText(SMALL_FONT, loadError_.c_str(), W - 2 * MARGIN, 8);
  int y = MARGIN + 70;
  for (const auto& l : lines) {
    renderer.drawText(SMALL_FONT, MARGIN, y, l.c_str(), true);
    y += renderer.getLineHeight(SMALL_FONT);
  }
  renderer.drawText(SMALL_FONT, MARGIN, renderer.getScreenHeight() - 30, "Press Back to exit.", true);
  renderer.displayBuffer(HalDisplay::FULL_REFRESH);
}

// Device-correct button-hint bar (positions labels at the device's physical
// buttons via mapLabels + theme drawButtonHints). Replaces the old generic
// centered footer text, so hints match the real X4 controls.
void GateActivity::drawHints(const gate::Screen& s) {
  const char* back = "Back";
  const char* confirm = "Select";
  const char* prev = "";
  const char* next = "";
  switch (s.kind) {
    case gate::Screen::Kind::Intro:
      confirm = "Begin";
      break;
    case gate::Screen::Kind::Ending:
      back = "";
      confirm = "Continue";
      break;
    case gate::Screen::Kind::Beat:
      if (s.menu.size() > 1) {
        prev = "Up";
        next = "Down";
      } else {
        back = "";
        confirm = "Continue";
      }
      break;
    default:
      if (s.menu.size() > 1) {
        prev = "Up";
        next = "Down";
      } else {
        confirm = "Continue";
      }
      break;
  }
  const auto labels = mappedInput.mapLabels(back, confirm, prev, next);
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
}

void GateActivity::drawScreen(const gate::Screen& s) {
  renderer.clearScreen();
  const int W = renderer.getScreenWidth();
  const int H = renderer.getScreenHeight();
  const int contentW = W - 2 * MARGIN;
  // Reserve the theme's button-hint band at the bottom for drawHints().
  const int contentBottom = H - UITheme::getInstance().getMetrics().buttonHintsHeight;

  // ---- Title screen: composed group (title + subtitle) high, menu centered in
  // the lower region; controls stay tertiary in the button-hint bar. -----------
  if (s.kind == gate::Screen::Kind::Title) {
    int ty = MARGIN + gate::layout::kTitleGroupTopPad;
    for (const auto& ln : gate::text::wrap(
             s.title, contentW, gate::layout::kHeaderMaxLines,
             [&](const std::string& str) { return renderer.getTextWidth(TITLE_FONT, str.c_str(), TITLE_STYLE); })) {
      renderer.drawCenteredText(TITLE_FONT, ty, ln.c_str(), true, TITLE_STYLE);
      ty += renderer.getLineHeight(TITLE_FONT);
    }
    ty += gate::layout::kTitleSubtitleGap;
    if (!s.body.empty() && !s.body[0].empty()) {
      for (const auto& ln : gate::text::wrap(s.body[0], contentW, 2, [&](const std::string& str) {
             return renderer.getTextWidth(SMALL_FONT, str.c_str());
           })) {
        renderer.drawCenteredText(SMALL_FONT, ty, ln.c_str(), true);
        ty += renderer.getLineHeight(SMALL_FONT);
      }
    }
    const int groupBottom = ty;
    const int blockH = menuBlockHeight(s);
    int titleMenuTop = groupBottom + (contentBottom - groupBottom - blockH) / 2;
    if (titleMenuTop < groupBottom + 24) titleMenuTop = groupBottom + 24;
    drawMenu(s, titleMenuTop, contentBottom - 4);
    drawHints(s);
    return;
  }

  // ---- Header (game screens): authored heading WRAPS (never ellipsizes) -----
  int y = MARGIN;
  if (!s.title.empty() && s.kind != gate::Screen::Kind::Beat) {
    // Beats stay sparse (no header) for a full-screen moment.
    for (const auto& ln : gate::text::wrap(
             s.title, contentW, gate::layout::kHeaderMaxLines,
             [&](const std::string& str) { return renderer.getTextWidth(TITLE_FONT, str.c_str(), TITLE_STYLE); })) {
      renderer.drawText(TITLE_FONT, MARGIN, y, ln.c_str(), true, TITLE_STYLE);
      y += renderer.getLineHeight(TITLE_FONT);
    }
    y += gate::layout::kHeaderGapBelowTitle;
    renderer.drawLine(MARGIN, y, W - MARGIN, y, true);
    y += gate::layout::kHeaderGapBelowRule;
  }
  const int contentTop = y;

  // ---- Character select: vertically centered cards -------------------------
  if (s.kind == gate::Screen::Kind::CharacterSelect) {
    const int blockH = menuBlockHeight(s);
    int top = contentTop + (contentBottom - contentTop - blockH) / 2;
    if (top < contentTop) top = contentTop;
    drawMenu(s, top, contentBottom - 4);
    drawHints(s);
    return;
  }

  // ---- Bottom-anchored menu block (shared by remaining kinds) --------------
  const int menuRows = static_cast<int>(s.menu.size());
  const int menuBlockH = menuBlockHeight(s);
  const int menuTop = (menuRows > 0) ? (contentBottom - gate::layout::kMenuGapAboveBlock - menuBlockH) : contentBottom;

  // ---- World events (autonomous reactions), just above the menu ------------
  int weTop = menuTop;
  if (!s.worldEvents.empty()) {
    const std::string joined = std::string("\xC2\xBB ") + joinLines(s.worldEvents, "  \xC2\xBB ");
    auto weLines = gate::text::wrap(joined, contentW, gate::layout::kWorldEventsMaxLines, [&](const std::string& str) {
      return renderer.getTextWidth(SMALL_FONT, str.c_str());
    });
    const int lh = renderer.getLineHeight(SMALL_FONT);
    weTop = menuTop - gate::layout::kWorldEventsGapBelow - static_cast<int>(weLines.size()) * lh;
    int wy = weTop;
    for (const auto& l : weLines) {
      renderer.drawText(SMALL_FONT, MARGIN, wy, l.c_str(), true);
      wy += lh;
    }
  }

  // ---- Beat: large centered text in the space above the menu ---------------
  if (s.kind == gate::Screen::Kind::Beat) {
    const std::string text = joinLines(s.body, " ");
    auto lines = gate::text::wrap(text, contentW, 5, [&](const std::string& str) {
      return renderer.getTextWidth(BEAT_FONT, str.c_str(), BEAT_STYLE);
    });
    const int lh = renderer.getLineHeight(BEAT_FONT);
    const int region = weTop - contentTop;
    int by = contentTop + (region - static_cast<int>(lines.size()) * lh) / 2;
    if (by < contentTop) by = contentTop;
    for (const auto& l : lines) {
      renderer.drawCenteredText(BEAT_FONT, by, l.c_str(), true, BEAT_STYLE);
      by += lh;
    }
    if (menuRows > 0) drawMenu(s, menuTop, contentBottom - 4);
    drawHints(s);
    return;
  }

  // ---- Art placeholder (labelled box) --------------------------------------
  int bodyTop = contentTop;
  const bool showArt = !s.art.empty() && (s.kind == gate::Screen::Kind::Dialogue ||
                                          s.kind == gate::Screen::Kind::Intro || s.kind == gate::Screen::Kind::Ending);
  if (showArt) {
    const int artH = gate::layout::kArtBoxHeight;
    if (contentTop + artH + 8 < weTop) {  // only if it doesn't crush the text
      renderer.drawRect(MARGIN, contentTop, contentW, artH, true);
      std::string tag = std::string("[art: ") + s.art + "]";
      renderer.drawCenteredText(SMALL_FONT, contentTop + (artH - renderer.getLineHeight(SMALL_FONT)) / 2, tag.c_str(),
                                true);
      bodyTop = contentTop + artH + gate::layout::kArtGapBelow;
    }
  }

  const int bodyBottom = weTop;

  // ---- Body text (paragraph-aware, clamped so the menu stays visible) ------
  const int bodyLH = renderer.getLineHeight(BODY_FONT);
  const int avail = bodyBottom - bodyTop - 6;
  const int maxLines = avail > 0 ? avail / bodyLH : 0;
  if (maxLines > 0) {
    int by = bodyTop;
    int used = 0;
    for (const auto& para : s.body) {
      if (used >= maxLines) break;
      if (para.empty()) {  // blank line = paragraph gap
        by += bodyLH / 2;
        continue;
      }
      auto wrapped = gate::text::wrap(para, contentW, maxLines - used, [&](const std::string& str) {
        return renderer.getTextWidth(BODY_FONT, str.c_str());
      });
      for (const auto& l : wrapped) {
        renderer.drawText(BODY_FONT, MARGIN, by, l.c_str(), true);
        by += bodyLH;
        if (++used >= maxLines) break;
      }
    }
  }

  // ---- Menu + hints --------------------------------------------------------
  if (menuRows > 0) drawMenu(s, menuTop, contentBottom - 4);
  drawHints(s);
}

void GateActivity::render(RenderLock&&) {
  if (!loadedOk_) {
    drawError();
    return;
  }
  const gate::Screen s = game_->buildScreen();

  // Full refresh on a scene change (phase/node/kind), fast refresh for cursor
  // movement within the same scene, to keep selection snappy while clearing
  // ghosting whenever the composition actually changes.
  std::string sceneKey = std::string(kindName(s.kind)) + "|" + std::to_string(static_cast<int>(game_->phase())) + "|" +
                         game_->state().current_node;
  const bool sceneChanged = firstRender_ || sceneKey != lastSceneKey_ || s.kind == gate::Screen::Kind::Beat;
  lastSceneKey_ = sceneKey;
  firstRender_ = false;

  drawScreen(s);
  renderer.displayBuffer(sceneChanged ? HalDisplay::FULL_REFRESH : HalDisplay::FAST_REFRESH);
}

#endif  // GATE_ENABLED
