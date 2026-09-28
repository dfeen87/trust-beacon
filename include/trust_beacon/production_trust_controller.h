#pragma once

#include <cstdint>

namespace trust_beacon {

enum class State : std::uint8_t { HARD_OFF, SOFT_OFF, ON_IDLE, CAPTURING };
enum class Color : std::uint8_t { GREEN = 1, WHITE = 2 };

struct LedCmd {
  Color color;
  std::uint8_t brightness;
  bool blink;
  std::uint16_t on_ms;
  std::uint16_t off_ms;

  friend constexpr bool operator==(const LedCmd &lhs,
                                   const LedCmd &rhs) noexcept {
    return lhs.color == rhs.color && lhs.brightness == rhs.brightness &&
           lhs.blink == rhs.blink && lhs.on_ms == rhs.on_ms &&
           lhs.off_ms == rhs.off_ms;
  }
};

class ILed {
public:
  virtual ~ILed() = default;
  [[nodiscard]] virtual bool healthy() const noexcept = 0;
  virtual void off() noexcept = 0;
  // Implementations must authenticate commands and return true only after the
  // physical driver has acknowledged the requested output.
  [[nodiscard]] virtual bool set(const LedCmd &command) noexcept = 0;
};

class ICamera {
public:
  virtual ~ICamera() = default;
  virtual void disable() noexcept = 0;
  // The capture interlock must fail closed unless command is acknowledged by
  // the LED driver within the hardware's compliance deadline (normally 50 ms).
  [[nodiscard]] virtual bool can_capture(const LedCmd &command) noexcept = 0;
};

class IBattery {
public:
  virtual ~IBattery() = default;
  [[nodiscard]] virtual int percent() const noexcept = 0;
  [[nodiscard]] virtual bool in_case() const noexcept = 0;
};

class IPower {
public:
  virtual ~IPower() = default;
  [[nodiscard]] virtual std::uint32_t ms_in_state() const noexcept = 0;
  virtual void enter_hard_off() noexcept = 0;
};

class ILight {
public:
  virtual ~ILight() = default;
  [[nodiscard]] virtual bool is_bright() const noexcept = 0;
};

class IImu {
public:
  virtual ~IImu() = default;
  [[nodiscard]] virtual bool
  in_pocket(std::uint32_t duration_ms) const noexcept = 0;
};

class ProductionTrustController final {
public:
  ProductionTrustController(ILed &led, ICamera &camera, IBattery &battery,
                            IPower &power, ILight &light, IImu &imu) noexcept;

  // Applies the complete indication/capture transaction. False means the
  // requested state was not established and capture has been disabled.
  [[nodiscard]] bool update(State state, bool theater_mode) noexcept;

private:
  [[nodiscard]] LedCmd adapt(LedCmd command, State state) const noexcept;
  [[nodiscard]] bool apply(const LedCmd &command) noexcept;
  [[nodiscard]] bool fail_closed() noexcept;

  ILed &led_;
  ICamera &camera_;
  IBattery &battery_;
  IPower &power_;
  ILight &light_;
  IImu &imu_;
};

} // namespace trust_beacon
