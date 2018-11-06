#include "stdafx.h"
#include "TileView.h"
#include "SpectrumGame.h"	// TODO: doesn't belong here!

#include "Map_VS.h"
#include "Spectrum_PS.h"

std::mutex Tile::shared_lock;

////////////////////////////////////////////////////////////////////////////////

void TileView::SetMap(const std::vector<MapRoom> &rooms)
{
	auto min_x = std::min_element(rooms.begin(), rooms.end(), [](auto &a, auto &b) { return a.x < b.x; });
	auto min_y = std::min_element(rooms.begin(), rooms.end(), [](auto &a, auto &b) { return a.y < b.y; });
	auto max_x = std::max_element(rooms.begin(), rooms.end(), [](auto &a, auto &b) { return (a.x + a.width) < (b.x + b.width); });
	auto max_y = std::max_element(rooms.begin(), rooms.end(), [](auto &a, auto &b) { return (a.y + a.height) < (b.y + b.height); });

	m_Origin.x = min_x->x;
	m_Origin.y = min_y->y;
	m_TotalSize.cx = (max_x->x + max_x->width) - min_x->x;
	m_TotalSize.cy = (max_y->y + max_y->height) - min_y->y;

	m_num_rooms = static_cast<int>(rooms.size());
}

void TileView::SetPanels(const std::vector<Panel> &panels)
{
	m_panels = panels;
	m_num_panels = static_cast<int>(panels.size());
}

SIZE TileView::TotalSize() const
{
	return m_TotalSize;
}

POINTF TileView::WindowPointToWorldF(int window_x, int window_y)
{
	auto fx = (window_x - (m_translate_x / sVSConstants.scale_x)) / m_scale;
	auto fy = (window_y - (m_translate_y / sVSConstants.scale_y)) / m_scale;
	return { fx, fy };
}

POINT TileView::WindowPointToWorld(int x_, int y_)
{
	auto pt = WindowPointToWorldF(x_, y_);
	auto x = static_cast<int>(std::round(pt.x));
	auto y = static_cast<int>(std::round(pt.y));
	return { x, y };
}

POINT TileView::WorldPointToWindow(int x, int y)
{
	float fx = (m_translate_x / sVSConstants.scale_x) + (x * m_scale);
	float fy = (m_translate_y / sVSConstants.scale_y) + (y * m_scale);
	return { static_cast<int>(fx), static_cast<int>(fy) };
}

Tile *TileView::TileFromWindowPoint(int window_x, int window_y)
{
	auto pt = WindowPointToWorldF(window_x, window_y);

	auto &tiles = g_pGame->Tiles();
	for (auto it = tiles.rbegin(); it != tiles.rend(); ++it)
	{
		auto &tile = *it;
		if (pt.x < tile.x || pt.y < tile.y || pt.x >= (tile.x + tile.width) || pt.y >= (tile.y + tile.height))
			continue;

		return &tile;
	}

	return nullptr;
}

bool TileView::IsTilePointOnWindow(int x, int y)
{
	auto pt = WorldPointToWindow(x, y);
	return !(pt.x < 0 || pt.x >= m_WindowSize.cx || pt.y < 0 || pt.y >= m_WindowSize.cy);
}

bool TileView::IsTileVisible(
	const Tile &tile)
{
	int x1 = tile.x;
	int y1 = tile.y;
	auto pt = WorldPointToWindow(tile.x, tile.y);
	if (pt.x >= m_WindowSize.cx || pt.y >= m_WindowSize.cy)
		return false;

	pt = WorldPointToWindow(tile.x + tile.width - 1, tile.y + tile.height - 1);
	if (pt.x < 0 || pt.y < 0)
		return false;

	return true;
}

bool TileView::IsTileClipped(
	const Tile &tile)
{
	return !IsTilePointOnWindow(tile.x, tile.y) ||
		!IsTilePointOnWindow(tile.x + tile.width - 1, tile.y + tile.height - 1);
}

void TileView::UpdateView()
{
	sVSConstants.translate_x = m_translate_x.easedValue();
	sVSConstants.translate_y = m_translate_y.easedValue();
	sVSConstants.scale = m_scale.easedValue();
}

