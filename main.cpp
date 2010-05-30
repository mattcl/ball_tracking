#include <iostream>
#include <OpenCV/cv.h>
#include <OpenCV/highgui.h>
#include <stdio.h>
#include <ctype.h>
#include "tracker.h"
#include <sys/time.h>

extern "C" int connect_client(int* sockfd, char * buffer, const char * ip_addr);
extern "C" int read_client(int sockfd, char * buffer);
extern "C" int write_client(int sockfd, char * buffer, int length);

struct BallProfile {
	float vx;		// m/s
	float vy;		// m/s
	float vz;		// m/s
	float centerX;  // m
	float centerY;  // m
	float centerZ;  // m
	int stamp_c;
};

const char * def_ip = "192.168.2.4";

int connect = 1;

float front_width = 3.05;
float front_height = 2.06;
float top_width = 3.05;
float top_height = 3.05;
float x_offset = 2.21;
float y_offset = 0;
float z_offset = 0.32;
int offsetMaxMag = 500000;
float adjustmentFactor = 100000.0;

char ip_addr[20];

int debug = 1, debug_1 = 0, debug_2 = 0, debug_profile = 0;

void on_mouse(int event, int x, int y, int flags, void*param);
void onXOffsetChange(int pos);
void onYOffsetChange(int pos);
void onZOffsetChange(int pos);

int processArgs(int argc, char * const argv[]) {
	int found_ip = 0;
	for (int i = 1; i < argc; i++) {
		const char *s = argv[i];
		if (strcmp(s, "-fw") == 0) {
			if (sscanf(argv[++i], "%f", &front_width) != 1 || front_width <= 0) {
				return fprintf(stderr, "Invalid front camera width\n");
			}
		} else if (strcmp(s, "-fh") == 0) {
			if (sscanf(argv[++i], "%f", &front_height) != 1 || front_height <= 0) {
				return fprintf(stderr, "Invalid front camera height\n");
			}
		} else if (strcmp(s, "-tw") == 0) {
			if (sscanf(argv[++i], "%f", &top_width) != 1 || top_width <= 0) {
				return fprintf(stderr, "Invalid top camera width\n");
			}
		} else if (strcmp(s, "-th") == 0) {
			if (sscanf(argv[++i], "%f", &top_height) != 1 || top_height <= 0) {
				return fprintf(stderr, "Invalid top camera height\n");
			}
		} else if (strcmp(s, "-xo") == 0) {
			if (sscanf(argv[++i], "%f", &x_offset) != 1) {
				return fprintf(stderr, "Invalid x offset\n");
			}
		} else if (strcmp(s, "-yo") == 0) {
			if (sscanf(argv[++i], "%f", &y_offset) != 1) {
				return fprintf(stderr, "Invalid y offset\n");
			}
		} else if (strcmp(s, "-zo") == 0) {
			if (sscanf(argv[++i], "%f", &z_offset) != 1) {
				return fprintf(stderr, "Invalid z offset\n");
			}
		} else if (strcmp(s, "-ip") == 0) {
			if (sscanf(argv[++i], "%s", ip_addr) == 1) {
				found_ip = 1;
			} else {
				return fprintf(stderr, "Invalid server ip address\n");
			}
		} else if (strcmp(s, "-noc") == 0) {
			connect = 0;
			printf("skipping connection\n");
		}
	}
	if (!found_ip) {
		strcpy(ip_addr, def_ip);
		if(connect) {
			printf("No ip address provided for server. Defaulting to %s\n", ip_addr);
		}
	}
	return 0;
}

