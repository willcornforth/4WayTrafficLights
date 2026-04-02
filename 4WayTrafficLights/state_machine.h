#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <stdbool.h>

#include "types.h"

// Milliseconds during intermediate red plus amber state.
#define TRANSIENT_STATE_DURATION_MS 1500
// Milliseconds during which the light remains green before switching to yellow.
#define GREEN_STATE_DURATION_MS 30000
// Milliseconds during which the light remains yellow before switching to red.
#define PEDESTRIAN_CROSSING_DURATION_MS 15000

// Fault detection timeout threshold
#define FAULT_DETECTION_TIMEOUT_MS 500
// Sensor polling interval
#define SENSOR_POLLING_INTERVAL_MS 100
#define INTERVAL_PER_TICK_MS 100

#ifdef __cplusplus
extern "C" {
#endif

    void set_system_fault(state_context_t* ctx);

    // Aquires latest sensor data and updates the state context accordingly.
    void sensor_poll(state_context_t* ctx);

    // Executes one tick of the state machine, updating the system's state and virtual pins as necessary.
    void state_machine_tick(state_context_t* ctx);

    // Resets the system to a known initial state, clearing all virtual pins and context values.
    void reset_system(state_context_t* ctx);

#ifdef __cplusplus
}
#endif

#endif // STATE_MACHINE_H