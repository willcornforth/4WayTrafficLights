#include "hal_stub.h"

#include <Windows.h>
#define WIN32_LEAN_AND_MEAN

unsigned char virtual_pins[256] = { 0 };
unsigned long long simulated_start_time_ms = 0;

// Called in reset_system to initialize the HAL and set the simulated start time for the monotonic timer.
void hal_init() {
	simulated_start_time_ms = GetTickCount64();
}

char hal_write_pin(unsigned char pin, unsigned char value)
{
	if (pin < 0 || pin >= 256 || value < 0 || value > 1)
	{
		return PIN_INVALID;
	}

	// Crude implementation of simulated feedback verification.
	virtual_pins[pin] = value;

	return virtual_pins[pin] == value ? value : PIN_INVALID;
}

char hal_read_pin(unsigned char pin)
{
	if (pin < 0 || pin >= 256)
	{
		return PIN_INVALID;
	}

	return virtual_pins[pin];
}

// Simulates the heartbeat signal of a vehicle sensor by toggling the corresponding virtual pin's state.
void hal_simulate_sensor_heartbeat(unsigned char pin) {
	if (pin < 0 || pin >= 256)
	{
		return;
	}

	unsigned char current = hal_read_pin(pin);
	hal_write_pin(pin, current ^ PIN_SET);
}

unsigned long long hal_get_elapsed_time_ms() {
	// For simulation purposes, we can use the Windows API to get the current tick count in milliseconds.
	// In reality, this would be a monotonic timer that starts on system boot.
	return GetTickCount64() - simulated_start_time_ms;
}

void hal_delay(int milliseconds)
{
	Sleep(milliseconds);
}