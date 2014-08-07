#include "StdAfx.h"
#include "D3dDisplay.h"
#include "Public.h"
#include <D3DX11async.h>
#include <D3Dcommon.h>
#include <fstream>
#include <sstream>

using std::wstring;

wstring AnsiToUnicode(const char* pSrc)
{
	if (pSrc == NULL)
	{
		return NULL;
	}

	int len = strlen(pSrc);

	int unicodeLen = ::MultiByteToWideChar(CP_ACP, 0, pSrc, -1, NULL, 0);
	WCHAR* pUnicode = new WCHAR[unicodeLen+1];
	ZeroMemory(pUnicode, unicodeLen+1);
	::MultiByteToWideChar(CP_ACP, 0, pSrc, -1, (LPWSTR)pUnicode, unicodeLen);

	wstring ret;
	ret = pUnicode;
	SAFE_DELETE(pUnicode);

	return ret;
}

CD3dDisplay::CD3dDisplay(void):m_pD3d11Device(0),
m_pD3d11DeviceContext(0),
m_pDXGISwapChain(0),
m_pRenderTargetView(0),
m_pDepthStencilView(0),
m_pEffect(0),
m_pResterizerState(0),
m_nD3DFeatureLevel(D3D_FEATURE_LEVEL_11_0),
m_nD3DDriveType(D3D_DRIVER_TYPE_UNKNOWN),
m_pVertexBuff(0),
m_pInputLayOut(0),
m_pVs(0),
m_pPs(0),
m_pVsBuff(0),
m_pPsBuff(0),
m_pVsShaderError(0),
m_pPsShaderError(0),
m_pShaderResView(0),
m_pSamplerState(0),
m_dwHeight(0),
m_dwWidth(0),
m_isFullScreen(false),
m_dwElapseTime(0),
m_dwDeltaTime(0),
m_pSkullVertexBuffer(0),
m_pSkullIndexBuffer(0),
m_dwSkullVertexCnt(0),
m_dwSkullIndexCnt(0)
{
	m_vObjModelIndexBuff.clear();
	m_vObjModelVertexBuff.clear();
	m_eyePos = XMFLOAT3(.0f, .0f, -1.0f);
	m_localTranslation = XMMatrixIdentity();
}


CD3dDisplay::~CD3dDisplay(void)
{
	SAFE_RELEASE(m_pD3d11Device);
	SAFE_RELEASE(m_pD3d11DeviceContext);
	SAFE_RELEASE(m_pDXGISwapChain);
	SAFE_RELEASE(m_pResterizerState);

	SAFE_RELEASE(m_pVertexBuff);
	SAFE_RELEASE(m_pInputLayOut);
	SAFE_RELEASE(m_pVs);
	SAFE_RELEASE(m_pPs);
	SAFE_RELEASE(m_pVsBuff);
	SAFE_RELEASE(m_pPsBuff);
	SAFE_RELEASE(m_pVsShaderError);
	SAFE_RELEASE(m_pPsShaderError);
	SAFE_RELEASE(m_pShaderResView);
	SAFE_RELEASE(m_pSamplerState);

	SAFE_RELEASE(m_pSkullIndexBuffer);
	SAFE_RELEASE(m_pSkullVertexBuffer);

	BufferVectorIter iter = m_vObjModelIndexBuff.begin();
	while(iter != m_vObjModelIndexBuff.end())
	{
		SAFE_RELEASE((*iter));
		iter++;
	}

	iter = m_vObjModelVertexBuff.begin();
	while(iter != m_vObjModelVertexBuff.end())
	{
		SAFE_RELEASE((*iter));
		iter++;
	}
}

bool CD3dDisplay::InitDevice3D(HWND hWnd)
{
	HRESULT hr = S_OK;
	ID3D11DepthStencilState* pDepthStencilState = NULL;
	IDXGIFactory* pDXGIFactory = NULL;
	IDXGIAdapter* pDXGIAdapter = NULL;
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

	m_isFullScreen = false;

    UINT createFlag = 0;
    createFlag |= D3D11_CREATE_DEVICE_SINGLETHREADED;

#ifdef _DEBUG
    createFlag |= D3D11_CREATE_DEVICE_DEBUG;
#endif // _DEBUG

	CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pDXGIFactory);
	pDXGIFactory->EnumAdapters(0, &pDXGIAdapter);
	LARGE_INTEGER largeInterger;
	hr = pDXGIAdapter->CheckInterfaceSupport(__uuidof(ID3D11Device), &largeInterger);
	IDXGIOutput* pDXGIOutput = NULL;
	pDXGIAdapter->EnumOutputs(0, &pDXGIOutput);

	UINT dwModeCnt = 0;
	pDXGIOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_SCALING, &dwModeCnt, NULL);
	DXGI_MODE_DESC* pDxgiModeDesc = new DXGI_MODE_DESC[dwModeCnt];
	pDXGIOutput->GetDisplayModeList(DXGI_FORMAT_R32G32B32A32_SINT, DXGI_ENUM_MODES_SCALING, &dwModeCnt, pDxgiModeDesc);
	  
	SAFE_RELEASE(pDXGIFactory);
	SAFE_RELEASE(pDXGIAdapter);
	SAFE_RELEASE(pDXGIOutput);
	
    for (UINT i = 0; i < g_dwDriveTypeSize; i++)
    {
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

	//disable ¡°ALT+ENTER¡±
	IDXGIDevice* pDXGIDevice = NULL;
	m_pD3d11Device->QueryInterface(__uuidof(IDXGIDevice), (void**)&pDXGIDevice);
	pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&pDXGIAdapter);
	pDXGIAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&pDXGIFactory);
	pDXGIFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
	
	ID3D11Texture2D* pDSVTexture = NULL;

	D3D11_TEXTURE2D_DESC textureDesc;
	ZeroMemory(&textureDesc, sizeof(D3D11_TEXTURE2D_DESC));
	textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	textureDesc.Width = m_dwWidth;
	textureDesc.Height = m_dwHeight;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;
	hr = m_pD3d11Device->CreateTexture2D(&textureDesc, NULL, &pDSVTexture);
	if (FAILED(hr))
	{
		return false;
	}
	//ID3D11DepthStencilView* pDepthStencilView = NULL; 

	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	ZeroMemory(&depthStencilViewDesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
	depthStencilViewDesc.Format = textureDesc.Format;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;
	hr = m_pD3d11Device->CreateDepthStencilView(pDSVTexture, &depthStencilViewDesc, &m_pDepthStencilView);
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

	m_pD3d11DeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);


	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilDesc.StencilEnable = TRUE;
	depthStencilDesc.StencilReadMask = 0xff;
	depthStencilDesc.StencilWriteMask =  0xff;
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	hr = m_pD3d11Device->CreateDepthStencilState(&depthStencilDesc, &pDepthStencilState);
	if (FAILED(hr))
	{
		return false;
	}

	m_pD3d11DeviceContext->OMSetDepthStencilState(pDepthStencilState, 1);

	ID3DBlob* pEffectBuff = NULL;
	ID3DBlob* pEffectErrorBuff = NULL;
	hr = D3DX11CompileFromFile(	_T("FX/Effect.fx"),
								NULL,
								NULL,
								"",
								"fx_5_0",
								0,
								0,
								NULL,
								&pEffectBuff,
								&pEffectErrorBuff,
								NULL);
	if (FAILED(hr))
	{
		void* pTmp = pEffectErrorBuff->GetBufferPointer();
		return false;
	}

	hr = D3DX11CreateEffectFromMemory(pEffectBuff->GetBufferPointer(), pEffectBuff->GetBufferSize(), 0, m_pD3d11Device, &m_pEffect);
	SAFE_RELEASE(pEffectBuff);
	SAFE_RELEASE(pEffectErrorBuff);
	if (FAILED(hr))
	{
		return false;
	}

	D3D11_RASTERIZER_DESC rasterizerDesc;
	
	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	hr = m_pD3d11Device->CreateRasterizerState(&rasterizerDesc, &m_pResterizerState);
	if (FAILED(hr))
	{
		return false;
	}

    return LoadModelFromFile(_T(".\\RES\\ObjModel\\skull.txt"));
}

