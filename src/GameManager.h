#pragma once
#include "Player.h"
#include "ofMain.h"
#include "ofTrueTypeFont.h"

class GameManager {
public:
	ofTrueTypeFont scoreBoard;
	ofTrueTypeFont fpsFont;

	GameManager(Player &player1, Player &player2)
		: player1(player1), player2(player2), WIN_W(ofGetWidth()), WIN_H(ofGetHeight()) {
		scoreBoard.load(ofToDataPath("NotoSansNushu-Regular.ttf"), 42, true, true);
		scoreBoard.setLineHeight(28.0);
		scoreBoard.setLetterSpacing(1.05);
		fpsFont.load(ofToDataPath("verdana.ttf"), 22, true, true);
		startGame();
	}

	void startGame() {
		player1.score = 0;
		player2.score = 0;
		gameEnded = false;
		count = 0;
		winner = 0;
	}

	void update() {
		if (gameEnded) {
			count++;
			if (count >= 200) {
				startGame();
			}
			return;
		}

		// Game logic
		if (player1.score >= 11) {
			endGame(1);
		}
		if (player2.score >= 11) {
			endGame(2);
		}
	}

	void draw() {
		ofSetColor(255);
		scoreBoard.drawString("SCORE : " + ofToString(player1.score) + " | " + ofToString(player2.score), WIN_W / 3.0,
							  80);

		ofSetColor(25, 200, 111);
		fpsFont.drawString(ofToString((int)ofGetFrameRate()) + " FPS", WIN_W / 1.2, 30);

		if (gameEnded) {
			ofSetColor(255, 0, 0);
			scoreBoard.drawString("PLAYER " + ofToString(winner) + " WINS!", WIN_W / 2.0 - 150, WIN_H / 2.0);
		}
	}

	bool isGameEnded() const {
		return gameEnded;
	}

private:
	Player &player1;
	Player &player2;

	unsigned int count = 0;
	unsigned short int WIN_W;
	unsigned short int WIN_H;

	bool gameEnded = false;
	short winner = 0;

	void endGame(short winningPlayer) {
		gameEnded = true;
		winner = winningPlayer;
		count = 0;
	}
};
