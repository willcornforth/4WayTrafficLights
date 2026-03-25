#include <stdio.h>

#include "state_machine.h"
#include "types.h"

// Global state context for the traffic light system.
state_context_t global_context = { 0 };

int main(int argc, char *argv[])
{
	printf("[+] Starting traffic light system..\n");
	reset_system(&global_context);

	while (1)
	{
		global_context.tick_count++;
		//printf(
		//	"[+] Tick: %d T elasped: %.2fs\n", 
		//	global_context.tick_count,
		//	(((float)global_context.tick_count * INTERVAL_PER_TICK_MS) / 1000.f)
		//);

		sensor_poll(&global_context);
		state_machine_tick(&global_context);
	}

	return 0;
}