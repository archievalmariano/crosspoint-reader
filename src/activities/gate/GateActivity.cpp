#include "GateActivity.h"

#include <GfxRenderer.h>
#include <HalDisplay.h>
#include <Logging.h>

#include <string>

#include "MappedInputManager.h"
#include "fontIds.h"

#include "GateContent.generated.h"
#include "gate/Loader.h"

namespace {

// Font roles for this first physical layout (max available size is 18px).
constexpr int TITLE_FONT = NOTOSERIF_18_FONT_ID;
constexpr int BODY_FONT = NOTOSERIF_16_FONT_ID;
constexpr int MENU_FONT = NOTOSANS_16_FONT_ID;
constexpr int SMALL_FONT = NOTOSANS_14_FONT_ID;
constexpr int BEAT_FONT = NOTOSERIF_18_FONT_ID;

constexpr int MARGIN = 16;

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
    case gate::Screen::Kind::Title: return "title";
    case gate::Screen::Kind::CharacterSelect: return "select";
    case gate::Screen::Kind::Intro: return "intro";
    case gate::Screen::Kind::Dialogue: return "dialogue";
    case gate::Screen::Kind::Beat: return "beat";
    case gate::Screen::Kind::Ending: return "ending";
    case gate::Screen::Kind::Home: return "home";
    case gate::Screen::Kind::Debug: return "debug";
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
    const int rowH = labelH + (hasSub ? subH : 0) + 12;
    if (y + rowH > bottomLimit) break;  // never overflow into the footer
    const bool selected = static_cast<int>(i) == s.cursor;
    if (selected) renderer.fillRect(MARGIN, y, W - 2 * MARGIN, rowH, true);
    const bool black = !selected;  // white text on the black selection bar

