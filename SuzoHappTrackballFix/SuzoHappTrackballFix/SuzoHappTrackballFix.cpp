#include <stdlib.h>
#include <chrono>
#include <iostream>
#include <fstream>
#include <queue>
#include <string>
#include "interception.h"
#include "utils.h"

enum ScanCode
{
	SCANCODE_ESC = 0X01,
	SCANCODE_R = 19
};

int main()
{


	InterceptionContext context;
	InterceptionDevice device;
	InterceptionStroke stroke;

	context = interception_create_context();

	//interception_set_filter(context, interception_is_keyboard, INTERCEPTION_FILTER_KEY_DOWN | INTERCEPTION_FILTER_KEY_UP);
	interception_set_filter(context, interception_is_keyboard, INTERCEPTION_FILTER_KEY_DOWN);
	interception_set_filter(context, interception_is_mouse, INTERCEPTION_FILTER_MOUSE_MOVE);

	auto lastTimeStamp = std::chrono::system_clock::now();
	int lastValue = 0;
	int overrideCount = 0;
	int overrideDuration = 60;
	int maxYMovement = -60;
	int accelThreshold = -90000;
	int maxDataPoints = 500;
	int recCount = 0;
	int eventNumber = 0;

	struct dataPoint {
		int m_originalY;
		int m_modifiedY;
		int m_lastY;
		int m_time;
		double m_accel;
		dataPoint(int originalY, int modifiedY, int lastY, int time, double accel)
			: m_originalY(originalY), m_modifiedY(modifiedY), m_lastY(lastY), m_time(time), m_accel(accel)
		{}
	};
	
	std::queue<dataPoint> q;

	while (interception_receive(context, device = interception_wait(context), &stroke, 1) > 0) {

		if (interception_is_mouse(device)) {
			eventNumber++;
			InterceptionMouseStroke& mstroke = *(InterceptionMouseStroke*)&stroke;

			int originalYValue = mstroke.y;

			if (!(mstroke.flags & INTERCEPTION_MOUSE_MOVE_ABSOLUTE)) {

				auto timeStamp = std::chrono::system_clock::now();
				auto duration = timeStamp.time_since_epoch();

				std::chrono::duration<double> diffInTimestamps = timeStamp - lastTimeStamp;
				double diffInTimestampsSquared = pow(diffInTimestamps.count(), 2);

				double accel = 0;
				if (diffInTimestampsSquared != 0) {
					accel = (mstroke.y - lastValue) / (diffInTimestampsSquared);
				}

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
					//std::wcout << accel << " ---- " << mstroke.y << ' ' << lastValue << ' ' << ' ' << originalYValue << ' ' << diffInTimestamps.count() << ' ' << std::endl;
					//dataFile << duration.count() << ',' << originalYValue << ',' << mstroke.y << std::endl;
					
					if (accel < accelThreshold) {
						//std::wcout << "---start injecting---" << std::endl;
						mstroke.y = maxYMovement;
						overrideCount = overrideDuration;
					}
				}

				if (q.size() == maxDataPoints) {
					q.pop();
				}
				q.push(dataPoint(originalYValue, mstroke.y, lastValue, eventNumber, accel));

				lastTimeStamp = timeStamp;
				lastValue = mstroke.y;

				interception_send(context, device, &stroke, 1);
			}
		}

		/*
		if (interception_is_keyboard(device)) {
			InterceptionKeyStroke& kstroke = *(InterceptionKeyStroke*)&stroke;
			std::wcout << "keyboard" << kstroke.code << std::endl;
			
			if (kstroke.code == SCANCODE_R) {
				std::ofstream dataFile;
				recCount++;
				std::string filename = "data" + std::to_string(recCount) + ".txt";
				dataFile.open(filename);
				while (q.size() > 0) {
					dataPoint point = q.front();
					dataFile << point.m_time << ',' << point.m_lastY << ',' << point.m_originalY << ',' << point.m_modifiedY << ',' << point.m_accel << std::endl;
					q.pop();
				}
				dataFile.close();
			}
			//if (kstroke.code == SCANCODE_ESC) break;
			
		}
		*/
	}

	interception_destroy_context(context);
	return 0;
}