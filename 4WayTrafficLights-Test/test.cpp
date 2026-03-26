#include "pch.h"
#include "../4WayTrafficLights/state_machine.h"

TEST(test_system_fault, system) {
	state_context_t ctx = { 0 };
	set_system_fault(&ctx);

	EXPECT_TRUE(ctx.pair_a == OFF);
	EXPECT_TRUE(ctx.pair_b == OFF);
	EXPECT_TRUE(ctx.system_mode == FAULT);
}