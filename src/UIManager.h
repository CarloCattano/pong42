#pragma once

#include "ofxGui.h"

class UIManager {
public:
	void setup();
	void draw();

	ofxPanel gui;
	ofxIntSlider spacing_s;
	ofxFloatSlider particle_size_s;
};
