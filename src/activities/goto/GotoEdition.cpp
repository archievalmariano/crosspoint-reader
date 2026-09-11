#include "GotoEdition.h"

#include <ArduinoJson.h>
#include <Logging.h>

#include <utility>

// Compiled-in fixture edition (flash-resident). E0.5 source of truth: no SD
// read, no network. Five morning slots (TOP STORY / WEATHER / WORLD / MONEY /
// FLEX->SPORTS). `dateline`, `publishedAt`, and `excerptParagraphs` are all
// display-ready so the device never formats, summarizes, or ranks.
// TOP STORY text is a real-world reference case; the other four are clearly
// fixture/demo content (not scraped). Mirrors fixtures/goto-2026-09-11.json.
static constexpr char kGotoBuiltinEditionJson[] = R"json({
  "edition": "GOTO",
  "date": "2026-09-11",
  "dateline": "Friday · 11 September",
  "pages": [
    {
      "section": "TOP STORY",
      "headline": "QC court postpones Sara Duterte's arraignment on grave threats",
      "excerptParagraphs": [
        "The Quezon City Regional Trial Court 98 on Friday postponed the arraignment of Vice President Sara Duterte on grave threat charges due to a motion for inhibition filed by her camp.",
        "She was accompanied by her brother Rep. Paolo Duterte when she arrived at the QC Hall of Justice at 8:30 a.m. They briefly greeted their supporters before heading to the court."
      ],
      "source": "GMA News",
      "publishedAt": "8:30 AM",
      "url": "https://www.gmanetwork.com/news/topstories/nation/sara-duterte-arraignment-postponed/story/"
    },
    {
      "section": "WEATHER",
      "headline": "Habagat, LPA to bring rain over much of Luzon and Visayas",
      "excerptParagraphs": [
        "PAGASA said the southwest monsoon, enhanced by a low pressure area east of Mindanao, will bring scattered rains and thunderstorms over Luzon and the Visayas on Friday.",
        "The state weather bureau urged residents in low-lying and mountainous areas to watch for possible flash floods and landslides during heavy downpours."
      ],
      "source": "GMA News",
      "publishedAt": "6:00 AM",
      "url": "https://www.gmanetwork.com/news/topstories/weather/habagat-lpa-rain-forecast/story/"
    },
    {
      "section": "WORLD",
      "headline": "ASEAN foreign ministers back faster regional disaster response",
      "excerptParagraphs": [
        "Foreign ministers of the Association of Southeast Asian Nations agreed to speed up a shared logistics network meant to move relief across member states within 48 hours of a disaster.",
        "The measure follows a season of severe flooding across the region and is expected to be finalized at the bloc's leaders' summit later this year."
      ],
      "source": "GMA News",
      "publishedAt": "5:45 AM",
      "url": "https://www.gmanetwork.com/news/world/asean-disaster-response-network/story/"
    },
    {
      "section": "MONEY",
      "headline": "Peso holds near 57 to the dollar after central bank keeps rates steady",
      "excerptParagraphs": [
        "The peso traded close to 57 against the US dollar on Friday after the Bangko Sentral ng Pilipinas kept its benchmark interest rate unchanged, citing easing inflation.",
        "Analysts said firmer remittance inflows ahead of the holiday season could lend the currency modest support in the coming weeks."
      ],
      "source": "GMA News",
      "publishedAt": "7:15 AM",
      "url": "https://www.gmanetwork.com/news/money/economy/peso-bsp-rates-hold/story/"
    },
    {
      "section": "SPORTS",
      "headline": "Gilas Pilipinas opens window with a win to stay unbeaten at home",
      "excerptParagraphs": [
        "Gilas Pilipinas leaned on a strong third quarter to pull away for a win in its opening game of the FIBA qualifying window before a packed home crowd on Thursday night.",
        "The national team next plays on the road, with the coaching staff expected to rotate in fresh legs for the back-to-back."
      ],
      "source": "GMA News",
      "publishedAt": "9:40 PM",
      "url": "https://www.gmanetwork.com/news/sports/basketball/gilas-window-opener-win/story/"
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
    story.source = page["source"] | "";
    story.publishedAt = page["publishedAt"] | "";
    story.url = page["url"] | "";

    const JsonArrayConst paragraphs = page["excerptParagraphs"].as<JsonArrayConst>();
    story.excerptParagraphs.reserve(paragraphs.size());
    for (const JsonVariantConst paragraph : paragraphs) {
      const char* text = paragraph | "";
      if (text[0] != '\0') story.excerptParagraphs.emplace_back(text);
    }

    out.stories.push_back(std::move(story));
  }

  if (out.stories.empty()) {
    LOG_ERR("GOTO", "edition has no stories");
    return false;
  }
  return true;
}

bool loadBuiltinGotoEdition(GotoEdition& out) { return parseGotoEdition(kGotoBuiltinEditionJson, out); }
