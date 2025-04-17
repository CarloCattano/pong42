#include "ofApp.h"
#include "Player.hpp"
#include "fwd.hpp"
#include "ofAppRunner.h"
#include "ofUtils.h"

ofxCvGrayscaleImage depthOrig;
ofxCvGrayscaleImage depthProcessed;
ofxCvContourFinder depthContours;
ofxCvColorImage colorImageRGB;

void ofApp::setup() {
	WIN_W = ofGetWidth();
	WIN_H = ofGetHeight();

	ofSetWindowShape(WIN_W, WIN_H);
	ofSetFrameRate(60);
	ofEnableAlphaBlending();
	ofSetWindowTitle("OpticalFlowYo!");
	ofBackground(bgColor);

	bgColor = ofColor(0, 0, 0, 255);

	particle_size = 2.0f;
	spacing = 14;
	flowSensitivity = 0.1f;
	blurAmount = 1;
	bMirror = true;
	cvDownScale = 16;
	bContrastStretch = true;

	// store a minimum squared value to apply flow velocity
	minLengthSquared = 0.8 * 0.8; // 0.5 pixel squared

	player1 = Player(ofVec2f(64, 120), ofVec2f(42, 320));
	player2 = Player(ofVec2f(WIN_W - 64, WIN_H / 2.0), ofVec2f(42, 320));

	ofTrueTypeFont::setGlobalDpi(72); // Default is 96, but results in larger than normal pt size.
	scoreBoard.load(ofToDataPath("verdana.ttf"), 42, true,
					true);			   // filename via ofToDataPath, point size, antialiased?, full char-set?
	scoreBoard.setLineHeight(28.0);	   // Default is based on font size.
	scoreBoard.setLetterSpacing(1.05); // Default is based on font size.

	fpsFont.load(ofToDataPath("verdana.ttf"), 22, true, true);

	cam.setVerbose(true);
	cam.setup(1280, 720);

	sourceWidth = cam.getWidth();
	sourceHeight = cam.getHeight();

	depthOrig.allocate(sourceWidth, sourceHeight);
	depthProcessed.allocate(sourceWidth, sourceHeight);

	colorImg.allocate(sourceWidth, sourceHeight);

	uiManager.spacing_s.addListener(this, &ofApp::spacingChanged);
	uiManager.particle_size_s.addListener(this, &ofApp::particleSizeChanged);

	uiManager.setup();

	// Setup post-processing chain
	post.init(ofGetWidth(), ofGetHeight());
	post.createPass<FxaaPass>()->setEnabled(true);
	post.createPass<BloomPass>()->setEnabled(true);
	post.createPass<ZoomBlurPass>()->setEnabled(true);
	post.createPass<GodRaysPass>()->setEnabled(true);
	post.createPass<ConvolutionPass>()->setEnabled(true);

	zoomBlur = dynamic_cast<ZoomBlurPass *>(post[2].get());
	zoomBlur->setExposure(0.5);

	///////////////////////////////////////////////////////
}
//-----------------------------------------------------------------------------------------------------------
void ofApp::update() {
	updateCamera();

	AllocateImages();

	if (bNewFrame) {
		processNewFrame();
		calculateOpticalFlow();
	}

	float deltaTime = ofClamp(ofGetLastFrameTime(), 1.f / 5000.f, 1.f / 5.f);

	updateParticles(deltaTime);
	applyFlowToPlayers();

	player1.update();
	player2.update();

	collision();
}


//-----------------------------------------------------------------------------------------------------------

void ofApp::drawParticles() {
	size_t numParticles = particles.size();
	const ofPixels &vpix = colorImg.getPixels();

	for (size_t i = 0; i < numParticles; i++) {
		auto &particle = particles[i];
		int samplex = particle.pos.x;
		if (bMirror)
			samplex = colorImg.getWidth() - samplex;

		int sampley = particle.pos.y;

		if (samplex >= 0 && samplex < (int)vpix.getWidth() && sampley >= 0 && sampley < (int)vpix.getHeight()) {
			ofFloatColor vcolor = vpix.getColor(samplex, sampley);
			ofFloatColor color(1, 1, 1, 1);
			color = vcolor;
			ofSetColor(color);
			ofPushMatrix();

			float xmult = WIN_W / colorImg.getWidth();
			float ymult = WIN_H / colorImg.getHeight();

			float psize = particle.size * particle_size * (vcolor.getBrightness() * 0.8f + 0.2f);

			ofTranslate(particle.pos.x * xmult, particle.pos.y * ymult);
			ofDrawCircle(0, 0, psize);
			ofPopMatrix();
		}
	}
}

