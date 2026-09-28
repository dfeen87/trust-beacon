# Contributing to Trust Beacon

Thank you for helping improve Trust Beacon. Changes must preserve the
fail-closed camera behavior and the user-visible indicator contract.

## Development Workflow

1. Create a focused branch and add tests for behavioral changes.
2. Configure and run the Debug and Release test suites.
3. Run `clang-format` and `clang-tidy` before opening a pull request.
4. Explain any safety-contract impact in the pull request description.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
clang-format --dry-run --Werror include/trust_beacon/*.h src/*.cpp tests/*.cpp
```

Contributions must avoid dynamic allocation in controller operations and
exceptions at the hardware boundary. New capture paths must prove that the LED
is healthy and acknowledged before the camera interlock permits capture.

By contributing, you agree that your work is licensed under the MIT License.
