#pragma once

#include "Z80.h"
#include "Utils.h"

class Game;

struct MapRoom
{
	int room{ -1 };
	int x{ 0 };
	int y{ 0 };
	int width{ 0 };
	int height{ 0 };
	int offset_x{ 0 };
	int offset_y{ 0 };
};

enum class PanelAlignment { TopLeft, Top, TopRight, Left, Right, BottomLeft, Bottom, BottomRight };

struct Panel
{
	int offset_x{ 0 };
	int offset_y{ 0 };
	int width{ 0 };
	int height{ 0 };
	PanelAlignment align{ PanelAlignment::Top };
};

class Tile : public MapRoom
{
public:
	int screen_index = -1;
	zusize frame_cycles = 0;
	bool visible = true;
	bool running = true;
	bool drawing = true;
	bool mask_dirty = false;

	Game *pGame{ nullptr };
	Z80 z80{};
	std::vector<uint8_t> mem;

private:
	std::mutex mutex;

public:
	static std::mutex shared_lock;

	std::unique_lock<std::mutex> Lock()
	{
		return std::unique_lock<std::mutex>(mutex);
	}

	void CloneToTileNoLock(Tile &target_tile, uint16_t start_address = 0x4000)
	{
		if (target_tile.room != room)
		{
			std::copy(mem.begin() + start_address, mem.end(), target_tile.mem.begin() + start_address);
			target_tile.z80.state = z80.state;
		}
	}

	uint16_t DPeek(uint16_t address)
	{
		uint16_t value = mem[address++];
		value |= (mem[address] << 8);
		return value;
	}

	uint16_t DPeekDPeek(uint16_t address)
	{
		return DPeek(DPeek(address));
	}

	uint16_t DPoke(uint16_t address, uint16_t value)
	{
		mem[address++] = value & 0xff;
		mem[address] = value >> 8;
		return value;
	}
};

constexpr float DEFAULT_VIEW_MAGNIFY = 2.0f;

struct VSConstants
{
	uint32_t total_x{ 1 };
	uint32_t total_y{ 1 };
	float translate_x{ 0.0f };
	float translate_y{ 0.0f };

	float scale_x{ 1.0f };
	float scale_y{ 1.0f };
	float scale{ DEFAULT_VIEW_MAGNIFY };
	int32_t screen_bytes{ 0 };
};

struct PSConstants
{
	uint32_t flash_invert{ 0 };
	uint32_t transparent{ 1 };
	uint32_t padding[2]{};
};

struct InstanceType
{
	int32_t x;
	int32_t y;
	int32_t width;
	int32_t height;
	int32_t offset_x;
	int32_t offset_y;
	int32_t screen_index;
};


class TileView
{
public:
	TileView(HWND hwnd) : m_hwnd(hwnd) { }

	void SetMap(const std::vector<MapRoom> &rooms);
	void SetPanels(const std::vector<Panel> &panels);
	void SetScreenBytes(int screen_bytes) { sVSConstants.screen_bytes = screen_bytes; }

	SIZE TotalSize() const;
	float GetScale() const { return m_scale; }
	void SetSingleTile(bool single_tile) { m_single_tile = single_tile; }

	bool CheckTearingSupport();
	HRESULT InitD3D();
	void ExitD3D();
	HRESULT OnResize();
	HRESULT Render();

	void UpdateInstanceData();
	void UpdateScreens();
	void UpdateMasks();
	void UpdateView();
	void UpdateTiles();
	void UpdatePanels();
	void EnsureTileVisible(const Tile &tile, bool instant=false);
	void CentreViewOnTile(const Tile &tile, bool instant=false);
	void CentreOnWorldPointX(int x, bool instant=false);
	void CentreOnWorldPointY(int y, bool instant=false);
	void CentreViewOnWorldPoint(int x, int y, bool instant=false);
	void ScaleToFit(bool invert_fit);
	void SetViewScale(float scale, int nX = INT_MIN, int nY = INT_MIN, bool instant=false);
	void SetRelativeViewScale(float scale, int nX = INT_MIN, int nY = INT_MIN, bool instant=false);
	void SetViewRelativeOffset(int dX, int dY, bool scaled=true);
	void MoveTileRelativeOffset(Tile &tile, int dX, int dY);

