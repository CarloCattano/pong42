#pragma once

#include "Ball.hpp"
#include "Player.hpp"
#include "UIManager.h"
#include "ofMain.h"
#include "ofTrueTypeFont.h"
#include "ofxGui.h"
#include "ofxOpenCv.h"
#include "ofxPostProcessing.h"

#define PARTICLES
// #define UI

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

	ofVideoGrabber cam;

	ofxCvColorImage colorImg;
	ofxCvGrayscaleImage grayImage;

	ofxCvGrayscaleImage currentImage;

	cv::Mat previousMat;
	cv::Mat flowMat;

	vector<Particle> particles;

	bool bMirror;
	bool bContrastStretch;
	bool bDrawOptiFlowVectors;

	float cvDownScale;
	float minLengthSquared;

	int mode;
	int blurAmount;
	int spacing;

	Player player1;
	Player player2;

	Ball ball;

	ofTrueTypeFont scoreBoard;
	ofxPostProcessing post;
	ZoomBlurPass *zoomBlur;

	ofShader asciiShader;
	ofFbo particlesFbo;
	ofTexture asciiAtlas;

private:
	unsigned short int WIN_H;
	unsigned short int WIN_W;

	bool bNewFrame;

	float particle_size;
	ofColor bgColor;

	float flowSensitivity;

	glm::vec2 leftFlowVector;
	glm::vec2 rightFlowVector;

	ofTrueTypeFont font;
	ofTrueTypeFont fpsFont;

	void calculateNeighbors();
	void generateParticles(int w, int h);

	void spacingChanged(int &spacing);
	void particleSizeChanged(float &particle_size);

	void collision();

	void drawParticles();
	void updateCamera();
	void AllocateImages();
	void processNewFrame();
	void calculateOpticalFlow();
	void updateParticles(float deltaTime);
	void applyFlowToPlayers();

	int sourceWidth;
	int sourceHeight;

	UIManager uiManager;
};
