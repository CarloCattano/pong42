#include "ofApp.h"
#include <functional>
#include <unordered_map>
#include "EdgePass.h"
#include "Player.hpp"
#include "fwd.hpp"
#include "ofAppRunner.h"
#include "ofUtils.h"

ofxCvGrayscaleImage depthOrig;
ofxCvGrayscaleImage depthProcessed;
ofxCvContourFinder depthContours;
ofxCvColorImage colorImageRGB;

void ofApp::loadMapNames() {
	ofDirectory dir;
	dir.allowExt("png");
	dir.listDir("fontmaps");
	dir.sort();
	maps_count = dir.size() - 1;
	fontmaps.resize(maps_count);
	for (unsigned int i = 0; i < maps_count - 1; i++) {
		fontmaps[i] = dir.getPath(i);
	}
}

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

	ofVec2f paddleSize(64, 256);
	float centeredY = (WIN_H - paddleSize.y) / 2.0;

	player1 = Player(ofVec2f(64, centeredY), paddleSize);
	player2 = Player(ofVec2f(WIN_W - 64, centeredY), paddleSize);

	ofTrueTypeFont::setGlobalDpi(72); // Default is 96, but results in larger than normal pt size.
	scoreBoard.load(ofToDataPath("verdana.ttf"), 42, true, true);
	scoreBoard.setLineHeight(28.0);
	scoreBoard.setLetterSpacing(1.05);

	fpsFont.load(ofToDataPath("verdana.ttf"), 22, true, true);

	cam.setVerbose(false);
	cam.setup(1280, 720);

	sourceWidth = cam.getWidth();
	sourceHeight = cam.getHeight();

	depthOrig.allocate(sourceWidth, sourceHeight);
	depthProcessed.allocate(sourceWidth, sourceHeight);
	colorImg.allocate(sourceWidth, sourceHeight);

	// Setup post-processing chain
	post.init(WIN_W, WIN_H);

	post.createPass<BloomPass>()->setEnabled(false);
	post.createPass<ZoomBlurPass>()->setEnabled(false);
	post.createPass<EdgePass>()->setEnabled(false);

	zoomBlur = dynamic_cast<ZoomBlurPass *>(post[1].get());
	edgePass = dynamic_cast<EdgePass *>(post[2].get());

#ifdef UI
	uiManager.spacing_s.addListener(this, &ofApp::spacingChanged);
	uiManager.particle_size_s.addListener(this, &ofApp::particleSizeChanged);

	uiManager.exposure_s.addListener(this, &ofApp::postProcessingChanged);
	uiManager.weight_s.addListener(this, &ofApp::postProcessingChanged);

	uiManager.spread_s.addListener(this, &ofApp::asciiSpreadChanged);
	uiManager.asciiOffset_s.addListener(this, &ofApp::asciiOffsetChanged);
	uiManager.asciiMix_s.addListener(this, &ofApp::asciiMixChanged);

	uiManager.setup();
#endif

	zoomBlur->setExposure(0.25);
	zoomBlur->setWeight(0.6);
	zoomBlur->setDecay(0.9);
	zoomBlur->setDensity(0.1);

	loadMapNames();

	b_Ascii = true;
	asciiShader.load("shaders/ascii.vert", "shaders/ascii.frag");
	s_asciiFontScale = 2.0f;

	atlasSize_grid = ofVec2f(8.0f, 8.0f);
	atlasCellSize = 32.0f;
	asciiAtlas.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST); // Prevents blurring
	ofLoadImage(asciiAtlas, "fontmaps/edges.png");
	particlesFbo.allocate(WIN_W, WIN_H, GL_RGBA);

	// WEBSOCKET WIP
	webSocket.onMessage = [&](const std::string &msg) { ofLogNotice() << "Received: " << msg; };
	webSocket.connect("ws://ws.42ls.online/of-ws");

	sliderHandlers = {
		{ "slider_0",
		  [this](float val) {
			  spacing = scaleParameter(val, 128.0f, 10.0f);
			  spacing = ofClamp(spacing, 10.0f, 128.0f);
			  generateParticles(WIN_W, WIN_H);
		  } },
		{ "slider_1", [this](float val) { particle_size = scaleParameter(val, 5.0f, 0.1f); } },
		{ "slider_2", [this](float val) { zoomBlur->setWeight(scaleParameter(val, 1.5f, 0.5)); } },
		{ "slider_3", [this](float val) { zoomBlur->setDecay(scaleParameter(val, 0.9f)); } },
		{ "slider_4", [this](float val) { zoomBlur->setExposure(scaleParameter(val, 1.0f)); } },
		{ "slider_5", [this](float val) { zoomBlur->setDensity(scaleParameter(val, 0.1f)); } },
		{ "slider_6", [this](float val) { s_asciiFontScale = scaleParameter(val, 120.0f, 1.0); } },
		{ "slider_7", [this](float val) { s_asciiCharsetOffset = scaleParameter(val, 64.0); } },
		{ "slider_8", [this](float val) { s_asciiMix = scaleParameter(val, 1.0f); } },
		{ "slider_9", [this](float val) { loadTextureFromFile(floor((val / 1000.0f) * maps_count)); } },
	};

	togglesHandlers = {
		{ "toggle_0", [this](int val) { val == 1 ? b_Ascii = true : b_Ascii = false; } },
		{ "toggle_1", [this](int val) { zoomBlur->setEnabled(val); } },
		{ "toggle_2", [this](int val) { edgePass->setEnabled(val); } },
		{ "toggle_3", [this](int val) { post[0]->setEnabled(val); } },
	};
}

