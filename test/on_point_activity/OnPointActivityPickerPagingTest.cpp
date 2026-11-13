// The production OnPointActivity's collection picker with more collections than fit one page
// (TestRegions.cpp, ON_POINT_TEST_PICKER_COLLECTIONS = 5, 6, 8 or 9): the page label, the second and
// later pages, empty rows, wrapping, touch, swipes and reopening on the active collection's page.

#include <string>

#include "OnPointActivityHarness.h"

namespace {

using namespace harness;
using on_point::ON_POINT_STORE_PATH;

constexpr int N = ON_POINT_TEST_PICKER_COLLECTIONS;
constexpr int ROWS = 4;  // PICKER_VISIBLE_ROWS
constexpr int PAGES = (N + ROWS - 1) / ROWS;
constexpr int PAGE_LABEL_Y = 76;    // MAIN_ROUTE_LABEL_Y
constexpr int ROW_STEP = 104 + 22;  // ROW_HEIGHT + ROW_GAP

std::string label(const int collection) { return "STR_ON_POINT_COLLECTION " + std::to_string(collection); }
std::string routeCount(const int routes) {
  return std::to_string(routes) + (routes == 1 ? " STR_ON_POINT_ROUTE_COUNT_ONE" : " STR_ON_POINT_ROUTE_COUNT_MANY");
}
std::string pageLabel(const int page) { return std::to_string(page) + " / " + std::to_string(PAGES); }

class OnPointActivityPickerPagingTest : public ActivityTest {
 protected:
  void enterList() {
    enter();
    if (inDepartureView()) send(Event::Released, Button::Back);
    ASSERT_FALSE(inDepartureView());
  }
  void openPicker() {
    enterList();
    touch(Event::ScreenTap, 200, COLLECTION_ROW_Y + 20);
    ASSERT_TRUE(inPicker());
  }
  void storeActive(const char* id) {
    storage.files[ON_POINT_STORE_PATH] = std::string(R"({"version":1,"activeRegion":")") + id + R"("})";
  }
  bool stored(const std::string& fragment) {
    return storage.files.count(ON_POINT_STORE_PATH) != 0 &&
           storage.files[ON_POINT_STORE_PATH].find(fragment) != std::string::npos;
  }
  // Collection `first + row` (1-based numbering) is drawn on `row` with its route count.
  void expectPage(const int page) {
    EXPECT_TRUE(drewAt(pageLabel(page), PAGE_LABEL_Y)) << "page " << page;
    const int first = (page - 1) * ROWS;
    for (int row = 0; row < ROWS; ++row) {
      const int collection = first + row + 1;
      if (collection <= N) {
        EXPECT_TRUE(drewAt(label(collection), PICKER_ROW_Y[row] + 8)) << "row " << row;
        EXPECT_TRUE(drewAt(routeCount(collection), PICKER_ROW_Y[row] + 64)) << "row " << row;
      } else {
        EXPECT_EQ(textAt(PICKER_ROW_Y[row] + 8), "") << "empty row " << row;
        EXPECT_EQ(textAt(PICKER_ROW_Y[row] + 64), "") << "empty row " << row;
      }
    }
  }
};

// ---- Every configuration ----

TEST_F(OnPointActivityPickerPagingTest, OpensOnTheFirstPageWithAPageLabel) {
  openPicker();  // active collection c1
  expectPage(1);
  EXPECT_EQ(selectedPickerRow(), 0);
  EXPECT_TRUE(drewAt("STR_ON_POINT_CURRENT_COLLECTION", PICKER_ROW_Y[0] + 64));
}

TEST_F(OnPointActivityPickerPagingTest, ButtonsVisitEveryCollectionAndWrapAcrossPages) {
  openPicker();
  for (int step = 1; step <= N; ++step) {
    send(Event::Pressed, Button::NavNext);
    const int cursor = step % N;  // the last step wraps to c1
    SCOPED_TRACE(cursor);
    EXPECT_TRUE(drewAt(pageLabel(cursor / ROWS + 1), PAGE_LABEL_Y));
    EXPECT_EQ(selectedPickerRow(), cursor % ROWS);
  }
  send(Event::Pressed, Button::NavPrevious);  // c1 wraps back to the last collection, on the last page
  expectPage(PAGES);
  EXPECT_EQ(selectedPickerRow(), (N - 1) % ROWS);
  EXPECT_EQ(storage.mutations, 0);
}

TEST_F(OnPointActivityPickerPagingTest, SwipesPageWithoutWrapping) {
  openPicker();
  swipe(SwipeDir::Down);  // first page: nothing earlier
  expectPage(1);
  for (int page = 2; page <= PAGES; ++page) {
    swipe(SwipeDir::Up);
    expectPage(page);
    EXPECT_EQ(selectedPickerRow(), 0);  // the page's first row
  }
  swipe(SwipeDir::Up);  // last page: nothing later
  expectPage(PAGES);
  swipe(SwipeDir::Down);
  expectPage(PAGES - 1);
  EXPECT_EQ(selectedPickerRow(), 0);
}

TEST_F(OnPointActivityPickerPagingTest, SwipesLandOnTheFirstRowWhateverRowWasSelected) {
  openPicker();
  send(Event::Pressed, Button::NavNext);
  send(Event::Pressed, Button::NavNext);  // c3, row 2
  swipe(SwipeDir::Up);
  expectPage(2);
  EXPECT_EQ(selectedPickerRow(), 0);  // c5, not the row below c3's position
  if (N >= 6) {
    send(Event::Pressed, Button::NavNext);  // c6, row 1
    swipe(SwipeDir::Down);
    expectPage(1);
    EXPECT_EQ(selectedPickerRow(), 0);  // c1, not c2
  }
}

TEST_F(OnPointActivityPickerPagingTest, LastPageTouchSelectsOnlyDrawnRows) {
  openPicker();
  for (int page = 2; page <= PAGES; ++page) swipe(SwipeDir::Up);
  const int onPage = N - (PAGES - 1) * ROWS;  // collections on the last page
  for (int row = onPage; row < ROWS; ++row) {
    touch(Event::ScreenTouchDown, 200, PICKER_ROW_Y[row] + 30);
    touch(Event::ScreenTap, 200, PICKER_ROW_Y[row] + 30);
    EXPECT_TRUE(inPicker()) << "empty row " << row;
    EXPECT_EQ(selectedPickerRow(), 0) << "empty row " << row;
  }
  touch(Event::ScreenTap, 200, PICKER_ROW_Y[0] + 104 + 10);  // the gap below the first row
  EXPECT_TRUE(inPicker());
  touch(Event::ScreenTap, 200, PAGE_LABEL_Y + 4);  // the page label is not a row
  EXPECT_TRUE(inPicker());
  EXPECT_EQ(storage.mutations, 0);
  touch(Event::ScreenTap, 200, PICKER_ROW_Y[0] + 30);
  EXPECT_FALSE(inPicker());
  EXPECT_TRUE(drewAt(label((PAGES - 1) * ROWS + 1), COLLECTION_TEXT_Y));
  EXPECT_TRUE(stored(R"("activeRegion":"c)" + std::to_string((PAGES - 1) * ROWS + 1) + "\""));
}

TEST_F(OnPointActivityPickerPagingTest, FirstPageTouchNeverReachesALaterPage) {
  openPicker();
  touch(Event::ScreenTouchDown, 200, PICKER_ROW_Y[3] + 30);
  EXPECT_EQ(selectedPickerRow(), 3);
  expectPage(1);
  touch(Event::ScreenTap, 200, PICKER_ROW_Y[3] + 30);
  EXPECT_FALSE(inPicker());
  EXPECT_TRUE(stored(R"("activeRegion":"c4")"));
}

TEST_F(OnPointActivityPickerPagingTest, CancelReopensOnTheActiveCollectionsPage) {
  openPicker();  // active c1
  swipe(SwipeDir::Up);
  send(Event::Released, Button::Back);  // cancel
  EXPECT_FALSE(inPicker());
  EXPECT_TRUE(collectionRowSelected());
  touch(Event::ScreenTap, 200, COLLECTION_ROW_Y + 20);
  expectPage(1);
  EXPECT_EQ(selectedPickerRow(), 0);
  EXPECT_EQ(storage.mutations, 0);
}

TEST_F(OnPointActivityPickerPagingTest, UnknownStoredCollectionOpensOnTheFirstPage) {
  storeActive("rizal");
  const std::string card = storage.files[ON_POINT_STORE_PATH];
  openPicker();
  expectPage(1);
  EXPECT_EQ(selectedPickerRow(), 0);
  EXPECT_EQ(storage.files[ON_POINT_STORE_PATH], card);  // never rewritten
}

// ---- A collection on the last page is active ----

class LastPageActive : public OnPointActivityPickerPagingTest {
 protected:
  static constexpr int LAST_PAGE_FIRST = (PAGES - 1) * ROWS + 1;  // its first collection
  std::string id() const { return "c" + std::to_string(LAST_PAGE_FIRST); }
  void SetUp() override {
    OnPointActivityPickerPagingTest::SetUp();
    storeActive(id().c_str());
  }
};

TEST_F(LastPageActive, LaunchAndPickerOpenOnItsPage) {
  enter();
  EXPECT_TRUE(inDepartureView());  // its default board
  EXPECT_EQ(textAt(ROUTE_ORIGIN_Y), "BALAGTAS");
  send(Event::Released, Button::Back);
  EXPECT_TRUE(drewAt(label(LAST_PAGE_FIRST), COLLECTION_TEXT_Y));
  touch(Event::ScreenTap, 200, COLLECTION_ROW_Y + 20);
  ASSERT_TRUE(inPicker());
  expectPage(PAGES);
  EXPECT_EQ(selectedPickerRow(), 0);
  EXPECT_TRUE(drewAt("STR_ON_POINT_CURRENT_COLLECTION", PICKER_ROW_Y[0] + 64));
}

TEST_F(LastPageActive, BrowsingAwayThenCancellingReturnsToItsPage) {
  openPicker();
  send(Event::Pressed, Button::NavNext);  // wraps or moves off the active row
  send(Event::Pressed, Button::NavNext);
  send(Event::Released, Button::Back);
  touch(Event::ScreenTap, 200, COLLECTION_ROW_Y + 20);
  expectPage(PAGES);
  EXPECT_EQ(selectedPickerRow(), 0);
  EXPECT_EQ(storage.mutations, 0);
}

TEST_F(LastPageActive, SelectingItAgainChangesNothing) {
  openPicker();
  send(Event::Released, Button::Confirm);
  EXPECT_FALSE(inPicker());
  EXPECT_EQ(storage.mutations, 0);
}

TEST_F(LastPageActive, ItsMainRouteIsKeptAcrossRelaunch) {
  enterList();  // members c1..cN share routes 0..; pinned BALAGTAS, then CAYPOMBO
  send(Event::Pressed, Button::NavNext);
  send(Event::LongPressed, Button::Confirm);
  EXPECT_TRUE(stored("\"" + id() + R"(":"p2p-caypombo-sm-north-edsa")"));
  enter();
  EXPECT_EQ(textAt(ROUTE_ORIGIN_Y), "CAYPOMBO");
}

TEST_F(LastPageActive, SelectingAFirstPageCollectionThenRelaunchingOpensItsPage) {
  openPicker();
  for (int page = PAGES; page > 1; --page) swipe(SwipeDir::Down);
  touch(Event::ScreenTap, 200, PICKER_ROW_Y[1] + 30);  // c2
  EXPECT_TRUE(stored(R"("activeRegion":"c2")"));
  enter();
  send(Event::Released, Button::Back);
  touch(Event::ScreenTap, 200, COLLECTION_ROW_Y + 20);
  expectPage(1);
  EXPECT_EQ(selectedPickerRow(), 1);
}

// ---- Configuration-specific shape ----

#if ON_POINT_TEST_PICKER_COLLECTIONS == 5
TEST_F(OnPointActivityPickerPagingTest, FiveCollectionsShowFourThenOne) {
  static_assert(PAGES == 2);
  openPicker();
  expectPage(1);                                          // "1 / 2", c1-c4
  send(Event::Pressed, Button::NavPrevious);              // c1 -> c5
  expectPage(2);                                          // "2 / 2", c5 alone
  EXPECT_FALSE(drew("STR_ON_POINT_CURRENT_COLLECTION"));  // the active collection is on page 1
  send(Event::Pressed, Button::NavPrevious);              // c5 -> c4
  expectPage(1);
  EXPECT_EQ(selectedPickerRow(), 3);
}
#elif ON_POINT_TEST_PICKER_COLLECTIONS == 6
// The shipped shape since SOUTH LUZON: page 2 holds two collections and two empty rows.
TEST_F(OnPointActivityPickerPagingTest, SixCollectionsShowTwoRowsOnPageTwo) {
  static_assert(PAGES == 2);
  openPicker();
  send(Event::Pressed, Button::NavPrevious);  // c1 wraps to c6, page 2 row 1
  expectPage(2);                              // c5, c6, then two empty rows
  EXPECT_EQ(selectedPickerRow(), 1);
  touch(Event::ScreenTouchDown, 200, PICKER_ROW_Y[0] + 30);  // c5
  EXPECT_EQ(selectedPickerRow(), 0);
  for (const int row : {2, 3}) {
    touch(Event::ScreenTap, 200, PICKER_ROW_Y[row] + 30);  // empty rows do nothing
    EXPECT_TRUE(inPicker()) << row;
    EXPECT_EQ(selectedPickerRow(), 0) << row;
  }
  send(Event::Pressed, Button::NavNext);  // c5 -> c6
  EXPECT_EQ(selectedPickerRow(), 1);
  send(Event::Pressed, Button::NavNext);  // c6 wraps to c1 on page 1
  expectPage(1);
  EXPECT_EQ(selectedPickerRow(), 0);
  send(Event::Pressed, Button::NavPrevious);           // and back to c6
  touch(Event::ScreenTap, 200, PICKER_ROW_Y[1] + 30);  // select c6 by touch
  EXPECT_FALSE(inPicker());
  EXPECT_TRUE(drewAt(label(6), COLLECTION_TEXT_Y));
  EXPECT_TRUE(stored(R"("activeRegion":"c6")"));

  enter();  // relaunch: the picker reopens on c6's row with CURRENT
  send(Event::Released, Button::Back);
  touch(Event::ScreenTap, 200, COLLECTION_ROW_Y + 20);
  expectPage(2);
  EXPECT_EQ(selectedPickerRow(), 1);
  EXPECT_TRUE(drewAt("STR_ON_POINT_CURRENT_COLLECTION", PICKER_ROW_Y[1] + 64));
}
#elif ON_POINT_TEST_PICKER_COLLECTIONS == 8
TEST_F(OnPointActivityPickerPagingTest, EightCollectionsFillTwoPages) {
  static_assert(PAGES == 2);
  openPicker();
  swipe(SwipeDir::Up);
  expectPage(2);  // c5-c8, no empty row
}
#elif ON_POINT_TEST_PICKER_COLLECTIONS == 9
TEST_F(OnPointActivityPickerPagingTest, NineCollectionsUseThreePages) {
  static_assert(PAGES == 3);
  openPicker();
  EXPECT_TRUE(drewAt("1 / 3", PAGE_LABEL_Y));
  swipe(SwipeDir::Up);
  swipe(SwipeDir::Up);
  expectPage(3);  // c9 alone
  touch(Event::ScreenTap, 200, PICKER_ROW_Y[0] + 30);
  EXPECT_TRUE(stored(R"("activeRegion":"c9")"));
}
#endif

static_assert(ROW_STEP == 126, "PICKER_ROW_Y spacing");

}  // namespace
