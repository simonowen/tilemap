#pragma once

#include "Game.h"
#include "Z80.h"

constexpr unsigned SPECTRUM_SCREEN_BYTES = 6912;
constexpr int SPECTRUM_WIDTH_PIXELS = 256;
constexpr int SPECTRUM_HEIGHT_PIXELS = 192;
const int SPECTRUM_CYCLES_PER_SECOND = 3'500'000;
const int SPECTRUM_FRAMES_PER_SECOND = 50;
const int SPECTRUM_CYCLES_PER_FRAME = SPECTRUM_CYCLES_PER_SECOND / SPECTRUM_FRAMES_PER_SECOND;
const int SPECTRUM_CYCLES_PER_INT = 32;
const int SPECTRUM_CYCLES_BEFORE_INT = SPECTRUM_CYCLES_PER_FRAME - SPECTRUM_CYCLES_PER_INT;

#define Z80_AF(t)	Z_Z80_STATE_AF(&(t).z80.state)
#define Z80_BC(t)	Z_Z80_STATE_BC(&(t).z80.state)
#define Z80_DE(t)	Z_Z80_STATE_DE(&(t).z80.state)
#define Z80_HL(t)	Z_Z80_STATE_HL(&(t).z80.state)
#define Z80_IX(t)	Z_Z80_STATE_IX(&(t).z80.state)
#define Z80_IY(t)	Z_Z80_STATE_IY(&(t).z80.state)
#define Z80_PC(t)	Z_Z80_STATE_PC(&(t).z80.state)
#define Z80_SP(t)	Z_Z80_STATE_SP(&(t).z80.state)
#define Z80_A(t)    Z_Z80_STATE_A(&(t).z80.state)
#define Z80_F(t)    Z_Z80_STATE_F(&(t).z80.state)
#define Z80_B(t)    Z_Z80_STATE_B(&(t).z80.state)
#define Z80_C(t)    Z_Z80_STATE_C(&(t).z80.state)
#define Z80_D(t)    Z_Z80_STATE_D(&(t).z80.state)
#define Z80_E(t)    Z_Z80_STATE_E(&(t).z80.state)
#define Z80_H(t)    Z_Z80_STATE_H(&(t).z80.state)
#define Z80_L(t)    Z_Z80_STATE_L(&(t).z80.state)
#define Z80_IXH(t)  Z_Z80_STATE_IXH(&(t).z80.state)
#define Z80_IXL(t)  Z_Z80_STATE_IXL(&(t).z80.state)
#define Z80_IYH(t)  Z_Z80_STATE_IYH(&(t).z80.state)
#define Z80_IYL(t)  Z_Z80_STATE_IYL(&(t).z80.state)
#define Z80_I(t)    Z_Z80_STATE_I(&(t).z80.state)
#define Z80_R(t)    Z_Z80_STATE_R(&(t).z80.state)
#define Z80_STATE(t)	((t).z80.state)

inline void Push(Tile &tile, uint16_t value)
{
	Z80_SP(tile) -= 2;
	tile.DPoke(Z80_SP(tile), value);
}

inline uint16_t Pop(Tile &tile)
{
	uint16_t value = tile.DPeek(Z80_SP(tile));
	Z80_SP(tile) += 2;
	return value;
}

inline void Jump(Tile &tile, uint16_t address)
{
	Z80_PC(tile) = address;
}

inline void Call(Tile &tile, uint16_t address)
{
	Push(tile, Z80_PC(tile));
	Jump(tile, address);
}

inline void Ret(Tile &tile)
{
	Jump(tile, Pop(tile));
}


class SpectrumGame : public Game
{
public:
	SpectrumGame(std::shared_ptr<TileView> m_pTileView, int start_room=0)
		: Game(m_pTileView, start_room)
	{
		m_pTileView->SetScreenBytes(SPECTRUM_SCREEN_BYTES);
	}

	void Poke(std::vector<uint8_t> &mem, uint16_t address, uint8_t value, uint8_t expected_value)
	{
		if (mem[address] != expected_value)
			throw std::exception("incompatible snapshot");

		mem[address] = value;
	}

		void Hook(std::vector<uint8_t> &mem, uint16_t address, uint8_t expected_opcode, HookFunction fn)
	{
		const uint8_t BREAKPOINT_OPCODE = 0x64;	// LD H,H

		if (mem[address] != expected_opcode)
			throw std::exception("snapshot is incompatible with code hooks");	// TODO: show location and expected/found byte

		mem[address] = BREAKPOINT_OPCODE;
		m_hooks[address] = fn;
	}

	void OnHook(uint16_t address, Tile &tile)
	{
		auto cpu = &tile.z80.state;
		auto it = m_hooks.find(address);

		if (it != m_hooks.end())
			(it->second)(tile);
		else
			Z80_PC(tile)++;
	}

	void RunFrame() override final;
	void CloneTile(const Tile &from_tile, Tile &to_tile) override final;
	void BlankScreen(Tile &tile) override final;
	void CreateTileMask(Tile &tile, uint8_t *pDisplay, uint8_t *pMask) override final;
	virtual std::vector<POINT> FillHints(const Tile &tile);

	std::vector<uint8_t> &load_snapshot(_In_ const std::wstring &filename);
	void SetMap(const std::vector<MapRoom> &map) override final;

	ZZ80State snapshot_cpu_state;
	std::vector<uint8_t> snapshot_mem;
};
