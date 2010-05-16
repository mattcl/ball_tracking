/*
 *  Tracker.cpp
 *  CS225
 *
 */

#include "tracker.h"
#include <OpenCV/cv.h>
#include <OpenCV/highgui.h>


Tracker::Tracker(char* name, int cameraIndex, CvMouseCallback on_mouse, float physical_width, float physical_height) {
	// initialize all variables
	this->name = name;
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
	
	backproject_mode = 0;
	show_tracking_window = 0;
	
	first_iter = 1;
	dx = 0;
	dy = 0;
	dt = 0;
	
	width = physical_width;
	height = physical_height;
	
	
	
	start_clock = clock();
	
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
			start_clock = clock();
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
		
		cvCamShift(backproject, track_window, cvTermCriteria(CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 10, 1), &track_comp, &track_box);
		track_window = track_comp.rect;
		
		
		// draw the backprojection only
		if (backproject_mode) {
			cvCvtColor(backproject, img, CV_GRAY2BGR);
		}
		
		
		if(show_tracking_window) {
			// draw a rectangle representing the tracking window
			cvRectangle(img, cvPoint(track_window.x, track_window.y), 
						cvPoint(track_window.x + track_window.width, track_window.y + track_window.height), 
						CV_RGB(0, 0, 255), 2, CV_AA, 0);
		}
		
		// draw a circle around the center of the object
		cvCircle(img, cvPointFrom32f(track_box.center), 10, CV_RGB(225, 0, 0), 2, CV_AA, 0);
		
		if (!first_iter) {
			// compute dx and dy for this iteration
			dx = track_box.center.x - last_track_box.center.x;
			dy = -(track_box.center.y - last_track_box.center.y);
		} else {
			first_iter = 0;
		}
		float cclock = clock();
		dt = (cclock - start_clock) / CLOCKS_PER_SEC;
		start_clock = cclock;
		last_track_box = track_box;
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

void Tracker::printTrackedObjectProperties() {
	if (track_object) {
		printf("%s: Object at (%f, %f), dx = %f, dy = %f, dt = %f\n", name, track_box.center.x, track_box.center.y, dx, dy, dt);
	}
}

void Tracker::switchBackProjectMode() {
	backproject_mode ^= 1;
}

void Tracker::switchShowTrackingWindow() {
	show_tracking_window ^= 1;
}

void Tracker::reset() {
	if (track_object) {
		track_window.x = 5;
		track_window.y = 5;
		track_window.width = img->width - 10;
		track_window.height = img->height - 10;
	}
}

void Tracker::stopTracking() {
	track_object = 0;
	cvZero(histimg);
}

void Tracker::cleanUp() {
	cvReleaseCapture(&capture);
	cvDestroyWindow(name);
}

bool Tracker::isTracking() {
	return (track_object);
}

float Tracker::getPhysicalWidth() {
	return width;
}

float Tracker::getPhysicalHeight() {
	return height;
}

float Tracker::getDx() {
	return dx;
}

float Tracker::getDxMeters() {
	return dx * meters_per_pixel_x;
}

float Tracker::getVxMeters() {
	return getDxMeters() / dt;
}

float Tracker::getDy() {
	return dy;
}

float Tracker::getDyMeters() {
	return dy * meters_per_pixel_y;
}

float Tracker::getVyMeters() {
	return getDyMeters() / dt;
}

float Tracker::getDt() {
	return dt;
}

CvPoint2D32f Tracker::getCenter() {
	if(track_object)
		return track_box.center;
	return cvPoint2D32f(0, 0);
}

CvPoint2D32f Tracker::getCenterMeters() {
	CvPoint2D32f pt = getCenter();
	pt.x = width - pt.x * meters_per_pixel_x;
	pt.y = height - pt.y * meters_per_pixel_y;
	return pt;
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
	meters_per_pixel_x = width / cvGetSize(frame).width;
	meters_per_pixel_y = height / cvGetSize(frame).height;
	printProperties();
}

void Tracker::printProperties() {
	printf("%s: image size (pixels): %d x %d, physical size (m): %f x %f\n", 
		   name, cvGetSize(img).width, cvGetSize(img).height, width, height);
	printf("meters per pixel (x, y): (%f, %f)\n----\n", meters_per_pixel_x, meters_per_pixel_y);
}


