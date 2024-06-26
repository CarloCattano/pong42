#include "UIManager.h"

void UIManager::setup() {
    gui.setup();
    gui.add(spacing_s.setup("Spacing", 3, 3, 12));
    gui.add(particle_size_s.setup("Particle Size", 1.0f, 1.0f, 5.0f));
    gui.add(sine_distorsion_s.setup("Sine Distorsion", 0, 0, 40));
    gui.add(dist_freq_s.setup("Distorsion Frequency", 0.1f, 1.0f, 40.0f));
}

void UIManager::draw() {
    gui.draw();
}
