#pragma once
#include "SpectrumGame.h"

class JSW2_Game : public SpectrumGame
{
public:
	JSW2_Game(std::shared_ptr<TileView> pTileManager, bool cheat_pokes);

private:
	void InstallHooks(std::vector<uint8_t> &mem);
	std::vector<MapRoom> JSW2_Game::BuildMap(const std::vector<uint8_t> &mem);

	static constexpr int PANEL_HEIGHT = 56;
	static constexpr int TILE_WIDTH = SPECTRUM_WIDTH_PIXELS;
	static constexpr int TILE_HEIGHT = SPECTRUM_HEIGHT_PIXELS - PANEL_HEIGHT;

	static constexpr int START_ROOM_ADDR = 0x798f;
	static constexpr int CARTOGRAPHY_ROOM = 0x6c;

	int m_start_room{ -1 };
};
