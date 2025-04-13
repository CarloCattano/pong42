#include "Ball.hpp"

Ball::Ball()
	: speed(24)
	, pos(ofGetWidth() / 2.0, ofGetHeight() / 2.0)
	, size(42, 42)
	, dir(1, 1) {
}

Ball::~Ball() { }

void Ball::move(Player & player1, Player & player2) {
	if (pos.x <= size.x / 2.0) {
		pos = ofVec2f(ofGetWidth() / 2.0, ofGetHeight() / 2.0);
		dir.y = ofRandom(-1, 1);
		player2.score++;

	} else if (pos.x >= ofGetWidth() - 5) {
		pos = ofVec2f(ofGetWidth() / 2.0, ofGetHeight() / 2.0);
		dir.y = ofRandom(-1, 1);
		player1.score++;
	}

	if (pos.y < size.y / 2.0 || pos.y > ofGetHeight() - size.y / 2.0) {
		dir.y *= -1;
		dir.y += ofRandom(-0.1, 0.1);
	}

	pos += dir * speed;
}

void Ball::draw() {
	ofSetColor(255, 0, 255);
	ofDrawEllipse(pos, size.x, size.y);
}
