#include "ofApp.h"
#include "fwd.hpp"
#include "ofAppRunner.h"

//--------------------------------------------------------------
void ofApp::setup() {

	particle_size = 1.0f;

	WIN_W = ofGetWidth();
	WIN_H = ofGetHeight();

	ofSetFrameRate(60);

#ifdef _USE_LIVE_VIDEO
	vidGrabber.setVerbose(true);
	vidGrabber.setup(320, 240);
#else
	vidPlayer.load("Hand3s.mp4");
	vidPlayer.play();
	vidPlayer.setLoopState(OF_LOOP_NORMAL);
#endif

	blurAmount = 5;
	bMirror = true;
	cvDownScale = 12;
	bContrastStretch = false;

	// store a minimum squared value to apply flow velocity
	minLengthSquared = 0.5 * 0.5; // 0.5 pixel squared
 
    uiManager.spacing_s.addListener(this, &ofApp::spacingChanged);
    uiManager.particle_size_s.addListener(this, &ofApp::particleSizeChanged);
    uiManager.sine_distorsion_s.addListener(this, &ofApp::sineDistorsionChanged);
    uiManager.dist_freq_s.addListener(this, &ofApp::distFreqChanged);

    uiManager.setup();

	ofSetWindowTitle("OpticalFlowYo!");
}

void ofApp::generateParticles(int s_witdth, int s_height) {

	particles.clear();

	int numx = s_witdth / spacing;
	int numy = s_height / spacing;

	float delta = 0.5f;

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
}

ofColor circleColor = ofColor(255, 0, 0, 255);

