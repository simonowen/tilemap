#pragma once
#include "SpectrumGame.h"

class SD_Game : public SpectrumGame
{
public:
	SD_Game(std::shared_ptr<TileView> pTileManager, bool cheat_pokes);

private:
	void InstallHooks(std::vector<uint8_t> &mem);
	std::vector<MapRoom> BuildMap(const std::vector<uint8_t> &mem);
	std::vector<POINT> FillHints(const Tile &tile) override final;

private:
	static constexpr int MAP_BASE_X = 0x31;
	static constexpr int MAP_BASE_Y = 0x29;
	static constexpr int PANEL_HEIGHT = 0;
	static constexpr int TILE_WIDTH = SPECTRUM_WIDTH_PIXELS;
	static constexpr int TILE_HEIGHT = SPECTRUM_HEIGHT_PIXELS - PANEL_HEIGHT;
	static constexpr int TILE_OFFSET_X = 0;
	static constexpr int TILE_OFFSET_Y = PANEL_HEIGHT;

	constexpr static int START_ROOM = 0x4040;
};
