#pragma once
#include "SpectrumGame.h"

class SQ_Game : public SpectrumGame
{
public:
	SQ_Game(std::shared_ptr<TileView> pTileView, bool cheat_pokes);

private:
	void InstallHooks(std::vector<uint8_t> &mem);
	std::vector<MapRoom> BuildMap(const std::vector<uint8_t> &mem);

	static constexpr int MAP_WIDTH = 16;
	static constexpr int MAP_HEIGHT = 32;
	static constexpr int PANEL_HEIGHT = 48;
	static constexpr int TILE_WIDTH = SPECTRUM_WIDTH_PIXELS;
	static constexpr int TILE_HEIGHT = SPECTRUM_HEIGHT_PIXELS - PANEL_HEIGHT;
	static constexpr int TILE_OFFSET_X = 0;
	static constexpr int TILE_OFFSET_Y = PANEL_HEIGHT;

	static constexpr int START_ROOM = 8;
	static constexpr int DOWN_ONE_ROOM = 8 + 16;
	static constexpr int CORE_ROOM = 199;
};
