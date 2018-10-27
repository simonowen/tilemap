#pragma once
#include "SpectrumGame.h"

class TT_Game : public SpectrumGame
{
public:
	TT_Game(std::shared_ptr<TileView> pTileView, bool cheat_pokes);

private:
	void InstallHooks(std::vector<uint8_t> &mem);
	std::vector<MapRoom> BuildMap(const std::vector<uint8_t> &mem);

	static constexpr int PANEL_HEIGHT = 40;
	static constexpr int TILE_WIDTH = SPECTRUM_WIDTH_PIXELS;
	static constexpr int TILE_HEIGHT = SPECTRUM_HEIGHT_PIXELS - PANEL_HEIGHT;

	static constexpr int START_ROOM = 0;
	static constexpr int END_ROOM = 48;
	static constexpr int LIFT_ROOM = 42;

	bool m_task_completed{ false };
};
