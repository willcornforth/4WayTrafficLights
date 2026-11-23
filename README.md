# 4-Way Traffic Lights

A simulated 4-way traffic light control system developed for a High Integrity Systems in-course assessment. The project implements a C state machine for a junction with paired North–South and East–West lights, vehicle sensors, pedestrian requests, fault detection, and hardware-abstraction stubs.

The repository also contains a formal methods specification describing the state model, safety invariants, timing properties, fault handling, and traceability.

## Features

- Alternating North–South and East–West traffic phases
- Red, red–amber, green, and amber signal progression
- Vehicle-presence detection and green-phase demand handling
- Pedestrian crossing requests and timed crossing phases
- Sensor and hardware feedback fault detection
- All-red and complete fault shutdown states
- Hardware abstraction through a virtual GPIO/HAI interface
- Microsoft Visual Studio unit tests
- Formal state-machine documentation

## Repository Structure

```text
4WayTrafficLights/
├── main.c                  # Application entry point and main polling loop
├── state_machine.c         # Traffic-light state-machine implementation
├── state_machine.h         # Public state-machine API and timing constants
├── types.h                 # Shared types and system state definitions
├── hal_stub.c              # Virtual hardware abstraction implementation
├── hal_stub.h              # Virtual pin and HAL declarations
└── 4WayTrafficLights.vcxproj

4WayTrafficLights-Tests/
├── 4WayTrafficLights-Tests.cpp # C++/Microsoft unit tests
└── 4WayTrafficLights-Tests.vcxproj

4way-traffic-lights-formal-methods.md  # Formal specification
4WayTrafficLights.sln                  # Visual Studio solution
```

## Requirements

- Windows
- Visual Studio 2022 or later
- Desktop development with C++ workload
- MSVC toolchain
- Microsoft Visual Studio C++ unit-test framework

The solution project uses C17 for C sources and includes Win32/x64 configurations.

## Building

1. Open `4WayTrafficLights.sln` in Visual Studio.
2. Select a Debug or Release configuration and a platform (Win32 or x64).
3. Choose **Build > Build Solution**.

The application project is `4WayTrafficLights`; the unit-test project is `4WayTrafficLights-Tests`.

## Running

Set `4WayTrafficLights` as the startup project, then run it from Visual Studio or execute the generated application. The console entry point continuously polls sensors and advances the state machine.

The included HAL uses `virtual_pins` to represent GPIO outputs and sensor inputs, allowing the system to be exercised without physical traffic-light hardware.

## Running the Tests

Open **Test > Test Explorer** in Visual Studio and select **Run All Tests**. The test project uses the Microsoft Visual Studio C++ Unit Testing Framework.

The tests currently cover system fault handling and a simulated green-state transition fault scenario.

## Key Timing Values

| State or interval | Duration |
|---|---:|
| Intermediate red–amber | 1.5 seconds |
| Green phase | 30 seconds when traffic is waiting |
| Amber/pedestrian crossing | 15 seconds |
| Fault detection timeout | 500 milliseconds |
| Sensor polling interval | 100 milliseconds |
| Main-loop tick | 100 milliseconds |

## Safety and Fault Behaviour

The formal specification requires mutually exclusive traffic permits and an all-red safe state whenever conflicting demands or unsafe conditions are detected. Failure handling includes:

- Vehicle-sensor failure
- Failure to advance a signal
- Failure to illuminate or extinguish a light
- Desynchronisation of paired installations
- Invalid state or unsafe transition

On a critical fault, both paired installations are held at `OFF` and the system enters fault mode.

## Formal Specification

See [`4way-traffic-lights-formal-methods.md`](4way-traffic-lights-formal-methods.md) for the complete specification, including:

- State and transition definitions
- Timing properties
- Safety invariants
- Pedestrian operation
- Fault detection and recovery
- High-integrity C implementation constraints
- Traceability matrix

## Language Composition

- C: 93.8%
- C++: 6.2%

The C++ content is primarily the Microsoft Visual Studio test harness.