void TileView::ScaleToFit(bool invert_fit)
{
	float scale{};
	if ((m_TotalSize.cx * sVSConstants.scale_x > m_TotalSize.cy * sVSConstants.scale_y) ^ invert_fit)
		scale = static_cast<float>(m_WindowSize.cx) / m_TotalSize.cx;
	else
		scale = static_cast<float>(m_WindowSize.cy) / m_TotalSize.cy;

	m_scale.setValue(scale);
	CentreViewOnWorldPoint(m_Origin.x + m_TotalSize.cx / 2, m_Origin.y + m_TotalSize.cy / 2);

	UpdatePanels();
}

void TileView::SetViewScale(float scale, int nX, int nY, bool instant)
{
	if (nX == INT_MIN || nY == INT_MIN)
	{
#if 0
		// Preserve centre of active tile
		const auto &tile = g_pGame->ActiveTile();
		auto pt = WorldPointToWindow(tile.x + (tile.width / 2), tile.y + (tile.height / 2));
		nX = pt.x;
		nY = pt.y;
#else
		nX = m_WindowSize.cx / 2;
		nY = m_WindowSize.cy / 2;
#endif
	}

	scale = std::max(scale, 1.0f / pow(1.25f, 10.0f));
	scale = std::min(scale, 16.0f);

	float fX = nX - (m_translate_x / sVSConstants.scale_x);
	float fY = nY - (m_translate_y / sVSConstants.scale_y);
	fX *= scale / m_scale;
	fY *= scale / m_scale;
	fX = (nX - fX) * sVSConstants.scale_x;
	fY = (nY - fY) * sVSConstants.scale_y;

	m_translate_x.setValue(fX, instant);
	m_translate_y.setValue(fY, instant);
	m_scale.setValue(scale, instant);

	UpdatePanels();
}

void TileView::SetRelativeViewScale(float scale, int centre_x, int centre_y, bool instant)
{
	SetViewScale(m_scale * scale, centre_x, centre_y, instant);
}

void TileView::SetViewRelativeOffset(int dX, int dY, bool scaled)
{
	float fX = static_cast<float>(dX) * sVSConstants.scale_x;
	float fY = static_cast<float>(dY) * sVSConstants.scale_y;

	if (scaled)
	{
		fX *= m_scale;
		fY *= m_scale;
	}

	m_translate_x.setRelativeValue(fX, !scaled);
	m_translate_y.setRelativeValue(fY, !scaled);
}

void TileView::MoveTileRelativeOffset(Tile &tile, int dX, int dY)
{
	// Scale is ignored for precision.
	tile.x += dX;
	tile.y += dY;
	UpdateTiles();
}

//////////////////////////////////////////////////////////////////////////////

void TileView::CentreOnWorldPointX(int x, bool instant)
{
	float fX = (m_WindowSize.cx / 2) - (x * m_scale);
	m_translate_x.setValue(fX * sVSConstants.scale_x, instant);
}

void TileView::CentreOnWorldPointY(int y, bool instant)
{
	float fY = (m_WindowSize.cy / 2) - (y * m_scale);
	m_translate_y.setValue(fY * sVSConstants.scale_y, instant);
}

void TileView::CentreViewOnWorldPoint(int x, int y, bool instant)
{
	CentreOnWorldPointX(x, instant);
	CentreOnWorldPointY(y, instant);
}

void TileView::EnsureTileVisible(const Tile &tile, bool instant)
{
	// Margin around the target room that must also be visible (1/4)
	auto margin_width = tile.width / 4;
	auto margin_height = tile.height / 4;

	// Determine the bounding box, ignoring anything outside the map
	auto left = std::max(tile.x - margin_width, static_cast<int>(m_Origin.x));
	auto right = std::min(tile.x + tile.width + margin_width, static_cast<int>(m_Origin.x + m_TotalSize.cx));
	auto top = std::max(tile.y - margin_height, static_cast<int>(m_Origin.y));
	auto bottom = std::min(tile.y + tile.height + margin_height, static_cast<int>(m_Origin.y + m_TotalSize.cy));

	auto pt_tl = WorldPointToWindow(left, top);
	auto pt_br = WorldPointToWindow(right - 1, bottom - 1);

	if (pt_tl.x < 0 || pt_br.x >= m_WindowSize.cx)
		CentreOnWorldPointX(tile.x + (tile.width / 2), instant);

	if (pt_tl.y < 0 || pt_br.y >= m_WindowSize.cy)
		CentreOnWorldPointY(tile.y + (tile.height / 2), instant);
}

