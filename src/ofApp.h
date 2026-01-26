#pragma once

#include "UIManager.h"
#include "ofTrueTypeFont.h"
#include "ofxPostProcessing.h"
#include "ofxOpenCv.h"
#include "ofxGui.h"
#include "Particles.h"

#define UI
#define USE_VIDEO_FILE // Define this macro to use a video file instead of the webcam

class ofApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();

	glm::vec2 getOpticalFlowValueForPercent(float xpct, float ypct);

	void keyPressed(int key);
	void windowResized(int w, int h);

#ifdef USE_VIDEO_FILE
	ofVideoPlayer videoPlayer;
#else
	ofVideoGrabber cam;
#endif

	ofxCvColorImage     colorImg;
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

	ofxPostProcessing post;
	ZoomBlurPass     *zoomBlur;
	EdgePass         *edgePass;

	ofShader               asciiShader;
	ofFbo                  particlesFbo;
	std::vector<ofTexture> fontTextures;

	std::vector<std::string> fontmaps;
	unsigned int             maps_count;
	unsigned int             counter;

	int sourceWidth;
	int sourceHeight;

	unsigned short int WIN_H;
	unsigned short int WIN_W;

private:
	bool bNewFrame;

	bool b_Ascii;

	float   particle_size;
	ofColor bgColor;

	float flowSensitivity;

	ofxCvGrayscaleImage depthOrig;
	ofxCvGrayscaleImage depthProcessed;
	ofxCvContourFinder  depthContours;
	ofxCvColorImage     colorImageRGB;

	float   s_asciiFontScale;
	ofVec2f atlasSize_grid;
	float   atlasCellSize;
	float   s_asciiCellScale;
	float   s_asciiCharsetOffset;
	float   s_asciiMix;

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
	void asciiSizeChanged(float &size);

	void drawParticles();
	void updateCamera();
	void AllocateImages();
	void processNewFrame();
	void calculateOpticalFlow();
	void updateParticles();
	void applyFlowToPlayers();

	void loadTextureFromFile(int index);
	void loadMapNames();

	UIManager uiManager;

	float scaleParameter(float param, float scale, float base = 0.0f) {
		return base + (param / 1000.0f) * scale;
	}

	float randDetectionSpeed;
};