int main (int argc, char * const argv[]) {
	//3.75 width
	//2.65 height
	
	processArgs(argc, argv);
	
	int newsockfd;
	char buffer [256];
	if(connect) {
		printf("connecting to server: %s\n", ip_addr);
		connect_client(&newsockfd, buffer, ip_addr);
	}
	
	int trackbarX = x_offset*adjustmentFactor + offsetMaxMag;
	int trackbarY = y_offset*adjustmentFactor + offsetMaxMag;
	int trackbarZ = z_offset*adjustmentFactor + offsetMaxMag;
	
	cvNamedWindow("offset control", 0);
	cvCreateTrackbar("x offset", "offset control", &trackbarX, offsetMaxMag*2, onXOffsetChange);
	cvCreateTrackbar("y offset", "offset control", &trackbarY, offsetMaxMag*2, onYOffsetChange);
	cvCreateTrackbar("z offset", "offset control", &trackbarZ, offsetMaxMag*2, onZOffsetChange);
	
	Tracker frontCamera("front camera", 2, on_mouse, front_width, front_height);
	Tracker topCamera("top camera", 1, on_mouse, top_width, top_height);
	
	frontCamera.moveWindow(10, 50);
	topCamera.moveWindow(335, 50);
	cvMoveWindow("offset control", 660, 50);
	
	BallProfile currentProfile;
	
	int view = 1;
	
	printf("starting, pres ESC to quit\n");
	float startClock = clock();
	
	timeval tim;
	gettimeofday(&tim, NULL);
	double startTime = tim.tv_sec+(tim.tv_usec/1000000.0);
	
	long i = 0;
	int s = 0;
	int c;
	for(;;i++, s++) {
		frontCamera.nextFrame();
		topCamera.nextFrame();
		
		frontCamera.processMostRecentFrame();
		topCamera.processMostRecentFrame();
		
		frontCamera.handleSelectionEvents();
		topCamera.handleSelectionEvents();
		
		if (view) {
			frontCamera.displayImage();
			topCamera.displayImage();
		}
		
		if (frontCamera.isTracking() && topCamera.isTracking()) {
			gettimeofday(&tim, NULL);
			double curTime = tim.tv_sec+(tim.tv_usec/1000000.0);
			currentProfile.vx = (clock() - startClock)/CLOCKS_PER_SEC;
			currentProfile.vy = curTime - startTime;
			currentProfile.vz = frontCamera.getVyMeters();
			currentProfile.centerZ = (frontCamera.getPixelHeight() / 2 - frontCamera.getCenter().y)*frontCamera.getConversionY() + z_offset;
			currentProfile.centerY = 0;
			currentProfile.centerX = (frontCamera.getPixelWidth() / 2 - frontCamera.getCenter().x)*frontCamera.getConversionX() + x_offset;
		}
		
		if (debug) {
			//if(debug_1) frontCamera.printTrackedObjectProperties();
			//if(debug_2) topCamera.printTrackedObjectProperties();
			if(debug_profile && frontCamera.isTracking() && topCamera.isTracking()) {
				sprintf(buffer,"%35d %35d %35d %35d %35d %35d %35d   \n",
						(int)(s),
						(int)(1000000*currentProfile.centerX),
						(int)(1000000*currentProfile.centerY),
						(int)(1000000*currentProfile.centerZ),
						(int)(1000000*(currentProfile.vx)),
						(int)(1000000*(currentProfile.vy)),
						(int)(1000000*currentProfile.vz));

				//printf("%s", buffer);
				//memcpy(buffer, &currentProfile, sizeof(currentProfile));
				if (debug_1) {
					printf("%d pos (%f, %f, %f), vel (%f, %f, %f)\n", s, currentProfile.centerX, currentProfile.centerY, currentProfile.centerZ,
						   currentProfile.vx, currentProfile.vy, currentProfile.vz);
				}
				//printf("%s",buffer);
				if (connect) {
					try {
						write_client(newsockfd, buffer, strlen(buffer));
					}
					catch (...) {
						printf("terminating...\n");
						break;
					}
				}
			}
		}
		
		// check for end signal ("ESC" key)
		c = cvWaitKey(10);
		if ((char) c == 27) {
			break;
		}
		// handle keyboard inputs
		switch ((char) c) {
			case 'p':
				frontCamera.switchBackProjectMode();
				topCamera.switchBackProjectMode();
				break;
			case 'w':
				frontCamera.switchShowTrackingWindow();
				topCamera.switchShowTrackingWindow();
				break;
			case 'x':
				frontCamera.reset();
				topCamera.reset();
				break;
			case 'r':
			case 'g':
			case 'b':
			case 'n':
				frontCamera.switchColorMode(c);
				topCamera.switchColorMode(c);
				break;
			case '0':
				frontCamera.stopTracking();
				topCamera.stopTracking();
				break;
			case 'l':
				frontCamera.loadHistogramAndMask();
				topCamera.loadHistogramAndMask();
				break;
			case 'v':
				view ^= 1;
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
				printf("toggle transmitting %d\n", debug_profile);
				break;
			case '9':
				printf("current properties\n-------------------\n");
				printf("offsets (x, y, z): (%f, %f, %f)\n", x_offset, y_offset, z_offset);
				
				// compute average effective fps
				printf("average fps: %f\n\n", i / ((clock() - startClock) / CLOCKS_PER_SEC));
				
				// print slider values
				frontCamera.printSliderValues();
				topCamera.printSliderValues();
				break;

			default:
				break;
		}
	}
	
	printf("offsets (x, y, z): (%f, %f, %f)\n", x_offset, y_offset, z_offset);
	
	// compute average effective fps
	double ave = i / ((clock() - startClock) / CLOCKS_PER_SEC);
	printf("average fps: %f\n", ave);
	
	// print slider values
	frontCamera.printSliderValues();
	topCamera.printSliderValues();
	
	// clean up
	printf("cleaning up\n");
	frontCamera.cleanUp();
	topCamera.cleanUp();
    return 0;
}

void onXOffsetChange(int pos) {
	x_offset = (float) (pos - offsetMaxMag) / adjustmentFactor;
	//printf("x offset changed to: %f\n", x_offset);
}

void onYOffsetChange(int pos) {
	y_offset = (float) (pos - offsetMaxMag) / adjustmentFactor;
	//printf("y offset changed to: %f\n", y_offset);
}

void onZOffsetChange(int pos) {
	z_offset = (float) (pos - offsetMaxMag) / adjustmentFactor;
	//printf("z offset changed to: %f\n", z_offset);
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

