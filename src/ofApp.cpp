#include "ofApp.h"
#include <functional>
#include <optional>
#include <unordered_map>
#include "EdgePass.h"
#include "GameManager.h"
#include "Player.h"
#include "fwd.hpp"
#include "ofAppRunner.h"
#include "ofGraphics.h"
#include "ofMath.h"
#include "ofUtils.h"

void ofApp::setup() {
	WIN_W = ofGetWidth();
	WIN_H = ofGetHeight();

	ofSetVerticalSync(false);
	ofSetWindowShape(WIN_W, WIN_H);
	ofSetFrameRate(120);
	ofEnableAlphaBlending();
	ofSetWindowTitle("maiai");
	ofBackground(bgColor);

	bgColor = ofColor(0, 0, 0, 255);

	particle_size    = 8.0f;
	spacing          = 4.0f;
	flowSensitivity  = 0.40f;
	blurAmount       = 3;
	bMirror          = true;
	cvDownScale      = 16;
	bContrastStretch = true;

	// store a minimum squared value to apply flow velocity
	minLengthSquared = 0.7 * 0.7; // 0.5 pixel squared

	ofVec2f paddleSize(64, 224);
	float   centeredY = (WIN_H - paddleSize.y) / 2.0;

	player1 = Player(ofVec2f(64, centeredY), paddleSize);
	player2 = Player(ofVec2f(WIN_W - 64, centeredY), paddleSize);

	gameManager.emplace(player1, player2); // constructs in-place

	ofTrueTypeFont::setGlobalDpi(72);

	font.load("verdana.ttf", 22, true, true);
	font.setLineHeight(28.0);
	font.setLetterSpacing(1.05);

	cam.setDesiredFrameRate(60);
	cam.setup(1280, 720);

	sourceWidth  = cam.getWidth();
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

	counter = 0;
	b_Ascii = true;
	asciiShader.load("shaders/ascii.vert", "shaders/ascii.frag");
	s_asciiFontScale = 2.0f;

	atlasSize_grid = ofVec2f(8.0f, 8.0f);
	atlasCellSize  = 32.0f;
	asciiAtlas.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST); // Prevents blurring
	ofLoadImage(asciiAtlas, "fontmaps/edges.png");
	particlesFbo.allocate(WIN_W, WIN_H, GL_RGBA);

	// WEBSOCKET communication with fastAPI server
	webSocket.onMessage = [&](const std::string &msg) { ofLogNotice() << "Received: " << msg; };
	webSocket.connect("ws://ws.42ls.online/of-ws");

	sliderHandlers = {
		{ "slider_0",
		  [this](float val) {
		      spacing = scaleParameter(val, 128.0f, 10.0f);
		      spacing = ofClamp(spacing, 10.0f, 128.0f);
		      generateParticles(WIN_W, WIN_H);
		  } },
		{ "slider_1", [this](float val) { particle_size = scaleParameter(val, 15.0f, 1.0f); } },
		{ "slider_2", [this](float val) { zoomBlur->setWeight(scaleParameter(val, 1.5f, 0.5)); } },
		{ "slider_3", [this](float val) { zoomBlur->setDecay(scaleParameter(val, 0.9f)); } },
		{ "slider_4", [this](float val) { zoomBlur->setExposure(scaleParameter(val, 1.0f)); } },
		{ "slider_5", [this](float val) { zoomBlur->setDensity(scaleParameter(val, 0.1f)); } },
		{ "slider_6", [this](float val) { s_asciiFontScale     = scaleParameter(val, 120.0f, 1.0); } },
		{ "slider_7", [this](float val) { s_asciiCharsetOffset = scaleParameter(val, 64.0); } },
		{ "slider_8", [this](float val) { s_asciiMix           = scaleParameter(val, 1.0f); } },
		{ "slider_9", [this](float val) { loadTextureFromFile(floor((val / 1000.0f) * maps_count)); } },
	};

	togglesHandlers = {
		{ "toggle_0", [this](int val) { val == 1 ? b_Ascii = true : b_Ascii = false; } },
		{ "toggle_1", [this](int val) { zoomBlur->setEnabled(val); } },
		{ "toggle_2", [this](int val) { edgePass->setEnabled(val); } },
		{ "toggle_3", [this](int val) { post[0]->setEnabled(val); } },
	};

	classify.setup("yolov5n.onnx", "classes.txt", true);

	randDetectionSpeed = ofRandom(0.1f, 32.0f);
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

	if (gameManager) {
		gameManager->update();
	}

	player1.update();
	player2.update();


	// vertical line in the middle of the screen
	ofSetColor(255, 0, 0);
	ofDrawLine(WIN_W / 2.0, 0, WIN_W / 2.0, WIN_H);

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
	post.begin();
	particlesFbo.begin();

	ofBackgroundGradient(ofColor(0), bgColor);
	ofSetColor(255);

	if (grayImage.bAllocated) {
		ofSetColor(255, 255, 255, 255);
		drawParticles();
		ofSetColor(255, 255, 255, 255);
		drawDetectedObjects();
	}

	particlesFbo.end();

	ofSetColor(255);

	if (b_Ascii && asciiShader.isLoaded()) {
		asciiShader.begin();
		asciiShader.setUniformTexture("tex0", colorImg.getTexture(), 0);
		asciiShader.setUniformTexture("asciiAtlas", asciiAtlas, 1);
		asciiShader.setUniform1f("cellSize", atlasCellSize);
		asciiShader.setUniform2f("atlasSize", atlasSize_grid.x, atlasSize_grid.y);
		asciiShader.setUniform1f("scaleFont", s_asciiFontScale);
		asciiShader.setUniform1f("charsetOffset", s_asciiCharsetOffset);
		asciiShader.setUniform1f("time", ofGetElapsedTimef());
		asciiShader.setUniform1f("shader_mix", s_asciiMix);
	}

	particlesFbo.draw(0, 0);

	if (b_Ascii && asciiShader.isLoaded())
		asciiShader.end();

	drawDetectedObjects();

	post.end();

	//-----------------------------------------------------------------------------------------------------------
	player1.draw();
	player2.draw();

	if (!gameManager->isGameEnded()) {
		ball.draw();
		ball.move(player1, player2);
	}

	if (gameManager) {
		gameManager->draw();
	}

	// MIDDLE LINE
	ofSetLineWidth(6);
	ofSetColor(255, 255, 255, 100);

	for (int i = 0; i < WIN_H; i++) {
		if (i % 80 == 0) {
			ofDrawLine(WIN_W / 2.0, i, WIN_W / 2.0, i + 40);
		}
	}

	ofSetLineWidth(1);

	float centOffX = (ball.pos.x / WIN_W);
	float centOffY = 1 - (ball.pos.y / WIN_H);

	zoomBlur->setCenterX(ofLerp(zoomBlur->getCenterX(), centOffX, 0.5));
	zoomBlur->setCenterY(ofLerp(zoomBlur->getCenterY(), centOffY, 0.5));

