// Sabre Wulf (256 rooms)  *** INCOMPLETE ***
//
// ToDo:
//	- sync natives
//	- sync hippo/warthog
//	- sync orchid state?

#include "stdafx.h"
#include "SabreWulf.h"

SW_Game::SW_Game(std::shared_ptr<TileView> pTileView, bool cheat_pokes)
	: SpectrumGame(pTileView, START_ROOM)
{
	auto &mem = load_snapshot(L"sabrewulf.sna");

	// Tweaks
	mem[0xbd5a] = 0x3e;		// Skip menu tune
	mem[0xbd72] = 0x18;		// Skip start tune
	mem[0xb437] = 0xc9;		// Auto-start game
	mem[0xb42e] = 0xed;		// Auto-select Interface 2
	mem[0xac28] = 0xc9;		// Disable pause key

	if (cheat_pokes)
		mem[0xaa37] = 0x00;		// infinite lives

	InstallHooks(mem);
	SetMap(BuildMap(mem));
	m_pTileView->SetPanels({ { 0, 0, TILE_WIDTH, PANEL_HEIGHT, PanelAlignment::Top } });
}


void SW_Game::InstallHooks(std::vector<uint8_t> &mem)
{
	// Start of game
	Hook(mem, 0xb1dd, 0xaf /*XOR A*/, [&](Tile &tile)
	{
		if (tile.room == START_ROOM)
		{
			CloneToAllTiles(tile);
			ActivateTile(tile);
		}
	});

	// Entering-room helper
	auto entering_new_room = [&](Tile &tile)
	{
		auto room_addr = ENTITY_STATE_BASE + ENTITY_ROOM_OFFSET;
		auto new_room = tile.mem[room_addr];
		tile.mem[0x9703] = tile.room;

		if (IsActiveTile(tile) && new_room != tile.room)
		{
			auto &tile_new = FindRoomTile(new_room);
			auto lock = tile_new.Lock();

			CloneTile(tile, tile_new);
			tile_new.mem[room_addr] = new_room;

			ActivateTile(tile_new);
			tile_new.drawing = false;
		}

		if (!IsActiveTile(tile))
		{
			// Disable sabreman entity, which disables other spawning creatures.
			tile.mem[ENTITY_STATE_BASE] = 0x00;
		}

		tile.drawing = false;
	};

	// Entering a new room
	Hook(mem, 0xb201, 0xcd /*CALL nn*/, entering_new_room);
	Hook(mem, 0xb151, 0xcd /*CALL nn*/, entering_new_room);

	// After-room-drawn helper
	auto after_room_drawn = [&](Tile &tile)
	{
		tile.drawing = true;
	};

	// After room is drawn
	Hook(mem, 0xbbba, 0xc9 /*RET*/, after_room_drawn);
	Hook(mem, 0xb204, 0xc9 /*RET*/, after_room_drawn);

	auto copy_entity_state = [&](Tile &tile)
	{
		// IX points to the entity data base. Determine the offset into all entities.
		uint16_t instance_offset = Z80_IX(tile) - ENTITY_STATE_BASE;

		if (IsActiveTile(tile))
		{
			// Update the state data, under exclusive lock_guard access.
			std::lock_guard<std::shared_mutex> lock(m_state_mutex);
			memcpy(&m_state_data[instance_offset], &tile.mem[Z80_IX(tile)], ENTITY_STATE_SIZE);
		}
		else if (m_state_data[instance_offset])
		{
			// Take a copy of the data, under shared_lock access to state data.
			std::shared_lock<std::shared_mutex> lock(m_state_mutex);
			memcpy(&tile.mem[Z80_IX(tile)], &m_state_data[instance_offset], ENTITY_STATE_SIZE);
		}
	};

	// Coordinate update in active room, or copy in inactive room.
	Hook(mem, 0x9de2, 0xdd /*LD (IX+4),D*/, copy_entity_state);		// Fire
	Hook(mem, 0xa87d, 0xdd /*LD (IX+4),D*/, copy_entity_state);		// Wulf
	Hook(mem, 0xad2b, 0xdd /*LD (IX+4),D*/, copy_entity_state);		// Rhino
}

std::vector<MapRoom> SW_Game::BuildMap(const std::vector<uint8_t> &mem)
{
	return BuildRegularMap(MAP_WIDTH, MAP_HEIGHT, TILE_WIDTH, TILE_HEIGHT,
		0, 0, TILE_OFFSET_X, TILE_OFFSET_Y);
}