void TileView::CentreViewOnTile(const Tile &tile, bool instant)
{
	int x = tile.x + (tile.width / 2);
	int y = tile.y + (tile.height / 2);
	CentreViewOnWorldPoint(x, y, instant);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT TileView::Render()
{
	if (!m_pDeviceContext)
		return S_FALSE;

	if (m_layout_dirty)
		UpdateInstanceData();

	UpdateBuffer(m_pVSConstants.Get(), &sVSConstants, sizeof(sVSConstants));

	static int frame = 0;
	if (!(++frame & 0xf))
		sPSConstants.flash_invert = !sPSConstants.flash_invert;

	sPSConstants.transparent = 1;
	UpdateBuffer(m_pPSConstants.Get(), &sPSConstants, sizeof(sPSConstants));

	// Flip mode Presents release the render target, so we must rebind every time.
	m_pDeviceContext->OMSetRenderTargets(1, m_pRenderTargetView.GetAddressOf(), nullptr);

	static const FLOAT black[]{ 0.0f, 0.0f, 0.0f, 1.0f };
	m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView.Get(), black);

	if (m_single_tile)
	{
		const auto &tile = g_pGame->ActiveTile();
		m_pDeviceContext->DrawInstanced(4, 1, tile.screen_index * 4, tile.screen_index);
	}
	else
	{
		// Draw the room tiles.
		m_pDeviceContext->DrawInstanced(4, m_num_rooms, 0, 0);

		sVSConstants.translate_x = 0.0f;
		sVSConstants.translate_y = 0.0f;
		sVSConstants.scale = m_scale;
		UpdateBuffer(m_pVSConstants.Get(), &sVSConstants, sizeof(sVSConstants));

		sPSConstants.transparent = 0;
		UpdateBuffer(m_pPSConstants.Get(), &sPSConstants, sizeof(sPSConstants));

		// Draw the UI panels.
		m_pDeviceContext->DrawInstanced(4, m_num_panels, m_num_rooms * 4, m_num_rooms);
	}

	auto hr = m_pSwapChain->Present(0, (m_allow_tearing && !m_fullscreen) ? DXGI_PRESENT_ALLOW_TEARING : 0);
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		hr = InitD3D();
	else if (FAILED(hr))
		return Fail(hr, L"Present");

	return S_OK;
}

void TileView::UpdateScreens()
{
	HRESULT hr;
	D3D11_MAPPED_SUBRESOURCE ms{};
	if (SUCCEEDED(hr = m_pDeviceContext->Map(m_pDisplayStaging.Get(), NULL, D3D11_MAP_WRITE, NULL, &ms)))
	{
		for (auto &tile : g_pGame->Tiles())
		{
			if (tile.drawing)
			{
				auto offset = sVSConstants.screen_bytes * tile.screen_index;
				memcpy(
					reinterpret_cast<uint8_t*>(ms.pData) + offset,
					tile.mem.data() + 0x4000,
					sVSConstants.screen_bytes);
			}
		}

		m_pDeviceContext->Unmap(m_pDisplayStaging.Get(), 0);
	}

	m_pDeviceContext->CopyResource(m_pDisplay.Get(), m_pDisplayStaging.Get());
}

