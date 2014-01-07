#include "StdAfx.h"
#include "D3dDisplay.h"
#include "Public.h"
#include <xnamath.h>
#include <D3DX11async.h>
#include <D3Dcommon.h>
#include <string.h>


CD3dDisplay::CD3dDisplay(void):m_pD3d11Device(0),
m_pD3d11DeviceContext(0),
m_pDXGISwapChain(0),
m_nD3DFeatureLevel(D3D_FEATURE_LEVEL_11_0),
m_nD3DDriveType(D3D_DRIVER_TYPE_UNKNOWN)
{
}


CD3dDisplay::~CD3dDisplay(void)
{
    if (m_pD3d11Device) m_pD3d11Device->Release();
    if (m_pD3d11DeviceContext) m_pD3d11DeviceContext->Release();
    if (m_pDXGISwapChain) m_pDXGISwapChain->Release();
}

bool CD3dDisplay::InitDevice( HWND hWnd )
{
    HRESULT hr = S_OK;
    RECT cltRect;

    ZeroMemory(&cltRect, sizeof(RECT));
    GetClientRect(hWnd,&cltRect);
    UINT dwWidth = (UINT)(cltRect.right - cltRect.left);
    UINT dwHeight = (UINT)(cltRect.bottom - cltRect.top);

    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
    
    swapChainDesc.BufferDesc.Height = dwHeight;
    swapChainDesc.BufferDesc.Width = dwWidth;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 10;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = hWnd;
    swapChainDesc.Windowed = true;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createFlag = 0;
    createFlag |= D3D11_CREATE_DEVICE_SINGLETHREADED;

#ifdef _DEBUG
    createFlag |= D3D11_CREATE_DEVICE_DEBUG;
#endif // _DEBUG

    for (UINT i = 0; i < g_dwDriveTypeSize; i++)
    {
        /*hr = D3D11CreateDevice(  NULL,
                                 g_driveType[i],
                                 NULL,
                                 createFlag, 
                                 g_featureLevel, 
                                 g_dwFeatureLevelSize,
                                 D3D11_SDK_VERSION,
                                 &m_pD3d11Device,
                                 &m_nD3DFeatureLevel,
                                 &m_pD3d11DeviceContext);*/
        hr = D3D11CreateDeviceAndSwapChain( NULL,
                                            g_driveType[i],
                                            NULL,
                                            createFlag,
                                            g_featureLevel,
                                            g_dwFeatureLevelSize,
                                            D3D11_SDK_VERSION,
                                            &swapChainDesc,
                                            &m_pDXGISwapChain,
                                            &m_pD3d11Device,
                                            &m_nD3DFeatureLevel,
                                            &m_pD3d11DeviceContext);
        if (SUCCEEDED(hr))
        {
            m_nD3DDriveType = g_driveType[i];
            break;
        }
    }

    if (FAILED(hr))
    {
        return false;
    }   

    return true;
}

void CD3dDisplay::ClearTargetView()
{
    HRESULT hr = S_OK;
    ID3D11RenderTargetView* pTargetView = NULL;
    ID3D11Texture2D* pTexture = NULL;

    hr = m_pDXGISwapChain->GetBuffer(0, __uuidof(pTexture), reinterpret_cast<void**> (&pTexture));
    if (FAILED(hr))
    {
        return ;
    }

    hr = m_pD3d11Device->CreateRenderTargetView(pTexture, 0, &pTargetView);
    if (FAILED(hr))
    {
        pTexture->Release();
        return ;
    }

    m_pD3d11DeviceContext->OMSetRenderTargets(1, &pTargetView, 0);

    float colorArray[4] = {0,0,0,0};
    m_pD3d11DeviceContext->ClearRenderTargetView(pTargetView, colorArray);
    m_pDXGISwapChain->Present(0, 0);
}

void CD3dDisplay::DrawTriangle()
{
    ID3D11Buffer* pVertexBuff = NULL;
    ID3D11InputLayout* pInputLayOut = NULL;
	ID3D11VertexShader* pVs = NULL;
	ID3D11PixelShader* pPs = NULL;
	ID3DBlob* pVsBuff =  NULL;
	HRESULT hr = S_OK;

	XMFLOAT3 vertexPos[3] = 
	{
		XMFLOAT3(.0f, .0f, .0f),
		XMFLOAT3(1.0f, .0f, .0f),
		XMFLOAT3(.0f, 1.0f, .0f)
	};
    
    D3D11_BUFFER_DESC bufferDesc;
    ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
    bufferDesc.ByteWidth = 3 * sizeof(XMFLOAT3);
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA subresourceData;
	ZeroMemory(&subresourceData, sizeof(D3D11_SUBRESOURCE_DATA));
	subresourceData.pSysMem = vertexPos;
    hr = m_pD3d11Device->CreateBuffer(&bufferDesc, &subresourceData, &pVertexBuff);

	if (FAILED(hr))
	{
		return;
	}

	hr = D3DX11CompileFromFile( _T(""),
								0, 
								NULL,
								"VS_Main",
								"vs_4_0",
								0,
								0,
								NULL,
								&pVsBuff,
								NULL,
								NULL);
	if (FAILED(hr))
	{
		return;
	}

	hr = m_pD3d11Device->CreateVertexShader(pVsBuff->GetBufferPointer(), pVsBuff->GetBufferSize(), NULL, &pVs);
	if (FAILED(hr))
	{
		return;
	}
	
	D3D11_INPUT_ELEMENT_DESC inputElementDesc;
	ZeroMemory(&inputElementDesc, sizeof(D3D11_INPUT_ELEMENT_DESC));
	strcpy_s((char*)inputElementDesc.SemanticName, 20, "POSITION");
	inputElementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDesc.AlignedByteOffset = 0;
	inputElementDesc.InputSlot = 0;
	inputElementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	inputElementDesc.InstanceDataStepRate = 0;
	inputElementDesc.SemanticIndex = 0;

	hr = m_pD3d11Device->CreateInputLayout( &inputElementDesc, 1, pVsBuff->GetBufferPointer(), pVsBuff->GetBufferSize(), &pInputLayOut);
	if (FAILED(hr))
	{
		return;
	}


}