    std::string label = item.label;
    if (!item.enabled) label += "  (n/a)";
    renderer.drawText(MENU_FONT, MARGIN + 10, y + 6, label.c_str(), black);
    if (hasSub) {
      auto subLines = renderer.wrappedText(SMALL_FONT, item.sublabel.c_str(), W - 2 * MARGIN - 20, 1);
      if (!subLines.empty())
        renderer.drawText(SMALL_FONT, MARGIN + 10, y + 6 + labelH, subLines[0].c_str(), black);
    }
    y += rowH + 4;
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

void GateActivity::drawScreen(const gate::Screen& s) {
  renderer.clearScreen();
  const int W = renderer.getScreenWidth();
  const int H = renderer.getScreenHeight();
  const int contentW = W - 2 * MARGIN;

  // ---- Beat: sparse, large, vertically centered ----------------------------
  if (s.kind == gate::Screen::Kind::Beat) {
    const std::string text = joinLines(s.body, " ");
    auto lines = renderer.wrappedText(BEAT_FONT, text.c_str(), contentW, 5);
    const int lh = renderer.getLineHeight(BEAT_FONT);
    int y = (H - static_cast<int>(lines.size()) * lh) / 2;
    if (y < MARGIN) y = MARGIN;
    for (const auto& l : lines) {
      renderer.drawCenteredText(BEAT_FONT, y, l.c_str(), true);
      y += lh;
    }
    return;
  }

  // ---- Footer hint (device-appropriate; ignores desktop key hints) ---------
  const int footerH = renderer.getLineHeight(SMALL_FONT);
  const int footerY = H - footerH - 6;
  const char* hint = nullptr;
  switch (s.kind) {
    case gate::Screen::Kind::Ending: hint = "OK: continue"; break;
    case gate::Screen::Kind::Intro: hint = "OK: begin    Back"; break;
    default: hint = s.menu.size() > 1 ? "Up/Down: move   OK: select   Back" : "OK: continue   Back";
  }
  renderer.drawLine(MARGIN, footerY - 4, W - MARGIN, footerY - 4, true);
  renderer.drawCenteredText(SMALL_FONT, footerY, hint, true);

  // ---- Header --------------------------------------------------------------
  int y = MARGIN;
  const bool bigTitle = (s.kind == gate::Screen::Kind::Title);
  if (bigTitle) {
    renderer.drawCenteredText(TITLE_FONT, y + 10, s.title.c_str(), true);
    y += renderer.getLineHeight(TITLE_FONT) + 20;
  } else if (!s.title.empty()) {
    renderer.drawText(TITLE_FONT, MARGIN, y, s.title.c_str(), true);
    y += renderer.getLineHeight(TITLE_FONT) + 4;
    renderer.drawLine(MARGIN, y, W - MARGIN, y, true);
    y += 10;
  }
  const int contentTop = y;

  // ---- Character select: a simple list of tiles from the top ---------------
  if (s.kind == gate::Screen::Kind::CharacterSelect) {
    drawMenu(s, contentTop, footerY - 8);
    return;  // render() owns displayBuffer()
  }

  // ---- Bottom-anchored menu (Title/Dialogue/Intro/Ending/Home) -------------
  const int menuLabelH = renderer.getLineHeight(MENU_FONT);
  int menuRows = static_cast<int>(s.menu.size());
  int menuBlockH = 0;
  for (const auto& m : s.menu)
    menuBlockH += menuLabelH + (m.sublabel.empty() ? 0 : renderer.getLineHeight(SMALL_FONT)) + 16;
  const int menuTop = (menuRows > 0) ? (footerY - 10 - menuBlockH) : footerY;

  // ---- World events (autonomous reactions), just above the menu ------------
  int weTop = menuTop;
  if (!s.worldEvents.empty()) {
    const std::string joined = std::string("\xC2\xBB ") + joinLines(s.worldEvents, "  \xC2\xBB ");
    auto weLines = renderer.wrappedText(SMALL_FONT, joined.c_str(), contentW, 3);
    const int lh = renderer.getLineHeight(SMALL_FONT);
    weTop = menuTop - 8 - static_cast<int>(weLines.size()) * lh;
    int wy = weTop;
    for (const auto& l : weLines) {
      renderer.drawText(SMALL_FONT, MARGIN, wy, l.c_str(), true);
      wy += lh;
    }
  }

  // ---- Art placeholder (labelled box) --------------------------------------
  int bodyTop = contentTop;
  const bool showArt = !s.art.empty() && (s.kind == gate::Screen::Kind::Dialogue ||
                                          s.kind == gate::Screen::Kind::Intro ||
                                          s.kind == gate::Screen::Kind::Ending);
  if (showArt) {
    const int artH = 78;
    if (contentTop + artH + 8 < weTop) {  // only if it doesn't crush the text
      renderer.drawRect(MARGIN, contentTop, contentW, artH, true);
      std::string tag = std::string("[art: ") + s.art + "]";
      renderer.drawCenteredText(SMALL_FONT, contentTop + (artH - renderer.getLineHeight(SMALL_FONT)) / 2,
                                tag.c_str(), true);
      bodyTop = contentTop + artH + 10;
    }
  }

  // ---- Body text -----------------------------------------------------------
  const int bodyLH = renderer.getLineHeight(BODY_FONT);
  const int avail = weTop - bodyTop - 6;
  const int maxLines = avail > 0 ? avail / bodyLH : 0;
  if (maxLines > 0) {
    // Preserve authored paragraph breaks: wrap each paragraph, cap total lines.
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

  // ---- Menu ----------------------------------------------------------------
  if (menuRows > 0) drawMenu(s, menuTop, footerY - 8);
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
  std::string sceneKey = std::string(kindName(s.kind)) + "|" +
                         std::to_string(static_cast<int>(game_->phase())) + "|" +
                         game_->state().current_node;
  const bool sceneChanged = firstRender_ || sceneKey != lastSceneKey_ || s.kind == gate::Screen::Kind::Beat;
  lastSceneKey_ = sceneKey;
  firstRender_ = false;

  drawScreen(s);
  renderer.displayBuffer(sceneChanged ? HalDisplay::FULL_REFRESH : HalDisplay::FAST_REFRESH);
}
