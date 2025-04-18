#include "UIManager.h"

void UIManager::setup() {
	gui.setup();
	gui.add(particle_size_s.setup("Particle Size", 1.0f, 0.1f, 5.0f));
	gui.add(spacing_s.setup("Spacing", 10, 4, 32));
	gui.add(exposure_s.setup("Exposure", 0.3f, 0.01f, 2.0f));
	gui.add(weight_s.setup("Weight", 0.25f, 0.01f, 1.0f));
	gui.add(spread_s.setup("spread", 1, 1, 255));
	gui.add(asciiOffset_s.setup("asciiOffset", 0, 0, 128));
}

void UIManager::draw() {
	gui.draw();
}
