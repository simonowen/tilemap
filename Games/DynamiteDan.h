#pragma once
#include "SpectrumGame.h"

class DD_Game : public SpectrumGame
{
public:
	DD_Game(std::shared_ptr<TileView> pTileManager, bool cheat_pokes);

private:
	void InstallHooks(std::vector<uint8_t> &mem);
	std::vector<MapRoom> BuildMap(const std::vector<uint8_t> &mem);

private:
	static constexpr int MAP_WIDTH = 8;
	static constexpr int MAP_HEIGHT = 6;
	static constexpr int PANEL_HEIGHT = 32;
	static constexpr int TILE_WIDTH = SPECTRUM_WIDTH_PIXELS;
	static constexpr int TILE_HEIGHT = SPECTRUM_HEIGHT_PIXELS - PANEL_HEIGHT;
	static constexpr int TILE_GAP_X = -16;
	static constexpr int TILE_GAP_Y = -24;

	constexpr static int START_ROOM = 0x2b;
	constexpr static int DOWN_ONE_ROOM = START_ROOM - 8;
	constexpr static int SAFE_ROOM = 0x2b - 16;
};
