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
    D3D_FEATURE_LEVEL               m_nD3DFeatureLevel;
    D3D_DRIVER_TYPE                 m_nD3DDriveType;
};

