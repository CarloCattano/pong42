#include "ofApp.h"
#include "Player.hpp"
#include "fwd.hpp"
#include "ofAppRunner.h"
#include "ofUtils.h"

ofxCvGrayscaleImage		depthOrig;
ofxCvGrayscaleImage		depthProcessed;
ofxCvContourFinder		depthContours;

ofxCvColorImage			colorImageRGB;


void ofApp::setup() {
	WIN_W = ofGetWidth();
	WIN_H = ofGetHeight();

	ofSetWindowShape(WIN_W, WIN_H);
	ofSetFrameRate(30);
	ofEnableAlphaBlending();
	ofSetWindowTitle("OpticalFlowYo!");
	ofBackground(bgColor);

	bgColor = ofColor(0, 0, 0, 255);

	particle_size = 2.0f;
	spacing = 8;

	flowSensitivity = 0.24f;
	blurAmount = 1;
	bMirror = true;
	cvDownScale = 16;
	bContrastStretch = true;

	// store a minimum squared value to apply flow velocity
	minLengthSquared = 0.5 * 0.5; // 0.5 pixel squared

	player1 = Player(ofVec2f(40, 120), ofVec2f(24, 224));
	player2 = Player(ofVec2f(WIN_W - 40, WIN_H / 2.0), ofVec2f(24, 224));

	player1.playerAI = false;
	player2.playerAI = false;

	ofTrueTypeFont::setGlobalDpi(72); // Default is 96, but results in larger than normal pt size.
	scoreBoard.load(ofToDataPath("verdana.ttf"), 42, true, true); // filename via ofToDataPath, point size, antialiased?, full char-set?
	scoreBoard.setLineHeight(28.0); // Default is based on font size.
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
}

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
		ofLogNotice() << "Particles generated: " << particles.size();
	}
}

//-----------------------------------------------------------------------------------------------------------

void ofApp::update() {

	bool bNewFrame = false;

	cam.update();
	bNewFrame = cam.isFrameNew();

	colorImageRGB			= cam.getPixels();
	depthOrig = colorImageRGB;


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

		ofSetWindowShape(WIN_W, WIN_H);
		generateParticles(sourceWidth, sourceHeight);
	}

	if (bNewFrame) {

		colorImg.setFromPixels(cam.getPixels());
		grayImage = colorImg;

		if (bMirror)
			grayImage.mirror(false, true);

		currentImage.scaleIntoMe(grayImage);

		if (bContrastStretch)
			currentImage.contrastStretch();

		if (blurAmount > 0)
			currentImage.blurGaussian(blurAmount);

		cv::Mat currentMat = currentImage.getCvMat();
		cv::calcOpticalFlowFarneback(previousMat,
			currentMat,
			flowMat,
			0.5, // pyr_scale
			4, // levels
			4, // winsize
			2, // iterations
			4, // poly_n
			1.2, // poly_sigma
			cv::OPTFLOW_FARNEBACK_GAUSSIAN);

		currentMat.copyTo(previousMat);
	}

#ifdef PARTICLES
	float deltaTime = ofClamp(ofGetLastFrameTime(), 1.f / 5000.f, 1.f / 5.f);
#endif
	size_t numParticles = particles.size();

	glm::vec2 leftFlowVector(0, 0);
	glm::vec2 rightFlowVector(0, 0);

	for (size_t i = 0; i < numParticles; i++) {
		auto & particle = particles[i];
		float percentX = particle.pos.x / sourceWidth;
		float percentY = particle.pos.y / sourceHeight;
		glm::vec2 flowForce = getOpticalFlowValueForPercent(percentX, percentY);

		if (particle.pos.x + particle.size < sourceWidth / 2.0)
			leftFlowVector += flowForce;
		else
			rightFlowVector += flowForce;

#ifdef PARTICLES
		float len2 = glm::length2(flowForce);
		particle.vel /= 1.f + deltaTime;
		if (len2 > minLengthSquared) {
			particle.vel += flowForce * (20.0f * deltaTime);
			if (particle.bAtBasePos) {
				particle.timeNotTouched = 0.0f;
			}
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
#endif
	}

	leftFlowVector /= numParticles / 2;
	rightFlowVector /= numParticles / 2;

	if (leftFlowVector.x > flowSensitivity)
		player1.move(ofVec2f(0, 4));

	else if (leftFlowVector.x < -flowSensitivity)
		player1.move(ofVec2f(0, -4));

	if (rightFlowVector.x > flowSensitivity)
		player2.move(ofVec2f(0, -4));

	else if (rightFlowVector.x < -flowSensitivity)
		player2.move(ofVec2f(0, 4));

	player1.update();
	player2.update();

#ifdef JOYSTICK
	if (joy_.getAxis(1) < -0.5)
		player1.move(ofVec2f(0, -7));
	else if (joy_.getAxis(1) > 0.5)
		player1.move(ofVec2f(0, 7));

#endif

	collision();

	if (player1.playerAI) {
		if (ofGetFrameNum() % (int)ofRandom(2, 7) == 0) {
			player1.direction.y += ofRandom(-8, 8);
			player2.move(player1.direction);
		} else
			player1.chaseBall(ball.pos);
	}
	if (player2.playerAI) {
		if (ofGetFrameNum() % (int)ofRandom(2, 7) == 0) {
			player2.direction.y += ofRandom(-8, 8);
			player2.move(player2.direction);
		} else
			player2.chaseBall(ball.pos);
	}
}

