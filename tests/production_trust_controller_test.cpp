#include "trust_beacon/production_trust_controller.h"

#include <catch2/catch_test_macros.hpp>

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
  bool set(const LedCmd& cmd) noexcept override {
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
  bool can_capture(const LedCmd& cmd) noexcept override {
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

} // namespace

TEST_CASE("fail-safe paths disable capture") {
  Fixture unhealthy;
  unhealthy.led.is_healthy = false;
  CHECK_FALSE(unhealthy.controller.update(State::CAPTURING, false));
  CHECK(unhealthy.camera.disabled == 1);
  CHECK(unhealthy.camera.checked == 0);

  Fixture rejected;
  rejected.led.accepts = false;
  CHECK_FALSE(rejected.controller.update(State::CAPTURING, false));
  CHECK(rejected.camera.disabled == 1);
  CHECK(rejected.camera.checked == 0);
}

TEST_CASE("capture is bright and hardware-interlocked") {
  Fixture fixture;
  fixture.battery.cased = true;
  CHECK(fixture.controller.update(State::CAPTURING, true));
  const LedCmd expected{Color::WHITE, 255, false, 0, 0};
  CHECK(fixture.led.last == expected);
  CHECK(fixture.camera.last == expected);
  CHECK(fixture.camera.checked == 1);

  fixture.camera.permits = false;
  CHECK_FALSE(fixture.controller.update(State::CAPTURING, false));
  CHECK(fixture.camera.disabled == 1);
}

TEST_CASE("off states disable capture") {
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

TEST_CASE("idle adapts while theater suppresses only idle") {
  Fixture daylight;
  daylight.light.bright = true;
  CHECK(daylight.controller.update(State::ON_IDLE, false));
  CHECK(daylight.led.last.brightness == 120);
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

TEST_CASE("soft off requests hard off after its timeout") {
  Fixture fixture;
  fixture.power.elapsed = 7'200'000;
  CHECK(fixture.controller.update(State::SOFT_OFF, false));
  CHECK(fixture.power.hard_off == 1);
}
