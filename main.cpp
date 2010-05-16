#include <iostream>
#include <OpenCV/cv.h>
#include <OpenCV/highgui.h>
#include <stdio.h>
#include <ctype.h>
#include "tracker.h"

void on_mouse(int event, int x, int y, int flags, void*param);

int main (int argc, char * const argv[]) {
	
	Tracker topCamera("top camera", 2, on_mouse);
	Tracker sideCamera("side camera", 1, on_mouse);
	
	printf("starting, pres ESC to quit\n");
	double startClock = clock();
	long i = 0;
	for(;;i++) {
		topCamera.nextFrame();
		sideCamera.nextFrame();
		
		topCamera.processMostRecentFrame();
		sideCamera.processMostRecentFrame();
		
		topCamera.handleSelectionEvents();
		sideCamera.handleSelectionEvents();
		
		
		topCamera.displayImage();
		sideCamera.displayImage();
		
		/*
		// save images
		char name1[255], name2[255];
		sprintf(name1, "top%i.jpg", i);
		sprintf(name2, "side%i.jpg", i);
		cvSaveImage(name1,topFrame);
		cvSaveImage(name2, sideFrame);
		 */
		
		// check for end signal ("ESC" key)
		if((char) cvWaitKey(10) == 27) {
			break;
		}
		
		                                                                
	}
	
	// compute average effective fps
	double ave = i / ((clock() - startClock) / CLOCKS_PER_SEC);
	printf("average fps: %f\n", ave);
		
	// clean up
	printf("cleaning up");
	topCamera.cleanUp();
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

