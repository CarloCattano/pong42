#include "ofApp.h"
#include "ofMain.h"

//========================================================================
int main() {

	ofGLWindowSettings settings;
	settings.setSize(1920, 1200);
	settings.windowMode = OF_FULLSCREEN; //can also be OF_FULLSCREEN

	auto window = ofCreateWindow(settings);

	ofRunApp(window, std::make_shared<ofApp>());
	ofRunMainLoop();
}
