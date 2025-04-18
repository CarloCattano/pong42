#pragma once

#include "ofxGui.h"

class UIManager {
public:
	void setup();
	void draw();

	ofxPanel gui;
	ofxIntSlider spacing_s;
	ofxFloatSlider particle_size_s;

	ofxFloatSlider exposure_s;
	ofxFloatSlider weight_s;

	ofxFloatSlider cellSize_s;
	ofxFloatSlider spread_s;
	ofxFloatSlider asciiOffset_s;
};
