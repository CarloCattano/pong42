#include "UIManager.h"

void UIManager::setup() {
	gui.setup();
	gui.add(particle_size_s.setup("Particle Size", 1.0f, 1.0f, 10.0f));
	gui.add(sine_distorsion_s.setup("AMT", 0, 0, 40));
	gui.add(dist_freq_s.setup("Freq", 0.1f, 1.0f, 40.0f));

	particle_size_s.setSize(60, 40);
	dist_freq_s.setSize(60, 40);
	sine_distorsion_s.setSize(60, 40);
}

void UIManager::draw() {
	gui.draw();
}
