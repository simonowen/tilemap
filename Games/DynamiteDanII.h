#pragma once
#include "SpectrumGame.h"

class DD2_Game : public SpectrumGame
{
public:
	DD2_Game(std::shared_ptr<TileView> pTileView, bool cheat_pokes);

private:
	void InstallHooks(std::vector<uint8_t> &mem);
	std::vector<MapRoom> BuildMap(const std::vector<uint8_t> &mem);

	static constexpr int MAP_WIDTH = 64;
	static constexpr int MAP_HEIGHT = 3;
	static constexpr int PANEL_HEIGHT = 40;
	static constexpr int TILE_WIDTH = SPECTRUM_WIDTH_PIXELS;
	static constexpr int TILE_HEIGHT = SPECTRUM_HEIGHT_PIXELS - PANEL_HEIGHT;
	static constexpr int TILE_GAP_X = -16;
	static constexpr int TILE_GAP_Y = -24;

	static constexpr int AIRSHIP_ROOM = 0xc0;
	static constexpr int FIRST_ROOM = 0xbe;
	static constexpr int NUM_ISLANDS = 8;
};
