#pragma once
#include "ofMain.h"

class Player {

public:
	int score;
	float speed;
	ofVec2f pos;
	ofVec2f size;
	ofVec2f vel;
	ofVec2f direction;

	bool playerAI;

	void move(ofVec2f dir);
	void draw();
	void update();

	ofVec2f getDirection();

	void chaseBall(ofVec2f ballPos);

	Player();
	~Player();
	Player(ofVec2f pos, ofVec2f size);
};
