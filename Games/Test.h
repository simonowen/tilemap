#pragma once
#include "SpectrumGame.h"

class Test_Game : public SpectrumGame
{
public:
	Test_Game(std::shared_ptr<TileView> pTileManager, bool cheat_pokes);

private:
	std::vector<MapRoom> BuildMap(const std::vector<uint8_t> &mem);

	static constexpr int GRID_SIZE = 3;
	static constexpr int PANEL_HEIGHT = 8;
	static constexpr int START_ROOM = GRID_SIZE * GRID_SIZE / 2;
	static constexpr int MAP_WIDTH = GRID_SIZE;
	static constexpr int MAP_HEIGHT = GRID_SIZE;
	static constexpr int TILE_WIDTH = SPECTRUM_WIDTH_PIXELS;
	static constexpr int TILE_HEIGHT = SPECTRUM_HEIGHT_PIXELS;
};
