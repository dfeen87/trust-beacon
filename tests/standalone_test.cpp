#include "trust_beacon/production_trust_controller.h"

#include <cstdlib>

using namespace trust_beacon;

namespace {

struct Led final : ILed {
  bool is_healthy = true;
  bool accepts = true;
  LedCmd last{Color::GREEN, 0, false, 0, 0};
  [[nodiscard]] bool healthy() const noexcept override { return is_healthy; }
  void off() noexcept override {}
  bool set(const LedCmd& command) noexcept override {
    last = command;
    return accepts;
  }
};

struct Camera final : ICamera {
  bool permits = true;
  int disabled = 0;
  void disable() noexcept override { ++disabled; }
  bool can_capture(const LedCmd& /*command*/) noexcept override { return permits; }
};

struct Battery final : IBattery {
  [[nodiscard]] int percent() const noexcept override { return 50; }
  [[nodiscard]] bool in_case() const noexcept override { return false; }
};

struct Power final : IPower {
  [[nodiscard]] std::uint32_t ms_in_state() const noexcept override { return 0; }
  void enter_hard_off() noexcept override {}
};

struct Light final : ILight {
  [[nodiscard]] bool is_bright() const noexcept override { return false; }
};

struct Imu final : IImu {
  [[nodiscard]] bool in_pocket(std::uint32_t /*duration_ms*/) const noexcept override {
    return false;
  }
};

} // namespace

int main() {
  Led led;
  Camera camera;
  Battery battery;
  Power power;
  Light light;
  Imu imu;
  ProductionTrustController controller{led, camera, battery, power, light, imu};

  if (!controller.update(State::CAPTURING, false) ||
      !(led.last == LedCmd{Color::WHITE, 255, false, 0, 0})) {
    return EXIT_FAILURE;
  }

  led.is_healthy = false;
  if (controller.update(State::CAPTURING, false) || camera.disabled != 1) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
