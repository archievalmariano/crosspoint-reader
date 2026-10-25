// The production OnPointActivity over the shipped catalogue and collections (Stage 2):
// direct launch, the route list, the SELECT COLLECTION row and picker, and persistence through
// OnPointStore. Collections: bulacan {BALAGTAS, CAYPOMBO}, north-luzon {CUBAO, TRINOMA->CLARK,
// CLARK->BAGUIO}, airport-transfers {CLARK->NAIA T3, NAIA T3->PITX}.

#include <ArduinoJson.h>

#include <cstring>

#include "OnPointActivityHarness.h"

namespace {

using namespace harness;
using on_point::ON_POINT_STORE_PATH;

class OnPointActivityTest : public ActivityTest {
 protected:
  // Leaves the directly launched board for the list.
  void enterList() {
    enter();
    if (inDepartureView()) send(Event::Released, Button::Back);
    ASSERT_FALSE(inDepartureView());
  }
  std::string storedField(const char* key, const char* nested = nullptr) {
    JsonDocument doc;
    if (storage.files.count(ON_POINT_STORE_PATH) == 0 || deserializeJson(doc, storage.files[ON_POINT_STORE_PATH]))
      return "<none>";
    const char* value = nested ? doc[key][nested].as<const char*>() : doc[key].as<const char*>();
    return value ? value : "<null>";
  }
};

// ---- Direct launch and Back ----

TEST_F(OnPointActivityTest, LaunchOpensTheDefaultRouteBoardThenBackToTheListThenApps) {
  enter();
  EXPECT_TRUE(inDepartureView());
  EXPECT_EQ(textAt(ROUTE_ORIGIN_Y), "BALAGTAS");
  send(Event::Released, Button::Back);
  EXPECT_FALSE(inDepartureView());
  EXPECT_EQ(textAt(HEADING_Y), "STR_ON_POINT_MAIN_ROUTE");
  EXPECT_EQ(textAt(PINNED_ORIGIN_Y), "BALAGTAS");
  EXPECT_TRUE(pinnedSelected());
  EXPECT_EQ(textAt(ROW_Y[0] + 8), "CAYPOMBO");
  EXPECT_TRUE(drewAt("STR_ON_POINT_REGION_BULACAN", COLLECTION_TEXT_Y));
  EXPECT_EQ(activityManager.goToAppsCalls, 0);
  send(Event::Released, Button::Back);
  EXPECT_EQ(activityManager.goToAppsCalls, 1);
  EXPECT_EQ(activityManager.lastFocus, AppId::OnPoint);
  EXPECT_EQ(storage.mutations, 0);  // nothing written by launching or browsing
  EXPECT_EQ(SETTINGS.saves, 0);
}

TEST_F(OnPointActivityTest, LegacyMainRouteMigratesIntoItsCollection) {
  strcpy(SETTINGS.onPointMainRouteId, "p2p-trinoma-clark-airport");
  enter();
  EXPECT_EQ(textAt(ROUTE_ORIGIN_Y), "TRINOMA");
  send(Event::Released, Button::Back);
  EXPECT_TRUE(drewAt("STR_ON_POINT_REGION_NORTH_LUZON", COLLECTION_TEXT_Y));
  EXPECT_EQ(textAt(PINNED_ORIGIN_Y), "TRINOMA");
  EXPECT_EQ(storage.mutations, 0);
}

TEST_F(OnPointActivityTest, NoValidDefaultOpensTheListInTheNoMainState) {
  for (const char* legacy : {"p2p-retired-route", "P2P-BALAGTAS-TRINOMA", ""}) {
    SetUp();
    strcpy(SETTINGS.onPointMainRouteId, legacy);
    enter();
    EXPECT_FALSE(inDepartureView()) << legacy;
    EXPECT_EQ(textAt(HEADING_Y), "STR_ON_POINT_NO_MAIN_ROUTE") << legacy;
    EXPECT_EQ(textAt(PINNED_ORIGIN_Y), "BALAGTAS");  // first in sort order
    EXPECT_EQ(textAt(FOOTER_Y), "STR_ON_POINT_HOLD_SET_MAIN");
    send(Event::Released, Button::Back);
    EXPECT_EQ(activityManager.goToAppsCalls, 1);
    EXPECT_STREQ(SETTINGS.onPointMainRouteId, legacy);
    EXPECT_EQ(storage.mutations, 0);
  }
}

TEST_F(OnPointActivityTest, UnsetClockStillOpensTheBoardWithTheClockScreen) {
  clockReady = false;
  enter();
  EXPECT_TRUE(inDepartureView());
  EXPECT_TRUE(drew("STR_ON_POINT_NOT_SET"));
  EXPECT_TRUE(drew("STR_ON_POINT_NO_COUNTDOWN"));
}

TEST_F(OnPointActivityTest, BoardIsUnchangedByCollections) {
  enter();
  EXPECT_FALSE(collectionRowShown());
  EXPECT_FALSE(drew("STR_ON_POINT_NOT_SAVED"));
  EXPECT_TRUE(drewAt("2026-10-21", FOOTER_Y));  // the board footer: date and source
  EXPECT_TRUE(drew("STR_ON_POINT_PUBLISHED"));
  send(Event::Pressed, Button::NavNext);  // reverse direction
  EXPECT_EQ(textAt(ROUTE_ORIGIN_Y), "TRINOMA");
}

// ---- Stored preferences ----

TEST_F(OnPointActivityTest, StoredCollectionAndDefaultsDriveLaunch) {
  storage.files[ON_POINT_STORE_PATH] =
      R"({"version":1,"activeRegion":"airport-transfers","defaults":{"airport-transfers":"p2p-naia-pitx"},)"
      R"("legacyBaseline":"p2p-balagtas-trinoma"})";
  enter();
  EXPECT_EQ(textAt(ROUTE_ORIGIN_Y), "NAIA T3");
  send(Event::Released, Button::Back);
  EXPECT_TRUE(drewAt("STR_ON_POINT_REGION_AIRPORT_TRANSFERS", COLLECTION_TEXT_Y));
  EXPECT_EQ(textAt(PINNED_ORIGIN_Y), "NAIA T3");
  EXPECT_EQ(storage.mutations, 0);
}

TEST_F(OnPointActivityTest, InvalidStoredIdsAreHandledWithoutRewriting) {
  // Unknown active collection: the first is shown. A default from another collection: no main.
  storage.files[ON_POINT_STORE_PATH] = R"({"version":1,"activeRegion":"rizal","defaults":{"bulacan":"p2p-naia-pitx"},)"
                                       R"("legacyBaseline":"p2p-balagtas-trinoma"})";
  const std::string card = storage.files[ON_POINT_STORE_PATH];
  enter();
  EXPECT_FALSE(inDepartureView());
  EXPECT_TRUE(drewAt("STR_ON_POINT_REGION_BULACAN", COLLECTION_TEXT_Y));
  EXPECT_EQ(textAt(HEADING_Y), "STR_ON_POINT_NO_MAIN_ROUTE");
  EXPECT_EQ(storage.files[ON_POINT_STORE_PATH], card);
  EXPECT_EQ(storage.mutations, 0);
}

// ---- Route list: Stage 1 behaviour within a collection ----

TEST_F(OnPointActivityTest, HoldingConfirmSetsThisCollectionsMainRouteInTheStore) {
  enterList();
  send(Event::Pressed, Button::NavNext);  // CAYPOMBO
  send(Event::LongPressed, Button::Confirm);
  EXPECT_EQ(textAt(PINNED_ORIGIN_Y), "CAYPOMBO");
  EXPECT_TRUE(pinnedSelected());
  EXPECT_EQ(textAt(FOOTER_Y), "STR_ON_POINT_MAIN_ROUTE");
  EXPECT_EQ(storedField("defaults", "bulacan"), "p2p-caypombo-sm-north-edsa");
  EXPECT_EQ(SETTINGS.saves, 0);  // stock settings.json is never written
  EXPECT_STREQ(SETTINGS.onPointMainRouteId, "p2p-balagtas-trinoma");
  const int mutations = storage.mutations;
  send(Event::LongPressed, Button::Confirm);  // already the main route
  EXPECT_EQ(storage.mutations, mutations);

  enter();  // the next launch opens the new main route
  EXPECT_EQ(textAt(ROUTE_ORIGIN_Y), "CAYPOMBO");
}

TEST_F(OnPointActivityTest, TouchSelectsOpensAndSetsWithGapsIgnored) {
  strcpy(SETTINGS.onPointMainRouteId, "p2p-cubao-clark-airport");
  enterList();                              // north-luzon: CUBAO pinned; rows CLARK->BAGUIO, TRINOMA
  touch(Event::ScreenTouchDown, 200, 330);  // row 0
  EXPECT_EQ(selectedRow(), 0);
  EXPECT_FALSE(inDepartureView());
  touch(Event::ScreenTap, 200, 430);  // gap between rows 0 and 1
  EXPECT_FALSE(inDepartureView());
  touch(Event::ScreenTap, -5, 330);  // off screen
  EXPECT_FALSE(inDepartureView());
  touch(Event::ScreenLongPress, 200, 500);  // row 1 becomes the main route
  EXPECT_EQ(storedField("defaults", "north-luzon"), "p2p-trinoma-clark-airport");
  touch(Event::ScreenTap, 200, 150);  // pinned card opens it
  EXPECT_TRUE(inDepartureView());
  EXPECT_EQ(textAt(ROUTE_ORIGIN_Y), "TRINOMA");
}

TEST_F(OnPointActivityTest, InputWaitsWhileARenderIsInProgressExceptBack) {
  enterList();
  RenderLock::peekResult = true;
  send(Event::Pressed, Button::NavNext);
  EXPECT_TRUE(pinnedSelected());
  send(Event::LongPressed, Button::Confirm);
  send(Event::Released, Button::Confirm);
  EXPECT_EQ(storage.mutations, 0);
  EXPECT_FALSE(inPicker());
  send(Event::Released, Button::Back);
  EXPECT_EQ(activityManager.goToAppsCalls, 1);
}

// ---- The SELECT COLLECTION row ----

TEST_F(OnPointActivityTest, ButtonsCycleThroughTheRoutesAndTheCollectionRow) {
  enterList();  // bulacan: 0 = BALAGTAS (pinned), 1 = CAYPOMBO
  EXPECT_TRUE(collectionRowShown());
  EXPECT_FALSE(collectionRowSelected());
  send(Event::Pressed, Button::NavNext);
  EXPECT_EQ(selectedRow(), 0);
  send(Event::Pressed, Button::NavNext);  // last route -> the row
  EXPECT_TRUE(collectionRowSelected());
  EXPECT_EQ(selectedRow(), -1);
  EXPECT_FALSE(pinnedSelected());
  EXPECT_EQ(textAt(FOOTER_Y), "STR_ON_POINT_CHOOSE_COLLECTION");
  send(Event::Pressed, Button::NavNext);  // the row -> position 1
  EXPECT_EQ(selectedRow(), 0);
  EXPECT_FALSE(collectionRowSelected());
  send(Event::Pressed, Button::NavPrevious);  // position 1 -> the row
  EXPECT_TRUE(collectionRowSelected());
  send(Event::Pressed, Button::NavPrevious);  // the row -> the last route
  EXPECT_EQ(selectedRow(), 0);
  send(Event::LongPressed, Button::Confirm);  // hold-to-set unchanged
  EXPECT_EQ(textAt(PINNED_ORIGIN_Y), "CAYPOMBO");
}

TEST_F(OnPointActivityTest, NavPreviousFromThePinnedRouteStillGoesToTheLastRoute) {
  strcpy(SETTINGS.onPointMainRouteId, "p2p-cubao-clark-airport");
  enterList();  // north-luzon: CUBAO pinned, then CLARK (BAGUIO), TRINOMA
  send(Event::Pressed, Button::NavPrevious);
  EXPECT_EQ(selectedRow(), 1);
  EXPECT_FALSE(collectionRowSelected());
}

TEST_F(OnPointActivityTest, HoldingOnTheCollectionRowDoesNothing) {
  enterList();
  send(Event::Pressed, Button::NavPrevious);  // pinned -> last (CAYPOMBO)
  send(Event::Pressed, Button::NavPrevious);  // -> the row
  ASSERT_TRUE(collectionRowSelected());
  send(Event::LongPressed, Button::Confirm);
  EXPECT_EQ(storage.mutations, 0);
  EXPECT_EQ(textAt(PINNED_ORIGIN_Y), "BALAGTAS");
  touch(Event::ScreenLongPress, 200, COLLECTION_ROW_Y + 20);
  EXPECT_EQ(storage.mutations, 0);
  EXPECT_FALSE(inPicker());
}

TEST_F(OnPointActivityTest, ConfirmOnTheRowOpensThePickerAndBackRestoresTheList) {
  enterList();
  send(Event::Pressed, Button::NavNext);  // CAYPOMBO
  send(Event::Pressed, Button::NavNext);  // the row
  send(Event::Released, Button::Confirm);
  ASSERT_TRUE(inPicker());
  EXPECT_TRUE(drewAt("STR_ON_POINT_REGION_BULACAN", PICKER_ROW_Y[0] + 8));
  EXPECT_TRUE(drewAt("STR_ON_POINT_REGION_NORTH_LUZON", PICKER_ROW_Y[1] + 8));
  EXPECT_TRUE(drewAt("STR_ON_POINT_REGION_AIRPORT_TRANSFERS", PICKER_ROW_Y[2] + 8));
  EXPECT_TRUE(drewAt("2 STR_ON_POINT_ROUTE_COUNT_MANY", PICKER_ROW_Y[0] + 64));
  EXPECT_TRUE(drewAt("3 STR_ON_POINT_ROUTE_COUNT_MANY", PICKER_ROW_Y[1] + 64));
  EXPECT_TRUE(drewAt("STR_ON_POINT_CURRENT_COLLECTION", PICKER_ROW_Y[0] + 64));
  EXPECT_EQ(selectedPickerRow(), 0);
  EXPECT_EQ(textAt(FOOTER_Y), "STR_ON_POINT_SELECT_COLLECTION");
  send(Event::Pressed, Button::NavNext);  // move without selecting
  EXPECT_EQ(selectedPickerRow(), 1);

  send(Event::Released, Button::Back);  // cancel
  EXPECT_FALSE(inPicker());
  EXPECT_TRUE(collectionRowSelected());
  EXPECT_TRUE(drewAt("STR_ON_POINT_REGION_BULACAN", COLLECTION_TEXT_Y));
  EXPECT_EQ(textAt(ROW_Y[0] + 8), "CAYPOMBO");  // same page
  send(Event::Pressed, Button::NavPrevious);    // the row -> last route: the selection is intact
  EXPECT_EQ(selectedRow(), 0);
  EXPECT_EQ(storage.mutations, 0);
}

TEST_F(OnPointActivityTest, PickerWrapsAndSelectsWithButtons) {
  enterList();
  send(Event::Pressed, Button::NavPrevious);
  send(Event::Pressed, Button::NavPrevious);  // the row
  send(Event::Released, Button::Confirm);
  send(Event::Pressed, Button::NavPrevious);  // wraps to AIRPORT TRANSFERS
  EXPECT_EQ(selectedPickerRow(), 2);
  send(Event::Released, Button::Confirm);
  EXPECT_FALSE(inPicker());
  EXPECT_TRUE(drewAt("STR_ON_POINT_REGION_AIRPORT_TRANSFERS", COLLECTION_TEXT_Y));
  EXPECT_EQ(textAt(PINNED_ORIGIN_Y), "CLARK AIRPORT");  // its shipped default, pinned and selected
  EXPECT_TRUE(pinnedSelected());
  EXPECT_FALSE(collectionRowSelected());
  EXPECT_EQ(storedField("activeRegion"), "airport-transfers");

  enter();  // the next launch opens that collection's default board
  EXPECT_EQ(textAt(ROUTE_ORIGIN_Y), "CLARK AIRPORT");
}

TEST_F(OnPointActivityTest, TouchOpensThePickerAndSelectsACollection) {
  enterList();
  touch(Event::ScreenTouchDown, 200, COLLECTION_ROW_Y + 20);
  EXPECT_TRUE(collectionRowSelected());
  EXPECT_FALSE(inPicker());
  touch(Event::ScreenTap, 200, COLLECTION_ROW_Y + 20);
  ASSERT_TRUE(inPicker());
  touch(Event::ScreenTouchDown, 200, PICKER_ROW_Y[1] + 30);
  EXPECT_EQ(selectedPickerRow(), 1);
  touch(Event::ScreenTap, 200, PICKER_ROW_Y[1] + 104 + 10);  // the gap below row 1
  EXPECT_TRUE(inPicker());
  touch(Event::ScreenTap, 200, PICKER_ROW_Y[3] + 30);  // no fourth collection
  EXPECT_TRUE(inPicker());
  swipe(SwipeDir::Up);  // one page only
  EXPECT_EQ(selectedPickerRow(), 1);
  touch(Event::ScreenTap, 200, PICKER_ROW_Y[1] + 30);
  EXPECT_FALSE(inPicker());
  EXPECT_TRUE(drewAt("STR_ON_POINT_REGION_NORTH_LUZON", COLLECTION_TEXT_Y));
  EXPECT_EQ(textAt(PINNED_ORIGIN_Y), "CUBAO");
}

TEST_F(OnPointActivityTest, SelectingTheCurrentCollectionChangesNothing) {
  enterList();
  touch(Event::ScreenTap, 200, COLLECTION_ROW_Y + 20);
  send(Event::Released, Button::Confirm);  // BULACAN again
  EXPECT_FALSE(inPicker());
  EXPECT_TRUE(collectionRowSelected());
  EXPECT_EQ(storage.mutations, 0);
}

TEST_F(OnPointActivityTest, DefaultsAreKeptPerCollection) {
  enterList();
  send(Event::Pressed, Button::NavNext);
  send(Event::LongPressed, Button::Confirm);  // bulacan: CAYPOMBO
  touch(Event::ScreenTap, 200, COLLECTION_ROW_Y + 20);
  touch(Event::ScreenTap, 200, PICKER_ROW_Y[1] + 30);  // north-luzon: shipped default CUBAO
  EXPECT_EQ(textAt(PINNED_ORIGIN_Y), "CUBAO");
  send(Event::Pressed, Button::NavPrevious);  // TRINOMA
  send(Event::LongPressed, Button::Confirm);
  EXPECT_EQ(textAt(PINNED_ORIGIN_Y), "TRINOMA");
  touch(Event::ScreenTap, 200, COLLECTION_ROW_Y + 20);
  touch(Event::ScreenTap, 200, PICKER_ROW_Y[0] + 30);  // back to bulacan
  EXPECT_EQ(textAt(PINNED_ORIGIN_Y), "CAYPOMBO");
  EXPECT_EQ(storedField("defaults", "bulacan"), "p2p-caypombo-sm-north-edsa");
  EXPECT_EQ(storedField("defaults", "north-luzon"), "p2p-trinoma-clark-airport");
  EXPECT_EQ(storedField("activeRegion"), "bulacan");
  EXPECT_EQ(SETTINGS.saves, 0);
}

TEST_F(OnPointActivityTest, SwipeFromTheRowReturnsFocusToARoute) {
  enterList();
  send(Event::Pressed, Button::NavPrevious);
  send(Event::Pressed, Button::NavPrevious);
  ASSERT_TRUE(collectionRowSelected());
  swipe(SwipeDir::Up);  // one page: no target, focus stays
  EXPECT_TRUE(collectionRowSelected());
}

// ---- NOT SAVED ----

TEST_F(OnPointActivityTest, NewerSchemaFileShowsNotSavedAndIsNeverWritten) {
  storage.files[ON_POINT_STORE_PATH] = R"({"version":2,"activeRegion":"bulacan"})";
  const std::string card = storage.files[ON_POINT_STORE_PATH];
  enterList();
  EXPECT_EQ(textAt(FOOTER_Y), "STR_ON_POINT_MAIN_ROUTE");  // nothing changed yet: no notice
  send(Event::Pressed, Button::NavNext);
  send(Event::LongPressed, Button::Confirm);
  EXPECT_EQ(textAt(PINNED_ORIGIN_Y), "CAYPOMBO");         // the session follows the user
  EXPECT_EQ(textAt(FOOTER_Y), "STR_ON_POINT_NOT_SAVED");  // never MAIN ROUTE for an unsaved choice
  touch(Event::ScreenTap, 200, COLLECTION_ROW_Y + 20);
  EXPECT_EQ(textAt(FOOTER_Y), "STR_ON_POINT_NOT_SAVED");
  send(Event::Released, Button::Back);
  EXPECT_EQ(storage.mutations, 0);
  EXPECT_EQ(storage.files[ON_POINT_STORE_PATH], card);
}

TEST_F(OnPointActivityTest, FailedSaveShowsNotSavedUntilASaveSucceeds) {
  enterList();
  storage.failAt = storage.mutations;  // the next tmp write
  send(Event::Pressed, Button::NavNext);
  send(Event::LongPressed, Button::Confirm);
  EXPECT_EQ(textAt(PINNED_ORIGIN_Y), "CAYPOMBO");
  EXPECT_EQ(textAt(FOOTER_Y), "STR_ON_POINT_NOT_SAVED");
  storage.clearFaults();
  touch(Event::ScreenTap, 200, COLLECTION_ROW_Y + 20);
  touch(Event::ScreenTap, 200, PICKER_ROW_Y[1] + 30);  // a successful save covers both changes
  EXPECT_NE(textAt(FOOTER_Y), "STR_ON_POINT_NOT_SAVED");
  EXPECT_EQ(storedField("defaults", "bulacan"), "p2p-caypombo-sm-north-edsa");
}

}  // namespace