void ofApp::draw() {
	post.begin();

	ofBackgroundGradient(ofColor(0), bgColor);
	ofSetColor(255);

	if (grayImage.bAllocated)
		drawParticles();

	post.end();

	player1.draw();
	player2.draw();

	ball.draw();
	ball.move(player1, player2);


	ofSetColor(255, 255, 255);
	scoreBoard.drawString("SCORE : " + ofToString(player1.score) + " | " + ofToString(player2.score), WIN_W / 3.0, 40);

	ofSetColor(25, 200, 111);
	fpsFont.drawString(ofToString((int)ofGetFrameRate()) + " FPS", WIN_W / 1.2, 30);

	ofSetColor(bgColor);


	float centOffX = ball.pos.x / WIN_W;
	float centOffY = ball.pos.y / WIN_H;
	centOffY = 1.0 - centOffY;


	zoomBlur->setCenterX(centOffX);
	zoomBlur->setCenterY(centOffY);

#ifdef UI
	uiManager.draw();
#endif
}

//-----------------------------------------------------------------------------------------------------------

void ofApp::generateParticles(int s_width, int s_height) {
	particles.clear();

	int numx = s_width / spacing;
	int numy = s_height / spacing;

	float delta = 1.0f;

	for (int x = 0; x < numx; x++) {
		for (int y = 0; y < numy; y++) {
			glm::vec2 pos(spacing * delta + (float)x * spacing, spacing * delta + (float)y * spacing);

			Particle particle;

			particle.pos = pos;
			particle.size = spacing;
			particle.basePos = particle.pos;
			particles.push_back(particle);
		}
	}
	ofLogNotice() << "Particles generated: " << particles.size();
}


void ofApp::updateCamera() {
	cam.update();
	bNewFrame = cam.isFrameNew();

	colorImageRGB = cam.getPixels();
	depthOrig = colorImageRGB;
}


void ofApp::AllocateImages() {
	int scaledWidth = sourceWidth / cvDownScale;
	int scaledHeight = sourceHeight / cvDownScale;

	if (currentImage.getWidth() != scaledWidth || currentImage.getHeight() != scaledHeight) {
		previousMat = cv::Mat(scaledHeight, scaledWidth, CV_8UC1);
		flowMat = cv::Mat(scaledHeight, scaledWidth, CV_32FC2);
		currentImage.clear();
		currentImage.allocate(scaledWidth, scaledHeight);
		currentImage.set(0);
		previousMat.release();
		currentImage.getCvMat().copyTo(previousMat);
		flowMat.release();
		flowMat = cv::Mat(scaledHeight, scaledWidth, CV_32FC2);

		generateParticles(sourceWidth, sourceHeight);
	}
}


void ofApp::processNewFrame() {
	colorImg.setFromPixels(cam.getPixels());
	grayImage = colorImg;

	if (bMirror)
		grayImage.mirror(false, true);

	currentImage.scaleIntoMe(grayImage);

	if (bContrastStretch)
		currentImage.contrastStretch();

	if (blurAmount > 0)
		currentImage.blurGaussian(blurAmount);
}


void ofApp::calculateOpticalFlow() {
	cv::Mat currentMat = currentImage.getCvMat();
	cv::calcOpticalFlowFarneback(previousMat, currentMat, flowMat, 0.5, 4, 4, 2, 4, 1.2,
								 cv::OPTFLOW_FARNEBACK_GAUSSIAN);

	currentMat.copyTo(previousMat);
}


void ofApp::updateParticles(float deltaTime) {
	glm::vec2 leftFlowVector(0, 0);
	glm::vec2 rightFlowVector(0, 0);
	size_t numParticles = particles.size();

	for (auto &particle : particles) {
		float percentX = particle.pos.x / sourceWidth;
		float percentY = particle.pos.y / sourceHeight;
		glm::vec2 flowForce = getOpticalFlowValueForPercent(percentX, percentY);

		if (particle.pos.x + particle.size < sourceWidth / 2.0)
			leftFlowVector += flowForce;
		else
			rightFlowVector += flowForce;

		float len2 = glm::length2(flowForce);
		particle.vel /= 1.f + deltaTime;
		if (len2 > minLengthSquared) {
			particle.vel += flowForce * (20.0f * deltaTime);
			if (particle.bAtBasePos)
				particle.timeNotTouched = 0.0f;
			particle.bAtBasePos = false;
		} else {
			particle.timeNotTouched += deltaTime;
		}

		if (particle.timeNotTouched > 2.0) {
			particle.timeNotTouched = 0.0;
			if (!particle.bAtBasePos) {
				particle.pos = particle.basePos;
				particle.vel = { 0, 0 };
			}
			particle.bAtBasePos = true;
		}
		particle.pos += particle.vel * (10.0f * deltaTime);
	}

	leftFlowVector /= numParticles / 2;
	rightFlowVector /= numParticles / 2;

	this->leftFlowVector = leftFlowVector;
	this->rightFlowVector = rightFlowVector;
}


