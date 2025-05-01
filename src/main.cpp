#include "ofApp.h"
#include "ofMain.h"
#include "ofWindowSettings.h"

//========================================================================
int main() {
	ofSetupOpenGL(1920, 1200, OF_FULLSCREEN); // <-------- setup the GL context
	ofRunApp(new ofApp());
}
