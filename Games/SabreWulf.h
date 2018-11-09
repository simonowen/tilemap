#pragma once
#include "SpectrumGame.h"

class SW_Game : public SpectrumGame
{
public:
	SW_Game(std::shared_ptr<TileView> pTileView, bool cheat_pokes);

private:
	void InstallHooks(std::vector<uint8_t> &mem);
	std::vector<MapRoom> BuildMap(const std::vector<uint8_t> &mem);

	static constexpr int MAP_WIDTH = 16;
	static constexpr int MAP_HEIGHT = 16;
	static constexpr int PANEL_HEIGHT = 16;
	static constexpr int TILE_WIDTH = SPECTRUM_WIDTH_PIXELS;
	static constexpr int TILE_HEIGHT = SPECTRUM_HEIGHT_PIXELS - PANEL_HEIGHT;
	static constexpr int TILE_OFFSET_X = 0;
	static constexpr int TILE_OFFSET_Y = PANEL_HEIGHT;

	static constexpr int START_ROOM = 0xa8;
	static constexpr int ENTITY_STATE_BASE = 0x9702;
	static constexpr int ENTITY_COUNT = 25;
	static constexpr int ENTITY_STATE_SIZE = 0x0c;
	static constexpr int ENTITY_ROOM_OFFSET = 1;

	std::shared_mutex m_state_mutex;
	std::array<uint8_t, ENTITY_STATE_SIZE * ENTITY_COUNT> m_state_data;
};
