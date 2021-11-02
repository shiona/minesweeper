#include <iostream>
#include <algorithm>
#include <random>
#include <chrono>
#include <queue>
#include "GameField.h"

static const size_t INF = 123456789;
/*
static const int shiftCount = 8;
static int rowShifts[shiftCount] = {-1, -1, 0, 1, 1, 1, 0, -1};
static int colShifts[shiftCount] = {0, 1, 1, 1, 0, -1, -1, -1};
*/
enum class GameState
{
	INIT,
	IN_PROGRESS,
	WIN,
	LOSE
};

GameField::GameField(Config* config)
	: cfg(config), rowCount(cfg->getFieldRowCnt()), colCount(cfg->getFieldColCnt()), mineCount(cfg->getMineCnt()), pressedRow(INF), pressedCol(INF), gameState(GameState::INIT), topBarHeight(0)
{
	topBarHeight = 2 + cfg->getClip(Clip::SMILE_INIT)->h + 2;
	generateField();
}

/*
 * Return the Clip ID (Config.h) of a square on the grid for renderer
 */
size_t GameField::clipId(size_t row, size_t col, bool gameOver)
{
	size_t num = numbers[row][col];
	size_t content = back[row][col] == Mine ? Clip::CELL_MINE : num ? Clip::CELL_1 + numbers[row][col] - 1 : Clip::CELL_PRESSED;

	if (gameOver)
	{
		if (front[row][col] == Flag)
		{
			if (back[row][col] == Mine)
			{
				return Clip::CELL_FLAG;
			}
			else
			{
				return Clip::CELL_MINE_WRONG;
			}
		}
		return content;
	}

	// TODO: If both buttons are held, a sweeper_area squared should be PRESSED
	if (row == pressedRow && col == pressedCol)
	{
		switch (front[row][col])
		{
		case Blank:    return Clip::CELL_PRESSED;
		case Flag:     return Clip::CELL_FLAG_PRESSED;
		case Qm:       return Clip::CELL_QM_PRESSED;
		case Revealed: return content;
		}
	}
	else
	{
		switch (front[row][col])
		{
		case Blank:    return Clip::CELL_INIT;
		case Flag:     return Clip::CELL_FLAG;
		case Qm:       return Clip::CELL_QM;
		case Revealed: return content;
		}
	}
}

void GameField::render(Texture& texture, SDL_Renderer* const renderer)
{
	int x = 0;
	int xStep = cfg->getClip(Clip::CELL_INIT)->w;
	bool gameOver = gameState == GameState::LOSE;

	for (size_t col = 0; col < colCount; col++, x += xStep)
	{
		int y = topBarHeight;
		for (size_t row = 0; row < rowCount; row++)
		{
			const SDL_Rect* clip = cfg->getClip(clipId(row, col, gameOver));
			texture.render(x, y, clip, renderer);
			y += clip->h;
		}
	}
}

void GameField::cycleFlagQm(size_t row, size_t col, SmileBar* smileBar)
{
	switch (front[row][col])
	{
	case GridFront::Blank:
		front[row][col] = GridFront::Flag;
		smileBar->decrMines();
		break;
	case GridFront::Flag:
		front[row][col] = GridFront::Qm;
		smileBar->incrMines();
		break;
	case GridFront::Qm:
		front[row][col] = GridFront::Blank;
		break;
	case GridFront::Revealed:
		// Do nothing
		break;
	}
}

void GameField::reveal(size_t row, size_t col)
{
	if (front[row][col] == Blank)
	{
		front[row][col] = GridFront::Revealed;
		switch (back[row][col])
		{
		case GridBack::Mine:
			gameOver();
			break;
		case GridBack::Clear:
			openEmptyCells(row, col);
			break;
		}
	}
}


void GameField::revealArea(size_t row, size_t col)
{
	modifyAround(row, col, cfg->getSweeperSize(), [this](GridFront& /*unused*/, GridBack& /*unused*/, size_t r, size_t c) {
			reveal(r, c);
			});
}


