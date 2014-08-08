#pragma once

#include <xnamath.h>

#ifndef SAFE_DELETE
#define SAFE_DELETE( p ) \
    if( p )\
    {\
    delete (p); \
    p = NULL; \
    }
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE( p ) \
	if( p )\
	{\
	p->Release(); \
	}
#endif

D3D_DRIVER_TYPE g_driveType[] = 
{
    D3D_DRIVER_TYPE_HARDWARE,
    D3D_DRIVER_TYPE_WARP,
    D3D_DRIVER_TYPE_REFERENCE
};
unsigned int g_dwDriveTypeSize = ARRAYSIZE(g_driveType);

D3D_FEATURE_LEVEL g_featureLevel[] =
{
    D3D_FEATURE_LEVEL_11_0,
    D3D_FEATURE_LEVEL_10_1,
    D3D_FEATURE_LEVEL_10_0
};
unsigned int g_dwFeatureLevelSize = ARRAYSIZE(g_featureLevel);

struct VertexFmt 
{
	XMFLOAT3 position;
	XMFLOAT2 uv;
};

struct VertexWithOutUV
{
	XMFLOAT3 positon;
};

struct VertexUV
{
	XMFLOAT2 uv;
};

struct ConstantBuffer
{
	XMMATRIX worldMatrix;
	XMMATRIX viewMatrix;
	XMMATRIX projMatrix;
	XMMATRIX normalMatrix;
	XMFLOAT4 eyePos;
	FLOAT	 fElapseTime;
};

struct VertexFmtWithNormal
{
	XMFLOAT3 position;
	XMFLOAT3 normal;
	//XMFLOAT2 uv;
};

struct GeometryVertexFmt
{
	XMFLOAT3 postion;
	XMFLOAT3 normal;
	XMFLOAT2 uv;
};