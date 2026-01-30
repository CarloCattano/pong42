#include "UIManager.h"

void UIManager::setup() {
	gui.setup();
	gui.add(particle_size_s.setup("Particle Size", 4.0f, 1.0f, 15.0f));
	gui.add(spacing_s.setup("Spacing", 4, 2, 128));
	gui.add(flow_sensitivity_s.setup("Flow Sensitivity", 0.4f, 0.01f, 2.0f));
	gui.add(min_length_squared_s.setup("Min Length Squared", 0.5f, 0.1f, 15.0f));
	gui.add(exposure_s.setup("Exposure", 0.3f, 0.01f, 2.0f));
	gui.add(weight_s.setup("Weight", 0.25f, 0.01f, 1.0f));
	gui.add(spread_s.setup("spread", 32, 1, 120));
	gui.add(blur_amount_s.setup("Blur Amount", 3, 1, 10));

	gui.add(asciiOffset_s.setup("asciiOffset", 0, 0, 128));
	gui.add(asciiMix_s.setup("asciiMix", 0.5f, 0.0f, 1.4f));
	gui.add(asciiSize_s.setup("asciiSize", 1.0f, 0.1f, 4.0f));
	gui.add(asciiDisplacement_s.setup("asciiDisplacement", false));
	gui.add(asciiParticleThreshold_s.setup("asciiParticleThreshold", 0.02f, 0.0f, 1.0f));
	gui.add(asciiFullRange_s.setup("asciiFullRange", false));
}

void UIManager::draw() {
	gui.draw();
}
