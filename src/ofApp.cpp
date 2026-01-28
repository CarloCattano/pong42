#include "ofApp.h"
#include "EdgePass.h"
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
	cvDownScale      = 8;
	bContrastStretch = true;

	// store a minimum squared value to apply flow velocity
	minLengthSquared = 0.7 * 0.7; // 0.5 pixel squared

	ofVec2f paddleSize(64, 224);

	ofTrueTypeFont::setGlobalDpi(72);

	font.load("verdana.ttf", 22, true, true);
	font.setLineHeight(28.0);
	font.setLetterSpacing(1.05);
#ifdef USE_VIDEO_FILE
	videoPlayer.load("vid3.mp4");
	videoPlayer.setLoopState(OF_LOOP_NORMAL);
	videoPlayer.play();
	sourceWidth  = videoPlayer.getWidth();
	sourceHeight = videoPlayer.getHeight();
#else
	cam.setDesiredFrameRate(60);
	cam.setup(1280, 720);
	sourceWidth  = cam.getWidth();
	sourceHeight = cam.getHeight();
#endif

	depthOrig.allocate(sourceWidth, sourceHeight);
	depthProcessed.allocate(sourceWidth, sourceHeight);
	colorImg.allocate(sourceWidth, sourceHeight);

	// Setup Post-Processing chain ------------------------
	post.init(WIN_W, WIN_H);

	post.createPass<BloomPass>()->setEnabled(false);
	post.createPass<ZoomBlurPass>()->setEnabled(false);
	post.createPass<EdgePass>()->setEnabled(false);

    // Initialize YOLOv5 model
    classify.setup("yolov5n.onnx", "coco.names", true);


	zoomBlur = dynamic_cast<ZoomBlurPass *>(post[1].get());
	edgePass = dynamic_cast<EdgePass *>(post[2].get());
	// -----------------------------------------------------
#ifdef UI
	uiManager.spacing_s.addListener(this, &ofApp::spacingChanged);
	uiManager.particle_size_s.addListener(this, &ofApp::particleSizeChanged);

	uiManager.exposure_s.addListener(this, &ofApp::postProcessingChanged);
	uiManager.weight_s.addListener(this, &ofApp::postProcessingChanged);

	uiManager.spread_s.addListener(this, &ofApp::asciiSpreadChanged);
	uiManager.asciiOffset_s.addListener(this, &ofApp::asciiOffsetChanged);
	uiManager.asciiMix_s.addListener(this, &ofApp::asciiMixChanged);
	uiManager.asciiSize_s.addListener(this, &ofApp::asciiSizeChanged);

	uiManager.setup();
#endif

	zoomBlur->setExposure(0.25);
	zoomBlur->setWeight(0.6);
	zoomBlur->setDecay(0.9);
	zoomBlur->setDensity(0.1);

	loadMapNames(); // Populate fontmaps

	fontTextures.resize(maps_count);
	for (unsigned int i = 0; i < maps_count; i++) {
		ofLoadImage(fontTextures[i], fontmaps[i]);
		fontTextures[i].setTextureMinMagFilter(GL_NEAREST, GL_NEAREST); // Apply filter here, once per texture
	}

	counter = 0;
	b_Ascii = true;
	asciiShader.load("shaders/ascii.vert", "shaders/ascii.frag");
	s_asciiFontScale = 2.0f;
	s_asciiCellScale = 1.0f;

	atlasSize_grid = ofVec2f(8.0f, 8.0f);
	// Calculate atlasCellSize and set atlasSize uniform once, assuming all font maps have the same width
	if (maps_count > 0) {
		atlasCellSize = fontTextures[0].getWidth() / atlasSize_grid.x;
		asciiShader.begin();
		asciiShader.setUniform2f("atlasSize", atlasSize_grid.x, atlasSize_grid.y);
		asciiShader.setUniform1f("cellSize", atlasCellSize);
		asciiShader.setUniform1f("screenCellSize", atlasCellSize * s_asciiCellScale);
		asciiShader.end();
		// Also call loadTextureFromFile to initialize the shader with the first texture
		loadTextureFromFile(counter);
	}

	particlesFbo.allocate(WIN_W, WIN_H, GL_RGBA);
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
		asciiShader.setUniformTexture("asciiAtlas", fontTextures[counter], 1);
		asciiShader.setUniform1f("cellSize", atlasCellSize);
		asciiShader.setUniform1f("screenCellSize", atlasCellSize * s_asciiCellScale);
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

