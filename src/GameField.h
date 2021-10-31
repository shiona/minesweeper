#ifndef _GAME_FIELD_H
#define _GAME_FIELD_H

#include <functional>
#include <vector>

#include <SDL2/SDL.h>
#include "Config.h"
#include "Texture.h"
#include "SmileBar.h"

enum class GameState;

class SmileBar;

class GameField
{
	friend class SmileBar;

	public:
		GameField(Config* config);
		void render(Texture& texture, SDL_Renderer* const renderer);
		void handleEvent(SDL_Event* event, SmileBar* smileBar);

	private:
		enum GridFront {
			Blank,
			Flag,
			Qm,
			Revealed,
		};

		enum GridBack {
			Clear,
			Mine
		};

		Config* cfg;
		size_t rowCount;
		size_t colCount;
		size_t mineCount;
		std::vector<std::vector<GridFront>> front;
		std::vector<std::vector<GridBack>> back;
		std::vector<std::vector<size_t>> numbers;
		size_t pressedRow;
		size_t pressedCol;
		GameState gameState;
		int topBarHeight;
		void reset();
		// TODO: Parameters swapped, Going to break things
		bool insideField(int x, int y);
		void generateField();
		void openEmptyCells(size_t row, size_t col);
		void gameOver();
		void openAllFlags();
		bool isWin();
		void cycleFlagQm(size_t row, size_t col, SmileBar* smileBar);
		void reveal(size_t row, size_t col);
		void revealArea(size_t row, size_t col/*, size_t size*/);
		bool isSatisfied(size_t row, size_t col);
		void modifyAround(size_t row, size_t col, size_t size, std::function<void(GridFront&, GridBack&, size_t, size_t)> fn);
		unsigned int countAround(size_t row, size_t col, size_t size, bool (*p)(GridFront, GridBack));
		size_t clipId(size_t row, size_t col, bool gameOver);
};

#endif
