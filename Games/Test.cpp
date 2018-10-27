// Spectrum ROM Test (NxN tiles)

#include "stdafx.h"
#include "Test.h"

Test_Game::Test_Game(std::shared_ptr<TileView> pTileView, bool /*cheat_pokes*/)
: SpectrumGame(pTileView, START_ROOM)
{
	auto &mem = load_snapshot(L"tesdt.sna");
	SetMap(BuildMap(mem));
	pTileView->SetPanels({ { 0, TILE_HEIGHT - PANEL_HEIGHT, TILE_WIDTH, PANEL_HEIGHT, PanelAlignment::Bottom } });
}

std::vector<MapRoom> Test_Game::BuildMap(const std::vector<uint8_t> &mem)
{
	return BuildRegularMap(MAP_WIDTH, MAP_HEIGHT, TILE_WIDTH, TILE_HEIGHT);
}