#ifdef UI
	uiManager.draw();
#endif
}
//---------------------------------------------------------------------------------

void ofApp::generateParticles(int s_width, int s_height) {
	float effectiveSpacing = spacing;

	if (effectiveSpacing <= 0 || effectiveSpacing > 100) {
		effectiveSpacing = 10; // Default spacing
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

void ofApp::updateCamera() {
#ifdef USE_VIDEO_FILE
	videoPlayer.update();
	bNewFrame = videoPlayer.isFrameNew();
	if (bNewFrame) {
		colorImageRGB = videoPlayer.getPixels();
		depthOrig     = colorImageRGB;
	}
#else
	cam.update();
	bNewFrame     = cam.isFrameNew();
	colorImageRGB = cam.getPixels();
	depthOrig     = colorImageRGB;
#endif
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
#ifdef USE_VIDEO_FILE
	auto pixels = videoPlayer.getPixels();
#else
	auto pixels = cam.getPixels();
#endif
	colorImg.setFromPixels(pixels);
	grayImage = colorImg;

	if (bMirror)
		grayImage.mirror(false, true);

	currentImage.scaleIntoMe(grayImage);
	auto cvMat = cv::cvarrToMat(colorImg.getCvImage()).clone();

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

		        case ',':
		            // decrease ascii on-screen cell scale (makes characters smaller)
		            s_asciiCellScale -= 0.1f;
		            if (s_asciiCellScale < 0.1f) {
		                s_asciiCellScale = 0.1f;
		            }
		#ifdef UI
		            uiManager.asciiSize_s = s_asciiCellScale;
		#endif
		            // Ensure texture filtering is updated immediately even if UI is disabled
		            asciiSizeChanged(s_asciiCellScale);
		            break;

		        case '.':
		            // increase ascii on-screen cell scale (makes characters larger)
		            s_asciiCellScale += 0.1f;
		            if (s_asciiCellScale > 4.0f) {
		                s_asciiCellScale = 4.0f;
		            }
		#ifdef UI
		            uiManager.asciiSize_s = s_asciiCellScale;
		#endif
		            // Ensure texture filtering is updated immediately even if UI is disabled
		            asciiSizeChanged(s_asciiCellScale);
		            break;

		        case 'n':
		            if (maps_count > 0) {
		                counter > 0 ? counter-- : counter = maps_count - 1;
		                loadTextureFromFile(counter);
		            }
		            break;
		case 'm':
			if (maps_count > 0) {
				counter = (counter + 1) % maps_count; // More robust way to cycle
				loadTextureFromFile(counter);
			}
			break;
	}
}

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
void ofApp::asciiSizeChanged(float &size) {
	s_asciiCellScale = size;
	// Update current atlas texture filtering immediately so changes are visible without cycling textures
	if (maps_count > 0) {
		if (s_asciiCellScale < 1.0f) {
			// When rendering characters smaller than the atlas glyphs, use linear filtering to reduce aliasing
			fontTextures[counter].setTextureMinMagFilter(GL_LINEAR, GL_LINEAR);
		} else {
			// For 1:1 or scaled-up characters, nearest keeps the pixel-art look crisp
			fontTextures[counter].setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
		}
	}
}

//-----------------------------------------------------------------------------------------------------------

void ofApp::loadTextureFromFile(int index) {
	if (maps_count == 0) {
		ofLogWarning("ofApp") << "No font maps loaded. Cannot set texture.";
		return;
	}
	index = (int)index % maps_count;
	// Choose filtering depending on desired on-screen cell size:
	// If we're rendering smaller than the atlas glyph, use linear filtering (minification) to reduce aliasing.
	// Otherwise, keep nearest for a crisper, pixel-art look.
	if (s_asciiCellScale < 1.0f) {
		fontTextures[index].setTextureMinMagFilter(GL_LINEAR, GL_LINEAR);
	} else {
		fontTextures[index].setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
	}
	asciiShader.setUniformTexture("asciiAtlas", fontTextures[index], 1); // Use pre-loaded texture
}

void ofApp::loadMapNames() {
	ofDirectory dir;
	dir.allowExt("png");
	dir.listDir("fontmaps");
	dir.sort();
	maps_count = dir.size(); // Correctly get the number of found files
	fontmaps.resize(maps_count);
	for (unsigned int i = 0; i < maps_count; i++) { // Loop through all found files
		fontmaps[i] = dir.getPath(i);
	}
}
