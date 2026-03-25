#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "hal_stub.h"
#include "state_machine.h"
#include "types.h"

// This file houses the core logic of the traffic light state machine. 
// It should be called in a loop from main.c, and will update the state of the traffic lights based on the current state, sensor inputs, and timing.

// Gets the next light state following the cyclical state machine:
// RED -> RED_AND_YELLOW -> GREEN -> YELLOW -> RED
static light_state_t next_light_state(light_state_t current_state) {
	switch (current_state) {
	case OFF:
		return RED;
	case RED:
		return RED_AND_AMBER;
	case RED_AND_AMBER:
		return GREEN;
	case GREEN:
		return AMBER;
	case AMBER:
		return RED;
	default:
		static_assert(STATE_MAX == 5, "Unexpected number of light states. Please update next_light_state function.");
		return OFF;
	}
}

// Returns true if the given light state allows vehicles to proceed, false otherwise.
// *	As stated in the predicate: permissive(s) ⇔ s ∈ {GREEN, AMBER, RED_AND_AMBER}
// *	"If and only if, s is in the set of {GREEN, AMBER, RED_AND_AMBER}, then s is permissive."
static bool is_light_state_permissive(light_state_t state) {
	return state == GREEN || state == RED_AND_AMBER || state == AMBER;
}

// Determines the light state based on the current state of the red, amber, and green pins for a given pair.
static light_state_t determine_light_state_from_pins(
	unsigned char red,
	unsigned char amber,
	unsigned char green
) {
	if (red && amber && !green) {
		return RED_AND_AMBER;
	}
	else if (red && !amber && !green) {
		return RED;
	}
	else if (!red && !amber && green) {
		return GREEN;
	}
	else if (!red && amber && !green) {
		return AMBER;
	}
	else if (!red && !amber && !green) {
		return OFF;
	}
	else {
		printf("[-] Warning: Invalid light state detected from pins. Defaulting to OFF.\n");
		return OFF;
	}
}

// Writes the given light state to the corresponding pins for the specified pair.
static void set_light_state(unsigned char pair, light_state_t state) {
	printf("[+] (%.2fs) Setting light state for pair %c to %d\n", ((double)hal_get_elapsed_time_ms() / 1000.f), pair, (int)state);
	int red_pin;
	int amber_pin;
	int green_pin;

	if (pair == 'A') {
		red_pin = PAIR_A_LIGHT_RED_PIN;
		amber_pin = PAIR_A_LIGHT_AMBER_PIN;
		green_pin = PAIR_A_LIGHT_GREEN_PIN;
	}
	else if (pair == 'B') {
		red_pin = PAIR_B_LIGHT_RED_PIN;
		amber_pin = PAIR_B_LIGHT_AMBER_PIN;
		green_pin = PAIR_B_LIGHT_GREEN_PIN;
	}
	else {
		printf("[-] Error: Invalid light pair '%c'.\n", pair);
		return;
	}

	switch (state) {
	case OFF:
		hal_write_pin(red_pin, PIN_RESET);
		hal_write_pin(amber_pin, PIN_RESET);
		hal_write_pin(green_pin, PIN_RESET);
		break;
	case RED:
		hal_write_pin(red_pin, PIN_SET);
		hal_write_pin(amber_pin, PIN_RESET);
		hal_write_pin(green_pin, PIN_RESET);
		break;
	case RED_AND_AMBER:
		hal_write_pin(red_pin, PIN_SET);
		hal_write_pin(amber_pin, PIN_SET);
		hal_write_pin(green_pin, PIN_RESET);
		break;
	case GREEN:
		hal_write_pin(red_pin, PIN_RESET);
		hal_write_pin(amber_pin, PIN_RESET);
		hal_write_pin(green_pin, PIN_SET);
		break;
	case AMBER:
		hal_write_pin(red_pin, PIN_RESET);
		hal_write_pin(amber_pin, PIN_SET);
		hal_write_pin(green_pin, PIN_RESET);
		break;

	default:
		printf("[-] Error: Invalid light state %d. Forcing OFF for pair %c.\n", (int)state, pair);
		hal_write_pin(red_pin, PIN_RESET);
		hal_write_pin(amber_pin, PIN_RESET);
		hal_write_pin(green_pin, PIN_RESET);
		break;
	}
}

static bool validate_light_state(light_state_t state) {

}

// For system critical faults, that can't be handled gracefully.
// F ∈ {F2, F3, F4, F5 } ⇒ ◯( m = FAULT ∧ σA ​= OFF ∧ σB = OFF ∧ alert_raised)
static bool set_system_fault(state_context_t* ctx) {
	set_light_state('A', OFF);
	set_light_state('B', OFF);
	ctx->system_mode = FAULT;
}

static bool traffic_sensor_failure(state_context_t* ctx) {
	// F1: Vehicle sensor failure
	// F1 ∈ {¬heartbeat_A, ¬heartbeat_B} ⇒ ◯( m = FAULT ∧ σA ​= OFF ∧ σB = OFF ∧ alert_raised)
}

// Simulation build inclusion. Resets the system state and virtual pins to a known good state (e.g. 0).
void reset_system(state_context_t* ctx)
{
	hal_init();
	printf("[+] HAL stub initialized.\n");

	printf("[+] Zeroed HAL Virtual Pins [ 0x%p ]\n", (void*)virtual_pins);
	memset(virtual_pins, 0, sizeof(virtual_pins));

	printf("[+] Zeroed Global Context [ 0x%p ]\n", ctx);
	memset(ctx, 0, sizeof(state_context_t));

	// Set the system's initial state.
	set_light_state('A', RED);
	set_light_state('B', RED);

	printf("[+] System reset complete.\n");
}

