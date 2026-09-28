#include "trust_beacon/production_trust_controller.h"

#include <cstdlib>
#include <iostream>

using namespace trust_beacon;

namespace {

struct Led final : ILed {
  bool is_healthy = true;
  bool accepts = true;
  int set_count = 0;
  int off_count = 0;
  LedCmd last{Color::GREEN, 0, false, 0, 0};
  bool healthy() const noexcept override { return is_healthy; }
  void off() noexcept override { ++off_count; }
  bool set(const LedCmd &cmd) noexcept override {
    ++set_count;
    last = cmd;
    return accepts;
  }
};
struct Camera final : ICamera {
  bool permits = true;
  int disabled = 0;
  int checked = 0;
  LedCmd last{Color::GREEN, 0, false, 0, 0};
  void disable() noexcept override { ++disabled; }
  bool can_capture(const LedCmd &cmd) noexcept override {
    ++checked;
    last = cmd;
    return permits;
  }
};
struct Battery final : IBattery {
  int value = 50;
  bool cased = false;
  int percent() const noexcept override { return value; }
  bool in_case() const noexcept override { return cased; }
};
struct Power final : IPower {
  std::uint32_t elapsed = 0;
  int hard_off = 0;
  std::uint32_t ms_in_state() const noexcept override { return elapsed; }
  void enter_hard_off() noexcept override { ++hard_off; }
};
struct Light final : ILight {
  bool bright = false;
  bool is_bright() const noexcept override { return bright; }
};
struct Imu final : IImu {
  bool pocketed = false;
  bool in_pocket(std::uint32_t) const noexcept override { return pocketed; }
};

struct Fixture {
  Led led;
  Camera camera;
  Battery battery;
  Power power;
  Light light;
  Imu imu;
  ProductionTrustController controller{led, camera, battery, power, light, imu};
};

#define CHECK(expression)                                                      \
  do {                                                                         \
    if (!(expression)) {                                                       \
      std::cerr << "CHECK failed at line " << __LINE__ << ": " #expression     \
                << '\n';                                                       \
      std::exit(EXIT_FAILURE);                                                 \
    }                                                                          \
  } while (false)

void fail_safe_paths() {
  Fixture f;
  f.led.is_healthy = false;
  CHECK(!f.controller.update(State::CAPTURING, false));
  CHECK(f.camera.disabled == 1);
  CHECK(f.camera.checked == 0);

  Fixture rejected;
  rejected.led.accepts = false;
  CHECK(!rejected.controller.update(State::CAPTURING, false));
  CHECK(rejected.camera.disabled == 1);
  CHECK(rejected.camera.checked == 0);
}

void capture_is_bright_and_interlocked() {
  Fixture f;
  f.battery.cased = true;
  CHECK(f.controller.update(State::CAPTURING, true));
  const LedCmd expected{Color::WHITE, 255, false, 0, 0};
  CHECK(f.led.last == expected);
  CHECK(f.camera.last == expected);
  CHECK(f.camera.checked == 1);

  f.camera.permits = false;
  CHECK(!f.controller.update(State::CAPTURING, false));
  CHECK(f.camera.disabled == 1);
}

void off_states_disable_capture() {
  Fixture hard;
  CHECK(hard.controller.update(State::HARD_OFF, false));
  CHECK(hard.led.off_count == 1);
  CHECK(hard.camera.disabled == 1);

  Fixture soft;
  CHECK(soft.controller.update(State::SOFT_OFF, true));
  CHECK(soft.led.last == LedCmd({Color::GREEN, 255, true, 200, 1000}));
  CHECK(soft.camera.disabled == 1);

  soft.battery.value = 9;
  CHECK(soft.controller.update(State::SOFT_OFF, false));
  CHECK(soft.led.last == LedCmd({Color::GREEN, 80, true, 100, 2000}));
}

void idle_adapts_and_theater_suppresses_only_idle() {
  Fixture daylight;
  daylight.light.bright = true;
  CHECK(daylight.controller.update(State::ON_IDLE, false));
  CHECK(daylight.led.last.brightness == 105);
  CHECK(daylight.camera.disabled == 1);

  Fixture cased;
  cased.battery.cased = true;
  CHECK(cased.controller.update(State::ON_IDLE, false));
  CHECK(cased.led.last.brightness == 20);

  Fixture theater;
  CHECK(theater.controller.update(State::ON_IDLE, true));
  CHECK(theater.led.off_count == 1);
  CHECK(theater.camera.disabled == 1);
}

void soft_off_requests_hard_off() {
  Fixture f;
  f.power.elapsed = 7'200'000;
  CHECK(f.controller.update(State::SOFT_OFF, false));
  CHECK(f.power.hard_off == 1);
}

} // namespace

int main() {
  fail_safe_paths();
  capture_is_bright_and_interlocked();
  off_states_disable_capture();
  idle_adapts_and_theater_suppresses_only_idle();
  soft_off_requests_hard_off();
  std::cout << "All trust beacon tests passed\n";
}
