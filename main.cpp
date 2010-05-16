#include <iostream>
#include <OpenCV/cv.h>
#include <OpenCV/highgui.h>
#include <stdio.h>
#include <ctype.h>
#include "tracker.h"

extern "C" int connect_client();

struct BallProfile {
	float vx;		// m/s
	float vy;		// m/s
	float vz;		// m/s
	float centerX;  // m
	float centerY;  // m
	float centerZ;  // m
};

int debug = 0, debug_1 = 0, debug_2 = 0, debug_profile = 0;

void on_mouse(int event, int x, int y, int flags, void*param);

int main (int argc, char * const argv[]) {
	float x_offset = 1.7;
	float y_offset = .25;
	float z_offset = .68;
	
	connect_client();
	
	
	Tracker frontCamera("front camera", 2, on_mouse, 2.04, 1.64);
	Tracker sideCamera("side camera", 1, on_mouse, 2.04, 1.64);
	
	BallProfile currentProfile;
	
	printf("starting, pres ESC to quit\n");
	float startClock = clock();
	long i = 0;
	int c;
	for(;;i++) {
		frontCamera.nextFrame();
		sideCamera.nextFrame();
		
		frontCamera.processMostRecentFrame();
		sideCamera.processMostRecentFrame();
		
		frontCamera.handleSelectionEvents();
		sideCamera.handleSelectionEvents();
		
		
		frontCamera.displayImage();
		sideCamera.displayImage();
		
		if (frontCamera.isTracking() && sideCamera.isTracking()) {
			currentProfile.vx = sideCamera.getVxMeters();
			currentProfile.vy = frontCamera.getVyMeters();
			currentProfile.vz = frontCamera.getVyMeters();
			currentProfile.centerZ = frontCamera.getCenterMeters().y + z_offset - frontCamera.getPhysicalHeight() / 2;
			currentProfile.centerY = frontCamera.getCenterMeters().x - y_offset - frontCamera.getPhysicalWidth() / 2;
			currentProfile.centerX = sideCamera.getCenterMeters().x + x_offset - sideCamera.getPhysicalWidth() / 2;
			
		}
		
		if (debug) {
			if(debug_1) frontCamera.printTrackedObjectProperties();
			if(debug_2) sideCamera.printTrackedObjectProperties();
			if(debug_profile && frontCamera.isTracking() && sideCamera.isTracking()) {
				printf("pos (%f, %f, %f), vel (%f, %f, %f)\n", currentProfile.centerX, currentProfile.centerY, currentProfile.centerZ,
					   currentProfile.vx, currentProfile.vy, currentProfile.vz);
			}
		}
		
		// check for end signal ("ESC" key)
		c = cvWaitKey(10);
		if ((char) c == 27) {
			break;
		}
		switch ((char) c) {
			case 'b':
				frontCamera.switchBackProjectMode();
				sideCamera.switchBackProjectMode();
				break;
			case 'w':
				frontCamera.switchShowTrackingWindow();
				sideCamera.switchShowTrackingWindow();
				break;
			case 'r':
				frontCamera.reset();
				sideCamera.reset();
				break;
			case '0':
				frontCamera.stopTracking();
				sideCamera.stopTracking();
				break;

			case 'd':
				debug ^= 1;
				break;
			case '1':
				debug_1 ^= 1;
				break;
			case '2':
				debug_2 ^= 1;
				break;
			case '3':
				debug_profile ^= 1;
				break;

			default:
				break;
		}
	}
	
	// compute average effective fps
	double ave = i / ((clock() - startClock) / CLOCKS_PER_SEC);
	printf("average fps: %f\n", ave);
		
	// clean up
	printf("cleaning up");
	frontCamera.cleanUp();
	sideCamera.cleanUp();
    return 0;
}

void on_mouse(int event, int x, int y, int flags, void *param) {
	Tracker *t = (Tracker *)param;
	
	if (!t->img)
		return;
	
	if(t->img->origin)
		y = t->img->height - y;
	
	if(t->select_object) {
		t->selection.x = MIN(x, t->origin.x);
		t->selection.y = MIN(y, t->origin.y);
		t->selection.width = t->selection.x + CV_IABS(x - t->origin.x);
		t->selection.height = t->selection.y + CV_IABS(y - t->origin.y);
		
		t->selection.x = MAX(t->selection.x, 0);
		t->selection.y = MAX(t->selection.y, 0);
		t->selection.width = MIN(t->selection.width, t->img->width);
		t->selection.height = MIN(t->selection.height, t->img->height);
		t->selection.width -= t->selection.x;
		t->selection.height -= t->selection.y;
	}
	
	switch (event) {
		case CV_EVENT_LBUTTONDOWN:
			t->origin = cvPoint(x, y);
			t->selection = cvRect(x, y, 0, 0);
			t->select_object = 1;
			break;
		case CV_EVENT_LBUTTONUP:
			t->select_object = 0;
			if (t->selection.width > 0 && t->selection.height > 0) {
				t->track_object = -1;
			}
			break;
			
		default:
			break;
	}
}

