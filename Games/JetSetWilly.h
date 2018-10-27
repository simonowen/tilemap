#pragma once
#include "SpectrumGame.h"

class JSW_Game : public SpectrumGame
{
public:
	JSW_Game(std::shared_ptr<TileView> pTileView, bool cheat_pokes);

private:
	void InstallHooks(std::vector<uint8_t> &mem);
	std::vector<MapRoom> BuildMap(const std::vector<uint8_t> &mem);

	static constexpr int PANEL_HEIGHT = 56;
	static constexpr int TILE_WIDTH = SPECTRUM_WIDTH_PIXELS;
	static constexpr int TILE_HEIGHT = SPECTRUM_HEIGHT_PIXELS - PANEL_HEIGHT;

	static constexpr int START_ROOM_ADDR = 0x87eb;

	bool m_seamless = true;
	std::vector<uint8_t> m_entity_data;
};
