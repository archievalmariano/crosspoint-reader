#pragma once
// Host-test stand-in: one queued input event per loop() call.

class MappedInputManager {
 public:
  enum class Button { Back, Confirm, Left, Right, Up, Down, PageBack, PageForward, NavPrevious, NavNext, Power };
  enum class SwipeDir { None, Left, Right, Up, Down };
  enum class Event { None, Pressed, Released, LongPressed, ScreenTap, ScreenTouchDown, ScreenLongPress, Swipe };

  Event event = Event::None;
  Button button = Button::Back;
  int x = 0;
  int y = 0;
  SwipeDir swipe = SwipeDir::None;

  void clear() { event = Event::None; }

  bool wasPressed(Button b) const { return event == Event::Pressed && button == b; }
  bool wasReleased(Button b) const { return event == Event::Released && button == b; }
  bool wasLongPressed(Button b, unsigned long) const { return event == Event::LongPressed && button == b; }
  bool wasScreenTapped(int& outX, int& outY) const { return screen(Event::ScreenTap, outX, outY); }
  bool wasScreenTouchDown(int& outX, int& outY) const { return screen(Event::ScreenTouchDown, outX, outY); }
  bool wasScreenLongPress(int& outX, int& outY) const { return screen(Event::ScreenLongPress, outX, outY); }
  bool wasTapInRect(int rx, int ry, int rw, int rh) const {
    return event == Event::ScreenTap && x >= rx && x < rx + rw && y >= ry && y < ry + rh;
  }
  SwipeDir wasSwipe() const { return event == Event::Swipe ? swipe : SwipeDir::None; }

 private:
  bool screen(Event e, int& outX, int& outY) const {
    if (event != e) return false;
    outX = x;
    outY = y;
    return true;
  }
};
