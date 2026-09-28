# Trust Beacon

Fail-closed trust-indicator state machine for always-on AI-glasses firmware.

## User-visible contract

| State | Indicator | Camera |
|---|---|---|
| Hard off | Dark | Disabled |
| Soft off | Bright blinking green | Disabled |
| On, idle | Dim solid green (theater mode may hide it) | Disabled |
| Capturing | Bright solid white | Hardware-interlocked |

Low battery changes soft-off to a short green pulse every two seconds. Ambient
light scaling and in-case dimming affect only the idle indication; they can
never weaken the off or capture disclosure. A failed LED health check, command
acknowledgement, or capture interlock disables the camera.

## Integration requirements

This library is allocation-free and exception-free at its hardware boundary.
The platform implementation remains responsible for guarantees that software
alone cannot provide:

- Run the controller and LED driver from the always-on rail.
- Enforce secure boot and authenticate commands in `ILed::set`.
- Electrically gate the ISP using `ICamera::can_capture`, with an LED
  acknowledgement deadline of at most 50 ms.
- Monitor LED open/short/over-temperature conditions in `ILed::healthy`.
- Validate calibrated brightness across temperature, aging, and production
  bins, and use inaudible/spread-spectrum PWM where required.
- Mirror a trust-indicator fault to the paired device or charging case.

Treat a `false` result from `update` as a safety fault. The controller already
disables capture; callers should additionally record diagnostics and notify the
user without retrying capture.

## Build and test

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```