bool CD3dDisplay::InitDevice2D( HWND hWnd )
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
	vp.TopLeftY = .0f;
	vp.MinDepth = .0f;
	vp.MaxDepth = 1.0f;
	m_pD3d11DeviceContext->RSSetViewports(1, &vp);

	m_pD3d11DeviceContext->Draw(vertexSize, 0);
	m_pDXGISwapChain->Present(0, 0);
}

void CD3dDisplay::DrawSprite()
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

	
}

void CD3dDisplay::DrawCube()
{
	ID3D11Buffer* pVertexBuff = NULL;
	ID3D11Buffer* pIndexBuff = NULL;
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
		{ XMFLOAT3( -1.0f,  1.0f, -1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3(  1.0f,  1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT3(  1.0f,  1.0f,  1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT3( -1.0f,  1.0f,  1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

		{ XMFLOAT3( -1.0f, -1.0f, -1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3(  1.0f, -1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT3(  1.0f, -1.0f,  1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT3( -1.0f, -1.0f,  1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

		{ XMFLOAT3( -1.0f, -1.0f,  1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3( -1.0f, -1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT3( -1.0f,  1.0f, -1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT3( -1.0f,  1.0f,  1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

		{ XMFLOAT3(  1.0f, -1.0f,  1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3(  1.0f, -1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT3(  1.0f,  1.0f, -1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT3(  1.0f,  1.0f,  1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

		{ XMFLOAT3( -1.0f, -1.0f, -1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3(  1.0f, -1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT3(  1.0f,  1.0f, -1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT3( -1.0f,  1.0f, -1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

		{ XMFLOAT3( -1.0f, -1.0f,  1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3(  1.0f, -1.0f,  1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT3(  1.0f,  1.0f,  1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT3( -1.0f,  1.0f,  1.0f ), XMFLOAT2( 0.0f, 1.0f ) }
	};

	VertexWithOutUV vertexWithOutUV[] =
	{
		{ XMFLOAT3( -1.0f,  1.0f, -1.0f ) },
		{ XMFLOAT3(  1.0f,  1.0f, -1.0f ) },
		{ XMFLOAT3(  1.0f,  1.0f,  1.0f ) },
		{ XMFLOAT3( -1.0f,  1.0f,  1.0f ) },

		{ XMFLOAT3( -1.0f, -1.0f, -1.0f ) },
		{ XMFLOAT3(  1.0f, -1.0f, -1.0f ) },
		{ XMFLOAT3(  1.0f, -1.0f,  1.0f ) },
		{ XMFLOAT3( -1.0f, -1.0f,  1.0f ) },

		{ XMFLOAT3( -1.0f, -1.0f,  1.0f ) },
		{ XMFLOAT3( -1.0f, -1.0f, -1.0f ) },
		{ XMFLOAT3( -1.0f,  1.0f, -1.0f ) },
		{ XMFLOAT3( -1.0f,  1.0f,  1.0f ) },

		{ XMFLOAT3(  1.0f, -1.0f,  1.0f ) },
		{ XMFLOAT3(  1.0f, -1.0f, -1.0f ) },
		{ XMFLOAT3(  1.0f,  1.0f, -1.0f ) },
		{ XMFLOAT3(  1.0f,  1.0f,  1.0f ) },

		{ XMFLOAT3( -1.0f, -1.0f, -1.0f ) },
		{ XMFLOAT3(  1.0f, -1.0f, -1.0f ) },
		{ XMFLOAT3(  1.0f,  1.0f, -1.0f ) },
		{ XMFLOAT3( -1.0f,  1.0f, -1.0f ) },

		{ XMFLOAT3( -1.0f, -1.0f,  1.0f ) },
		{ XMFLOAT3(  1.0f, -1.0f,  1.0f ) },
		{ XMFLOAT3(  1.0f,  1.0f,  1.0f ) },
		{ XMFLOAT3( -1.0f,  1.0f,  1.0f ) },

		{ XMFLOAT3(.0f, .0f, .0f)},
		{ XMFLOAT3(1.0f, 1.0f, .0f)},
		{ XMFLOAT3(2.0f, 1.0f, .0f)},
		{ XMFLOAT3(1.0f, .0f, .0f)}
	};

	VertexUV tex[]=
	{
		{ XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT2( 0.0f, 1.0f ) },

		{ XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT2( 0.0f, 1.0f ) },

		{ XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT2( 0.0f, 1.0f ) },

		{ XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT2( 0.0f, 1.0f ) },

		{ XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT2( 0.0f, 1.0f ) },

		{ XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT2( 0.0f, 1.0f ) },

		{ XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT2(1.0f, 1.0f ) },
		{ XMFLOAT2( 0.0f, 1.0f ) }
	};

	WORD indices[] =
	{
		3,   1,  0,  2,  1,  3,
		6,   4,  5,  7,  4,  6,
		11,  9,  8, 10,  9, 11,
		14, 12, 13, 15, 12, 14,
		19, 17, 16, 18, 17, 19,
		22, 20, 21, 23, 20, 22,
		24, 26, 25, 26, 24, 27
	};

#define SEPARATE_POS_TEX
	
#ifdef SEPARATE_POS_TEX
	UINT vertexSize = ARRAYSIZE(vertexWithOutUV);
	UINT indexSize = ARRAYSIZE(indices);

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.ByteWidth = vertexSize * sizeof(VertexWithOutUV);
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA subresourceData;
	ZeroMemory(&subresourceData, sizeof(D3D11_SUBRESOURCE_DATA));
	subresourceData.pSysMem = vertexWithOutUV;
	hr = m_pD3d11Device->CreateBuffer(&bufferDesc, &subresourceData, &pVertexBuff);

	if (FAILED(hr))
	{
		return;
	}

	ID3D11Buffer* pTexBuffer = NULL;

	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.ByteWidth = vertexSize * sizeof(VertexUV);
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;

	ZeroMemory(&subresourceData, sizeof(D3D11_SUBRESOURCE_DATA));
	subresourceData.pSysMem = tex;
	hr = m_pD3d11Device->CreateBuffer(&bufferDesc, &subresourceData, &pTexBuffer);

	D3D11_BUFFER_DESC indexBufferDesc;
	ZeroMemory(&indexBufferDesc, sizeof(D3D11_BUFFER_DESC));
	indexBufferDesc.ByteWidth = indexSize * sizeof(WORD);
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA subresourceIndexData;
	ZeroMemory(&subresourceIndexData, sizeof(D3D11_SUBRESOURCE_DATA));
	subresourceIndexData.pSysMem = indices;
	hr = m_pD3d11Device->CreateBuffer(&indexBufferDesc, &subresourceIndexData, &pIndexBuff);

	if (FAILED(hr))
	{
		return;
	}	

	hr = D3DX11CompileFromFile( _T("FX/Cube.fx"),
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
		void* pTmp = pVsShaderError->GetBufferPointer();
		return;
	}

	hr = m_pD3d11Device->CreateVertexShader(pVsBuff->GetBufferPointer(), pVsBuff->GetBufferSize(), NULL, &pVs);
	if (FAILED(hr))
	{
		return;
	}

	hr = D3DX11CompileFromFile( _T("FX/Cube.fx"),
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
	inputElementDesc[1].AlignedByteOffset = 0;
	inputElementDesc[1].InputSlot = 1;
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

	ID3D11RasterizerState* pRasterizerState = NULL;
	D3D11_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
	rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	rasterizerDesc.FrontCounterClockwise = TRUE;
	rasterizerDesc.ScissorEnable = TRUE;
	hr = m_pD3d11Device->CreateRasterizerState(&rasterizerDesc, &pRasterizerState);
	if (FAILED(hr))
	{
		SAFE_RELEASE(pRasterizerState);
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

	XMMATRIX viewMatrix, projMatrix, worldMatrix;
	ZeroMemory(&viewMatrix, sizeof(XMMATRIX));
	ZeroMemory(&projMatrix, sizeof(XMMATRIX));
	ZeroMemory(&worldMatrix, sizeof(XMMATRIX));
	FXMVECTOR eyePos = XMVectorSet(1.0f, 1.0f, -1.0f, 1.0f);
	FXMVECTOR lookPos = XMVectorSet(.0f, .0f, .0f, 1.0f);
	FXMVECTOR upDir = XMVectorSet(.0f, 1.0f, .0f, .0f);

	//viewMatrix = XMMatrixLookAtLH(eyePos, lookPos, upDir);
	viewMatrix = XMMatrixIdentity();
	viewMatrix = XMMatrixTranspose(viewMatrix);
	projMatrix = XMMatrixPerspectiveFovLH(XM_PIDIV4, (float)m_dwWidth/m_dwHeight, 0.01f, 1000.0f);
	projMatrix = XMMatrixTranspose(projMatrix);
	worldMatrix = XMMatrixRotationRollPitchYaw(.0f, .7f, .7f);
	worldMatrix = XMMatrixMultiply(worldMatrix, XMMatrixTranslation(0.0f, 0.0f, 10.0f));
	worldMatrix = XMMatrixTranspose(worldMatrix);

	ID3D11Buffer* pConstantBuffer = NULL;

	ConstantBuffer constantBuffer;
	constantBuffer.worldMatrix = worldMatrix;
	constantBuffer.viewMatrix = viewMatrix;
	constantBuffer.projMatrix = projMatrix;
	constantBuffer.fElapseTime = (FLOAT)m_dwElapseTime/1000;

	D3D11_BUFFER_DESC buffDesc;
	ZeroMemory(&buffDesc, sizeof(D3D11_BUFFER_DESC));
	buffDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffDesc.Usage = D3D11_USAGE_DEFAULT;
	buffDesc.ByteWidth = sizeof(ConstantBuffer);
	subresourceData.pSysMem = &constantBuffer;
	hr = m_pD3d11Device->CreateBuffer(&buffDesc, &subresourceData, &pConstantBuffer);
	if (FAILED(hr))
	{
		return;
	}

	m_pD3d11DeviceContext->ClearDepthStencilView( m_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0 );
	float colorArray[4] = {.0,.0,.0,.0};
	m_pD3d11DeviceContext->ClearRenderTargetView(m_pRenderTargetView, colorArray);
	
	m_pD3d11DeviceContext->VSSetConstantBuffers(0, 1, &pConstantBuffer);

	m_pD3d11DeviceContext->IASetInputLayout(pInputLayOut);
	UINT stridesArray[2];
	stridesArray[0] = sizeof(VertexWithOutUV);
	stridesArray[1] = sizeof(VertexUV);
	UINT offsetsArray[2];
	offsetsArray[0] = 0;
	offsetsArray[1] = 0;

	ID3D11Buffer* bufferArray[2];
	bufferArray[0] = pVertexBuff;
	bufferArray[1] = pTexBuffer;
	m_pD3d11DeviceContext->IASetVertexBuffers(0, 2, bufferArray, stridesArray, offsetsArray);
	m_pD3d11DeviceContext->IASetIndexBuffer(pIndexBuff, DXGI_FORMAT_R16_UINT, 0);
	m_pD3d11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pD3d11DeviceContext->VSSetShader(pVs, 0, 0);
	m_pD3d11DeviceContext->PSSetShader(pPs, 0, 0);
	m_pD3d11DeviceContext->PSSetShaderResources(0, 1, &pShaderResView);
	m_pD3d11DeviceContext->PSSetSamplers(0, 1, &pSamplerState);
	m_pD3d11DeviceContext->RSSetState(pRasterizerState);

	D3D11_RECT rect;
	rect.left = 200;
	rect.right = 600;
	rect.top = 100;
	rect.bottom = 500;
	m_pD3d11DeviceContext->RSSetScissorRects(1, &rect);

	D3D11_VIEWPORT vp;
	vp.Height = (FLOAT)m_dwHeight;
	vp.Width = (FLOAT)m_dwWidth;
	vp.TopLeftX = .0f;
	vp.TopLeftY = .0f;
	vp.MinDepth = .0f;
	vp.MaxDepth = 1.0f;
	m_pD3d11DeviceContext->RSSetViewports(1, &vp);

	m_pD3d11DeviceContext->DrawIndexed(36, 0, 0);

	// draw rect
	SAFE_RELEASE(pConstantBuffer);
	worldMatrix = XMMatrixMultiply(worldMatrix, XMMatrixTranslation(-3.0f, 0.0f, 5.0f));
	worldMatrix = XMMatrixTranspose(worldMatrix);

	constantBuffer.worldMatrix = worldMatrix;

	ZeroMemory(&buffDesc, sizeof(D3D11_BUFFER_DESC));
	buffDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffDesc.Usage = D3D11_USAGE_DEFAULT;
	buffDesc.ByteWidth = sizeof(ConstantBuffer);
	subresourceData.pSysMem = &constantBuffer;
	hr = m_pD3d11Device->CreateBuffer(&buffDesc, &subresourceData, &pConstantBuffer);
	if (FAILED(hr))
	{
		return;
	}
	m_pD3d11DeviceContext->VSSetConstantBuffers(0, 1, &pConstantBuffer);
	m_pD3d11DeviceContext->DrawIndexed(6, 36, 0);

	m_pDXGISwapChain->Present(0, 0);
#else
	UINT vertexSize = ARRAYSIZE(vertexPos);
	UINT indexSize = ARRAYSIZE(indices);

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

	D3D11_BUFFER_DESC indexBufferDesc;
	ZeroMemory(&indexBufferDesc, sizeof(D3D11_BUFFER_DESC));
	indexBufferDesc.ByteWidth = indexSize * sizeof(WORD);
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA subresourceIndexData;
	ZeroMemory(&subresourceIndexData, sizeof(D3D11_SUBRESOURCE_DATA));
	subresourceIndexData.pSysMem = indices;
	hr = m_pD3d11Device->CreateBuffer(&indexBufferDesc, &subresourceIndexData, &pIndexBuff);

	if (FAILED(hr))
	{
		return;
	}	

	hr = D3DX11CompileFromFile( _T("FX/Cube.fx"),
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

	hr = D3DX11CompileFromFile( _T("FX/Cube.fx"),
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

	XMMATRIX viewMatrix, projMatrix, worldMatrix;
	ZeroMemory(&viewMatrix, sizeof(XMMATRIX));
	ZeroMemory(&projMatrix, sizeof(XMMATRIX));
	ZeroMemory(&worldMatrix, sizeof(XMMATRIX));
	FXMVECTOR eyePos = XMVectorSet(1.0f, 1.0f, -1.0f, 1.0f);
	FXMVECTOR lookPos = XMVectorSet(.0f, .0f, .0f, 1.0f);
	FXMVECTOR upDir = XMVectorSet(.0f, 1.0f, .0f, .0f);

	//viewMatrix = XMMatrixLookAtLH(eyePos, lookPos, upDir);
	viewMatrix = XMMatrixIdentity();
	viewMatrix = XMMatrixTranspose(viewMatrix);
	projMatrix = XMMatrixPerspectiveFovLH(XM_PIDIV4, (float)m_dwWidth/m_dwHeight, 0.01f, 1000.0f);
	projMatrix = XMMatrixTranspose(projMatrix);
	worldMatrix = XMMatrixRotationRollPitchYaw(.0f, .7f, .7f);
	worldMatrix = XMMatrixMultiply(worldMatrix, XMMatrixTranslation(0.0f, 0.0f, 10.0f));
	worldMatrix = XMMatrixTranspose(worldMatrix);

	ID3D11Buffer* pViewMatrixCB = NULL;
	ID3D11Buffer* pProjMatrixCB = NULL;
	ID3D11Buffer* pworldMatrixCB = NULL;

	D3D11_BUFFER_DESC buffDesc;
	ZeroMemory(&buffDesc, sizeof(D3D11_BUFFER_DESC));
	buffDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffDesc.ByteWidth = sizeof(XMMATRIX);
	buffDesc.Usage = D3D11_USAGE_DEFAULT;
	hr = m_pD3d11Device->CreateBuffer(&buffDesc, NULL, &pViewMatrixCB);
	if (FAILED(hr))
	{
		return;
	}
	hr = m_pD3d11Device->CreateBuffer(&buffDesc, NULL, &pProjMatrixCB);
	if (FAILED(hr))
	{
		return;
	}
	hr = m_pD3d11Device->CreateBuffer(&buffDesc, NULL, &pworldMatrixCB);
	if (FAILED(hr))
	{
		return;
	}

	m_pD3d11DeviceContext->UpdateSubresource(pViewMatrixCB, 0, 0, reinterpret_cast<void*> (&viewMatrix), 0, 0 );
	m_pD3d11DeviceContext->UpdateSubresource(pProjMatrixCB, 0, 0, reinterpret_cast<void*> (&projMatrix), 0, 0 );
	m_pD3d11DeviceContext->UpdateSubresource(pworldMatrixCB, 0, 0, reinterpret_cast<void*> (&worldMatrix), 0, 0 );

	m_pD3d11DeviceContext->ClearDepthStencilView( m_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0 );

	m_pD3d11DeviceContext->VSSetConstantBuffers(0, 1, &pViewMatrixCB);
	m_pD3d11DeviceContext->VSSetConstantBuffers(1, 1, &pProjMatrixCB);
	m_pD3d11DeviceContext->VSSetConstantBuffers(2, 1, &pworldMatrixCB);

	m_pD3d11DeviceContext->IASetInputLayout(pInputLayOut);
	UINT dwStrides = sizeof(VertexFmt);
	UINT dwOffsets = 0;
	m_pD3d11DeviceContext->IASetVertexBuffers(0, 1, &pVertexBuff, &dwStrides, &dwOffsets);
	m_pD3d11DeviceContext->IASetIndexBuffer(pIndexBuff, DXGI_FORMAT_R16_UINT, 0);
	m_pD3d11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pD3d11DeviceContext->VSSetShader(pVs, 0, 0);
	m_pD3d11DeviceContext->PSSetShader(pPs, 0, 0);
	m_pD3d11DeviceContext->PSSetShaderResources(0, 1, &pShaderResView);
	m_pD3d11DeviceContext->PSSetSamplers(0, 1, &pSamplerState);

	D3D11_VIEWPORT vp;
	vp.Height = (FLOAT)m_dwHeight;
	vp.Width = (FLOAT)m_dwWidth;
	vp.TopLeftX = .0f;
	vp.TopLeftY = .0f;
	vp.MinDepth = .0f;
	vp.MaxDepth = 1.0f;
	m_pD3d11DeviceContext->RSSetViewports(1, &vp);

	m_pD3d11DeviceContext->DrawIndexed(indexSize, 0, 0);
	m_pDXGISwapChain->Present(0, 0);
#endif
}

void CD3dDisplay::Effect()
{
	ID3D11Buffer* pVertexBuff = NULL;
	ID3D11Buffer* pIndexBuff = NULL;
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
		{ XMFLOAT3( -1.0f,  1.0f, -1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3(  1.0f,  1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT3(  1.0f,  1.0f,  1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT3( -1.0f,  1.0f,  1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

		{ XMFLOAT3( -1.0f, -1.0f, -1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3(  1.0f, -1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT3(  1.0f, -1.0f,  1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT3( -1.0f, -1.0f,  1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

		{ XMFLOAT3( -1.0f, -1.0f,  1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3( -1.0f, -1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT3( -1.0f,  1.0f, -1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT3( -1.0f,  1.0f,  1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

		{ XMFLOAT3(  1.0f, -1.0f,  1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3(  1.0f, -1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT3(  1.0f,  1.0f, -1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT3(  1.0f,  1.0f,  1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

		{ XMFLOAT3( -1.0f, -1.0f, -1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3(  1.0f, -1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT3(  1.0f,  1.0f, -1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT3( -1.0f,  1.0f, -1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

		{ XMFLOAT3( -1.0f, -1.0f,  1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3(  1.0f, -1.0f,  1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT3(  1.0f,  1.0f,  1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT3( -1.0f,  1.0f,  1.0f ), XMFLOAT2( 0.0f, 1.0f ) }
	};

	WORD indices[] =
	{
		3,   1,  0,  2,  1,  3,
		6,   4,  5,  7,  4,  6,
		11,  9,  8, 10,  9, 11,
		14, 12, 13, 15, 12, 14,
		19, 17, 16, 18, 17, 19,
		22, 20, 21, 23, 20, 22
	};

	UINT vertexSize = ARRAYSIZE(vertexPos);
	UINT indexSize = ARRAYSIZE(indices);

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

	D3D11_BUFFER_DESC indexBufferDesc;
	ZeroMemory(&indexBufferDesc, sizeof(D3D11_BUFFER_DESC));
	indexBufferDesc.ByteWidth = indexSize * sizeof(WORD);
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA subresourceIndexData;
	ZeroMemory(&subresourceIndexData, sizeof(D3D11_SUBRESOURCE_DATA));
	subresourceIndexData.pSysMem = indices;
	hr = m_pD3d11Device->CreateBuffer(&indexBufferDesc, &subresourceIndexData, &pIndexBuff);

	if (FAILED(hr))
	{
		return;
	}	

	ID3DX11EffectTechnique* pEffectTechnique = m_pEffect->GetTechniqueByName("ColorInversion");
	ID3DX11EffectPass* pEffectPass = NULL;
	if (pEffectTechnique)
	{
		pEffectPass = pEffectTechnique->GetPassByName("P1");
		if (!pEffectPass)
		{
			return;
		}
	}

	D3DX11_EFFECT_SHADER_DESC effectShaderDesc;
	D3DX11_PASS_SHADER_DESC passShaderDesc;
	ZeroMemory(&effectShaderDesc, sizeof(D3DX11_EFFECT_SHADER_DESC));
	ZeroMemory(&passShaderDesc, sizeof(D3DX11_PASS_SHADER_DESC));
	pEffectPass->GetVertexShaderDesc(&passShaderDesc);
	passShaderDesc.pShaderVariable->GetShaderDesc(0, &effectShaderDesc);

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

	hr = m_pD3d11Device->CreateInputLayout( inputElementDesc, 2, effectShaderDesc.pBytecode, effectShaderDesc.BytecodeLength, &pInputLayOut);
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

	XMMATRIX viewMatrix, projMatrix, worldMatrix;
	ZeroMemory(&viewMatrix, sizeof(XMMATRIX));
	ZeroMemory(&projMatrix, sizeof(XMMATRIX));
	ZeroMemory(&worldMatrix, sizeof(XMMATRIX));
	FXMVECTOR eyePos = XMVectorSet(1.0f, 1.0f, -1.0f, 1.0f);
	FXMVECTOR lookPos = XMVectorSet(.0f, .0f, .0f, 1.0f);
	FXMVECTOR upDir = XMVectorSet(.0f, 1.0f, .0f, .0f);

	//viewMatrix = XMMatrixLookAtLH(eyePos, lookPos, upDir);
	viewMatrix = XMMatrixIdentity();
	projMatrix = XMMatrixPerspectiveFovLH(XM_PIDIV4, (float)(m_dwWidth/m_dwHeight), 0.01f, 1000.0f);
	worldMatrix = XMMatrixRotationRollPitchYaw(.0f, .7f, .7f);
	worldMatrix = XMMatrixMultiply(worldMatrix, XMMatrixTranslation(0.0f, 0.0f, 10.0f));
	
	m_pD3d11DeviceContext->ClearDepthStencilView( m_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0 );

	m_pD3d11DeviceContext->IASetInputLayout(pInputLayOut);
	UINT dwStrides = sizeof(VertexFmt);
	UINT dwOffsets = 0;
	m_pD3d11DeviceContext->IASetVertexBuffers(0, 1, &pVertexBuff, &dwStrides, &dwOffsets);
	m_pD3d11DeviceContext->IASetIndexBuffer(pIndexBuff, DXGI_FORMAT_R16_UINT, 0);
	m_pD3d11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ID3DX11EffectMatrixVariable* pMatrix =  m_pEffect->GetVariableByName("viewMatrix")->AsMatrix();
	if (!pMatrix)
	{
		return;
	}
	pMatrix->SetMatrix((float*)&viewMatrix);

	pMatrix = m_pEffect->GetVariableByName("projMatrix")->AsMatrix();
	if (!pMatrix)
	{
		return;
	}
	pMatrix->SetMatrix((float*)&projMatrix);

	pMatrix = m_pEffect->GetVariableByName("worldMatrix")->AsMatrix();
	if (!pMatrix)
	{
		return;
	}
	pMatrix->SetMatrix((float*)&worldMatrix);

	ID3DX11EffectShaderResourceVariable* pShaderRes = m_pEffect->GetVariableByName("colorMap")->AsShaderResource();
	if (!pShaderRes)
	{
		return;
	}
	pShaderRes->SetResource(pShaderResView);

	ID3DX11EffectSamplerVariable* pSampler = m_pEffect->GetVariableByName("colorSampler")->AsSampler();
	if (!pSampler)
	{
		return;
	}
	pSampler->SetSampler(0, pSamplerState);

	D3D11_VIEWPORT vp;
	vp.Height = (FLOAT)m_dwHeight;
	vp.Width = (FLOAT)m_dwWidth;
	vp.TopLeftX = .0f;
	vp.TopLeftY = .0f;
	vp.MinDepth = .0f;
	vp.MaxDepth = 1.0f;
	m_pD3d11DeviceContext->RSSetViewports(1, &vp);

	m_pD3d11DeviceContext->RSSetState(m_pResterizerState);

	hr = pEffectPass->Apply(0, m_pD3d11DeviceContext);
	if (FAILED(hr))
	{
		return;
	}
	m_pD3d11DeviceContext->DrawIndexed(indexSize, 0, 0);
	m_pDXGISwapChain->Present(0, 0);
}

void CD3dDisplay::MultiTexture()
{
	ID3D11Buffer* pVertexBuff = NULL;
	ID3D11Buffer* pIndexBuff = NULL;
	ID3D11InputLayout* pInputLayOut = NULL;
	ID3D11VertexShader* pVs = NULL;
	ID3D11PixelShader* pPs = NULL;
	ID3DBlob* pVsBuff =  NULL;
	ID3DBlob* pPsBuff = NULL;
	ID3DBlob* pVsShaderError = NULL;
	ID3DBlob* pPsShaderError = NULL;
	ID3D11ShaderResourceView* pShaderResView = NULL;
	ID3D11ShaderResourceView* pSecondShaderResView = NULL;
	ID3D11SamplerState* pSamplerState = NULL;
	HRESULT hr = S_OK;

	VertexFmt vertexPos[] =
	{
		{ XMFLOAT3( -1.0f,  1.0f, -1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3(  1.0f,  1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT3(  1.0f,  1.0f,  1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT3( -1.0f,  1.0f,  1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

		{ XMFLOAT3( -1.0f, -1.0f, -1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3(  1.0f, -1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT3(  1.0f, -1.0f,  1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT3( -1.0f, -1.0f,  1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

		{ XMFLOAT3( -1.0f, -1.0f,  1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3( -1.0f, -1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT3( -1.0f,  1.0f, -1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT3( -1.0f,  1.0f,  1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

		{ XMFLOAT3(  1.0f, -1.0f,  1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3(  1.0f, -1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT3(  1.0f,  1.0f, -1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT3(  1.0f,  1.0f,  1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

		{ XMFLOAT3( -1.0f, -1.0f, -1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3(  1.0f, -1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT3(  1.0f,  1.0f, -1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT3( -1.0f,  1.0f, -1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

		{ XMFLOAT3( -1.0f, -1.0f,  1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
		{ XMFLOAT3(  1.0f, -1.0f,  1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
		{ XMFLOAT3(  1.0f,  1.0f,  1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
		{ XMFLOAT3( -1.0f,  1.0f,  1.0f ), XMFLOAT2( 0.0f, 1.0f ) }
	};

	WORD indices[] =
	{
		3,   1,  0,  2,  1,  3,
		6,   4,  5,  7,  4,  6,
		11,  9,  8, 10,  9, 11,
		14, 12, 13, 15, 12, 14,
		19, 17, 16, 18, 17, 19,
		22, 20, 21, 23, 20, 22
	};

	UINT vertexSize = ARRAYSIZE(vertexPos);
	UINT indexSize = ARRAYSIZE(indices);

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

	D3D11_BUFFER_DESC indexBufferDesc;
	ZeroMemory(&indexBufferDesc, sizeof(D3D11_BUFFER_DESC));
	indexBufferDesc.ByteWidth = indexSize * sizeof(WORD);
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA subresourceIndexData;
	ZeroMemory(&subresourceIndexData, sizeof(D3D11_SUBRESOURCE_DATA));
	subresourceIndexData.pSysMem = indices;
	hr = m_pD3d11Device->CreateBuffer(&indexBufferDesc, &subresourceIndexData, &pIndexBuff);

	if (FAILED(hr))
	{
		return;
	}	

	ID3DX11EffectTechnique* pEffectTechnique = m_pEffect->GetTechniqueByName("ColorInversion");
	ID3DX11EffectPass* pEffectPass = NULL;
	if (pEffectTechnique)
	{
		pEffectPass = pEffectTechnique->GetPassByName("P2");
		if (!pEffectPass)
		{
			return;
		}
	}

	D3DX11_EFFECT_SHADER_DESC effectShaderDesc;
	D3DX11_PASS_SHADER_DESC passShaderDesc;
	ZeroMemory(&effectShaderDesc, sizeof(D3DX11_EFFECT_SHADER_DESC));
	ZeroMemory(&passShaderDesc, sizeof(D3DX11_PASS_SHADER_DESC));
	pEffectPass->GetVertexShaderDesc(&passShaderDesc);
	passShaderDesc.pShaderVariable->GetShaderDesc(0, &effectShaderDesc);

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

	hr = m_pD3d11Device->CreateInputLayout( inputElementDesc, 2, effectShaderDesc.pBytecode, effectShaderDesc.BytecodeLength, &pInputLayOut);
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

	hr = D3DX11CreateShaderResourceViewFromFile( m_pD3d11Device,
												_T("RES/decal2.dds"),
												NULL,
												NULL,
												&pSecondShaderResView,
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

	XMMATRIX viewMatrix, projMatrix, worldMatrix;
	ZeroMemory(&viewMatrix, sizeof(XMMATRIX));
	ZeroMemory(&projMatrix, sizeof(XMMATRIX));
	ZeroMemory(&worldMatrix, sizeof(XMMATRIX));
	FXMVECTOR eyePos = XMVectorSet(1.0f, 1.0f, -1.0f, 1.0f);
	FXMVECTOR lookPos = XMVectorSet(.0f, .0f, .0f, 1.0f);
	FXMVECTOR upDir = XMVectorSet(.0f, 1.0f, .0f, .0f);

	//viewMatrix = XMMatrixLookAtLH(eyePos, lookPos, upDir);
	viewMatrix = XMMatrixIdentity();
	projMatrix = XMMatrixPerspectiveFovLH(XM_PIDIV4, (float)(m_dwWidth/m_dwHeight), 0.01f, 1000.0f);
	worldMatrix = XMMatrixRotationRollPitchYaw(.0f, .7f, .7f);
	worldMatrix = XMMatrixMultiply(worldMatrix, XMMatrixTranslation(0.0f, 0.0f, 10.0f));

	m_pD3d11DeviceContext->ClearDepthStencilView( m_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0 );

	m_pD3d11DeviceContext->IASetInputLayout(pInputLayOut);
	UINT dwStrides = sizeof(VertexFmt);
	UINT dwOffsets = 0;
	m_pD3d11DeviceContext->IASetVertexBuffers(0, 1, &pVertexBuff, &dwStrides, &dwOffsets);
	m_pD3d11DeviceContext->IASetIndexBuffer(pIndexBuff, DXGI_FORMAT_R16_UINT, 0);
	m_pD3d11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ID3DX11EffectMatrixVariable* pMatrix =  m_pEffect->GetVariableByName("viewMatrix")->AsMatrix();
	if (!pMatrix)
	{
		return;
	}
	pMatrix->SetMatrix((float*)&viewMatrix);

	pMatrix = m_pEffect->GetVariableByName("projMatrix")->AsMatrix();
	if (!pMatrix)
	{
		return;
	}
	pMatrix->SetMatrix((float*)&projMatrix);

	pMatrix = m_pEffect->GetVariableByName("worldMatrix")->AsMatrix();
	if (!pMatrix)
	{
		return;
	}
	pMatrix->SetMatrix((float*)&worldMatrix);

	ID3DX11EffectShaderResourceVariable* pShaderRes = m_pEffect->GetVariableByName("colorMap")->AsShaderResource();
	if (!pShaderRes)
	{
		return;
	}
	pShaderRes->SetResource(pShaderResView);

	pShaderRes = m_pEffect->GetVariableByName("colorMap2")->AsShaderResource();
	if (!pShaderRes)
	{
		return;
	}
	pShaderRes->SetResource(pSecondShaderResView);

	ID3DX11EffectSamplerVariable* pSampler = m_pEffect->GetVariableByName("colorSampler")->AsSampler();
	if (!pSampler)
	{
		return;
	}
	pSampler->SetSampler(0, pSamplerState);

	D3D11_VIEWPORT vp;
	vp.Height = (FLOAT)m_dwHeight;
	vp.Width = (FLOAT)m_dwWidth;
	vp.TopLeftX = .0f;
	vp.TopLeftY = .0f;
	vp.MinDepth = .0f;
	vp.MaxDepth = 1.0f;
	m_pD3d11DeviceContext->RSSetViewports(1, &vp);

	m_pD3d11DeviceContext->RSSetState(m_pResterizerState);

	hr = pEffectPass->Apply(0, m_pD3d11DeviceContext);
	if (FAILED(hr))
	{
		return;
	}
	m_pD3d11DeviceContext->DrawIndexed(indexSize, 0, 0);
	m_pDXGISwapChain->Present(0, 0);
}

bool CD3dDisplay::LoadModelFromFile( std::wstring szFileName)
{
	HRESULT hr = S_OK;

	std::wifstream fileIn;
	fileIn.open((szFileName.c_str()), std::ios::in, 0x10);
	if (!fileIn)
	{
		return false;
	}

	std::wstring strIgnore;

	fileIn>>strIgnore>>m_dwSkullVertexCnt;
	fileIn>>strIgnore>>m_dwSkullIndexCnt;
	fileIn>>strIgnore>>strIgnore>>strIgnore>>strIgnore;

	VertexFmtWithNormal *vertexPos = new VertexFmtWithNormal[m_dwSkullVertexCnt];
	for (int i =0; i < m_dwSkullVertexCnt; i++)
	{
		fileIn>>vertexPos[i].position.x>>vertexPos[i].position.y>>vertexPos[i].position.z;
		fileIn>>vertexPos[i].normal.x>>vertexPos[i].normal.y>>vertexPos[i].normal.z;
	}
	
	fileIn>>strIgnore>>strIgnore>>strIgnore;

	unsigned int *indices = new unsigned int[3*m_dwSkullIndexCnt];
	for (int i = 0; i < m_dwSkullIndexCnt; i++)
	{
		fileIn >>indices[3*i]>>indices[3*i+1]>>indices[3*i+2];
	}

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	bufferDesc.ByteWidth = sizeof(VertexFmtWithNormal) * m_dwSkullVertexCnt;
	D3D11_SUBRESOURCE_DATA subResourceData;
	ZeroMemory(&subResourceData, sizeof(D3D11_SUBRESOURCE_DATA));
	subResourceData.pSysMem = vertexPos;
	hr = m_pD3d11Device->CreateBuffer(&bufferDesc, &subResourceData, &m_pSkullVertexBuffer);
	if (FAILED(hr))
	{
		SAFE_RELEASE(m_pSkullVertexBuffer);
		return false;
	}

	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.ByteWidth = sizeof(unsigned int) * 3 * m_dwSkullIndexCnt;
	subResourceData.pSysMem = indices;
	hr = m_pD3d11Device->CreateBuffer(&bufferDesc, &subResourceData, &m_pSkullIndexBuffer);
	if (FAILED(hr))
	{
		SAFE_RELEASE(m_pSkullIndexBuffer);
		return false;
	}

	return true;
}

void CD3dDisplay::DrawObjModel()
{
	HRESULT hr = S_OK;
	ID3DBlob* pVsBuff = NULL;
	ID3DBlob* pVsShaderError = NULL;
	ID3DBlob* pPsBuff = NULL;
	ID3DBlob* pPsShaderError = NULL;
	ID3D11VertexShader* pVs = NULL;
	ID3D11PixelShader* pPs = NULL;
	ID3D11InputLayout* pInputLayout = NULL;
	ID3D11RasterizerState* pRasterizerState = NULL;

	hr = D3DX11CompileFromFile( _T("FX/ObjModel.fx"),
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
		void* str = pVsShaderError->GetBufferPointer();
		wstring strError = AnsiToUnicode((char*)str);
		OutputDebugString(strError.c_str());
		return;
	}

	hr = m_pD3d11Device->CreateVertexShader(pVsBuff->GetBufferPointer(), pVsBuff->GetBufferSize(), NULL, &pVs);
	if (FAILED(hr))
	{
		return;
	}

	hr = D3DX11CompileFromFile( _T("FX/ObjModel.fx"),
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
		void* str = pPsShaderError->GetBufferPointer();
		wstring strError = AnsiToUnicode((char*)str);
		OutputDebugString(strError.c_str());
		return;
	}

	hr = m_pD3d11Device->CreatePixelShader(pPsBuff->GetBufferPointer(), pPsBuff->GetBufferSize(), NULL, &pPs);
	if (FAILED(hr))
	{
		return;
	}

	D3D11_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	rasterizerDesc.FrontCounterClockwise = FALSE;
	hr = m_pD3d11Device->CreateRasterizerState(&rasterizerDesc, &pRasterizerState);
	if (FAILED(hr))
	{
		return;
	}

	D3D11_INPUT_ELEMENT_DESC inputDesc[2];
	ZeroMemory(&inputDesc, sizeof(inputDesc));
	inputDesc[0].SemanticName = "POSITION";
	inputDesc[0].InputSlot = 0;
	inputDesc[0].AlignedByteOffset = 0;
	inputDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputDesc[0].SemanticIndex = 0;
	inputDesc[1].SemanticName = "NORMAL";
	inputDesc[1].InputSlot = 0;
	inputDesc[1].AlignedByteOffset = 12;
	inputDesc[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputDesc[1].SemanticIndex = 0;

	hr = m_pD3d11Device->CreateInputLayout(	inputDesc,
											2,
											pVsBuff->GetBufferPointer(),
											pVsBuff->GetBufferSize(),
											&pInputLayout);
	if (FAILED(hr))
	{
		return ;
	}

	XMMATRIX viewMatrix, projMatrix, worldMatrix, worldViewProjNormalMatrix;
	ZeroMemory(&viewMatrix, sizeof(XMMATRIX));
	ZeroMemory(&projMatrix, sizeof(XMMATRIX));
	ZeroMemory(&worldMatrix, sizeof(XMMATRIX));
	FXMVECTOR eyePos = XMVectorSet(m_eyePos.x, m_eyePos.y, m_eyePos.z, 1.0f);
	FXMVECTOR lookPos = XMVectorSet(.0f, .0f, .0f, 1.0f);
	FXMVECTOR upDir = XMVectorSet(.0f, 1.0f, .0f, .0f);

	viewMatrix = XMMatrixIdentity();
	//viewMatrix = XMMatrixTranspose(viewMatrix);
	FXMVECTOR lookAtPos = XMVectorSet(.0f, .0f, .0f, 1.0f);
	viewMatrix = XMMatrixLookAtLH(eyePos, lookAtPos, XMVectorSet(.0f, 1.0f, .0f, 1.0f));
	projMatrix = XMMatrixPerspectiveFovLH(XM_PIDIV4, (float)m_dwWidth/m_dwHeight, 0.01f, 1000.0f);
	projMatrix = XMMatrixTranspose(projMatrix);
	worldMatrix = m_localTranslation;//XMMatrixRotationRollPitchYaw(.0f, .7f+3.14*0.1*m_dwElapseTime*0.001, .0f);
	worldMatrix = XMMatrixMultiply(worldMatrix, XMMatrixTranslation(0.0f, -2.0f, 20.0f));
	worldMatrix = XMMatrixTranspose(worldMatrix);

	worldViewProjNormalMatrix = XMMatrixMultiply(worldMatrix, viewMatrix);
	worldViewProjNormalMatrix = XMMatrixMultiply(worldViewProjNormalMatrix, projMatrix);
	XMVECTOR determinant = XMMatrixDeterminant(worldViewProjNormalMatrix);
	worldViewProjNormalMatrix = XMMatrixInverse(&determinant, worldViewProjNormalMatrix);
	worldViewProjNormalMatrix = XMMatrixTranspose(worldViewProjNormalMatrix);

	ID3D11Buffer* pConstantBuffer = NULL;

	ConstantBuffer constantBuffer;
	constantBuffer.projMatrix = projMatrix;
	constantBuffer.viewMatrix = viewMatrix;
	constantBuffer.worldMatrix = worldMatrix;
	constantBuffer.normalMatrix = worldViewProjNormalMatrix;
	XMStoreFloat4(&constantBuffer.eyePos, eyePos);
	constantBuffer.fElapseTime = (float)m_dwElapseTime;

	D3D11_BUFFER_DESC buffDesc;
	ZeroMemory(&buffDesc, sizeof(D3D11_BUFFER_DESC));
	buffDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffDesc.ByteWidth = sizeof(ConstantBuffer);
	buffDesc.Usage = D3D11_USAGE_DEFAULT;
	D3D11_SUBRESOURCE_DATA subResourceData;
	ZeroMemory(&subResourceData, sizeof(D3D11_SUBRESOURCE_DATA));
	subResourceData.pSysMem = &constantBuffer;
	hr = m_pD3d11Device->CreateBuffer(&buffDesc, &subResourceData, &pConstantBuffer);
	if (FAILED(hr))
	{
		SAFE_RELEASE(pConstantBuffer);
		return;
	}

	m_pD3d11DeviceContext->ClearDepthStencilView( m_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0 );
	float colorArray[4] = {.0,.0,.0,.0};
	m_pD3d11DeviceContext->ClearRenderTargetView(m_pRenderTargetView, colorArray);

	m_pD3d11DeviceContext->VSSetConstantBuffers(0, 1, &pConstantBuffer);

	m_pD3d11DeviceContext->IASetInputLayout(pInputLayout);
	UINT dwStrides = sizeof(VertexFmtWithNormal);
	UINT dwOffsets = 0;
	m_pD3d11DeviceContext->IASetVertexBuffers(0, 1, &m_pSkullVertexBuffer, &dwStrides, &dwOffsets);
	m_pD3d11DeviceContext->IASetIndexBuffer(m_pSkullIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	m_pD3d11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pD3d11DeviceContext->VSSetShader(pVs, 0, 0);
	m_pD3d11DeviceContext->PSSetShader(pPs, 0, 0);
	m_pD3d11DeviceContext->RSSetState(pRasterizerState);

	D3D11_VIEWPORT vp;
	vp.Height = (FLOAT)m_dwHeight;
	vp.Width = (FLOAT)m_dwWidth;
	vp.TopLeftX = .0f;
	vp.TopLeftY = .0f;
	vp.MinDepth = .0f;
	vp.MaxDepth = 1.0f;
	m_pD3d11DeviceContext->RSSetViewports(1, &vp);

	m_pD3d11DeviceContext->DrawIndexed(3 * m_dwSkullIndexCnt, 0, 0);
	m_pDXGISwapChain->Present(0, 0);

	SAFE_RELEASE(pVsBuff);
	SAFE_RELEASE(pVsShaderError);
	SAFE_RELEASE(pPsBuff);
	SAFE_RELEASE(pPsShaderError);
	SAFE_RELEASE(pVs);
	SAFE_RELEASE(pPs);
	SAFE_RELEASE(pInputLayout);
	SAFE_RELEASE(pConstantBuffer);
}

void CD3dDisplay::ToggleFullScreen()
{
	m_isFullScreen = !m_isFullScreen;

	m_pDXGISwapChain->SetFullscreenState(m_isFullScreen, NULL);
}

void CD3dDisplay::UpdateElapseTime()
{
	if (m_dwElapseTime == 0)
	{
		m_dwElapseTime = GetTickCount();
		m_dwDeltaTime = 0;
	}
	else
	{
		m_dwDeltaTime = (GetTickCount() >= m_dwElapseTime)?(GetTickCount() - m_dwElapseTime):0;
		m_dwElapseTime = GetTickCount();
		/*std::wstringstream debugString;
		debugString<<"The elapse time is:"<<m_dwElapseTime/1000<<"\n";
		OutputDebugString(debugString.str().c_str());*/
	}
}

void CD3dDisplay::SetEyePos( float x, float y, float z )
{
	XMMATRIX translation = XMMatrixIdentity();// = XMMatrixRotationRollPitchYaw(z * .01f, x * .01f, x * .1f);
	float fAngle = .0f;
	if (x > .00001f || x < -.00001f)
	{
		fAngle = x * 0.001f;
		translation._11 *= cos(fAngle);
		translation._12 = -1 * sin(fAngle);
		translation._21 = cos(fAngle);
		translation._22 *= sin(fAngle);
		//xmmatrixtra
		FXMVECTOR eyePos = XMVectorSet(m_eyePos.x, m_eyePos.y, m_eyePos.z, 1.0f);
		XMVECTOR eyePosTrans = XMVector3Transform(eyePos, translation);
		XMStoreFloat3(&m_eyePos, eyePosTrans);
	}

	if (y > .00001f || y < -.00001f)
	{
		fAngle = y * 0.00001f;
		translation = XMMatrixIdentity();
		translation._22 *= cos(fAngle);
		translation._23 = -1 * sin(fAngle);
		translation._32 = cos(fAngle);
		translation._33 *= sin(fAngle);
		//xmmatrixtra
		FXMVECTOR eyePos = XMVectorSet(m_eyePos.x, m_eyePos.y, m_eyePos.z, 1.0f);
		XMVECTOR eyePosTrans = XMVector3Transform(eyePos, translation);
		XMStoreFloat3(&m_eyePos, eyePosTrans);
	}
}

XMFLOAT3 CD3dDisplay::GetEyePos()
{
	return m_eyePos;
}

void CD3dDisplay::SetLocalTranslation( float x, float y, float z )
{
	XMMATRIX localTrans = XMMatrixRotationRollPitchYaw(x * .01f, y * .01f, z * .01f);
	m_localTranslation = XMMatrixMultiply(m_localTranslation, localTrans);
}
