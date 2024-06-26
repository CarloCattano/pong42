#pragma once

#include "ofxGui.h"

class UIManager {
public:
    void setup();
    void draw();

    ofxPanel gui;
    ofxIntSlider spacing_s;
    ofxFloatSlider particle_size_s;
    ofxIntSlider sine_distorsion_s;
    ofxFloatSlider dist_freq_s;
};