//-----------------------------------------------------------------------------------------------------------
void ofApp::update() {
	updateCamera();

	AllocateImages();

	if (bNewFrame) {
		processNewFrame();
		calculateOpticalFlow();
	}

	updateParticles();
	applyFlowToPlayers();

	player1.update();
	player2.update();

	collision();

	// update params from websocket
	if (webSocket.isConnected) {
		ofWebSocket::ParsedData data = webSocket.parsedData;

		auto it = sliderHandlers.find(data.id);
		if (it != sliderHandlers.end()) {
			it->second(data.param);
		}

		auto t_it = togglesHandlers.find(data.id);
		if (t_it != togglesHandlers.end()) {
			t_it->second(data.param);
		}
	}
}


//-----------------------------------------------------------------------------------------------------------
void ofApp::draw() {
	//-----------------------------------------------------------------------------------------------------------
	post.begin();

	particlesFbo.begin();

	ofBackgroundGradient(ofColor(0), bgColor);
	ofSetColor(255);

	if (grayImage.bAllocated) {
		ofSetColor(255, 255, 255, 50);
		drawParticles();
		ofSetColor(255, 255, 255, 255);
	}

	particlesFbo.end();

	ofSetColor(255);

	if (b_Ascii) {
		asciiShader.begin();
		asciiShader.setUniformTexture("tex0", particlesFbo.getTexture(), 0);
		asciiShader.setUniformTexture("asciiAtlas", asciiAtlas, 1);
		asciiShader.setUniform1f("cellSize", atlasCellSize);
		asciiShader.setUniform2f("atlasSize", atlasSize_grid.x, atlasSize_grid.y);
		asciiShader.setUniform1f("scaleFont", s_asciiFontScale);
		asciiShader.setUniform1f("charsetOffset", s_asciiCharsetOffset);
		asciiShader.setUniform1f("time", ofGetElapsedTimef());
		asciiShader.setUniform1f("shader_mix", s_asciiMix);
	}

	particlesFbo.draw(0, 0);

	if (b_Ascii)
		asciiShader.end();

	post.end();

	//-----------------------------------------------------------------------------------------------------------

	player1.draw();
	player2.draw();

	ball.move(player1, player2);

	ball.draw();

	ofSetColor(255, 255, 255);
	scoreBoard.drawString("SCORE : " + ofToString(player1.score) + " | " + ofToString(player2.score), WIN_W / 3.0, 40);

	ofSetColor(25, 200, 111);
	fpsFont.drawString(ofToString((int)ofGetFrameRate()) + " FPS", WIN_W / 1.2, 30);

	ofSetColor(bgColor);


	float centOffX = (ball.pos.x / WIN_W);
	float centOffY = 1 - (ball.pos.y / WIN_H);

	zoomBlur->setCenterX(ofLerp(zoomBlur->getCenterX(), centOffX, 0.5));
	zoomBlur->setCenterY(ofLerp(zoomBlur->getCenterY(), centOffY, 0.5));

	// delta with time to smooth travel all hue range
	float delta = ofMap(ofGetElapsedTimef(), 0, 1, 0, 1);

	edgePass->setHue(ofLerp(edgePass->getSaturation(), 0.0f, delta));
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

//-------------------------------------------------------------------------------------

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


void ofApp::updateParticles() {
	float deltaTime = ofClamp(ofGetLastFrameTime(), 1.f / 5000.f, 1.f / 5.f);
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
			particle.vel += flowForce * (30.0f * deltaTime);
			if (particle.bAtBasePos)
				particle.timeNotTouched = 0.0f;
			particle.bAtBasePos = false;
		} else {
			particle.timeNotTouched += deltaTime;
		}

		if (particle.timeNotTouched > 0.1) {
			particle.timeNotTouched = 0.0;
		}

		glm::vec2 basePos = particle.basePos;
		glm::vec2 diff = basePos - particle.pos;
		float dist = glm::length(diff);
		if (dist > 0.1f) {
			glm::vec2 dir = glm::normalize(diff);
			particle.vel += dir * (dist * 0.5f * deltaTime);
		}

		particle.vel *= 0.99f;
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
		player2.move(ofVec2f(0, -player2.speed));
	else if (rightFlowVector.x < -flowSensitivity)
		player2.move(ofVec2f(0, player2.speed));
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
		ball.speed *= 1.01f;
	}

	if (ball.pos.x - ball.size.x / 2 < player2.pos.x + player2.size.x / 2 &&
		ball.pos.x + ball.size.x / 2 > player2.pos.x - player2.size.x / 2 &&
		ball.pos.y - ball.size.y / 2 < player2.pos.y + player2.size.y / 2 &&
		ball.pos.y + ball.size.y / 2 > player2.pos.y - player2.size.y / 2) {
		ball.dir.x *= -1;
		ball.speed *= 1.01f;

		if (player2.getDirection().y == ball.dir.y) {
			ball.speed *= 1.06f;
		} else if (player2.getDirection().y == -ball.dir.y) {
			ball.speed *= 0.87f;
		} else
			ball.speed *= 1.0f;
	}
}


