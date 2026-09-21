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

namespace {

// Map a shared Gate font role to the device font id that reproduces it. The
// sizes/weights are read from gate::layout::fontSpec -- the single source of truth
// shared with the desktop preview -- so there is no second typography spec to
// drift. This is the ONLY place CrossPoint font ids appear for Gate; the portable
// engine stays free of them. Faces match the preview's families/weights
// (candidate C). A role whose (serif,px,bold) has no device face resolves to 0
// and trips the static_assert below.
constexpr int gateDeviceFontId(gate::layout::Font role) {
  const gate::layout::FontSpec s = gate::layout::fontSpec(role);
  if (s.serif && s.bold && s.px == 22) return NOTOSERIF_22_BOLD_FONT_ID;  // Title
  if (s.serif && s.bold && s.px == 24) return NOTOSERIF_24_BOLD_FONT_ID;  // Beat
  if (s.serif && !s.bold && s.px == 19) return NOTOSERIF_19_FONT_ID;      // Body
  if (!s.serif && !s.bold && s.px == 18) return NOTOSANS_18_FONT_ID;      // Menu (reused)
  if (!s.serif && !s.bold && s.px == 15) return NOTOSANS_15_FONT_ID;      // Small
  return 0;
}

constexpr int TITLE_FONT = gateDeviceFontId(gate::layout::Font::Title);
constexpr int BODY_FONT = gateDeviceFontId(gate::layout::Font::Body);
constexpr int MENU_FONT = gateDeviceFontId(gate::layout::Font::Menu);
constexpr int SMALL_FONT = gateDeviceFontId(gate::layout::Font::Small);
constexpr int BEAT_FONT = gateDeviceFontId(gate::layout::Font::Beat);
static_assert(TITLE_FONT && BODY_FONT && MENU_FONT && SMALL_FONT && BEAT_FONT,
              "A Gate typography role has no matching device font face. Add the face "
              "(lib/EpdFont) or extend gateDeviceFontId() to keep preview/device in sync.");

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

// Draw the menu starting at `top`; returns the y just below the last row.
int GateActivity::drawMenu(const gate::Screen& s, int top, int bottomLimit) {
  const int W = renderer.getScreenWidth();
  const int labelH = renderer.getLineHeight(MENU_FONT);
  const int subH = renderer.getLineHeight(SMALL_FONT);
  int y = top;
  for (size_t i = 0; i < s.menu.size(); ++i) {
    const auto& item = s.menu[i];
    const bool hasSub = !item.sublabel.empty();
    const int rowH = labelH + (hasSub ? subH : 0) + gate::layout::kMenuRowPad;
    if (y + rowH > bottomLimit) break;  // never overflow into the footer
    const bool selected = static_cast<int>(i) == s.cursor;
    if (selected) renderer.fillRect(MARGIN, y, W - 2 * MARGIN, rowH, true);
    const bool black = !selected;  // white text on the black selection bar

    std::string label = item.label;
    if (!item.enabled) label += "  (n/a)";
    label = renderer.truncatedText(MENU_FONT, label.c_str(), W - 2 * MARGIN - 2 * gate::layout::kMenuTextInset);
    renderer.drawText(MENU_FONT, MARGIN + gate::layout::kMenuTextInset, y + 6, label.c_str(), black);
    if (hasSub) {
      auto subLines = renderer.wrappedText(SMALL_FONT, item.sublabel.c_str(), W - 2 * MARGIN - 20, 1);
      if (!subLines.empty())
        renderer.drawText(SMALL_FONT, MARGIN + gate::layout::kMenuTextInset, y + 6 + labelH, subLines[0].c_str(),
                          black);
    }
    y += rowH + gate::layout::kMenuRowGap;
  }
  return y;
}

void GateActivity::drawError() {
  renderer.clearScreen();
  const int W = renderer.getScreenWidth();
  renderer.drawText(TITLE_FONT, MARGIN, MARGIN, "The Gate Is Open!", true);
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

  // ---- Header --------------------------------------------------------------
  int y = MARGIN;
  const bool bigTitle = (s.kind == gate::Screen::Kind::Title);
  if (bigTitle) {
    renderer.drawCenteredText(TITLE_FONT, y + 10, s.title.c_str(), true);
    y += renderer.getLineHeight(TITLE_FONT) + 16;
  } else if (!s.title.empty() && s.kind != gate::Screen::Kind::Beat) {
    // Beats stay sparse (no header) for a full-screen moment.
    renderer.drawText(TITLE_FONT, MARGIN, y, renderer.truncatedText(TITLE_FONT, s.title.c_str(), contentW).c_str(),
                      true);
    y += renderer.getLineHeight(TITLE_FONT) + 4;
    renderer.drawLine(MARGIN, y, W - MARGIN, y, true);
    y += 10;
  }
  const int contentTop = y;

  // ---- Character select: a simple list of tiles from the top ---------------
  if (s.kind == gate::Screen::Kind::CharacterSelect) {
    drawMenu(s, contentTop, contentBottom - 4);
    drawHints(s);
    return;
  }

  // ---- Bottom-anchored menu block (shared by remaining kinds) --------------
  const int menuLabelH = renderer.getLineHeight(MENU_FONT);
  const int menuRows = static_cast<int>(s.menu.size());
  int menuBlockH = 0;
  for (const auto& m : s.menu)
    menuBlockH += menuLabelH + (m.sublabel.empty() ? 0 : renderer.getLineHeight(SMALL_FONT)) +
                  gate::layout::kMenuRowPad + gate::layout::kMenuRowGap;
  const int menuTop = (menuRows > 0) ? (contentBottom - gate::layout::kMenuGapAboveBlock - menuBlockH) : contentBottom;

  // ---- World events (autonomous reactions), just above the menu ------------
  int weTop = menuTop;
  if (!s.worldEvents.empty()) {
    const std::string joined = std::string("\xC2\xBB ") + joinLines(s.worldEvents, "  \xC2\xBB ");
    auto weLines = renderer.wrappedText(SMALL_FONT, joined.c_str(), contentW, gate::layout::kWorldEventsMaxLines);
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
    auto lines = renderer.wrappedText(BEAT_FONT, text.c_str(), contentW, 5);
    const int lh = renderer.getLineHeight(BEAT_FONT);
    const int region = weTop - contentTop;
    int by = contentTop + (region - static_cast<int>(lines.size()) * lh) / 2;
    if (by < contentTop) by = contentTop;
    for (const auto& l : lines) {
      renderer.drawCenteredText(BEAT_FONT, by, l.c_str(), true);
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

  // ---- Title: show the device control hint under the intro text ------------
  int bodyBottom = weTop;
  if (bigTitle) {
    const int hintLH = renderer.getLineHeight(SMALL_FONT);
    const int hintY = menuTop - hintLH - 8;
    renderer.drawCenteredText(SMALL_FONT, hintY, profile_.controlHint, true);
    bodyBottom = hintY - 4;
  }

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
      auto wrapped = renderer.wrappedText(BODY_FONT, para.c_str(), contentW, maxLines - used);
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