void TileView::UpdateMasks()
{
	HRESULT hr;
	D3D11_MAPPED_SUBRESOURCE msDisplay{}, msMask{};
	if (SUCCEEDED(hr = m_pDeviceContext->Map(m_pDisplayStaging.Get(), NULL, D3D11_MAP_READ, NULL, &msDisplay)))
	{
		if (SUCCEEDED(hr = m_pDeviceContext->Map(m_pMaskStaging.Get(), NULL, D3D11_MAP_READ_WRITE, NULL, &msMask)))
		{
			for (auto &tile : g_pGame->Tiles())
			{
				if (tile.mask_dirty)
				{
					auto offset = sVSConstants.screen_bytes * tile.screen_index;
					g_pGame->CreateTileMask(tile,
						reinterpret_cast<uint8_t*>(msDisplay.pData) + offset,
						reinterpret_cast<uint8_t*>(msMask.pData) + offset);
					tile.mask_dirty = false;
				}
			}

			m_pDeviceContext->Unmap(m_pMaskStaging.Get(), 0);
		}

		m_pDeviceContext->Unmap(m_pDisplayStaging.Get(), 0);
	}

	m_pDeviceContext->CopyResource(m_pMask.Get(), m_pMaskStaging.Get());
}

HRESULT TileView::OnResize()
{
	RECT rect{};
	GetClientRect(m_hwnd, &rect);
	m_WindowSize = { rect.right, rect.bottom };

	if (!m_pDeviceContext)
		return S_FALSE;

	// Clear the swapchain reference to the render target, and ensure it's flushed.
	m_pDeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
	m_pRenderTargetView = nullptr;
	m_pDeviceContext->Flush();

	BOOL fullScreen;
	if (SUCCEEDED(m_pSwapChain->GetFullscreenState(&fullScreen, nullptr)))
		m_fullscreen = (fullScreen == TRUE);

	// Auto-resize to the current window client area.
	UINT swapChainFlags = m_allow_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
	auto hr = m_pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, swapChainFlags);

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	hr = m_pSwapChain->GetDesc1(&swapChainDesc);

	ComPtr<ID3D11Resource> pBackBufferPtr;
	if (FAILED(hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Resource), &pBackBufferPtr)))
		return Fail(hr, L"SwapChain->GetBuffer");

	hr = m_pDevice->CreateRenderTargetView(pBackBufferPtr.Get(), NULL, &m_pRenderTargetView);
	pBackBufferPtr = nullptr;
	if (FAILED(hr))
		return Fail(hr, L"CreateRenderTargetView (back)");

	auto viewport = CD3D11_VIEWPORT(
		0.0f,
		0.0f,
		static_cast<float>(swapChainDesc.Width),
		static_cast<float>(swapChainDesc.Height));
	m_pDeviceContext->RSSetViewports(1, &viewport);

	// Preserve origin around change of viewport.
	auto offset_x = m_translate_x / sVSConstants.scale_x;
	auto offset_y = m_translate_y / sVSConstants.scale_y;
	sVSConstants.scale_x = m_TotalSize.cx / viewport.Width;
	sVSConstants.scale_y = m_TotalSize.cy / viewport.Height;
	m_translate_x.setValue(offset_x * sVSConstants.scale_x, true);
	m_translate_y.setValue(offset_y * sVSConstants.scale_y, true);

	UpdatePanels();
	return hr;
}

void TileView::UpdateInstanceData()
{
	std::vector<InstanceType> instances;
	for (auto &tile : g_pGame->Tiles())
		instances.push_back({ tile.x, tile.y, tile.width, tile.height, tile.offset_x, tile.offset_y, tile.screen_index });

	auto &tiles = g_pGame->Tiles();
	const auto &active_tile = g_pGame->ActiveTile();
	for (const auto &panel : m_panels)
	{
		int x, y;
		switch (panel.align)
		{
		case PanelAlignment::TopLeft:
		case PanelAlignment::Left:
		case PanelAlignment::BottomLeft:
			x = 0;
			break;
		case PanelAlignment::Top:
		case PanelAlignment::Bottom:
			x = (static_cast<int>(m_WindowSize.cx / m_scale) - panel.width) / 2;
			break;
		case PanelAlignment::TopRight:
		case PanelAlignment::Right:
		case PanelAlignment::BottomRight:
			x = static_cast<int>(m_WindowSize.cx / m_scale) - panel.width;
			break;
		}

		switch (panel.align)
		{
		case PanelAlignment::TopLeft:
		case PanelAlignment::Top:
		case PanelAlignment::TopRight:
			y = 0;
			break;
		case PanelAlignment::Left:
		case PanelAlignment::Right:
			y = (static_cast<int>(m_WindowSize.cy / m_scale) - panel.height) / 2;
			break;
		case PanelAlignment::BottomLeft:
		case PanelAlignment::Bottom:
		case PanelAlignment::BottomRight:
			y = static_cast<int>(m_WindowSize.cy / m_scale) - panel.height;
			break;
		}

		instances.push_back({ x, y, panel.width, panel.height, panel.offset_x, panel.offset_y, active_tile.screen_index });
	}

	UpdateBuffer(m_pInstanceBuffer.Get(), instances.data(), sizeof(InstanceType) * instances.size());
	m_layout_dirty = false;
}

