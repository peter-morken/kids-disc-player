#include "input.h"

#include <Arduino.h>

#include <atomic>

#include "board.h"
#include "controls.h"

using namespace discplayer;

namespace {

std::atomic<int> pendingDetents{0};
std::atomic<bool> pendingLongPress{false};

void inputTask(void*) {
  QuadratureDecoder decoder;
  ButtonDetector button(30, KNOB_LONG_PRESS_MS);
  for (;;) {
    const int step = decoder.update(digitalRead(KNOB_A_PIN), digitalRead(KNOB_B_PIN));
    if (step) pendingDetents += KNOB_REVERSED ? -step : step;
    if (button.update(millis(), digitalRead(KNOB_SWITCH_PIN) == LOW) == ButtonEvent::LongPress)
      pendingLongPress = true;
    vTaskDelay(1);  // 1 ms at the Arduino core's 1 kHz tick
  }
}

}  // namespace

void inputBegin() {
  pinMode(KNOB_A_PIN, INPUT_PULLUP);
  pinMode(KNOB_B_PIN, INPUT_PULLUP);
  pinMode(KNOB_SWITCH_PIN, INPUT_PULLUP);
  // Same core as loop(), higher priority: preempts it briefly each tick.
  xTaskCreatePinnedToCore(inputTask, "input", 2048, nullptr, 2, nullptr, 1);
}

int takeKnobDetents() { return pendingDetents.exchange(0); }

bool takeLongPress() { return pendingLongPress.exchange(false); }