#ifdef UI
	uiManager.draw();
#endif
}
//---------------------------------------------------------------------------------


void ofApp::drawDetectedObjects() {
	if (!colorImg.bAllocated) {
		return;
	}

	ofNoFill();
	ofSetColor(255, 0, 255, 255);

	float scaleX = (float)WIN_W / colorImg.getWidth();
	float scaleY = (float)WIN_H / colorImg.getHeight();


	for (auto res : results) {
		auto rect = res.rect;

		if (res.label.empty())
			continue;

		if (bMirror) {
			rect.x = colorImg.getWidth() - rect.x - rect.width;
		}

		ofRectangle scaledRect(rect.x * scaleX, rect.y * scaleY, rect.width * scaleX, rect.height * scaleY);

		for (int i = 0; i < 4; i++) {
			if (ofRandom(0, 1) > 0.5) {
				ofSetLineWidth(sin(ofGetElapsedTimef() * randDetectionSpeed) * 16 + 1);
				ofDrawRectangle(scaledRect.x + i * 2, scaledRect.y + i * 2, scaledRect.width - i * ofRandom(2.0f, 6.0f),
				                scaledRect.height - i * ofRandom(2.0f, 6.0f));
			}
		}
		ofSetLineWidth(1);

		int yOffset = 0;

		glm::vec3 labely = scaledRect.getTopLeft() + glm::vec3(0, yOffset, 0);

		ofSetColor(0, 255, 25, 255);
		font.drawString(res.label, labely.x, labely.y);
	}
	ofFill();
}

//-----------------------------------------------------------------------------------------------------------


