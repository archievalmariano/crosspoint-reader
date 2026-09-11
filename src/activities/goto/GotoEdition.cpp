#include "GotoEdition.h"

#include <ArduinoJson.h>
#include <Logging.h>

#include <utility>

// Compiled-in fixture edition (flash-resident). E0 source of truth: no SD read,
// no network. `dateline` is display-ready so the device never formats dates.
// Mirrors project-goto/fixtures/goto-2026-09-11.json.
static constexpr char kGotoBuiltinEditionJson[] = R"json({
  "edition": "GOTO",
  "date": "2026-09-11",
  "dateline": "Friday · 11 September",
  "pages": [
    {
      "section": "TOP STORY",
      "headline": "Marikina river hits second alarm as habagat rains stall over Luzon",
      "dek": "Rescuers pre-positioned overnight as the southwest monsoon dumped a month of rain on the capital in a day."
    },
    {
      "section": "NATION",
      "headline": "Senate panel advances bill to shield disaster relief funds from realignment",
      "dek": "The measure would ring-fence calamity budgets after audits flagged diverted allocations in three provinces."
    },
    {
      "section": "BUSINESS",
      "headline": "Peso steadies near 57 to the dollar after central bank holds rates",
      "dek": "Policymakers kept the benchmark unchanged, citing easing inflation and firmer remittance inflows."
    },
    {
      "section": "WORLD",
      "headline": "ASEAN ministers agree to fast-track regional disaster response network",
      "dek": "The pact sets up shared logistics hubs meant to move aid across member states within 48 hours."
    },
    {
      "section": "CULTURE",
      "headline": "Restored 1976 Filipino film to open the Cinemalaya retrospective",
      "dek": "Archivists spent two years rebuilding the print from surviving reels found in a provincial vault."
    },
    {
      "section": "ENTERTAINMENT",
      "headline": "Homegrown streaming series lands first international distribution deal",
      "dek": "The Cebu-shot drama will reach viewers in twelve countries starting next month, its producers said."
    }
  ]
})json";

bool parseGotoEdition(const char* json, GotoEdition& out) {
  JsonDocument doc;
  const DeserializationError err = deserializeJson(doc, json);
  if (err) {
    LOG_ERR("GOTO", "edition parse failed: %s", err.c_str());
    return false;
  }

  out.label = doc["edition"] | "GOTO";
  out.dateIso = doc["date"] | "";
  // dateline is optional; fall back to the raw ISO date when absent.
  out.dateline = doc["dateline"] | out.dateIso.c_str();

  const JsonArrayConst pages = doc["pages"].as<JsonArrayConst>();
  out.stories.clear();
  out.stories.reserve(pages.size());
  for (const JsonObjectConst page : pages) {
    GotoStory story;
    story.section = page["section"] | "";
    story.headline = page["headline"] | "";
    story.dek = page["dek"] | "";
    out.stories.push_back(std::move(story));
  }

  if (out.stories.empty()) {
    LOG_ERR("GOTO", "edition has no stories");
    return false;
  }
  return true;
}

bool loadBuiltinGotoEdition(GotoEdition& out) { return parseGotoEdition(kGotoBuiltinEditionJson, out); }
