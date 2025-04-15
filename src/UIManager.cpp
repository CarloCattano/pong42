#include "UIManager.h"

void UIManager::setup() {
	gui.setup();
	gui.add(particle_size_s.setup("Particle Size", 1.0f, 1.0f, 10.0f));

	particle_size_s.setSize(60, 40);
}

void UIManager::draw() {
	gui.draw();
}
