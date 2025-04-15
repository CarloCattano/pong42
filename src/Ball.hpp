#pragma once
#include "Player.hpp"
#include "ofMain.h"

class Ball {
public:
	int score;
	int speed;
	ofVec2f pos;
	ofVec2f size;
	ofVec2f dir;

	void move(Player &player1, Player &player2);
	void draw();
	bool collide(const Player &player);
	Ball();
	~Ball();
};
