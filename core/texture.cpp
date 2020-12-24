#include "texture.h"

extern ID3D11Device* g_pd3dDevice;
extern ID3D11DeviceContext* g_pd3dDeviceContext;

ImTextureID CreateTexture2D(uint32 width, uint32 height)
{
    ImTextureID tex = new SInternalTexture();
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    HRESULT hr = g_pd3dDevice->CreateTexture2D(&desc, nullptr, &tex->texture);

    // Create texture view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    g_pd3dDevice->CreateShaderResourceView(tex->texture, &srvDesc, &tex->view);

    tex->size = width * height * 4;
    return tex;
}

void FreeTexture2D(ImTextureID texture)
{
    texture->view->Release();
    texture->texture->Release();
    delete texture;
}

void UpdateTexture2D(ImTextureID texture, void* data, uint32 size)
{
    assert(texture->size == size);
    void* mapped = MapTexture2D(texture);
    memcpy(mapped, data, size);
    UnmapTexture2D(texture);
}

void* MapTexture2D(ImTextureID texture)
{
    D3D11_MAPPED_SUBRESOURCE mapped;
    g_pd3dDeviceContext->Map(texture->texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    return mapped.pData;
}

void UnmapTexture2D(ImTextureID texture)
{
    g_pd3dDeviceContext->Unmap(texture->texture, 0);
}