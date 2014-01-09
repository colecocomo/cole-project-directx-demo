#pragma once
#include <d3d11.h>

class CD3dDisplay
{
public:
    CD3dDisplay(void);
    virtual ~CD3dDisplay(void);

public:
    bool InitDevice(HWND hWnd);
    void ClearTargetView();

    void DrawTriangle();

private:
    ID3D11Device*                   m_pD3d11Device;
    ID3D11DeviceContext*            m_pD3d11DeviceContext;
    IDXGISwapChain*                 m_pDXGISwapChain;
	ID3D11RenderTargetView*			m_pRenderTargetView;
    D3D_FEATURE_LEVEL               m_nD3DFeatureLevel;
    D3D_DRIVER_TYPE                 m_nD3DDriveType;

	UINT							m_dwHeight;
	UINT							m_dwWidth;
};