void TileView::UpdateTiles()
{
	m_layout_dirty = true;
}

void TileView::UpdatePanels()
{
	m_layout_dirty = true;
}

bool TileView::CheckTearingSupport()
{
	ComPtr<IDXGIFactory4> factory4;
	auto hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory4));
	BOOL allowTearing = FALSE;
	if (SUCCEEDED(hr))
	{
		ComPtr<IDXGIFactory5> factory5;
		hr = factory4.As(&factory5);
		if (SUCCEEDED(hr))
			hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
	}

	return SUCCEEDED(hr) && allowTearing;
}

void TileView::ExitD3D()
{
	if (m_pDeviceContext)
	{
		// As recommended by "Deferred Destruction Issues with Flip Presentation Swap Chains"
		m_pDeviceContext->ClearState();
		m_pDeviceContext->Flush();
	}

	if (m_pSwapChain)
		m_pSwapChain->SetFullscreenState(FALSE, nullptr);

	m_pDisplay = nullptr;
	m_pMask = nullptr;
	m_pPalette = nullptr;
	m_pDisplayStaging = nullptr;
	m_pMaskStaging = nullptr;
	m_pInstanceBuffer = nullptr;
	m_pVSConstants = nullptr;
	m_pPSConstants = nullptr;
	m_pInputLayout = nullptr;
	m_pRenderTargetView = nullptr;
	m_pShaderResourceViewDisplay = nullptr;
	m_pShaderResourceViewPalette = nullptr;
	m_pVertexShader = nullptr;
	m_pPixelShader = nullptr;
	m_pSamplerState = nullptr;
	m_pRasterizerState = nullptr;

	m_pSwapChain = nullptr;
	m_pDeviceContext = nullptr;
	m_pDevice = nullptr;
}