// TODO: Refactor these two to use same code
void GameField::modifyAround(size_t row, size_t col, size_t size, std::function<void(GridFront&, GridBack&, size_t, size_t)> fn)
{
	size_t delta = size/2; // Rounds down, good
	size_t miny = row < delta ? 0 : row-delta;
	size_t minx = col < delta ? 0 : col-delta;
	size_t maxy = std::min(row + delta, rowCount-1);
	size_t maxx = std::min(col + delta, colCount-1);
	for (size_t x = minx; x <= maxx; x++)
	{
		for (size_t y = miny; y <= maxy; y++)
		{
			// Do not handle for center
			if (y != row || x != col)
			{
				fn(front[y][x], back[y][x], y, x);
			}
		}
	}
}

unsigned int GameField::countAround(size_t row, size_t col, size_t size, bool (*p)(GridFront, GridBack))
{
	size_t delta = size/2; // Rounds down, good
	size_t miny = row < delta ? 0 : row-delta;
	size_t minx = col < delta ? 0 : col-delta;
	size_t maxy = std::min(row + delta, rowCount-1);
	size_t maxx = std::min(col + delta, colCount-1);

	unsigned int result = 0;

	for (size_t x = minx; x <= maxx; x++)
	{
		for (size_t y = miny; y <= maxy; y++)
		{
			if (y != row || x != col)
			{
				if (p(front[y][x], back[y][x]))
				{
					result++;
				}
			}
		}
	}
	return result;
}

bool GameField::isSatisfied(size_t row, size_t col)
	//[[ expects: front[row][col] == GridFront::Revealed, back[row][col] == GridBack::Clear ]]
{
	//[[ assert: front[row][col] == GridFront::Revealed ]];
	//[[ assert: back[row][col] :: GridBack::Clear ]];
	unsigned int flags = countAround(row, col, cfg->getSweeperSize(),
			[](GridFront front, GridBack /*unused*/) { return front == GridFront::Flag; });
	return flags == numbers[row][col];
}

void GameField::handleEvent(SDL_Event* event, SmileBar* smileBar)
{
	if (gameState != GameState::IN_PROGRESS && gameState != GameState::INIT)
	{
		return;
	}
	int x = (event->button).x;
	int y = (event->button).y;

	const SDL_Rect* clip = cfg->getClip(Clip::CELL_INIT);
	size_t row = (y - topBarHeight) / clip->h;
	size_t col = x / clip->w;
	auto btnType = (event->button).button;

	if (event->type == SDL_MOUSEBUTTONDOWN)
	{
		switch (btnType) {
		case SDL_BUTTON_LEFT:
			if (insideField(x, y))
			{
				pressedRow = row;
				pressedCol = col;
				smileBar->smileState = Clip::SMILE_WONDER;
			}
			break;
		case SDL_BUTTON_RIGHT:
			if (insideField(x, y))
			{
				cycleFlagQm(row, col, smileBar);
			}
		}
	}
	else if (event->type == SDL_MOUSEBUTTONUP && btnType == SDL_BUTTON_LEFT)
	{
		// TODO: Optional values
		if(pressedRow != INF && pressedCol != INF)
		{
			if (front[pressedRow][pressedCol] == GridFront::Blank)
			{
				if (gameState == GameState::INIT)
				{
					gameState = GameState::IN_PROGRESS;
					smileBar->startTimer();
				}
				reveal(pressedRow, pressedCol);
			}
			else if (front[pressedRow][pressedCol] == GridFront::Revealed)
			{
				// If the right button is held down
				if (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_RMASK)
				{
					if (isSatisfied(pressedRow, pressedCol))
					{
						revealArea(pressedRow, pressedCol);
					}
				}
			}
			pressedRow = pressedCol = INF;
			smileBar->smileState = Clip::SMILE_INIT;
		}
	}
	else if (event->type == SDL_MOUSEMOTION)
	{
		// Are any buttons being held down?
		if (reinterpret_cast<SDL_MouseMotionEvent*>(event)->state)
		{
			if (insideField(x, y))
			{
				pressedRow = row;
				pressedCol = col;	
				smileBar->smileState = Clip::SMILE_WONDER;
			}
			else 
			{
				pressedRow = pressedCol = INF;
				smileBar->smileState = Clip::SMILE_INIT;
			}
		}
	}

	if (gameState == GameState::LOSE)
	{
		smileBar->smileState = Clip::SMILE_LOSE;
		smileBar->stopTimer();
	}
	else if (isWin())
	{
		std::cout<<"WIN \\o/\n";
		gameState = GameState::WIN;
		smileBar->stopTimer();
		smileBar->smileState = Clip::SMILE_WIN;
		openAllFlags();
		// TODO: Leaderboard?
	}
}