void ofApp::loadTextureFromFile(int index) {
	index = (int)index % maps_count;
	ofLoadImage(asciiAtlas, fontmaps[index]);
	asciiShader.setUniformTexture("asciiAtlas", asciiAtlas, 1);
	atlasCellSize = asciiAtlas.getWidth() / atlasSize_grid.x;
	asciiShader.setUniform2f("atlasSize", atlasSize_grid.x, atlasSize_grid.y);
}


// CALLBACKS
//-----------------------------------------------------------------------------------------------------------
void ofApp::keyPressed(int key) {
	// unsigned idx = key - '0';
	// if (idx < post.size()) {
	// 	post[idx]->setEnabled(!post[idx]->getEnabled());
	// 	return;
	// }

	static int counter = 0;

	switch (key) {
		case 'q':
			webSocket.close();
			ofExit();
			break;

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
			spacing >= 32 ? spacing = 32 : spacing;
			spacingChanged(spacing);
			break;
		case 'a':
			spacing -= 1;
			spacing <= 2 ? spacing = 2 : spacing;
			spacingChanged(spacing);
			break;
		case 'p':
			particle_size += 0.01f;
			if (particle_size > 128.0f) {
				particle_size = 128.0f;
			}
			break;
		case 'o':
			particle_size -= 0.01f;
			if (particle_size < 0.01f) {
				particle_size = 0.01f;
			}
			break;

		case 'x':
			b_Ascii = !b_Ascii;
			if (b_Ascii) {
				ofLogNotice() << "ASCII SHADER ON";
				asciiShader.load("shaders/ascii.vert", "shaders/ascii.frag");
			}
			break;

		case 'm':
			counter = counter % maps_count;
			ofLoadImage(asciiAtlas, fontmaps[counter]);
			asciiShader.setUniformTexture("asciiAtlas", asciiAtlas, 1);
			atlasCellSize = asciiAtlas.getWidth() / atlasSize_grid.x;
			asciiShader.setUniform2f("atlasSize", atlasSize_grid.x, atlasSize_grid.y);
			break;
	}
}

//-----------------------------------------------------------------------------------------------------------

void ofApp::windowResized(int w, int h) {
	WIN_W = w;
	WIN_H = h;

	particlesFbo.allocate(WIN_W, WIN_H, GL_RGBA);
	post.init(WIN_W, WIN_H);

	ofSetWindowShape(WIN_W, WIN_H);
}

void ofApp::spacingChanged(int &spacing) {
	this->spacing = spacing;
	spacing = ofClamp(spacing, 2, 32);

	generateParticles(WIN_W, WIN_H);
}

void ofApp::particleSizeChanged(float &particle_size) {
	this->particle_size = particle_size;
}

void ofApp::postProcessingChanged(float &exposure) {
	(void)exposure;
	zoomBlur->setExposure(uiManager.exposure_s);
	zoomBlur->setWeight(uiManager.weight_s);
}

void ofApp::asciiSpreadChanged(int &spread) {
	s_asciiFontScale = spread;
}

void ofApp::asciiOffsetChanged(int &offset) {
	s_asciiCharsetOffset = offset;
}

void ofApp::asciiMixChanged(float &mix) {
	s_asciiMix = mix;
}
