#include <iostream>
#include "GameLoop.h"
#include "Config.h"
#include "Texture.h"

GameLoop::GameLoop(Config* config) : cfg(config), window(nullptr), renderer(nullptr), smileBar(nullptr), gameField(nullptr), running(false)
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		std::cerr << "Can't initialize SDL: " << SDL_GetError() << '\n';
	}
	else
	{
		window = SDL_CreateWindow("Minesweeper",
				SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
				cfg->getWinWidth(), cfg->getWinHeight(),
				SDL_WINDOW_SHOWN);

		if (window == nullptr)
		{
			std::cerr << "Can't create window: " << SDL_GetError() << '\n';
		}
		else
		{
			renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

			if (renderer == nullptr)
			{
				std::cerr << "Can't create renderer: " << SDL_GetError() << '\n';
			}
			else
			{
				SDL_SetRenderDrawColor(renderer,
						Config::BACKGROUND_COLOR_R,
						Config::BACKGROUND_COLOR_G,
						Config::BACKGROUND_COLOR_B,
						Config::BACKGROUND_COLOR_OPAQUE);
				SDL_RenderPresent(renderer);
				smileBar = new SmileBar(cfg);
				gameField = new GameField(cfg);
			}
		}
	}
}

GameLoop::~GameLoop()
{
	if (smileBar != nullptr)
	{
		delete smileBar;
	}
	if (gameField != nullptr)
	{
		delete gameField;
	}
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	running = false;
	SDL_Quit();
}

void GameLoop::run()
{
	running = true;
	SDL_Event event;
	Texture texture(renderer, cfg->getSpritePath());

	// Initial render
	SDL_RenderClear(renderer);
	smileBar->render(texture, renderer);
	gameField->render(texture, renderer);
	SDL_RenderPresent(renderer);

	while (running)
	{
		SDL_WaitEvent(&event);
		bool change = onEvent(&event);

		if(change)
		{
			SDL_SetRenderDrawColor(renderer,
					Config::BACKGROUND_COLOR_R,
					Config::BACKGROUND_COLOR_G,
					Config::BACKGROUND_COLOR_B,
					Config::BACKGROUND_COLOR_OPAQUE);
			SDL_RenderClear(renderer);
			smileBar->render(texture, renderer);
			gameField->render(texture, renderer);
			SDL_RenderPresent(renderer);
		}
	}
}

bool GameLoop::onEvent(SDL_Event* event)
{
	bool change = false;
	if (event->type == SDL_QUIT)
	{
		running = false;
	}
	else
	{
		change |= gameField->handleEvent(event, smileBar);
		change |= smileBar->handleEvent(event, gameField);
	}
	return change;
}