//-----------------------------------------------------------------------------------------------------------

void ofApp::applyFlowToPlayers() {
	if (leftFlowVector.x > flowSensitivity)
		player1.move(ofVec2f(0, player1.speed));
	else if (leftFlowVector.x < -flowSensitivity)
		player1.move(ofVec2f(0, -player1.speed));

	if (rightFlowVector.x > flowSensitivity)
		player2.move(ofVec2f(0, player2.speed));
	else if (rightFlowVector.x < -flowSensitivity)
		player2.move(ofVec2f(0, -player2.speed));
}


//-----------------------------------------------------------------------------------------------------------
glm::vec2 ofApp::getOpticalFlowValueForPercent(float xpct, float ypct) {
	glm::vec2 flowVector(0, 0);

	if (flowMat.empty() || !grayImage.bAllocated) {
		return flowVector;
	}

	int tx = xpct * (float)flowMat.cols;
	int ty = ypct * (float)flowMat.rows;

	if (tx >= flowMat.cols) {
		tx = flowMat.cols - 1;
	}

	if (ty >= flowMat.rows) {
		ty = flowMat.rows - 1;
	}
	if (tx < 0)
		tx = 0;
	if (ty < 0)
		ty = 0;

	const cv::Point2f &fxy = flowMat.at<cv::Point2f>(ty, tx);

	flowVector = glm::vec2(fxy.x, fxy.y);
	if (glm::length2(flowVector) > minLengthSquared) {
		return flowVector;
	}
	return glm::vec2(0.0, 0.0);
}

void ofApp::collision() {
	if (ball.pos.x - ball.size.x / 2 < player1.pos.x + player1.size.x / 2 &&
		ball.pos.x + ball.size.x / 2 > player1.pos.x - player1.size.x / 2 &&
		ball.pos.y - ball.size.y / 2 < player1.pos.y + player1.size.y / 2 &&
		ball.pos.y + ball.size.y / 2 > player1.pos.y - player1.size.y / 2) {
		ball.dir.x *= -1;
	}

	if (ball.pos.x - ball.size.x / 2 < player2.pos.x + player2.size.x / 2 &&
		ball.pos.x + ball.size.x / 2 > player2.pos.x - player2.size.x / 2 &&
		ball.pos.y - ball.size.y / 2 < player2.pos.y + player2.size.y / 2 &&
		ball.pos.y + ball.size.y / 2 > player2.pos.y - player2.size.y / 2) {
		ball.dir.x *= -1;
	}
}


// CALLBACKS
//-----------------------------------------------------------------------------------------------------------
void ofApp::keyPressed(int key) {
	unsigned idx = key - '0';
	if (idx < post.size()) {
		post[idx]->setEnabled(!post[idx]->getEnabled());
		return;
	}

	switch (key) {
		case OF_KEY_UP:
			cvDownScale += 1.f;
			break;
		case OF_KEY_DOWN:
			cvDownScale -= 1.0f;
			if (cvDownScale < 2) {
				cvDownScale = 2;
			}
			break;
		case 's':
			spacing += 1;
			spacingChanged(spacing);
			break;
		case 'a':
			spacing -= 1;
			spacingChanged(spacing);
			break;
		case 'p':
			particle_size += 1.0;
			particle_size > 30.0 ? particle_size = 30.0 : particle_size;
			break;
		case 'o':
			particle_size -= 1.0;
			particle_size < 1.0 ? particle_size = 1.0 : particle_size;
			break;
		default:
			break;
	}
}

//-----------------------------------------------------------------------------------------------------------

void ofApp::windowResized(int w, int h) {
	WIN_W = w;
	WIN_H = h;

	ofSetWindowShape(WIN_W, WIN_H);
}

void ofApp::spacingChanged(int &spacing) {
	this->spacing = spacing;
	generateParticles(WIN_W, WIN_H);
}

void ofApp::particleSizeChanged(float &particle_size) {
	this->particle_size = particle_size;
}
