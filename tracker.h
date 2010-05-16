/*
 *  tracker.h
 *  CS225
 */

#ifndef _tracker_h
#define _tracker_h

#include <OpenCV/cv.h>
#include <OpenCV/highgui.h>

class Tracker {
public:
	IplImage *img;
	int select_object;
	int track_object;
	CvRect selection;
	CvPoint origin;
	
	Tracker(char* name, int cameraIndex, CvMouseCallback on_mouse);
	~Tracker();
	void nextFrame();
	void processMostRecentFrame();
	void handleSelectionEvents();
	void displayImage();
	void printTrackedObjectProperties();
	void cleanUp();
	
	float getDx();
	float getDy();
	CvPoint2D32f getCenter();
	
	
	static CvScalar hsv2rgb( float hue ) {
		int rgb[3], p, sector;
		static const int sector_data[][3]=
		{{0,2,1}, {1,2,0}, {1,0,2}, {2,0,1}, {2,1,0}, {0,1,2}};
		hue *= 0.033333333333333333333333333333333f;
		sector = cvFloor(hue);
		p = cvRound(255*(hue - sector));
		p ^= sector & 1 ? 255 : 0;
		
		rgb[sector_data[sector][0]] = 255;
		rgb[sector_data[sector][1]] = 0;
		rgb[sector_data[sector][2]] = p;
		
		return cvScalar(rgb[2], rgb[1], rgb[0],0);
	}
private:
	char* name;
	CvCapture *capture;
	
	IplImage *hsv;
	IplImage *hue;
	IplImage *mask;
	IplImage *backproject;
	IplImage *histimg;
	CvHistogram *hist;
	
	int vmin, vmax, smin;
	
	CvRect track_window;
	CvBox2D track_box;
	CvBox2D last_track_box;
	CvConnectedComp track_comp;
	
	int first_iter;
	float dx;
	float dy;
	
	int hdims;
	float hranges_arr[2];
	
	//void on_mouse(int event, int x, int y, int flags, void*param);
	void initializeImageVariables(IplImage *frame);
};

#endif