// Polls the current state of the sensors and updates the context.
void sensor_poll(state_context_t* ctx)
{
	if(ctx == NULL) {
		printf("[-] Error: Null context provided to sensor poll.\n");
		return;
	}	

	// Simulate the passage of time for the vehicle sensor heartbeats.
	hal_simulate_sensor_heartbeat(PAIR_A_VEHICLE_SENSOR_HEARTBEAT_PIN);
	hal_simulate_sensor_heartbeat(PAIR_B_VEHICLE_SENSOR_HEARTBEAT_PIN);

	// Read the current state of the pedestrian buttons and update the virtual pins accordingly
	// and set the state machine's context.
	ctx->vehicle_sensor_a = hal_read_pin(PAIR_A_VEHICLE_SENSOR_PIN);
	ctx->vehicle_sensor_b = hal_read_pin(PAIR_B_VEHICLE_SENSOR_PIN);

	ctx->vehicle_sensor_heartbeat_a = hal_read_pin(PAIR_A_VEHICLE_SENSOR_HEARTBEAT_PIN);
	ctx->vehicle_sensor_heartbeat_b = hal_read_pin(PAIR_B_VEHICLE_SENSOR_HEARTBEAT_PIN);

	ctx->pedestrian_button_a = hal_read_pin(PAIR_A_PED_BUTTON_PIN);
	ctx->pedestrian_button_b = hal_read_pin(PAIR_B_PED_BUTTON_PIN);

	unsigned char a_red = hal_read_pin(PAIR_A_LIGHT_RED_PIN);
	unsigned char a_amber = hal_read_pin(PAIR_A_LIGHT_AMBER_PIN);
	unsigned char a_green = hal_read_pin(PAIR_A_LIGHT_GREEN_PIN);
	ctx->pair_a = determine_light_state_from_pins(a_red, a_amber, a_green);

	unsigned char b_red = hal_read_pin(PAIR_B_LIGHT_RED_PIN);
	unsigned char b_amber = hal_read_pin(PAIR_B_LIGHT_AMBER_PIN);
	unsigned char b_green = hal_read_pin(PAIR_B_LIGHT_GREEN_PIN);
	ctx->pair_b = determine_light_state_from_pins(b_red, b_amber, b_green);
}

void state_machine_tick(state_context_t* ctx) {
	// Begin state machine...
	if(ctx == NULL) {
		printf("[-] Error: Null context provided to state machine.\n");
		return;
	}
	
	// Both directions are permissive which shouldn't be allowed.
	// V = { (σA, σB) | ¬( permissive(σA) ∧ permissive(σB) ) }
	if (is_light_state_permissive(ctx->pair_a) && is_light_state_permissive(ctx->pair_b)) {
		ctx->pair_a = OFF;
		ctx->pair_b = OFF;
		ctx->system_mode = FAULT;
		printf("[-] Fault detected: Both directions are permissive. Entering fault mode.\n");
	}

	// Constrain the system to fault when one or more lights are off.
	// (σA = OFF ∨ σB = OFF) ⇒ (σA = OFF ∧ σB = OFF)
	if ((ctx->pair_a == OFF || ctx->pair_b == OFF) && !(ctx->pair_a == OFF && ctx->pair_b == OFF)) {
		ctx->pair_a = OFF; // Need to consider writing back to HAL...
		ctx->pair_b = OFF;
		ctx->system_mode = FAULT;

		printf("[-] Fault detected: One or more lights are off. Entering fault mode.\n");
	}

	if (ctx->system_mode == FAULT)
		return;

	// When setting states, the ctx doesn't update till the next tick to prevent infinite loops.
	// Fault detection is handled in set_light_state and takes precedent over normal states.
	unsigned char master_pair = ctx->pair_flip_flop == false ? 'A' : 'B';
	light_state_t master_state = ctx->pair_flip_flop == false ? ctx->pair_a : ctx->pair_b;

	bool next_state_time_reached  = hal_get_elapsed_time_ms()- ctx->phase_time >= TRANSIENT_STATE_DURATION_MS;
	bool green_state_time_reached = hal_get_elapsed_time_ms() - ctx->phase_time >= GREEN_STATE_DURATION_MS;
	bool state_changed = false;

	// Master cycle begins.
	if (master_state == RED && next_state_time_reached) {
		set_light_state(master_pair, RED_AND_AMBER);
		state_changed = true;
	}

	if (master_state == RED_AND_AMBER && next_state_time_reached) {
		set_light_state(master_pair, GREEN);
		state_changed = true;
	}

	if (master_state == GREEN && green_state_time_reached) {
		set_light_state(master_pair, AMBER);
		state_changed = true;
	}

	if (master_state == AMBER && next_state_time_reached) {
		set_light_state(master_pair, RED);
		state_changed = true;

		// Flip flop state to track which pair is master for the next cycle.
		ctx->pair_flip_flop = !ctx->pair_flip_flop;
	}

	// If state changed, save phase time for the next state transition.
	if (state_changed) {
		ctx->phase_time = hal_get_elapsed_time_ms();
	}

	hal_delay(INTERVAL_PER_TICK_MS);
}