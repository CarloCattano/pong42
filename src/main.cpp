#include "ofApp.h"
#include "ofMain.h"

//========================================================================
int main() {
	// ofGLWindowSettings settings;
	// // settings.setSize(1920, 1200);
	// settings.windowMode = OF_FULLSCREEN; // can also be OF_FULLSCREEN
	// settings.setGLVersion(3, 2);
	// auto window = ofCreateWindow(settings);
	//
	// ofRunApp(window, std::make_shared<ofApp>());
	// ofRunMainLoop();
	ofSetupOpenGL(1024, 768, OF_WINDOW); // <-------- setup the GL context

	// settings.setGLVersion(3, 2);
	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	ofRunApp(new ofApp());
}