	POINTF WindowPointToWorldF(int x, int y);
	POINT WindowPointToWorld(int x_, int y_);
	POINT WorldPointToWindow(int x, int y);
	bool WorldPointToTile(float fx, float fy, int &x_, int &y_);
	Tile *TileFromWindowPoint(int window_x, int window_y);
	bool IsTilePointOnWindow(int x, int y);
	bool IsTileVisible(_In_ const Tile &tile);
	bool IsTileClipped(_In_ const Tile &tile);

private:
	static constexpr int DEFAULT_ANIMATION_DURATION_MS = 200;

	template <typename TData>
	HRESULT UpdateBuffer(ID3D11Buffer *pBuffer, const TData *buf, size_t buf_size)
	{
		HRESULT hr;
		D3D11_MAPPED_SUBRESOURCE ms{};
		if (SUCCEEDED(hr = m_pDeviceContext->Map(pBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms)))
		{
			memcpy(ms.pData, buf, buf_size);
			m_pDeviceContext->Unmap(pBuffer, 0);
		}
		return hr;
	}

private:
	HWND m_hwnd;

	int m_num_rooms{ 0 };
	int m_num_panels{ 0 };
	bool m_layout_dirty{ false };
	std::vector<Panel> m_panels{};

	POINT m_Origin{};
	SIZE m_TotalSize{};
	SIZE m_TileOffset{};
	SIZE m_TileSize{};
	SIZE m_WindowSize{};

	EasedValue<float> m_scale{ DEFAULT_VIEW_MAGNIFY, std::chrono::milliseconds(DEFAULT_ANIMATION_DURATION_MS) };
	EasedValue<float> m_translate_x{ 0.0f, std::chrono::milliseconds(DEFAULT_ANIMATION_DURATION_MS) };
	EasedValue<float> m_translate_y{ 0.0f, std::chrono::milliseconds(DEFAULT_ANIMATION_DURATION_MS) };

	VSConstants sVSConstants;
	PSConstants sPSConstants;

	bool m_allow_tearing{ false };
	bool m_fullscreen{ false };
	bool m_single_tile{ false };

	ComPtr<ID3D11Device> m_pDevice;
	ComPtr<ID3D11DeviceContext> m_pDeviceContext;
	ComPtr<IDXGISwapChain1> m_pSwapChain;

	ComPtr<ID3D11Buffer> m_pDisplay;
	ComPtr<ID3D11Buffer> m_pMask;
	ComPtr<ID3D11Buffer> m_pPalette;
	ComPtr<ID3D11Buffer> m_pDisplayStaging;
	ComPtr<ID3D11Buffer> m_pMaskStaging;
	ComPtr<ID3D11Buffer> m_pInstanceBuffer;
	ComPtr<ID3D11Buffer> m_pVSConstants;
	ComPtr<ID3D11Buffer> m_pPSConstants;
	ComPtr<ID3D11InputLayout> m_pInputLayout;
	ComPtr<ID3D11RenderTargetView> m_pRenderTargetView;
	ComPtr<ID3D11ShaderResourceView> m_pShaderResourceViewDisplay;
	ComPtr<ID3D11ShaderResourceView> m_pShaderResourceViewMask;
	ComPtr<ID3D11ShaderResourceView> m_pShaderResourceViewPalette;
	ComPtr<ID3D11VertexShader> m_pVertexShader;
	ComPtr<ID3D11PixelShader> m_pPixelShader;
	ComPtr<ID3D11SamplerState> m_pSamplerState;
	ComPtr<ID3D11RasterizerState> m_pRasterizerState;
};
