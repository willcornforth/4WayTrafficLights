#include "pch.h"

#ifdef __cplusplus
extern "C" {
#include "../4WayTrafficLights/state_machine.h"
}
#endif

TEST(test_system_fault, system_invariants) {
	state_context_t ctx = { 0 };

	set_system_fault(&ctx);

	EXPECT_TRUE(ctx.pair_a == OFF && ctx.pair_b == OFF && ctx.system_mode == FAULT);
}