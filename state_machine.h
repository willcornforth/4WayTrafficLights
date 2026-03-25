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

// System state as defined in as a tuple
//S=⟨σ_A, σ_B, m, sens_A, sens_B, ped, t_phase, t_ped⟩
typedef struct {
	unsigned int tick_count;

    light_state_t pair_a;
    light_state_t pair_b;

    system_mode_t system_mode; // Normal, fault.
    bool pair_flip_flop;

    unsigned char vehicle_sensor_a;
    unsigned char vehicle_sensor_heartbeat_a;
    unsigned char last_vehicle_sensor_heartbeat_a;
    unsigned long long last_heartbeat_time_a;

    unsigned char vehicle_sensor_b;
    unsigned char vehicle_sensor_heartbeat_b;
	unsigned char last_vehicle_sensor_heartbeat_b;
    unsigned long long last_heartbeat_time_b;

	unsigned char pedestrian_button_a;
    unsigned char pedestrian_button_b;

    unsigned long long phase_time;
    unsigned long long pedestrian_time;
} state_context_t;

// Aquires latest sensor data and updates the state context accordingly.
void sensor_poll(state_context_t* ctx);

// Executes one tick of the state machine, updating the system's state and virtual pins as necessary.
void state_machine_tick(state_context_t* ctx);

// Resets the system to a known initial state, clearing all virtual pins and context values.
void reset_system(state_context_t* ctx);

#endif // STATE_MACHINE_H