/*
 *  Tracker.cpp
 *  CS225
 *
 */

#include "tracker.h"
#include <OpenCV/cv.h>
#include <OpenCV/highgui.h>


Tracker::Tracker(char* mname, int cameraIndex, CvMouseCallback on_mouse) {
	// initialize all variables
	name = mname;
	select_object = 0;
	track_object = 0;
	vmin = 10;
	vmax = 256;
	smin = 30;
	
	img         = 0;
	hsv         = 0;
	hue         = 0;
	mask        = 0;
	backproject = 0;
	histimg     = 0;
	hist        = 0;
	
	hdims = 16;
	hranges_arr[0] = 0;
	hranges_arr[1] = 180;
	
	// set up the camera capture/window/trackbars
	capture = cvCaptureFromCAM(cameraIndex);
	cvNamedWindow(name, 0);
	cvSetMouseCallback(name, on_mouse, this);
	cvCreateTrackbar("Vmin", name, &vmin, 256, 0);
	cvCreateTrackbar("Vmax", name, &vmax, 256, 0);
	cvCreateTrackbar("Smin", name, &smin, 256, 0);
	
}

Tracker::~Tracker() {
	
}

void Tracker::nextFrame() {
	IplImage *frame = cvQueryFrame(capture);
	if (!frame) {
		printf("error, could not query frame!");
		exit(-1);
	}
	
	if (!img) {
		initializeImageVariables(frame);
	}
	
	cvCopy(frame, img, 0);
	cvCvtColor(img, hsv, CV_BGR2HSV);
}

void Tracker::processMostRecentFrame() {
	if (track_object) {
		int _vmin = vmin, _vmax = vmax;
		cvInRangeS(hsv, cvScalar(0, smin, MIN(_vmin, _vmax), 0), cvScalar(180, 256, MAX(_vmin, _vmax), 0), mask);
		cvSplit(hsv, hue, 0, 0, 0);
		
		
		if( track_object < 0 ) {
			float max_val = 0.f;
			cvSetImageROI( hue, selection );
			cvSetImageROI( mask, selection );
			cvCalcHist( &hue, hist, 0, mask );
			cvGetMinMaxHistValue( hist, 0, &max_val, 0, 0 );
			cvConvertScale( hist->bins, hist->bins, max_val ? 255. / max_val : 0., 0 );
			cvResetImageROI( hue );
			cvResetImageROI( mask );
			track_window = selection;
			track_object = 1;
			
			cvZero( histimg );
			int bin_w = histimg->width / hdims;
			int j;
			for( j = 0; j < hdims; j++ )
			{
				int val = cvRound( cvGetReal1D(hist->bins,j)*histimg->height/255 );
				CvScalar color = hsv2rgb(j*180.f/hdims);
				cvRectangle( histimg, cvPoint(j*bin_w,histimg->height),
							cvPoint((j+1)*bin_w,histimg->height - val),
							color, -1, 8, 0 );
			}
		}
		
		cvCalcBackProject(&(hue), backproject, hist);
		cvAnd(backproject, mask, backproject, 0);
		
		if (true) {
			cvCvtColor(backproject, img, CV_GRAY2BGR);
		}
	}
}

void Tracker::handleSelectionEvents() {
	if (select_object && selection.width > 0 && selection.height > 0) {
		cvSetImageROI(img, selection);
		cvXorS(img, cvScalarAll(255), img, 0);
		cvResetImageROI(img);
	}
}

void Tracker::displayImage() {
	cvShowImage(name, img);
}

void Tracker::cleanUp() {
	cvReleaseCapture(&capture);
	cvDestroyWindow(name);
}

// Private Methods
void Tracker::initializeImageVariables(IplImage *frame) {
	img = cvCreateImage(cvGetSize(frame), 8, 3);
	img->origin = frame->origin;
	hsv = cvCreateImage(cvGetSize(frame), 8, 3);
	hue = cvCreateImage(cvGetSize(frame), 8, 1);
	mask = cvCreateImage(cvGetSize(frame), 8, 1);
	backproject = cvCreateImage(cvGetSize(frame), 8, 1);
	float *hranges = hranges_arr;
	hist = cvCreateHist(1, &hdims, CV_HIST_ARRAY, &hranges, 1);
	histimg = cvCreateImage(cvSize(320, 200), 8, 3);
	cvZero(histimg);
}


