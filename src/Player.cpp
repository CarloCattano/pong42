#include "Player.hpp"
#include "ofVec2f.h"

Player::Player(ofVec2f pos, ofVec2f size) {
	this->pos = pos;
	this->size = size;
	this->score = 0;
	this->speed = 5.0;
	this->direction = ofVec2f(0, 1);
}

Player::Player() {
	this->pos = ofVec2f(0, 0);
	this->size = ofVec2f(0, 0);
}

Player::~Player() { }

void Player::update() {
	move(this->direction);
}

void Player::move(ofVec2f dir) {
	this->pos += dir * speed;

	int boundOffset = size.y / 2.0;
	if (pos.y - boundOffset < 0) {
		pos.y = boundOffset;
	} else if (pos.y + boundOffset > ofGetHeight()) {
		pos.y = ofGetHeight() - boundOffset;
	}
	this->direction = dir;
}

ofVec2f Player::getDirection() {
	return direction;
}

void Player::chaseBall(ofVec2f ballPos) {
	if (ballPos.y > pos.y) {
		move(ofVec2f(0, ofRandom(1, 6)));
	} else if (ballPos.y < pos.y) {
		move(ofVec2f(0, ofRandom(-6, -1)));
	}
}

void Player::draw() {
	ofSetColor(0, 180, 10);
	ofDrawRectangle(pos.x - size.x / 2.0, pos.y - size.y / 2.0, size.x, size.y);
}