void GameField::reset()
{
	for (size_t row = 0; row < rowCount; row++)
	{
		for (size_t col = 0; col < colCount; col++)
		{
			front[row][col] = Blank;
		}
	}
	generateField();
	pressedRow = INF;
	pressedCol = INF;
	gameState = GameState::INIT;
}

bool GameField::insideField(int x, int y)
{
	return x >= 0 && x <= static_cast<int>(cfg->getWinWidth())
		&& y >= topBarHeight && y <= static_cast<int>(cfg->getWinHeight());
}

void GameField::generateField()
{
	const int totalCellCount = rowCount * colCount;
	std::vector<int> cells = std::vector<int>(totalCellCount, 0);
	std::fill_n(cells.begin(), mineCount, -1);

	size_t seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::shuffle(cells.begin(), cells.end(), std::default_random_engine(seed));

	front = std::vector(rowCount, std::vector(colCount, GridFront::Blank));
	back = std::vector(rowCount, std::vector(colCount, GridBack::Clear));
	numbers = std::vector(rowCount, std::vector(colCount, size_t(0)));

	// Set the mines
	for (int i = 0; i < totalCellCount; i++)
	{
		if (cells[i] < 0)
		{
			int row = i / colCount;
			int col = i % colCount;
			back[row][col] = GridBack::Mine;
		}
	}

	// Calculate numbers for the grid
	for (size_t row = 0; row < rowCount; row++)
	{
		for (size_t col = 0; col < colCount; col++)
		{
			if (back[row][col] == Mine)
				continue;

			numbers[row][col] = countAround(row, col, cfg->getSweeperSize(),
					[](GridFront /*unused*/, GridBack back) {
					return back == Mine; });
		}
	}
}

void GameField::openEmptyCells(size_t row, size_t col)
{
	std::queue<std::pair<size_t, size_t>> queue;
	queue.push({row, col});

	while (!queue.empty())
	{
		std::pair<size_t, size_t> cell = queue.front();
		queue.pop();

		size_t row = cell.first;
		size_t col = cell.second;

		if(numbers[row][col] == 0)
		{
			modifyAround(row, col, cfg->getSweeperSize(),
					[&queue](GridFront& front, GridBack& /*unused*/, size_t newRow, size_t newCol) {
					// Check if this is correct or how Flags and esp. Qms are handled
					if(front == Blank) {
						front = Revealed;
						queue.push({newRow, newCol}); }
					});
		}
	}
}

void GameField::gameOver()
{
	gameState = GameState::LOSE;
}

void GameField::openAllFlags()
{
	for (size_t row = 0; row < rowCount; row++)
	{
		for (size_t col = 0; col < colCount; col++)
		{
			if (back[row][col] == Mine)
			{
				front[row][col] = Flag;
			}
		}
	}
}

bool GameField::isWin()
{
	if (gameState == GameState::LOSE)
	{
		return false;
	}

	for (size_t row = 0; row < rowCount; row++)
	{
		for (size_t col = 0; col < colCount; col++)
		{
			if (back[row][col] == Clear && front[row][col] != Revealed)
			{
				return false;
			}
		}
	}
	return true;
}
