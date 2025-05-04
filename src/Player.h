#pragma once
#include "ofMain.h"

class Player {
public:
	int     score;
	float   speed;
	ofVec2f pos;
	ofVec2f size;

	void stop();
	void draw();
	void update();

	ofVec2f getDirection();
	void    setDirection(float newDir);

	Player(ofVec2f pos, ofVec2f size);
	Player();
	~Player();

private:
	ofVec2f direction;
	void    move(ofVec2f dir);
};
