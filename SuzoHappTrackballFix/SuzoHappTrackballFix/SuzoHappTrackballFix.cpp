#include <stdlib.h>
#include <chrono>
#include <iostream>
#include <fstream>
#include "interception.h"
#include "utils.h"

enum ScanCode
{
	SCANCODE_ESC = 0X01
};

int main()
{
	//std::ofstream dataFile;
	//dataFile.open("data.txt");


	InterceptionContext context;
	InterceptionDevice device;
	InterceptionStroke stroke;

	context = interception_create_context();

	interception_set_filter(context, interception_is_keyboard, INTERCEPTION_FILTER_KEY_DOWN | INTERCEPTION_FILTER_KEY_UP);
	interception_set_filter(context, interception_is_mouse, INTERCEPTION_FILTER_MOUSE_MOVE);

	auto lastTimeStamp = std::chrono::system_clock::now();
	int lastValue = 0;
	int overrideCount = 0;
	int overrideDuration = 60;
	int maxYMovement = -60;
	int accelThreshold = -90000;

	while (interception_receive(context, device = interception_wait(context), &stroke, 1) > 0) {

		if (interception_is_mouse(device)) {

			InterceptionMouseStroke& mstroke = *(InterceptionMouseStroke*)&stroke;

			int originalYValue = mstroke.y;

			if (!(mstroke.flags & INTERCEPTION_MOUSE_MOVE_ABSOLUTE)) {

				auto timeStamp = std::chrono::system_clock::now();
				auto duration = timeStamp.time_since_epoch();

				if (overrideCount != 0) {
					mstroke.y = maxYMovement;
					overrideCount--;
					/*
					if (overrideCount == 0) {
						std::wcout << "---stop injecting---" << std::endl;
					}*/
				}
				else
				{
					std::chrono::duration<double> diffInTimestamps = timeStamp - lastTimeStamp;
					double diffInTimestampsSquared = pow(diffInTimestamps.count(), 2);

					double accel = 0;
					if (diffInTimestampsSquared != 0) {
						accel = (mstroke.y - lastValue) / (diffInTimestampsSquared);
					}

					//std::wcout << accel << " ---- " << mstroke.y << ' ' << lastValue << ' ' << ' ' << originalYValue << ' ' << diffInTimestamps.count() << ' ' << std::endl;
					//dataFile << duration.count() << ',' << originalYValue << ',' << mstroke.y << std::endl;
					
					if (accel < accelThreshold) {
						//std::wcout << "---start injecting---" << std::endl;
						mstroke.y = maxYMovement;
						overrideCount = overrideDuration;
					}
				}

				lastTimeStamp = timeStamp;
				lastValue = mstroke.y;

				interception_send(context, device, &stroke, 1);
			}
		}

		if (interception_is_keyboard(device)) {
			InterceptionKeyStroke& kstroke = *(InterceptionKeyStroke*)&stroke;

			interception_send(context, device, &stroke, 1);

			//if (kstroke.code == SCANCODE_ESC) break;
		}
	}

	interception_destroy_context(context);
	//dataFile.close();

	return 0;
}