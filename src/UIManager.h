#pragma once

#include "ofxGui.h"

class UIManager {
public:
	void setup();
	void draw();

	ofxPanel        gui;
	ofxIntSlider    spacing_s;
	ofxFloatSlider  particle_size_s;

	ofxFloatSlider  exposure_s;
	ofxFloatSlider  weight_s;
	ofxFloatSlider  flow_sensitivity_s;
	ofxFloatSlider  min_length_squared_s;
	ofxIntSlider    blur_amount_s;

	ofxIntSlider    spread_s;
	ofxIntSlider    asciiOffset_s;
	ofxFloatSlider  asciiMix_s;
	ofxFloatSlider  asciiSize_s;
	ofxToggle       asciiDisplacement_s;
	ofxFloatSlider  asciiParticleThreshold_s;
	ofxToggle       asciiFullRange_s;
};