HRESULT TileView::InitD3D()
{
	ExitD3D();

	sVSConstants.total_x = m_TotalSize.cx;
	sVSConstants.total_y = m_TotalSize.cy;

	sPSConstants.flash_invert = 0;
	sPSConstants.transparent = 1;

	D3D_FEATURE_LEVEL featureLevel;
	std::vector<D3D_FEATURE_LEVEL> featureLevels =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};

	DWORD createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	auto hr = D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		createDeviceFlags,
		featureLevels.data(),
		static_cast<UINT>(featureLevels.size()),
		D3D11_SDK_VERSION,
		&m_pDevice,
		&featureLevel,
		&m_pDeviceContext);
	if (FAILED(hr))
		return Fail(hr, L"D3D11CreateDevice");

	if (featureLevel >= D3D_FEATURE_LEVEL_10_0 && featureLevel < D3D_FEATURE_LEVEL_11_0)
	{
		D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS features{};
		if (FAILED(hr = m_pDevice->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &features, sizeof(features))))
			return Fail(hr, L"CheckFeatureSupport");
		else if (!(features.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x))
			throw std::exception("Sorry, your D3D hardware lacks structured buffer support");
	}

	ComPtr<IDXGIDevice2> pDXGIDevice;
	if (FAILED(hr = m_pDevice.As(&pDXGIDevice)))
	{
		m_pDeviceContext = nullptr;
		return Fail(hr, L"QueryInterface(IDXGIDevice2)");
	}

	ComPtr<IDXGIAdapter> pDXGIAdapter;
	if (FAILED(hr = pDXGIDevice->GetAdapter(&pDXGIAdapter)))
		return Fail(hr, L"pDXGIDevice->GetAdapter");

	ComPtr<IDXGIFactory2> pDXGIFactory;
	if (FAILED(hr = pDXGIAdapter->GetParent(__uuidof(IDXGIFactory2), &pDXGIFactory)))
		return Fail(hr, L"pDXGIDevice->GetParent(IDXGIFactory2)");

	m_allow_tearing = CheckTearingSupport();

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = m_WindowSize.cx;
	swapChainDesc.Height = m_WindowSize.cy;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = IsWindows8OrGreater() ? 2 : 1;
	swapChainDesc.Scaling = IsWindows8OrGreater() ? DXGI_SCALING_NONE : DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = IsWindows8OrGreater() ? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	swapChainDesc.Flags = m_allow_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	hr = pDXGIFactory->CreateSwapChainForHwnd(
		m_pDevice.Get(), m_hwnd, &swapChainDesc, nullptr, nullptr, &m_pSwapChain);
	if (FAILED(hr))
		return Fail(hr, L"CreateSwapChainForHwnd");

	if (FAILED(hr = OnResize()))
		return hr;

	if (FAILED(hr = m_pDevice->CreateVertexShader(g_Map_VS, sizeof(g_Map_VS), NULL, &m_pVertexShader)))
		return Fail(hr, L"CreateVertexShader");
	m_pDeviceContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);

	if (FAILED(hr = m_pDevice->CreatePixelShader(g_Spectrum_PS, sizeof(g_Spectrum_PS), NULL, &m_pPixelShader)))
		return Fail(hr, L"CreatePixelShader");
	m_pDeviceContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

	D3D11_BUFFER_DESC instanceBufferDesc{};
	instanceBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	instanceBufferDesc.ByteWidth = sizeof(InstanceType) * (m_num_rooms + m_num_panels);
	instanceBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	instanceBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	instanceBufferDesc.MiscFlags = 0;
	instanceBufferDesc.StructureByteStride = 0;

	if (FAILED(hr = m_pDevice->CreateBuffer(&instanceBufferDesc, nullptr, &m_pInstanceBuffer)))
		return Fail(hr, L"CreateBuffer IB");

	UpdateInstanceData();

	UINT stride = sizeof(InstanceType);
	UINT offset = 0;
	m_pDeviceContext->IASetVertexBuffers(0, 1, m_pInstanceBuffer.GetAddressOf(), &stride, &offset);
	m_pDeviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	D3D11_INPUT_ELEMENT_DESC ied{};
	ied.Format = DXGI_FORMAT_R32_UINT;
	ied.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	ied.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
	ied.InstanceDataStepRate = 1;

	std::vector<D3D11_INPUT_ELEMENT_DESC> layouts;
	ied.SemanticName = "TILE_X";
	layouts.push_back(ied);
	ied.SemanticName = "TILE_Y";
	layouts.push_back(ied);
	ied.SemanticName = "TILE_W";
	layouts.push_back(ied);
	ied.SemanticName = "TILE_H";
	layouts.push_back(ied);
	ied.SemanticName = "OFFSET_X";
	layouts.push_back(ied);
	ied.SemanticName = "OFFSET_Y";
	layouts.push_back(ied);
	ied.SemanticName = "SCREEN";
	layouts.push_back(ied);

	if (FAILED(hr = m_pDevice->CreateInputLayout(
		layouts.data(), static_cast<UINT>(layouts.size()), g_Map_VS, sizeof(g_Map_VS), &m_pInputLayout)))
	{
		return Fail(hr, L"Create IL");
	}
	m_pDeviceContext->IASetInputLayout(m_pInputLayout.Get());

	D3D11_BUFFER_DESC scd{};
	scd.Usage = D3D11_USAGE_DYNAMIC;
	scd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	scd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	scd.ByteWidth = sizeof(VSConstants);
	if (FAILED(hr = m_pDevice->CreateBuffer(&scd, nullptr, &m_pVSConstants)))
		return Fail(hr, L"CreateBuffer VS consts");
	m_pDeviceContext->VSSetConstantBuffers(0, 1, m_pVSConstants.GetAddressOf());

	scd.ByteWidth = sizeof(PSConstants);
	if (FAILED(hr = m_pDevice->CreateBuffer(&scd, nullptr, &m_pPSConstants)))
		return Fail(hr, L"CreateBuffer PS consts");
	m_pDeviceContext->PSSetConstantBuffers(0, 1, m_pPSConstants.GetAddressOf());

	D3D11_BUFFER_DESC bd{};
	bd.ByteWidth = sVSConstants.screen_bytes * m_num_rooms;
	bd.Usage = D3D11_USAGE_STAGING;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
	bd.StructureByteStride = sizeof(uint32_t);

	std::vector<uint8_t> mask_fill(bd.ByteWidth, 0xff);
	D3D11_SUBRESOURCE_DATA init_data{};
	init_data.pSysMem = mask_fill.data();

	if (FAILED(hr = m_pDevice->CreateBuffer(&bd, NULL, &m_pDisplayStaging)))
		return Fail(hr, L"CreateBuffer display staging");

	if (FAILED(hr = m_pDevice->CreateBuffer(&bd, &init_data, &m_pMaskStaging)))
		return Fail(hr, L"CreateBuffer map staging");

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

	if (FAILED(hr = m_pDevice->CreateBuffer(&bd, NULL, &m_pDisplay)))
		return Fail(hr, L"CreateBuffer display");

	if (FAILED(hr = m_pDevice->CreateBuffer(&bd, NULL, &m_pMask)))
		return Fail(hr, L"CreateBuffer masks");

	m_pDeviceContext->CopyResource(m_pMask.Get(), m_pMaskStaging.Get());

	D3D11_SHADER_RESOURCE_VIEW_DESC srvd{};
	srvd.Format = DXGI_FORMAT_UNKNOWN;
	srvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvd.Buffer.FirstElement = 0;
	srvd.Buffer.NumElements = bd.ByteWidth / bd.StructureByteStride;

	if (FAILED(hr = m_pDevice->CreateShaderResourceView(m_pDisplay.Get(), &srvd, &m_pShaderResourceViewDisplay)))
		return Fail(hr, L"Create SRV display");

	if (FAILED(hr = m_pDevice->CreateShaderResourceView(m_pMask.Get(), &srvd, &m_pShaderResourceViewMask)))
		return Fail(hr, L"Create SRV mask");

	std::vector<float> palette;
	for (int i = 0; i < 16; ++i)
	{
		// ToDo: system should supply palette.
		auto intensity = (i >= 8) ? 1.0f : (223.0f / 255.0f);
		palette.push_back((i & 2) ? intensity : 0.0f);
		palette.push_back((i & 4) ? intensity : 0.0f);
		palette.push_back((i & 1) ? intensity : 0.0f);
		palette.push_back(1.0f);
	}

	bd.StructureByteStride = sizeof(FLOAT) * 4;
	bd.ByteWidth = 16 * bd.StructureByteStride;
	bd.Usage = D3D11_USAGE_IMMUTABLE;

	init_data.pSysMem = palette.data();
	if (FAILED(hr = m_pDevice->CreateBuffer(&bd, &init_data, &m_pPalette)))
		return Fail(hr, L"CreateBuffer palette");

	srvd.BufferEx.NumElements = bd.ByteWidth / bd.StructureByteStride;
	if (FAILED(hr = m_pDevice->CreateShaderResourceView(m_pPalette.Get(), &srvd, &m_pShaderResourceViewPalette)))
		return Fail(hr, L"Create SRV palette");

	ID3D11ShaderResourceView *resources[] = {
		m_pShaderResourceViewPalette.Get(),
		m_pShaderResourceViewDisplay.Get(),
		m_pShaderResourceViewMask.Get()
	};
	m_pDeviceContext->PSSetShaderResources(0, _countof(resources), resources);

	m_pDeviceContext->VSSetConstantBuffers(0, 0, nullptr);

	CD3D11_RASTERIZER_DESC rd{ D3D11_DEFAULT };
	if (FAILED(m_pDevice->CreateRasterizerState(&rd, &m_pRasterizerState)))
		return Fail(hr, L"Create RS");
	m_pDeviceContext->RSSetState(m_pRasterizerState.Get());

	return S_OK;
}
