#include "ofApp.h"
#include "ofMain.h"
#include "ofWindowSettings.h"

//========================================================================
int main() {
	ofSetupOpenGL(1920, 1200, OF_WINDOW); // <-------- setup the GL context
	ofRunApp(new ofApp());
}
