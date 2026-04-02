#ifndef TYPES_H
#define TYPES_H

typedef enum
{
    OFF = 0,
	RED,
	RED_AND_AMBER,
	GREEN,
	AMBER,
	STATE_MAX
} light_state_t;

typedef enum {
        NORMAL = 0,
        FAULT = 1,
} system_mode_t;

typedef struct {
	unsigned char sensor_value;
	unsigned char heartbeat;
	unsigned char last_heartbeat;
	unsigned long long last_heartbeat_time;
} vehicle_sensor_t;

// System state as defined in as a tuple
//S=⟨σ_A, σ_B, m, sens_A, sens_B, ped, t_phase, t_ped⟩
typedef struct {
    unsigned int tick_count;

    light_state_t pair_a;
    light_state_t pair_b;

    system_mode_t system_mode; // Normal, fault.
    bool pair_flip_flop;

    vehicle_sensor_t vehicle_sensor_a;
    vehicle_sensor_t vehicle_sensor_b;

    unsigned char pedestrian_button_a;
    unsigned char pedestrian_button_b;

    unsigned char pedestrian_crossing_a;
    unsigned char pedestrian_crossing_b;

    unsigned long long phase_time;
    unsigned long long pedestrian_time;
} state_context_t;

#endif // TYPES_H