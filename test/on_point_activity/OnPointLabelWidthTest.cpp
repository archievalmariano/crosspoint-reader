// The English collection labels fit their places, measured with the real built-in fonts
// (LABEL_FONT = Ubuntu 10 bold, ROUTE_FONT = Noto Sans 16 bold) and the strings in
// lib/I18n/translations/english.yaml, over a 480 px portrait width (OnPointActivity.cpp).

#include <gtest/gtest.h>

#include <algorithm>
#include <fstream>
#include <map>
#include <string>

#include "EpdFont.h"
#include "builtinFonts/notosans_14_bold.h"
#include "builtinFonts/notosans_16_bold.h"
#include "builtinFonts/ubuntu_10_bold.h"

namespace {

constexpr int SCREEN_WIDTH = 480;
constexpr int MARGIN = 24;
constexpr int CONTENT_X = MARGIN + 24;  // after the selection bar
constexpr int LABEL_GAP = 16;
constexpr int CHEVRON_WIDTH = 10;
constexpr int CHEVRON_GAP = 12;

std::map<std::string, std::string> englishStrings() {
  std::map<std::string, std::string> strings;
  std::ifstream yaml(ON_POINT_ENGLISH_YAML);
  std::string line;
  while (std::getline(yaml, line)) {
    const size_t colon = line.find(": \"");
    if (line.rfind("STR_", 0) != 0 || colon == std::string::npos || line.back() != '"') continue;
    strings[line.substr(0, colon)] = line.substr(colon + 3, line.size() - colon - 4);
  }
  return strings;
}

int width(const EpdFontData& data, const std::string& text) {
  const EpdFont font(&data);
  int w = 0, h = 0;
  font.getTextDimensions(text.c_str(), &w, &h);
  return w;
}

const std::map<std::string, std::string>& strings() {
  static const auto s = englishStrings();
  return s;
}

const std::string& english(const char* key) {
  const auto it = strings().find(key);
  EXPECT_NE(it, strings().end()) << key << " missing from english.yaml";
  static const std::string empty;
  return it == strings().end() ? empty : it->second;
}

std::vector<std::string> collectionNameKeys() {
  std::vector<std::string> keys;
  for (const auto& [key, _] : strings())
    if (key.rfind("STR_ON_POINT_REGION_", 0) == 0) keys.push_back(key);
  return keys;
}

TEST(OnPointLabelWidth, EveryShippedCollectionHasAName) {
  for (const char* key :
       {"STR_ON_POINT_REGION_BULACAN", "STR_ON_POINT_REGION_NORTH_LUZON", "STR_ON_POINT_REGION_AIRPORT_TRANSFERS"})
    EXPECT_FALSE(english(key).empty()) << key;
  EXPECT_EQ(collectionNameKeys().size(), 3u);  // no names for unshipped (future) collections
}

TEST(OnPointLabelWidth, CollectionNamesFitTheRow) {
  const int label = width(ubuntu_10_bold, english("STR_ON_POINT_COLLECTION"));
  const int budget = SCREEN_WIDTH - MARGIN - CHEVRON_WIDTH - CHEVRON_GAP - (CONTENT_X + label + LABEL_GAP);
  for (const std::string& key : collectionNameKeys()) {
    const int name = width(ubuntu_10_bold, strings().at(key));
    EXPECT_LE(name, budget) << key << " \"" << strings().at(key) << "\": " << name << " px of " << budget;
  }
}

TEST(OnPointLabelWidth, CollectionNamesFitThePicker) {
  const int budget = SCREEN_WIDTH - MARGIN - CONTENT_X;
  for (const std::string& key : collectionNameKeys()) {
    const int name = width(notosans_16_bold, strings().at(key));
    EXPECT_LE(name, budget) << key << ": " << name << " px of " << budget;
  }
  // The route count and CURRENT share the second line.
  const int count = width(ubuntu_10_bold, "99 " + english("STR_ON_POINT_ROUTE_COUNT_MANY"));
  const int current = width(ubuntu_10_bold, english("STR_ON_POINT_CURRENT_COLLECTION"));
  EXPECT_LE(count + LABEL_GAP + current, budget);
}

TEST(OnPointLabelWidth, FooterLabelsFitBesideTheSourceLabel) {
  const int source = std::max(width(ubuntu_10_bold, english("STR_ON_POINT_PUBLISHED")),
                              width(ubuntu_10_bold, english("STR_ON_POINT_PROVISIONAL")));
  const int budget = SCREEN_WIDTH - 2 * MARGIN - source - LABEL_GAP;
  for (const char* key : {"STR_ON_POINT_NOT_SAVED", "STR_ON_POINT_CHOOSE_COLLECTION", "STR_ON_POINT_SELECT_COLLECTION",
                          "STR_ON_POINT_MAIN_ROUTE", "STR_ON_POINT_HOLD_SET_MAIN"})
    EXPECT_LE(width(ubuntu_10_bold, english(key)), budget) << key;
}

TEST(OnPointLabelWidth, NoticeFitsTheList) {
  // Drawn in FOLLOWING_FONT (Noto Sans 14 bold), like TIMETABLE UNAVAILABLE.
  for (const char* key : {"STR_ON_POINT_NO_ROUTES_IN_COLLECTION", "STR_ON_POINT_TIMETABLE_UNAVAILABLE"}) {
    const int w = width(notosans_14_bold, english(key));
    EXPECT_LE(w, SCREEN_WIDTH - 2 * MARGIN) << key << ": " << w;
  }
}

}  // namespace
