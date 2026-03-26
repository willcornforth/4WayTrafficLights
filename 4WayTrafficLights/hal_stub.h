#ifndef HAL_STUB_H
#define HAL_STUB_H

#include "state_machine.h"

// Define constants for pin states
typedef enum {
	PIN_RESET = 0,
	PIN_SET = 1,
	PIN_INVALID = -1
} hal_pin_value_t;

// Explicity define each pinout for our virtual GPIO.
typedef enum 
{
	PAIR_A_LIGHT_RED_PIN = 0,
	PAIR_A_LIGHT_AMBER_PIN ,
	PAIR_A_LIGHT_GREEN_PIN ,
	PAIR_A_VEHICLE_SENSOR_PIN ,
	PAIR_A_VEHICLE_SENSOR_HEARTBEAT_PIN ,
	PAIR_A_PED_BUTTON_PIN ,
	PAIR_B_LIGHT_RED_PIN,
	PAIR_B_LIGHT_AMBER_PIN,
	PAIR_B_LIGHT_GREEN_PIN,
	PAIR_B_VEHICLE_SENSOR_PIN,
	PAIR_B_VEHICLE_SENSOR_HEARTBEAT_PIN,
	PAIR_B_PED_BUTTON_PIN,
	PIN_MAX
} hal_pin_t;

// Virtual pin states for the simulation.
extern unsigned char virtual_pins[256];

void hal_init();
void hal_simulate_sensor_heartbeat(unsigned char pin);
char hal_write_pin(unsigned char pin, unsigned char value);
char hal_read_pin(unsigned char pin);
unsigned long long hal_get_elapsed_time_ms();
void hal_delay(int milliseconds);

#endif // HAL_STUB_H