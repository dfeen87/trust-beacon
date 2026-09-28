#include "trust_beacon/production_trust_controller.h"

#include <algorithm>

namespace trust_beacon {
namespace {

constexpr int kLowBatteryPercent = 10;
constexpr std::uint32_t kHardOffTimeoutMs = 7'200'000U;
constexpr std::uint32_t kPocketTimeoutMs = 300'000U;

constexpr LedCmd kLowBatteryOff{Color::GREEN, 80U, true, 100U, 2'000U};
constexpr LedCmd kSoftOff{Color::GREEN, 255U, true, 200U, 1'000U};
constexpr LedCmd kIdle{Color::GREEN, 35U, false, 0U, 0U};
constexpr LedCmd kCapturing{Color::WHITE, 255U, false, 0U, 0U};

} // namespace

ProductionTrustController::ProductionTrustController(ILed &led, ICamera &camera,
                                                     IBattery &battery,
                                                     IPower &power,
                                                     ILight &light,
                                                     IImu &imu) noexcept
    : led_(led), camera_(camera), battery_(battery), power_(power),
      light_(light), imu_(imu) {}

bool ProductionTrustController::fail_closed() noexcept {
  camera_.disable();
  return false;
}

bool ProductionTrustController::apply(const LedCmd &command) noexcept {
  if (!led_.set(command)) {
    return fail_closed();
  }
  return true;
}

LedCmd ProductionTrustController::adapt(LedCmd command,
                                        State state) const noexcept {
  // Compliance and explicit OFF indications must never be attenuated. The case
  // optimization is restricted to the non-capturing idle indication.
  if (state != State::ON_IDLE) {
    return command;
  }

  if (battery_.in_case()) {
    command.brightness = 20U;
    return command;
  }

  if (light_.is_bright()) {
    constexpr unsigned kDaylightScale = 3U;
    const auto scaled =
        static_cast<unsigned>(command.brightness) * kDaylightScale;
    command.brightness = static_cast<std::uint8_t>(
        std::min(scaled, static_cast<unsigned>(UINT8_MAX)));
  }
  return command;
}

bool ProductionTrustController::update(State state,
                                       bool theater_mode) noexcept {
  if (!led_.healthy()) {
    return fail_closed();
  }

  switch (state) {
  case State::HARD_OFF:
    camera_.disable();
    led_.off();
    return true;

  case State::SOFT_OFF: {
    camera_.disable();
    if (battery_.percent() < kLowBatteryPercent) {
      return apply(kLowBatteryOff);
    }
    const bool timed_out = power_.ms_in_state() >= kHardOffTimeoutMs;
    const bool pocketed = imu_.in_pocket(kPocketTimeoutMs);
    if (timed_out || pocketed) {
      power_.enter_hard_off();
    }
    return apply(kSoftOff);
  }

  case State::ON_IDLE:
    camera_.disable();
    if (theater_mode) {
      led_.off();
      return true;
    }
    return apply(adapt(kIdle, state));

  case State::CAPTURING: {
    // Apply exactly the command checked by the independent camera interlock.
    // LED acknowledgement precedes enabling capture to avoid any dark window.
    const LedCmd command = adapt(kCapturing, state);
    if (!apply(command)) {
      return false;
    }
    if (!camera_.can_capture(command)) {
      return fail_closed();
    }
    return true;
  }
  }

  // Defensive handling for corrupted enum values crossing a C/IPC boundary.
  led_.off();
  return fail_closed();
}

} // namespace trust_beacon
