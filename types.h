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

#endif // TYPES_H