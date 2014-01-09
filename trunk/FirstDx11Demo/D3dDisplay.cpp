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
m_pRenderTargetView(0),
m_nD3DFeatureLevel(D3D_FEATURE_LEVEL_11_0),
m_nD3DDriveType(D3D_DRIVER_TYPE_UNKNOWN),
m_dwHeight(0),
m_dwWidth(0)
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
    m_dwWidth = (UINT)(cltRect.right - cltRect.left);
    m_dwHeight = (UINT)(cltRect.bottom - cltRect.top);

    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
    
    swapChainDesc.BufferDesc.Height = m_dwHeight;
    swapChainDesc.BufferDesc.Width = m_dwWidth;
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

	ID3D11Texture2D* pTexture = NULL;

	hr = m_pDXGISwapChain->GetBuffer(0, __uuidof(pTexture), reinterpret_cast<void**> (&pTexture));
	if (FAILED(hr))
	{
		return false;
	}

	hr = m_pD3d11Device->CreateRenderTargetView(pTexture, 0, &m_pRenderTargetView);
	if (FAILED(hr))
	{
		pTexture->Release();
		return false;
	}

	m_pD3d11DeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, 0);

    return true;
}

void CD3dDisplay::ClearTargetView()
{
    HRESULT hr = S_OK;

    float colorArray[4] = {0,0,0,0};
    m_pD3d11DeviceContext->ClearRenderTargetView(m_pRenderTargetView, colorArray);
    m_pDXGISwapChain->Present(0, 0);
}

void CD3dDisplay::DrawTriangle()
{
    ID3D11Buffer* pVertexBuff = NULL;
    ID3D11InputLayout* pInputLayOut = NULL;
	ID3D11VertexShader* pVs = NULL;
	ID3D11PixelShader* pPs = NULL;
	ID3DBlob* pVsBuff =  NULL;
	ID3DBlob* pPsBuff = NULL;
	ID3DBlob* pVsShaderError = NULL;
	ID3DBlob* pPsShaderError = NULL;
	ID3D11ShaderResourceView* pShaderResView = NULL;
	ID3D11SamplerState* pSamplerState = NULL;
	HRESULT hr = S_OK;

	VertexFmt vertexPos[] = 
	{
		{XMFLOAT3(-1.0f, 1.0f, .0f), XMFLOAT2(.0f, .0f)},
		{XMFLOAT3(1.0f, 1.0f, .0f), XMFLOAT2(1.0f, 0.0f)},
		{XMFLOAT3(-1.0f, -1.0f, .0f), XMFLOAT2(0.0f, 1.0f)},

		{XMFLOAT3(-1.0f, -1.0f, .0f), XMFLOAT2(.0f, 1.0f)},
		{XMFLOAT3(1.0f, 1.0f, .0f), XMFLOAT2(1.0f, 0.0f)},
		{XMFLOAT3(1.0f, -1.0f, .0f), XMFLOAT2(1.0f, 1.0f)}
	};

	UINT vertexSize = ARRAYSIZE(vertexPos);
    
    D3D11_BUFFER_DESC bufferDesc;
    ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
    bufferDesc.ByteWidth = vertexSize * sizeof(VertexFmt);
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

	hr = D3DX11CompileFromFile( _T("FX/SolidGreenColor.fx"),
								0, 
								NULL,
								"VS_Main",
								"vs_4_0",
								0,
								0,
								NULL,
								&pVsBuff,
								&pVsShaderError,
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



	hr = D3DX11CompileFromFile( _T("FX/SolidGreenColor.fx"),
								0, 
								NULL,
								"PS_Main",
								"ps_4_0",
								0,
								0,
								NULL,
								&pPsBuff,
								&pPsShaderError,
								NULL);
	if (FAILED(hr))
	{
		void* pTmp = pPsShaderError->GetBufferPointer();
		return;
	}

	hr = m_pD3d11Device->CreatePixelShader(pPsBuff->GetBufferPointer(), pPsBuff->GetBufferSize(), NULL, &pPs);
	if (FAILED(hr))
	{
		return;
	}
	
	D3D11_INPUT_ELEMENT_DESC inputElementDesc[2];
	ZeroMemory(inputElementDesc, sizeof(inputElementDesc));
	inputElementDesc[0].SemanticName = "POSITION";
	inputElementDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDesc[0].AlignedByteOffset = 0;
	inputElementDesc[0].InputSlot = 0;
	inputElementDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	inputElementDesc[0].InstanceDataStepRate = 0;
	inputElementDesc[0].SemanticIndex = 0;
	inputElementDesc[1].SemanticName = "TEXCOORD";
	inputElementDesc[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDesc[1].AlignedByteOffset = 12;
	inputElementDesc[1].InputSlot = 0;
	inputElementDesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	inputElementDesc[1].InstanceDataStepRate = 0;
	inputElementDesc[1].SemanticIndex = 0;

	hr = m_pD3d11Device->CreateInputLayout( inputElementDesc, 2, pVsBuff->GetBufferPointer(), pVsBuff->GetBufferSize(), &pInputLayOut);
	if (FAILED(hr))
	{
		return;
	}

	hr = D3DX11CreateShaderResourceViewFromFile( m_pD3d11Device,
												 _T("RES/decal.dds"),
												 NULL,
												 NULL,
												 &pShaderResView,
												 NULL);
	if (FAILED(hr))
	{
		return;
	}

	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc, sizeof(D3D11_SAMPLER_DESC));
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = m_pD3d11Device->CreateSamplerState(&samplerDesc, &pSamplerState);
	if (FAILED(hr))
	{
		return;
	}

	m_pD3d11DeviceContext->IASetInputLayout(pInputLayOut);
	UINT dwStrides = sizeof(VertexFmt);
	UINT dwOffsets = 0;
	m_pD3d11DeviceContext->IASetVertexBuffers(0, 1, &pVertexBuff, &dwStrides, &dwOffsets);
	m_pD3d11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pD3d11DeviceContext->VSSetShader(pVs, 0, 0);
	m_pD3d11DeviceContext->PSSetShader(pPs, 0, 0);
	m_pD3d11DeviceContext->PSSetShaderResources(0, 1, &pShaderResView);
	m_pD3d11DeviceContext->PSSetSamplers(0, 1, &pSamplerState);

	D3D11_VIEWPORT vp;
	vp.Height = (FLOAT)m_dwHeight;
	vp.Width = (FLOAT)m_dwWidth;
	vp.TopLeftX = .0f;
	vp.TopLeftY =  .0f;
	vp.MinDepth = .0f;
	vp.MaxDepth = 1.0f;
	m_pD3d11DeviceContext->RSSetViewports(1, &vp);

	m_pD3d11DeviceContext->Draw(vertexSize, 0);
	m_pDXGISwapChain->Present(0, 0);
}
