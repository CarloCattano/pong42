#pragma once

#include "ofMain.h"

#include "ofxGui.h"
#include "ofxOpenCv.h"
#include "UIManager.h"

#define _USE_LIVE_VIDEO

class SpinCube {
public:
	glm::vec2   pos = { 0, 0 };
	float       xAngleAcc = 0.0;
	float       xAxisAngle = 0.0f;
	float       yAxisAngle = 0.0f;
	float       yAngleAcc = 0.0;
	float       size = 30.0;
};

class Particle {
public:
	glm::vec2   pos = { 0, 0 };
	glm::vec2   basePos = { 0, 0 };
	glm::vec2   vel = { 0, 0 };

	float       size = 30.0;
	float       timeNotTouched = 0.0f;
	bool        bAtBasePos = false;
};

class FlowLine {
public:
	glm::vec2   pos = { 0, 0 };
	glm::vec2   flow = { 1, 0 };
	glm::vec2   target = { 1, 0 };
	float       targetAngle = 0.0;
	float       angle = 0.0f;
	float       brightness = 0.5;
	float       size = 30.0;
};

class ofApp : public ofBaseApp {
public:
	enum AppMode {
		MODE_SPIN_CUBES = 0,
		MODE_PARTICLES,
		MODE_LINES
	};

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
	ofxCvGrayscaleImage previousImage;
	ofxCvGrayscaleImage diffImage;

	cv::Mat previousMat;
	cv::Mat flowMat;

	vector<SpinCube> spinCubes;
	vector<Particle> particles;
	vector<FlowLine> flowLines;

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
    int     motionAmount;
    bool    getMotion;
    ofColor bgColor;

	void generateParticles(int w, int h);

	void spacingChanged(int & spacing);
	void particleSizeChanged(float & particle_size);
	void sineDistorsionChanged(int & sine_distorsion);
	void distFreqChanged(float & dist_freq);
	/**/
	/*ofxPanel        gui;*/
	/*ofxIntSlider    spacing_s;*/
	/*ofxFloatSlider  particle_size_s;*/
	/*ofxIntSlider    sine_distorsion_s;*/
	/*ofxFloatSlider  dist_freq_s;*/

    UIManager uiManager;
};
