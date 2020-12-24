#pragma once
#include <stdio.h>
#include <d3d11.h>
#include "types.h"

enum ESamplerType
{
	SAMPLER_FONT = 0,
	SAMPLER_LINEAR,
	SAMPLER_NEAREST,
	SAMPLER_SIZE
};

struct SInternalTexture
{
	ID3D11Texture2D* texture = nullptr;
	ID3D11ShaderResourceView* view = nullptr;
	uint32 size = 0;
	ESamplerType sampler = SAMPLER_NEAREST;
};

#define ImTextureID SInternalTexture*

ImTextureID CreateTexture2D(uint32 width, uint32 height);
void FreeTexture2D(ImTextureID texture);
void UpdateTexture2D(ImTextureID texture, void* data, uint32 size);

void* MapTexture2D(ImTextureID texture);
void UnmapTexture2D(ImTextureID texture);