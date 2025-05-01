#pragma once

#include "Ball.hpp"
#include "Player.hpp"
#include "UIManager.h"
#include "ofMain.h"
#include "ofTrueTypeFont.h"
#include "ofWebSocket.h"
#include "ofxGui.h"
#include "ofxOpenCv.h"
#include "ofxPostProcessing.h"

#include "GameManager.h"
#include "Particles.h"
#include "yolo5ImageClassify.h"

#define UI

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

	ParticleSystem particleSystem;

	ofShader particleShader;

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

	ofxPostProcessing post;
	ZoomBlurPass *zoomBlur;
	EdgePass *edgePass;

	ofShader asciiShader;
	ofFbo particlesFbo;
	ofTexture asciiAtlas;

	ofWebSocket webSocket;
	std::unordered_map<std::string, std::function<void(float)> > sliderHandlers;
	std::unordered_map<std::string, std::function<void(int)> > togglesHandlers;

	std::vector<std::string> fontmaps;
	unsigned int maps_count;
	unsigned int counter;

	yolo5ImageClassify classify;
	vector<yolo5ImageClassify::Result> results;

	int sourceWidth;
	int sourceHeight;

	unsigned short int WIN_H;
	unsigned short int WIN_W;

private:
	bool bNewFrame;

	bool b_Ascii;

	float particle_size;
	ofColor bgColor;

	float flowSensitivity;

	ofxCvGrayscaleImage depthOrig;
	ofxCvGrayscaleImage depthProcessed;
	ofxCvContourFinder depthContours;
	ofxCvColorImage colorImageRGB;

	float s_asciiFontScale;
	ofVec2f atlasSize_grid;
	float atlasCellSize;
	float s_asciiCharsetOffset;
	float s_asciiMix;

	glm::vec2 leftFlowVector;
	glm::vec2 rightFlowVector;

	ofTrueTypeFont font;

	void calculateNeighbors();
	void generateParticles(int w, int h);

	void spacingChanged(int &spacing);
	void particleSizeChanged(float &particle_size);
	void postProcessingChanged(float &exposure);
	void asciiSpreadChanged(int &spread);
	void asciiOffsetChanged(int &offset);
	void asciiMixChanged(float &mix);

	void collision();

	void drawParticles();
	void updateCamera();
	void AllocateImages();
	void processNewFrame();
	void calculateOpticalFlow();
	void updateParticles();
	void applyFlowToPlayers();

	void drawDetectedObjects();

	void loadTextureFromFile(int index);
	void loadMapNames();


	UIManager uiManager;

	std::optional<GameManager> gameManager;

	float scaleParameter(float param, float scale, float base = 0.0f) {
		return base + (param / 1000.0f) * scale;
	}
};
