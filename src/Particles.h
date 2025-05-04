#pragma once

#include "ofGraphicsConstants.h"
#include "ofMain.h"

class ParticleSystem {
public:
	struct Particle {
		glm::vec2    pos;
		glm::vec2    vel;
		glm::vec2    basePos;
		float        size           = 4.0f;
		bool         bAtBasePos     = true;
		float        timeNotTouched = 0.0f;
		ofFloatColor color;
	};

	ParticleSystem() {
		glEnable(GL_PROGRAM_POINT_SIZE);
		ofEnablePointSprites();
	}

	void clear() {
		particles.clear();
	}

	void generateParticles(int width, int height, float spacing) {
		clear();

		int   numx   = width / spacing;
		int   numy   = height / spacing;
		float offset = spacing;

		particles.reserve(numx * numy);

		for (int x = 0; x < numx; x++) {
			for (int y = 0; y < numy; y++) {
				glm::vec2 pos(offset + x * spacing, offset + y * spacing);
				Particle  particle;

				particle.pos     = pos;
				particle.size    = spacing;
				particle.basePos = pos;
				particle.color   = ofFloatColor(1.0f, 1.0f, 1.0f, 1.0f);

				particles.push_back(particle);
			}
		}
	}

	void updateParticles(const cv::Mat &flowMat, float deltaTime, float minLengthSquared, float sourceWidth,
	                     float sourceHeight, bool bMirror) {
		leftFlowVector    = glm::vec2(0, 0);
		rightFlowVector   = glm::vec2(0, 0);
		size_t leftCount  = 0;
		size_t rightCount = 0;

		for (auto &particle : particles) {
			float     percentX  = particle.pos.x / sourceWidth;
			float     percentY  = particle.pos.y / sourceHeight;
			glm::vec2 flowForce = getOpticalFlowValueForPercent(flowMat, percentX, percentY, minLengthSquared);

			if (particle.pos.x + particle.size < sourceWidth / 2.0) {
				leftFlowVector += flowForce;
				leftCount++;
			} else {
				rightFlowVector += flowForce;
				rightCount++;
			}

			float len2 = glm::length2(flowForce);
			particle.vel /= 1.f + deltaTime;

			if (len2 > minLengthSquared) {
				particle.vel += flowForce * (30.0f * deltaTime);
				if (particle.bAtBasePos)
					particle.timeNotTouched = 0.0f;
				particle.bAtBasePos = false;
			} else {
				particle.timeNotTouched += deltaTime;
			}

			if (particle.timeNotTouched > 0.1) {
				particle.timeNotTouched = 0.0;
			}

			glm::vec2 basePos = particle.basePos;
			glm::vec2 diff    = basePos - particle.pos;
			float     dist    = glm::length(diff);

			if (dist > 0.1f) {
				glm::vec2 dir = glm::normalize(diff);
				particle.vel += dir * (dist * 0.5f * deltaTime);
			}

			particle.vel *= 0.99f;
			particle.pos += particle.vel * (10.0f * deltaTime);
		}

		if (leftCount > 0)
			leftFlowVector /= leftCount;
		if (rightCount > 0)
			rightFlowVector /= rightCount;
	}

	void updateColors(const ofPixels &pixels, float particle_size, bool bMirror) {
		int imgW = pixels.getWidth();
		int imgH = pixels.getHeight();

		for (auto &particle : particles) {
			int samplex = bMirror ? imgW - (int)particle.pos.x : (int)particle.pos.x;
			int sampley = (int)particle.pos.y;
			if (samplex >= 0 && samplex < imgW && sampley >= 0 && sampley < imgH) {
				particle.color   = pixels.getColor(samplex, sampley);
				float brightness = particle.color.getBrightness();
				particle.size    = particle_size * (brightness * 0.8f + 0.2f);
			} else {
				// Hide out-of-bounds particles visually
				particle.color.a = 0.0f;
				particle.size    = 0.0f;
			}
		}
	}

	void draw(float xmult, float ymult, float particle_size) {
		mesh.clear();
		mesh.setMode(OF_PRIMITIVE_PATCHES);

		for (const auto &particle : particles) {
			if (particle.color.a > 0.0f && particle.size > 0.0f) {
				glm::vec3 pos3D(particle.pos.x * xmult, particle.pos.y * ymult, 0.0f);
				mesh.addVertex(pos3D);
				mesh.addColor(particle.color);
			}
		}

		ofPushStyle();
		glPointSize(particle_size); // TODO: Fixed size for now (can be dynamic with shader)
		ofEnableBlendMode(OF_BLENDMODE_ADD);

		mesh.draw();

		ofDisableBlendMode();
		ofDisablePointSprites();
		ofPopStyle();
	}

	glm::vec2 getLeftFlowVector() const {
		return leftFlowVector;
	}

	glm::vec2 getRightFlowVector() const {
		return rightFlowVector;
	}

	size_t getParticleCount() const {
		return particles.size();
	}

private:
	std::vector<Particle> particles;
	glm::vec2             leftFlowVector;
	glm::vec2             rightFlowVector;

	ofVboMesh mesh;
	ofShader  shader;

	glm::vec2 getOpticalFlowValueForPercent(const cv::Mat &flowMat, float xpct, float ypct, float minLengthSquared) {
		glm::vec2 flowVector(0, 0);

		if (flowMat.empty()) {
			return flowVector;
		}

		int tx = xpct * static_cast<float>(flowMat.cols);
		int ty = ypct * static_cast<float>(flowMat.rows);

		tx = ofClamp(tx, 0, flowMat.cols - 1);
		ty = ofClamp(ty, 0, flowMat.rows - 1);

		const cv::Point2f &fxy = flowMat.at<cv::Point2f>(ty, tx);
		flowVector             = glm::vec2(fxy.x, fxy.y);

		if (glm::length2(flowVector) > minLengthSquared) {
			return flowVector;
		}
		return glm::vec2(0.0, 0.0);
	}
};
