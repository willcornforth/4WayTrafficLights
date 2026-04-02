#include "pch.h"
#include "CppUnitTest.h"

extern "C" {
#include "../4WayTrafficLights/types.h"
#include "../4WayTrafficLights/state_machine.h"
#include "../4WayTrafficLights/hal_stub.h"
}

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace My4WayTrafficLightsTests
{
	TEST_CLASS(M4WayTrafficLightsTests)
	{
	public:
		
		TEST_METHOD(SetSystemFault)
		{
			state_context_t ctx = { 0 };

			set_system_fault(&ctx);

			return Assert::IsTrue(ctx.pair_a == OFF && ctx.pair_b == OFF && ctx.system_mode == FAULT);
		}

		TEST_METHOD(GreenStateTransistion)
		{
			state_context_t ctx = { 0 };
			reset_system(&ctx);


			for (int i = 0; i < 3; i++)
			{
				ctx.tick_count++;

				if (i == 0) {
					hal_write_pin(PAIR_A_LIGHT_GREEN_PIN, -1);
				}

				sensor_poll(&ctx);
				state_machine_tick(&ctx);
			}

			Assert::IsTrue(ctx.system_mode == FAULT);
		}
	};
}