//--------------------------------------------------------------
void ofApp::update() {
	ofBackground(100, 100, 100);

	bool bNewFrame = false;
	int sourceWidth = vidGrabber.getWidth();
	int sourceHeight = vidGrabber.getHeight();

#ifdef _USE_LIVE_VIDEO
	vidGrabber.update();
	bNewFrame = vidGrabber.isFrameNew();
	sourceWidth = vidGrabber.getWidth();
	sourceHeight = vidGrabber.getHeight();
#else
	vidPlayer.update();
	bNewFrame = vidPlayer.isFrameNew();
	sourceWidth = vidPlayer.getWidth();
	sourceHeight = vidPlayer.getHeight();
#endif

	int scaledWidth = sourceWidth / cvDownScale;
	int scaledHeight = sourceHeight / cvDownScale;

	if (currentImage.getWidth() != scaledWidth || currentImage.getHeight() != scaledHeight) {
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

#ifdef _USE_LIVE_VIDEO
		colorImg.setFromPixels(vidGrabber.getPixels());
#else
		colorImg.setFromPixels(vidPlayer.getPixels());
#endif

		previousImage.resize(currentImage.getWidth(), currentImage.getHeight());
		diffImage.absDiff(currentImage, previousImage);
		diffImage.threshold(10);
		
        motionAmount = diffImage.countNonZeroInRegion(0, 0, diffImage.getWidth(), diffImage.getHeight());

		ofPixels & pixels = diffImage.getPixels();

        std::pair<int, int>                 pos;
        std::map<std::pair<int,int>, int>   normals;

		for (int y = 0; y < diffImage.getWidth(); y++) {
			for (int x = 0; x < diffImage.getHeight(); x++) {
	            pos = std::make_pair(x, y);
    
                if (pixels.getColor(x, y).getBrightness() > 170) {
					ofVec2f normal(x / (float)diffImage.getWidth(), y / (float)diffImage.getHeight());
                    normals[pos] = 1;
                } else 
                    normals[pos] = 0;
			}
		}
	
        previousImage = currentImage.getPixels();
		//-----------------------------------------------------------------

		grayImage = colorImg;

		if (bMirror) {
			grayImage.mirror(false, true);
		}

		currentImage.scaleIntoMe(grayImage);

		if (bContrastStretch) {
			currentImage.contrastStretch();
		}

		if (blurAmount > 0) {
			currentImage.blurGaussian(blurAmount);
		}

		cv::Mat currentMat = currentImage.getCvMat();
		cv::calcOpticalFlowFarneback(previousMat,
			currentMat,
			flowMat,
			0.5, // pyr_scale
			4, // levels
			16, // winsize
			2, // iterations
			7, // poly_n
			1.5, // poly_sigma
			cv::OPTFLOW_FARNEBACK_GAUSSIAN);

		currentMat.copyTo(previousMat);
	}

	float deltaTime = ofClamp(ofGetLastFrameTime(), 1.f / 5000.f, 1.f / 5.f);

    // check the pixel at the center of the screen with the optical flow
    glm::vec2 centerForce = getOpticalFlowValueForPercent(0.5, 0.5);

    if(centerForce.x > 0.2) {
        circleColor = ofColor(0, 255, 0, 255);
        sine_distorsion = 4;
        
    } else if(centerForce.x < -0.2) {
        circleColor = ofColor(255, 0, 0, 255);
        sine_distorsion = 0;
    }
	
	size_t numParticles = particles.size();
	
    for (size_t i = 0; i < numParticles; i++) {
		auto & particle = particles[i];
		float percentX = particle.pos.x / sourceWidth;
		float percentY = particle.pos.y / sourceHeight;
		glm::vec2 flowForce = getOpticalFlowValueForPercent(percentX, percentY);
		float len2 = glm::length2(flowForce);

		particle.vel /= 1.f + deltaTime;
		if (len2 > minLengthSquared) {
			// ok lets add some velocity
			particle.vel += flowForce * (30.0f * deltaTime);
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
	}
}

//--------------------------------------------------------------
void ofApp::draw() {
	ofBackgroundGradient(ofColor(0), ofColor(20));
	ofSetColor(255);

	if (grayImage.bAllocated) {
			ofSetColor(80);
			/*currentImage.draw(0, 0, WIN_W, WIN_H);*/
			size_t numParticles = particles.size();
			const ofPixels & vpix = colorImg.getPixels();

			for (size_t i = 0; i < numParticles; i++) {
				auto & particle = particles[i];
				int samplex = particle.pos.x;
				if (bMirror) {
					samplex = colorImg.getWidth() - samplex;
				}
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
	
                float distorsion = sin(ofGetElapsedTimef() * dist_freq + particle.pos.y * 0.1) * sine_distorsion;

                /*float distorsion = 0.0f;*/

				ofTranslate(particle.pos.x * xmult + distorsion, particle.pos.y * ymult);
				ofDrawCircle(0, 0, psize);
				ofPopMatrix();
			}
	}

	diffImage.draw(0, 0, colorImg.getWidth(), colorImg.getHeight());

    // draw a circle in the center of the screen
    ofSetColor(circleColor);
    ofDrawCircle(ofGetWidth() / 2.0, ofGetHeight() / 2.0, 16);

	ofDrawBitmapStringHighlight(ofToString(ofGetFrameRate()), ofGetWidth() / 2, 20);

	ofSetColor(255);
    uiManager.draw();
}

//--------------------------------------------------------------
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

//--------------------------------------------------------------
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
	case 'm':
		bMirror = !bMirror;
		break;
	case 'c':
		bContrastStretch = !bContrastStretch;
		break;
	case OF_KEY_RIGHT:
		blurAmount++;
		if (blurAmount > 255) blurAmount = 255;
		break;
	case OF_KEY_LEFT:
		blurAmount--;
		if (blurAmount < 0) blurAmount = 0;
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

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {
	WIN_W = w;
	WIN_H = h;
}

void ofApp::spacingChanged(int & spacing) {
	this->spacing = spacing;
	generateParticles(ofGetWidth(), ofGetHeight());
}

void ofApp::particleSizeChanged(float & particle_size) {
	this->particle_size = particle_size;
}

void ofApp::sineDistorsionChanged(int & sine_distorsion) {
	this->sine_distorsion = sine_distorsion;
}

void ofApp::distFreqChanged(float & dist_freq) {
	this->dist_freq = dist_freq;
}
