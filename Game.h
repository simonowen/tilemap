#pragma once
#include "TileView.h"

class Game;
extern std::shared_ptr<Game> g_pGame;

struct SparseMapRoom
{
	int room;
	int tile_x;
	int tile_y;
};

class Game
{
public:
	template <class TGame>
	static std::unique_ptr<Game> Create(std::shared_ptr<TileView> pTileView, bool cheat_pokes)
	{
		return std::make_unique<TGame>(pTileView, cheat_pokes);
	}

	Game(std::shared_ptr<TileView> pTileView, int start_room) :
		m_pTileView(pTileView),
		m_current_room(start_room)
	{
	}

	virtual ~Game() {}

public:
	virtual void SetMap(const std::vector<MapRoom> &map) = 0;
	virtual void RunFrame() = 0;

	virtual void CloneTile(const Tile &from_tile, Tile &to_tile) = 0;

	void CloneToAllTiles(const Tile &from_tile, std::function<void(Tile&)> func = [](Tile&) {})
	{
		for (auto &tile : m_tiles)
		{
			if (tile.room != from_tile.room)
			{
				auto lock = tile.Lock();
				CloneTile(from_tile, tile);
				func(tile);
			}
		}
	}

	using HookFunction = std::function<void(Tile&)>;
	virtual void Hook(std::vector<uint8_t> &mem, uint16_t address, uint8_t expected_opcode, HookFunction fn) = 0;
	virtual void OnHook(uint16_t address, Tile &tile) = 0;

	virtual void BlankScreen(Tile &tile) = 0;
	virtual void CreateTileMask(Tile &tile, uint8_t *pDisplay, uint8_t *pMask) = 0;

	Tile &FindRoomTile(int room)
	{
		for (auto &td : m_tiles)
		{
			if (td.room == room)
				return td;
		}

		throw std::logic_error("non-existent room requested");
	}

	Tile &ActiveTile()
	{
		return FindRoomTile(m_current_room);
	}

	bool IsActiveTile(const Tile &tile)
	{
		return tile.room == m_current_room;
	}

	void ActivateTile(Tile &tile, bool force_centre=false)
	{
		m_current_room = tile.room;
		tile.running = true;

		if (force_centre)
			m_pTileView->CentreViewOnTile(tile);
		else
			m_pTileView->EnsureTileVisible(tile);

		m_pTileView->UpdatePanels();
	}
	int GetCurrentRoom() { return m_current_room; };

	std::vector<Tile> &Tiles()
	{
		return m_tiles;
	}

	std::vector<MapRoom> BuildRegularMap(
		int map_width, int map_height,
		int tile_width, int tile_height,
		int gap_x=0, int gap_y=0,
		int offset_x=0, int offset_y=0,
		std::function<int(int, int, int)> room_func = [](int x, int y, int pitch) { return y * pitch + x; })
	{
		std::vector<MapRoom> tiles;
		tiles.reserve(map_width * map_height);

		for (int tile_y = 0; tile_y < map_height; ++tile_y)
		{
			for (int tile_x = 0; tile_x < map_width; ++tile_x)
			{
				MapRoom tile;
				tile.room = room_func(tile_x, tile_y, map_width);
				tile.x = TileToPixels(tile_x, tile_width, gap_x);
				tile.y = TileToPixels(tile_y, tile_height, gap_y);
				tile.width = tile_width;
				tile.height = tile_height;
				tile.offset_x = offset_x;
				tile.offset_y = offset_y;
				tiles.push_back(tile);
			}
		}

		return tiles;
	}

	std::vector<MapRoom> BuildSparseMap(
		const std::vector<SparseMapRoom> &map_rooms,
		int tile_width, int tile_height,
		int gap_x = 0, int gap_y = 0,
		int offset_x = 0, int offset_y = 0)
	{
		std::vector<MapRoom> tiles;
		tiles.reserve(map_rooms.size());

		for (auto &room : map_rooms)
		{
			MapRoom tile;
			tile.room = room.room;
			tile.x = TileToPixels(room.tile_x, tile_width, gap_x);
			tile.y = TileToPixels(room.tile_y, tile_height, gap_y);
			tile.width = tile_width;
			tile.height = tile_height;
			tile.offset_x = offset_x;
			tile.offset_y = offset_y;
			tiles.push_back(tile);
		}

		return tiles;
	}

public:
	std::shared_ptr<TileView> m_pTileView;
	ThreadPool m_thread_pool;
	std::vector<std::future<void>> m_thread_futures;

protected:
	int m_current_room = -1;
	std::vector<Tile> m_tiles;
	std::map<uint16_t, HookFunction> m_hooks;
};
