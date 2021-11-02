#ifndef _GAME_LOOP_H
#define _GAME_LOOP_H

#include <SDL2/SDL.h>
#include "SmileBar.h"
#include "GameField.h"

class GameLoop
{
	public:
		GameLoop(Config* config);
		~GameLoop();
		void run();

	private:
		Config* cfg;
		SDL_Window* window;
		SDL_Renderer* renderer;
		SmileBar* smileBar;
		GameField* gameField;
		bool running;
		bool onEvent(SDL_Event* event);
};

#endif