//-----------------------------------------------------------------------------------------------------------

void ofApp::draw() {

	ofBackgroundGradient(ofColor(0), bgColor);
	ofSetColor(255);

#ifdef PARTICLES
	if (grayImage.bAllocated) {

		size_t numParticles = particles.size();
		const ofPixels & vpix = colorImg.getPixels();

		for (size_t i = 0; i < numParticles; i++) {
			auto & particle = particles[i];
			int samplex = particle.pos.x;

			if (bMirror)
				samplex = colorImg.getWidth() - samplex;

			if (particle.pos.x < 0 || particle.pos.x > WIN_W || particle.pos.y < 0 || particle.pos.y > WIN_H) {
				continue;
			}

			ofFloatColor vcolor = vpix.getColor(samplex, particle.pos.y);
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
#endif

	ball.draw();
	player1.draw();
	player2.draw();

	ball.move(player1, player2);
	//colorImageRGB.draw(0, 0, ofGetWidth() / 4, ofGetHeight() / 4 );
	ofColor(255, 255, 255);
	cam.draw(242, 42, WIN_W / 6.0, WIN_H / 6.0);


	ofSetColor(255, 255, 255);
	scoreBoard.drawString("SCORE : " + ofToString(player1.score) + " | " + ofToString(player2.score), WIN_W / 3.0, 40);

	ofSetColor(25, 200, 111);
	fpsFont.drawString(ofToString((int)ofGetFrameRate()) + " FPS", WIN_W / 1.2, 30);

	ofSetColor(bgColor);

#ifdef UI
	uiManager.draw();
#endif
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
	if (tx < 0) tx = 0;
	if (ty < 0) ty = 0;

	const cv::Point2f & fxy = flowMat.at<cv::Point2f>(ty, tx);

	flowVector = glm::vec2(fxy.x, fxy.y);
	if (glm::length2(flowVector) > minLengthSquared) {
		return flowVector;
	}
	return glm::vec2(0.0, 0.0);
}

void ofApp::collision() {
	// Check collision with player1
	if (ball.pos.x - ball.size.x / 2 < player1.pos.x + player1.size.x / 2 && ball.pos.x + ball.size.x / 2 > player1.pos.x - player1.size.x / 2 && ball.pos.y - ball.size.y / 2 < player1.pos.y + player1.size.y / 2 && ball.pos.y + ball.size.y / 2 > player1.pos.y - player1.size.y / 2) {
		ball.dir.x *= -1;
	}

	// Check collision with player2 (if player2 is implemented)
	if (ball.pos.x - ball.size.x / 2 < player2.pos.x + player2.size.x / 2 && ball.pos.x + ball.size.x / 2 > player2.pos.x - player2.size.x / 2 && ball.pos.y - ball.size.y / 2 < player2.pos.y + player2.size.y / 2 && ball.pos.y + ball.size.y / 2 > player2.pos.y - player2.size.y / 2) {
		ball.dir.x *= -1;
	}
}

//-----------------------------------------------------------------------------------------------------------
void ofApp::keyPressed(int key) {
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
		particle_size += 0.1;
		particle_size > 40.0 ? particle_size = 40.0 : particle_size;
		break;
	case 'o':
		particle_size -= 0.1;
		particle_size < 0.01 ? particle_size = 0.01 : particle_size;
		break;
	case ' ':
		break;
	}
}

//-----------------------------------------------------------------------------------------------------------

void ofApp::windowResized(int w, int h) {
	WIN_W = w;
	WIN_H = h;


	ofSetWindowShape(WIN_W, WIN_H);
}

void ofApp::spacingChanged(int & spacing) {
	this->spacing = spacing;
	generateParticles(WIN_W, WIN_H);
}

void ofApp::particleSizeChanged(float & particle_size) {
	this->particle_size = particle_size;
}

