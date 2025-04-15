#pragma once

#include "Ball.hpp"
#include "Player.hpp"
#include "UIManager.h"
#include "ofMain.h"
#include "ofTrueTypeFont.h"
#include "ofxGui.h"
#include "ofxOpenCv.h"

/*#define JOYSTICK*/

#define PARTICLES
#define UI

#ifdef JOYSTICK
	#include "ofxJoystick.h"
#endif

// #define KINECT

#define _USE_LIVE_VIDEO

class Particle {
public:
	glm::vec2 pos = { 0, 0 };
	glm::vec2 basePos = { 0, 0 };
	glm::vec2 vel = { 0, 0 };

	float size = 20.0;
	float timeNotTouched = 0.0f;
	bool bAtBasePos = false;
};

class ofApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();

	glm::vec2 getOpticalFlowValueForPercent(float xpct, float ypct);

	void keyPressed(int key);

	void windowResized(int w, int h);

#ifdef KINECT
	ofxKinect cam;
#endif

#ifdef _USE_LIVE_VIDEO
	ofVideoGrabber cam;
#else
	//ofVideoPlayer vidPlayer;
#endif

	ofxCvColorImage colorImg;
	ofxCvGrayscaleImage grayImage;

	ofxCvGrayscaleImage currentImage;

	cv::Mat previousMat;
	cv::Mat flowMat;

	vector<Particle> particles;

	int blurAmount;
	bool bMirror;
	float cvDownScale;
	bool bContrastStretch;
	float minLengthSquared;
	bool bDrawOptiFlowVectors;
	int mode;
	int spacing;

	Player player1;
	Player player2;

	Ball ball;

	ofVec2f player1flow;
	ofVec2f player2flow;

	ofTrueTypeFont scoreBoard;

private:
	unsigned short int WIN_H;
	unsigned short int WIN_W;

	float particle_size;
	ofColor bgColor;

	float flowSensitivity;

	ofTrueTypeFont font;
	ofTrueTypeFont fpsFont;

	void calculateNeighbors();
	void generateParticles(int w, int h);

	void spacingChanged(int & spacing);
	void particleSizeChanged(float & particle_size);

	void collision();

	int sourceWidth;
	int sourceHeight;

	UIManager uiManager;
#ifdef JOYSTICK
	ofxJoystick joy_;
#endif
};
