#include "Ball.h"
#include "ofAppRunner.h"

Ball::Ball() : speed(8), pos(ofGetWidth() / 2.0, ofGetHeight() / 2.0), size(42, 42), dir(1, 1) {
}

Ball::~Ball() {
}

void Ball::move(Player &player1, Player &player2) {
	if (pos.x <= size.x / 2.0) {
		pos   = ofVec2f(ofGetWidth() / 2.0, ofGetHeight() / 2.0);
		dir.y = ofRandom(-1, 1);
		player2.score++;
		dir.x *= -1;

	} else if (pos.x >= ofGetWidth() - 5) {
		pos   = ofVec2f(ofGetWidth() / 2.0, ofGetHeight() / 2.0);
		dir.y = ofRandom(-1, 1);
		player1.score++;
		dir.x *= -1;
	}

	if (pos.y < size.y / 2.0 || pos.y > ofGetHeight() - size.y / 2.0) {
		dir.y *= -1;
		dir.y += ofRandom(-0.1, 0.1);
	}
	float deltaTime = ofGetLastFrameTime() * 100.0f;
	pos += dir * speed * deltaTime;
}

void Ball::draw() {
	ofSetColor(255, 0, 255);
	ofDrawEllipse(pos, size.x, size.y);
}