void ofApp::generateParticles(int s_width, int s_height) {
	float effectiveSpacing = spacing;

	if (effectiveSpacing <= 0 || effectiveSpacing > 100) {
		effectiveSpacing = 20;
	}
	particleSystem.generateParticles(s_width, s_height, effectiveSpacing);
}

void ofApp::updateParticles() {
	float deltaTime = ofClamp(ofGetLastFrameTime(), 1.f / 120.f, 1.f / 10.f); // reasonable clamp
	particleSystem.updateParticles(flowMat, deltaTime, minLengthSquared, sourceWidth, sourceHeight, bMirror);

	leftFlowVector  = particleSystem.getLeftFlowVector();
	rightFlowVector = particleSystem.getRightFlowVector();
}

void ofApp::drawParticles() {
	const ofPixels &vpix = colorImg.getPixels();

	int   imgW  = vpix.getWidth();
	int   imgH  = vpix.getHeight();
	float xmult = WIN_W / (float)imgW;
	float ymult = WIN_H / (float)imgH;

	particleSystem.updateColors(vpix, particle_size, bMirror);
	particleSystem.draw(xmult, ymult, particle_size);
}

//-------------------------------------------------------------------------------------

void ofApp::updateCamera() {
	cam.update();
	bNewFrame     = cam.isFrameNew();
	colorImageRGB = cam.getPixels();
	depthOrig     = colorImageRGB;
}

void ofApp::AllocateImages() {
	int scaledWidth  = sourceWidth / cvDownScale;
	int scaledHeight = sourceHeight / cvDownScale;

	if (currentImage.getWidth() != scaledWidth || currentImage.getHeight() != scaledHeight) {
		previousMat = cv::Mat(scaledHeight, scaledWidth, CV_8UC1);
		flowMat     = cv::Mat(scaledHeight, scaledWidth, CV_32FC2);
		// currentImage.clear();
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
	auto pixels = cam.getPixels();
	colorImg.setFromPixels(pixels);
	grayImage = colorImg;

	if (bMirror)
		grayImage.mirror(false, true);

	currentImage.scaleIntoMe(grayImage);

	auto cvMat = cv::cvarrToMat(colorImg.getCvImage());

	if (ofGetFrameNum() % 3 == 0) {
		results = classify.classifyFrame(cvMat);
	}

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


//-----------------------------------------------------------------------------------------------------------

void ofApp::applyFlowToPlayers() {
	static int player1cooldown = 0;
	static int player2cooldown = 0;

	int coolDownTimeOut = 30;

	if (leftFlowVector.y > flowSensitivity) {
		player1.setDirection(1.0);
		player1cooldown = 0;
	} else if (leftFlowVector.y < -flowSensitivity) {
		player1.setDirection(-1.0);
		player1cooldown = 0;
	} else
		player1cooldown += 1;


	if (rightFlowVector.y > flowSensitivity) {
		player2.setDirection(1.0);
		player2cooldown = 0;
	} else if (rightFlowVector.y < -flowSensitivity) {
		player2.setDirection(-1.0);
		player2cooldown = 0;
	} else
		player2cooldown += 1;


	if (player1cooldown > coolDownTimeOut) {
		player1cooldown = 0;
		player1.stop();
	}

	if (player2cooldown > coolDownTimeOut) {
		player2cooldown = 0;
		player2.stop();
	}
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
	unsigned idx = key - '0';
	if (idx < post.size()) {
		post[idx]->setEnabled(!post[idx]->getEnabled());
		return;
	}

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
			counter++;
			loadTextureFromFile(counter);
			break;
	}
}

//-----------------------------------------------------------------------------------------------------------

void ofApp::windowResized(int w, int h) {
	WIN_W = ofGetWidth();
	WIN_H = ofGetHeight();

	if (particlesFbo.getWidth() != WIN_W || particlesFbo.getHeight() != WIN_H) {
		particlesFbo.allocate(WIN_W, WIN_H, GL_RGBA);
		post.init(WIN_W, WIN_H);
	}

	ofSetWindowShape(WIN_W, WIN_H);
}

void ofApp::spacingChanged(int &spacing) {
	this->spacing = spacing;
	spacing       = ofClamp(spacing, 2, 32);

	generateParticles(WIN_W, WIN_H);
}

void ofApp::particleSizeChanged(float &particle_size) {
	particle_size       = ofLerp(particle_size, particle_size, ofGetLastFrameTime());
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
