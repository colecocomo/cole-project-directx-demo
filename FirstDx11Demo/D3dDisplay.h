#pragma once
//#include <d3d11.h>
#include <D3DX11.h>
#include <d3dx11effect.h>
#include <string>
#include <vector>
#include <dxgi.h>
#include <xnamath.h>

class CD3dDisplay
{
public:
    CD3dDisplay(void);
    virtual ~CD3dDisplay(void);

public:
    bool InitDevice2D(HWND hWnd);
	bool InitDevice3D(HWND hWnd);
    void ClearTargetView();

    void DrawTriangle();
	void DrawSprite();
	void DrawCube();

	void DrawObjModel();

	void Effect();

	void MultiTexture();

	bool LoadModelFromFile(	std::wstring szFileName );

	void ToggleFullScreen();

	void UpdateElapseTime();

	void SetEyePos(float x, float y, float z);

	XMFLOAT3 GetEyePos();

	void SetLocalTranslation(float x, float y, float z);

	void GenerateGeometry(unsigned int dwWidth, unsigned int dwHeight);

	void DrawGeometry();

private:
    ID3D11Device*                   m_pD3d11Device;
    ID3D11DeviceContext*            m_pD3d11DeviceContext;
    IDXGISwapChain*                 m_pDXGISwapChain;
	ID3D11RenderTargetView*			m_pRenderTargetView;
	ID3D11DepthStencilView*			m_pDepthStencilView;
	ID3DX11Effect*					m_pEffect;
	ID3D11RasterizerState*			m_pResterizerState;

    D3D_FEATURE_LEVEL               m_nD3DFeatureLevel;
    D3D_DRIVER_TYPE                 m_nD3DDriveType;

	// render content
	ID3D11Buffer*				m_pVertexBuff;
	ID3D11InputLayout*			m_pInputLayOut;
	ID3D11VertexShader*			m_pVs;
	ID3D11PixelShader*			m_pPs;
	ID3DBlob*					m_pVsBuff;
	ID3DBlob*					m_pPsBuff;
	ID3DBlob*					m_pVsShaderError;
	ID3DBlob*					m_pPsShaderError;
	ID3D11ShaderResourceView*	m_pShaderResView;
	ID3D11SamplerState*			m_pSamplerState;

	UINT							m_dwHeight;
	UINT							m_dwWidth;

	typedef std::vector<ID3D11Buffer*> BufferVector;
	typedef BufferVector::iterator BufferVectorIter;
	BufferVector				m_vObjModelVertexBuff;
	BufferVector				m_vObjModelIndexBuff;

	bool						m_isFullScreen;

	unsigned int				m_dwElapseTime;
	unsigned int				m_dwDeltaTime;

	// skull model
	ID3D11Buffer*				m_pSkullVertexBuffer;
	ID3D11Buffer*				m_pSkullIndexBuffer;
	unsigned int				m_dwSkullVertexCnt;
	unsigned int				m_dwSkullIndexCnt;

	XMFLOAT3					m_eyePos;
	XMMATRIX					m_localTranslation;

	// geometry
	ID3D11Buffer*				m_pGeometryVertexBuffer;
	unsigned int				m_dwGeometryWidth;
	unsigned int				m_dwGeometryHeight;
};

