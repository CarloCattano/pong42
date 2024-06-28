#pragma once

#include "ofMain.h"

#include "ofxGui.h"
#include "ofxOpenCv.h"
#include "UIManager.h"

#define _USE_LIVE_VIDEO

class Particle {
public:
	glm::vec2   pos = { 0, 0 };
	glm::vec2   basePos = { 0, 0 };
	glm::vec2   vel = { 0, 0 };

	float       size = 30.0;
	float       timeNotTouched = 0.0f;
	bool        bAtBasePos = false;
};


class ofApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();

	glm::vec2 getOpticalFlowValueForPercent(float xpct, float ypct);

	void keyPressed(int key);

	void windowResized(int w, int h);
#ifdef _USE_LIVE_VIDEO
	ofVideoGrabber vidGrabber;
#else
	ofVideoPlayer vidPlayer;
#endif

	ofxCvColorImage colorImg;
	ofxCvGrayscaleImage grayImage;

	ofxCvGrayscaleImage currentImage;

	cv::Mat previousMat;
	cv::Mat flowMat;

	vector<Particle> particles;

	int     blurAmount;
	bool    bMirror;
	float   cvDownScale;
	bool    bContrastStretch;
	float   minLengthSquared;
	bool    bDrawOptiFlowVectors;
	int     mode;
    
private:
	unsigned short int WIN_W;
	unsigned short int WIN_H;

	float   particle_size;
	int     spacing;
	int     sine_distorsion;
	float   dist_freq;
    ofColor bgColor;

	void generateParticles(int w, int h);

	void spacingChanged(int & spacing);
	void particleSizeChanged(float & particle_size);
	void sineDistorsionChanged(int & sine_distorsion);
	void distFreqChanged(float & dist_freq);
 
    UIManager uiManager;
};
