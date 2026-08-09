#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")

#include <chrono>

#include "metro/MetroModel.h"
#include "metro/MetroSkeleton.h"
#include "metro/MetroMotion.h"
#include "metro/MetroTexture.h"
#include "metro/MetroFileSystem.h"
#include "metro/MetroTexturesDatabase.h"

#include "RenderPanel.h"

// model viewer shaders
#include "shaders/ModelViewerVS.hlsl.h"
#include "shaders/ModelViewerPS.hlsl.h"
#include "shaders/ModelViewerWireframePS.hlsl.h"

// cubemap viewer shaders
#include "shaders/CubemapViewerVS.hlsl.h"
#include "shaders/CubemapViewerPS.hlsl.h"

#include "UIHelpers.h"


static DXGI_FORMAT sPixelFormatToTextureFormat[] = {
    DXGI_FORMAT_BC1_UNORM,
    DXGI_FORMAT_BC3_UNORM,
    DXGI_FORMAT_R8G8B8A8_UNORM,
    DXGI_FORMAT_R8G8B8A8_SNORM,
    DXGI_FORMAT_BC6H_UF16,
    DXGI_FORMAT_BC7_UNORM,
    DXGI_FORMAT_R8G8_UNORM,
    DXGI_FORMAT_R8G8_SNORM,
    DXGI_FORMAT_D32_FLOAT_S8X24_UINT,       // ???
    DXGI_FORMAT_D32_FLOAT,
    DXGI_FORMAT_R32_FLOAT,
    DXGI_FORMAT_R16G16B16A16_FLOAT,
    DXGI_FORMAT_R16G16_FLOAT,
    DXGI_FORMAT_R16G16B16A16_UINT,
    DXGI_FORMAT_R8_UNORM,
    DXGI_FORMAT_R8_UINT,
    DXGI_FORMAT_R10G10B10A2_UNORM,
    DXGI_FORMAT_R10G10B10A2_UNORM,          // we don't have RGB10_SNORM_A2_UNORM on PC DirectX
    DXGI_FORMAT_R11G11B10_FLOAT,
    DXGI_FORMAT_R16_UNORM,
    DXGI_FORMAT_R32_UINT,
    DXGI_FORMAT_R32G32B32A32_FLOAT,
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_B8G8R8A8_UNORM
};


namespace MetroEX {

    RenderPanel::RenderPanel()
        : Panel()
        , mSwapChain(nullptr)
        , mDevice(nullptr)
        , mDeviceContext(nullptr)
        , mRenderTargetView(nullptr)
        , mDepthStencilBuffer(nullptr)
        , mDepthStencilState(nullptr)
        , mDepthStencilView(nullptr)
        , mRasterState(nullptr)
        , mRasterStateWireframe(nullptr)
        // model viewer
        , mModelViewerVS(nullptr)
        , mModelViewerPS(nullptr)
        , mModelViewerWireframePS(nullptr)
        , mModelInputLayout(nullptr)
        , mModelConstantBuffer(nullptr)
        , mModelGeometries(nullptr)
        , mModelTextures(nullptr)
        // model viewer stuff
        , mModel(nullptr)
        // animation
        , mAnimation(nullptr)
        , mAnimTimer(nullptr)
        // cubemap viewer stuff
        , mCamera(nullptr)
        , mCubemap(nullptr)
        , mCubemapTexture(nullptr)
        , mCubemapViewerVS(nullptr)
        , mCubemapViewerPS(nullptr)
        //
        , mLastLMPos(0.0f, 0.0f)
        , mZoom(1.0f)
        , mShowWireframe(false)
        , mShowCollision(false)
        , mConstantBufferData(nullptr)
    {
        this->components = gcnew System::ComponentModel::Container();

        mModelTextures = gcnew System::Collections::Generic::Dictionary<String^, IntPtr>(0);
        mCubemapTexture = new RenderTexture;
        mCubemapTexture->tex = nullptr;
        mCubemapTexture->srv = nullptr;

        mAnimation = new Animation;

        mAnimTimer = gcnew Timer(this->components);
        mAnimTimer->Interval = 16;
        mAnimTimer->Tick += gcnew System::EventHandler(this, &RenderPanel::AnimationTimer_Tick);
        mAnimTimer->Stop();
    }

    bool RenderPanel::InitGraphics() {
        HRESULT result;
        ID3D11Texture2D* backBuffer = nullptr;

        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        D3D11_TEXTURE2D_DESC depthBufferDesc = {};
        D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
        D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
        D3D11_RASTERIZER_DESC rasterDesc = {};
        D3D11_VIEWPORT viewport = {};

        swapChainDesc.BufferCount = 1;
        swapChainDesc.BufferDesc.Width = this->Size.Width;
        swapChainDesc.BufferDesc.Height = this->Size.Height;
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
        swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
        swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.OutputWindow = (HWND)this->Handle.ToPointer();
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Windowed = true;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
        pin_ptr<IDXGISwapChain*> swapChainPtr(&mSwapChain);
        pin_ptr<ID3D11Device*> devicePtr(&mDevice);
        pin_ptr<ID3D11DeviceContext*> contextPtr(&mDeviceContext);

        UINT deviceFlags = 0;
#ifdef _DEBUG
        deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
                                               nullptr, deviceFlags, &featureLevel, 1,
                                               D3D11_SDK_VERSION, &swapChainDesc,
                                               swapChainPtr, devicePtr, nullptr, contextPtr);
        if (FAILED(result)) {
            return false;
        }

        mCamera = new Camera();
        mCamera->SetViewport(ivec4(0, 0, this->Size.Width, this->Size.Height));
        mCamera->SetViewPlanes(0.0f, 1.0f);
        mCamera->LookAt(vec3(0.0f), vec3(0.0f, 0.0f, 1.0f));

        if (!this->CreateRenderTargets()) {
            return false;
        }

        rasterDesc.AntialiasedLineEnable = false;
        rasterDesc.CullMode = D3D11_CULL_NONE;
        rasterDesc.DepthBias = 0;
        rasterDesc.DepthBiasClamp = 0.0f;
        rasterDesc.DepthClipEnable = true;
        rasterDesc.FillMode = D3D11_FILL_SOLID;
        rasterDesc.FrontCounterClockwise = false;
        rasterDesc.MultisampleEnable = false;
        rasterDesc.ScissorEnable = false;
        rasterDesc.SlopeScaledDepthBias = 0.0f;

        pin_ptr<ID3D11RasterizerState*> rsPtr(&mRasterState);
        result = mDevice->CreateRasterizerState(&rasterDesc, rsPtr);
        if (FAILED(result)) {
            return false;
        }

        rasterDesc.DepthBias = -1000;
        rasterDesc.DepthBiasClamp = 0.001f;
        rasterDesc.SlopeScaledDepthBias = 0.01f;
        rasterDesc.FillMode = D3D11_FILL_WIREFRAME;
        pin_ptr<ID3D11RasterizerState*> rsWirePtr(&mRasterStateWireframe);
        result = mDevice->CreateRasterizerState(&rasterDesc, rsWirePtr);
        if (FAILED(result)) {
            return false;
        }

        mDeviceContext->RSSetState(mRasterState);

        depthStencilDesc.DepthEnable = true;
        depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
        depthStencilDesc.StencilEnable = false;

        pin_ptr<ID3D11DepthStencilState*> dssPtr(&mDepthStencilState);
        result = mDevice->CreateDepthStencilState(&depthStencilDesc, dssPtr);
        if (FAILED(result)) {
            return false;
        }

        mDeviceContext->OMSetDepthStencilState(mDepthStencilState, 1);

        pin_ptr<ID3D11VertexShader*> vsPtr(&mModelViewerVS);
        result = mDevice->CreateVertexShader(sModelViewerVSData, sizeof(sModelViewerVSData), nullptr, vsPtr);
        if (FAILED(result)) {
            return false;
        }

        pin_ptr<ID3D11PixelShader*> psPtr(&mModelViewerPS);
        result = mDevice->CreatePixelShader(sModelViewerPSData, sizeof(sModelViewerPSData), nullptr, psPtr);
        if (FAILED(result)) {
            return false;
        }

        pin_ptr<ID3D11PixelShader*> psWirePtr(&mModelViewerWireframePS);
        result = mDevice->CreatePixelShader(sModelViewerWireframePSData, sizeof(sModelViewerWireframePSData), nullptr, psWirePtr);
        if (FAILED(result)) {
            return false;
        }

        D3D11_INPUT_ELEMENT_DESC vsDesc[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, offsetof(MetroVertex, pos),     D3D11_INPUT_PER_VERTEX_DATA, 0},
            { "TEXCOORD", 0, DXGI_FORMAT_R8G8B8A8_UINT,      0, offsetof(MetroVertex, bones),   D3D11_INPUT_PER_VERTEX_DATA, 0},
            { "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(MetroVertex, normal),  D3D11_INPUT_PER_VERTEX_DATA, 0},
            { "TEXCOORD", 2, DXGI_FORMAT_R8G8B8A8_UNORM,     0, offsetof(MetroVertex, weights), D3D11_INPUT_PER_VERTEX_DATA, 0},
            { "TEXCOORD", 3, DXGI_FORMAT_R32G32_FLOAT,       0, offsetof(MetroVertex, uv0),     D3D11_INPUT_PER_VERTEX_DATA, 0}
        };
        pin_ptr<ID3D11InputLayout*> ilPtr(&mModelInputLayout);
        result = mDevice->CreateInputLayout(vsDesc, sizeof(vsDesc) / sizeof(vsDesc[0]), sModelViewerVSData, sizeof(sModelViewerVSData), ilPtr);
        if (FAILED(result)) {
            return false;
        }

        mConstantBufferData = new ConstantBufferData;
        mConstantBufferData->modelBSphere = vec4(1.0f);
        mConstantBufferData->matModel = MatIdentity;
        mConstantBufferData->matView = MatIdentity;
        mConstantBufferData->matProjection = MatIdentity;

        pin_ptr<ID3D11Buffer*> cbPtr(&mModelConstantBuffer);
        D3D11_BUFFER_DESC desc = {};
        D3D11_SUBRESOURCE_DATA subData = {};

        desc.ByteWidth = sizeof(ConstantBufferData);
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        subData.pSysMem = mConstantBufferData;
        result = mDevice->CreateBuffer(&desc, &subData, cbPtr);
        if (FAILED(result)) {
            return false;
        }

        D3D11_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.MaxAnisotropy = 4;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        samplerDesc.MinLOD = -(std::numeric_limits<float>::max)();
        samplerDesc.MaxLOD = (std::numeric_limits<float>::max)();

        pin_ptr<ID3D11SamplerState*> samplerStatePtr(&mModelTextureSampler);
        result = mDevice->CreateSamplerState(&samplerDesc, samplerStatePtr);
        if (FAILED(result)) {
            return false;
        }

        pin_ptr<ID3D11VertexShader*> cubeVsPtr(&mCubemapViewerVS);
        result = mDevice->CreateVertexShader(sCubemapViewerVSData, sizeof(sCubemapViewerVSData), nullptr, cubeVsPtr);
        if (FAILED(result)) {
            return false;
        }

        pin_ptr<ID3D11PixelShader*> cubePsPtr(&mCubemapViewerPS);
        result = mDevice->CreatePixelShader(sCubemapViewerPSData, sizeof(sCubemapViewerPSData), nullptr, cubePsPtr);
        if (FAILED(result)) {
            return false;
        }

        return true;
    }

    void RenderPanel::SetModel(MetroModel* model) {
        mCubemap = nullptr;

        if (mModel != model) {
            MySafeDelete(mModel);

            mModel = model;
            mCurrentMotion = nullptr;

            mCamera->SwitchMode(Camera::Mode::Arcball);

            this->ResetAnimation();
            this->CreateModelGeometries();
            this->CreateTextures();
            this->ResetCamera(false);
            this->Render();
        }
    }

    void RenderPanel::SetLod(const int lodId) {
        if (mModel && lodId >= 0 && lodId <= 2) {
            MetroModel* model = mModel;

            if (lodId == 1 && model->HasLodModel(0))
                mModel = model->GetLodModel(0);
            else if (lodId == 2 && model->HasLodModel(1))
                mModel = model->GetLodModel(1);
            mCurrentMotion = nullptr;

            mCamera->SwitchMode(Camera::Mode::Arcball);

            this->ResetAnimation();
            this->CreateModelGeometries();
            this->CreateTextures();
            this->Render();

            mModel = model;
        }
    }

    MetroModel* RenderPanel::GetModel() {
        return mModel;
    }

    void RenderPanel::SetCubemap(MetroTexture* cubemap) {
        MySafeDelete(mModel);

        mCubemap = cubemap;

        this->CreateModelGeometries();
        this->CreateTextures();
        this->Render();
    }

    void RenderPanel::SetShowWireframe(const bool wireframe) {
        if (mShowWireframe != wireframe) {
            mShowWireframe = wireframe;
            this->Render();
        }
    }

    void RenderPanel::SetShowCollision(const bool collision) {
        if (mShowCollision != collision) {
            mShowCollision = collision;
            this->Render();
        }
    }

    void RenderPanel::SwitchMotion(const size_t idx) {
        if (mModel && mModel->IsAnimated()) {
            mCurrentMotion = mModel->GetMotion(idx);
            mAnimation->time = 0.0f;
        }
    }

    bool RenderPanel::IsPlayingAnim() {
        return mAnimTimer->Enabled;
    }

    void RenderPanel::PlayAnim(const bool play) {
        if (play && mModel && mModel->IsAnimated() && mCurrentMotion) {
            this->ResetAnimation();
            mAnimTimer->Start();
        } else {
            mAnimTimer->Stop();
        }
    }

    void RenderPanel::ResetCamera(const bool forceRender) {
        mZoom = 1.5f;

        const float r = mConstantBufferData->modelBSphere.w;
        const float nearZ = r * 0.05f;
        const float farZ = r * 100.0f;

        mCamera->LookAt(vec3(r * 0.3f, r, r), vec3(0.0f));

        this->UpdateCamera();
        mCamera->SetViewPlanes(nearZ, farZ);

        mConstantBufferData->matModel = MatIdentity;

        if (forceRender) {
            this->Render();
        }
    }


    bool RenderPanel::CreateRenderTargets() {
        if (!mDevice || !mDeviceContext || !mSwapChain) {
            return false;
        }

        MySafeRelease(mRenderTargetView);
        MySafeRelease(mDepthStencilBuffer);
        MySafeRelease(mDepthStencilView);

        ID3D11Texture2D* backBuffer = nullptr;
        pin_ptr<ID3D11Texture2D*> backBufferPtr(&backBuffer);
        HRESULT hr = mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backBufferPtr);
        if (FAILED(hr)) {
            return false;
        }
        pin_ptr<ID3D11RenderTargetView*> rtvPtr(&mRenderTargetView);
        hr = mDevice->CreateRenderTargetView(backBuffer, nullptr, rtvPtr);
        backBuffer->Release();

        D3D11_TEXTURE2D_DESC depthBufferDesc = {};
        D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
        D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};

        depthBufferDesc.Width = this->Size.Width;
        depthBufferDesc.Height = this->Size.Height;
        depthBufferDesc.MipLevels = 1;
        depthBufferDesc.ArraySize = 1;
        depthBufferDesc.Format = DXGI_FORMAT_D32_FLOAT;
        depthBufferDesc.SampleDesc.Count = 1;
        depthBufferDesc.SampleDesc.Quality = 0;
        depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;


        pin_ptr<ID3D11Texture2D*> dsbPtr(&mDepthStencilBuffer);
        hr = mDevice->CreateTexture2D(&depthBufferDesc, nullptr, dsbPtr);
        if (FAILED(hr)) {
            return false;
        }

        depthStencilViewDesc.Format = depthBufferDesc.Format;
        depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

        pin_ptr<ID3D11DepthStencilView*> dsvPtr(&mDepthStencilView);
        hr = mDevice->CreateDepthStencilView(mDepthStencilBuffer, &depthStencilViewDesc, dsvPtr);
        if (FAILED(hr)) {
            return false;
        }

        mDeviceContext->OMSetRenderTargets(1, rtvPtr, mDepthStencilView);

        D3D11_VIEWPORT viewport = {};
        viewport.Width = scast<float>(this->Size.Width);
        viewport.Height = scast<float>(this->Size.Height);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;

        mDeviceContext->RSSetViewports(1, &viewport);

        mCamera->SetViewport(ivec4(0, 0, this->Size.Width, this->Size.Height));

        return true;
    }

    void RenderPanel::UpdateCamera() {
        if (mModel) {
            const float r = mConstantBufferData->modelBSphere.w * mZoom;
            vec3 invDir = -mCamera->GetDirection();
            mCamera->InitArcball(invDir * r, vec3(0.0f));
        }
    }

    void RenderPanel::CreateModelGeometries() {
        if (!mDevice) {
            return;
        }

        if (mModelGeometries) {
            for each (RenderGeometry* rg in mModelGeometries) {
                if (rg) {
                    MySafeRelease(rg->vb);
                    MySafeRelease(rg->ib);
                    delete rg;
                }
            }

            MySafeDelete(mModelGeometries);
        }

        if (!mModel) {
            return;
        }

        const size_t numMeshes = mModel->GetNumMeshes();
        mModelGeometries = gcnew array<RenderGeometry*>(scast<int>(numMeshes));

        mConstantBufferData->modelBSphere = vec4(0.0f, 0.0f, 0.0f, 1.0f);

        AABBox modelBBox;
        modelBBox.Reset();
        for (size_t i = 0; i < numMeshes; ++i) {
            const MetroMesh* mesh = mModel->GetMesh(i);
            if (!mesh->vertices.empty() && !mesh->faces.empty()) {
                RenderGeometry* rg = new RenderGeometry;

                AABBox bbox;
                bbox.Reset();

                rg->texture = nullptr;
                rg->numFaces = mesh->faces.size();
                rg->isCollision = mesh->isCollision;

                D3D11_BUFFER_DESC desc = {};
                D3D11_SUBRESOURCE_DATA subData = {};

                //vb
                desc.ByteWidth = scast<UINT>(mesh->vertices.size() * sizeof(MetroVertex));
                desc.Usage = D3D11_USAGE_DEFAULT;
                desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
                subData.pSysMem = mesh->vertices.data();
                mDevice->CreateBuffer(&desc, &subData, &rg->vb);

                //ib
                desc.ByteWidth = scast<UINT>(mesh->faces.size() * sizeof(MetroFace));
                desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
                subData.pSysMem = mesh->faces.data();
                mDevice->CreateBuffer(&desc, &subData, &rg->ib);

                for (const MetroVertex& v : mesh->vertices) {
                    bbox.Absorb(v.pos);
                }

                modelBBox.Absorb(bbox.minimum);
                modelBBox.Absorb(bbox.maximum);

                mConstantBufferData->modelBSphere = vec4(modelBBox.Center(), Length(modelBBox.Extent()));

                mModelGeometries[scast<int>(i)] = rg;
            } else {
                mModelGeometries[scast<int>(i)] = nullptr;
            }
        }
    }

    void RenderPanel::CreateTextures() {
        for each (IntPtr ptr in mModelTextures->Values) {
            RenderTexture* rt = rcast<RenderTexture*>(ptr.ToPointer());
            MySafeRelease(rt->srv);
            MySafeRelease(rt->tex);
        }
        mModelTextures->Clear();

        MySafeRelease(mCubemapTexture->srv);
        MySafeRelease(mCubemapTexture->tex);

        if (mCubemap) {
            this->CreateRenderTexture(mCubemap, mCubemapTexture);
            return;
        } else if (!mModel) {
            return;
        }

        const MetroFileSystem& mfs = MetroFileSystem::Get();

        const size_t numMeshes = mModel->GetNumMeshes();
        for (size_t i = 0; i < numMeshes; ++i) {
            const MetroMesh* mesh = mModel->GetMesh(i);
            RenderGeometry* rg = mModelGeometries[scast<int>(i)];
            if (rg) {
                const CharString& textureName = mesh->materials.front();
                const CharString sourceName = MetroTexturesDatabase::Get().GetSourceName(textureName);

                String^ texNameManaged = ToNetString(textureName);

                const bool contains = mModelTextures->ContainsKey(texNameManaged);
                if (!contains) {
                    const MetroTextureInfo* mti = MetroTexturesDatabase::Get().GetInfoByName(textureName);

                    CharString texturePath = CharString(R"(content\textures\)") + (sourceName.empty() ? textureName : sourceName);

                    CharString texturePathBest = (mti == nullptr) ? texturePath + ".512" : (texturePath + '.' + std::to_string(mti->width));
                    MyHandle textureFile = mfs.FindFile(texturePathBest);

                    if (textureFile == kInvalidHandle) {
                        texturePathBest += "c";
                        textureFile = mfs.FindFile(texturePathBest);
                    }

                    if (textureFile == kInvalidHandle) {
                        texturePathBest = texturePath + ".bin";
                        textureFile = mfs.FindFile(texturePathBest);
                    }

                    if (textureFile != kInvalidHandle) {
                        MemStream stream = mfs.OpenFileStream(textureFile);
                        if (stream) {
                            MetroTexture texture;
                            if (texture.LoadFromData(stream, textureFile)) {
                                RenderTexture* rt = new RenderTexture;
                                this->CreateRenderTexture(&texture, rt);
                                mModelTextures->Add(texNameManaged, IntPtr(rt));

                                rg->texture = rt;
                            }
                        }
                    }
                } else {
                    rg->texture = rcast<RenderTexture*>(mModelTextures[texNameManaged].ToPointer());
                }
            }
        }
    }

    void RenderPanel::CreateRenderTexture(const MetroTexture* srcTexture, RenderTexture* rt) {
        D3D11_TEXTURE2D_DESC desc = {};

        const DXGI_FORMAT textureFormat = sPixelFormatToTextureFormat[scast<uint32_t>(srcTexture->GetFormat())];

        desc.Width = scast<UINT>(srcTexture->GetWidth());
        desc.Height = scast<UINT>(srcTexture->GetHeight());
        desc.MipLevels = scast<UINT>(srcTexture->GetNumMips());
        desc.ArraySize = srcTexture->IsCubemap() ? 6 : 1;
        desc.Format = textureFormat;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        if (srcTexture->IsCubemap()) {
            desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
        }

        MyArray<D3D11_SUBRESOURCE_DATA> subDesc(desc.ArraySize * desc.MipLevels);

        const UINT blockSize = textureFormat == DXGI_FORMAT_BC1_UNORM ? 8 : 16;

        size_t counter = 0;
        const uint8_t* dataPtr = srcTexture->GetRawData();
        for (size_t i = 0; i < desc.ArraySize; ++i) {
            UINT mipWidth = desc.Width;
            UINT mipHeight = desc.Height;
            for (size_t j = 0; j < desc.MipLevels; ++j, ++counter) {
                const UINT numBlocksW = (mipWidth + 3) / 4;
                const UINT numBlocksH = (mipHeight + 3) / 4;

                subDesc[counter].pSysMem = dataPtr;
                subDesc[counter].SysMemPitch = numBlocksW * blockSize;
                subDesc[counter].SysMemSlicePitch = subDesc[counter].SysMemPitch * numBlocksH;

                dataPtr += subDesc[counter].SysMemSlicePitch;

                mipWidth >>= 1;
                mipHeight >>= 1;
            }
        }

        pin_ptr<ID3D11Texture2D*> texPtr(&rt->tex);
        HRESULT hr = mDevice->CreateTexture2D(&desc, subDesc.data(), texPtr);
        if (SUCCEEDED(hr)) {
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = textureFormat;
            srvDesc.ViewDimension = srcTexture->IsCubemap() ? D3D11_SRV_DIMENSION_TEXTURECUBE : D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = ~0u;
            srvDesc.Texture2D.MostDetailedMip = 0;

            pin_ptr<ID3D11ShaderResourceView*> srvPtr(&rt->srv);
            hr = mDevice->CreateShaderResourceView(rt->tex, &srvDesc, srvPtr);
            if (FAILED(hr)) {
                MySafeRelease(rt->tex);
            }
        }
    }


    struct HierarchyBone {
        AnimBone                srcBone;
        MyArray<HierarchyBone*> children;
    };

    static void FlattenHierarchyToArray(AnimBone*& arr, const HierarchyBone* hb) {
        // parents go first
        for (const HierarchyBone* next : hb->children) {
            *arr = next->srcBone;
            ++arr;
        }

        // children go next
        for (const HierarchyBone* next : hb->children) {
            FlattenHierarchyToArray(arr, next);
        }
    }

    void RenderPanel::ResetAnimation() {
        mAnimTimer->Stop();

        for (auto& b : mConstantBufferData->bones) {
            b = MatIdentity;
        }

        mAnimation->time = 0.0f;
        if (mModel && mModel->IsAnimated()) {
            const MetroSkeleton* skeleton = mModel->GetSkeleton();
            const size_t numBones = skeleton->GetNumBones();

            MyArray<HierarchyBone> hierarchy(numBones);

            size_t rootBoneIdx = 0;
            for (size_t i = 0; i < numBones; ++i) {
                AnimBone& b = mAnimation->bones[i];
                b.idx = i;
                b.parentIdx = skeleton->GetBoneParentIdx(i);

                hierarchy[b.idx].srcBone = b;
                if (b.parentIdx != MetroBone::InvalidIdx) {
                    hierarchy[b.parentIdx].children.push_back(&hierarchy[b.idx]);
                } else {
                    rootBoneIdx = i;
                }

                mAnimation->bindPoseInv[i] = MatInverse(skeleton->GetBoneFullTransform(i));
            }

            // now we flatten our hierarchy so that parent bones are always come befor their children
            mAnimation->bones[0] = mAnimation->bones[rootBoneIdx];
            AnimBone* arr = &mAnimation->bones[1];
            FlattenHierarchyToArray(arr, &hierarchy[rootBoneIdx]);
        }
    }

    void RenderPanel::UpdateAnimation(const float dt) {
        if (mModel && mModel->IsAnimated() && mCurrentMotion) {
            const MetroSkeleton* skeleton = mModel->GetSkeleton();

            const size_t numBones = std::min<size_t>(mCurrentMotion->GetNumBones(), skeleton->GetNumBones());
            const float animLen = mCurrentMotion->GetMotionTimeInSeconds();

            if (animLen <= 0.0f) {
                return;
            }

            if (mAnimation->time >= animLen) {
                mAnimation->time -= animLen;
            }

            for (size_t i = 0; i < numBones; ++i) {
                const AnimBone& b = mAnimation->bones[i];

                quat q; vec3 t;
                if (mCurrentMotion->IsBoneAnimated(b.idx)) {
                    q = mCurrentMotion->GetBoneRotationAtTime(b.idx, mAnimation->time);
                    t = mCurrentMotion->GetBonePositionAtTime(b.idx, mAnimation->time);
                } else {
                    q = skeleton->GetBoneRotation(b.idx);
                    t = skeleton->GetBonePosition(b.idx);
                }

                mat4& m = mConstantBufferData->bones[b.idx];

                m = MatFromQuat(q);
                m[3] = vec4(t, 1.0f);

                if (b.parentIdx != MetroBone::InvalidIdx) {
                    m = mConstantBufferData->bones[b.parentIdx] * m;
                }
            }

            for (size_t i = 0; i < numBones; ++i) {
                mat4& m = mConstantBufferData->bones[i];
                m = m * mAnimation->bindPoseInv[i];
            }

            mAnimation->time += dt;

            this->Render();
        }
    }

    void RenderPanel::Render() {
        static const float clearColor[4] = { 40.0f / 255.0f, 113.0f / 255.0f, 134.0f / 255.0f, 1.0f };

        if (mDeviceContext && mSwapChain) {
            mDeviceContext->ClearRenderTargetView(mRenderTargetView, clearColor);
            mDeviceContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

            mat4 view = mCamera->GetTransform();
            mat4 projMat = mCamera->GetProjection();
            mConstantBufferData->matView = view;
            mConstantBufferData->matProjection = projMat;
            mConstantBufferData->matModelView = view * mConstantBufferData->matModel;
            mConstantBufferData->matModelViewProj = projMat * mConstantBufferData->matModelView;
            mConstantBufferData->camParams.x = Deg2Rad(mCamera->GetFovY());
            mConstantBufferData->camParams.y = mCamera->GetAspect();

            D3D11_MAPPED_SUBRESOURCE subRes = {};
            mDeviceContext->Map(mModelConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subRes);
            memcpy(subRes.pData, mConstantBufferData, sizeof(ConstantBufferData));
            mDeviceContext->Unmap(mModelConstantBuffer, 0);

            pin_ptr<ID3D11Buffer*> cbPtr(&mModelConstantBuffer);
            mDeviceContext->VSSetConstantBuffers(0, 1, cbPtr);
            mDeviceContext->PSSetConstantBuffers(0, 1, cbPtr);

            mDeviceContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            pin_ptr<ID3D11SamplerState*> samplerPtr(&mModelTextureSampler);
            mDeviceContext->PSSetSamplers(0, 1, samplerPtr);

            if (mModel) {
                mDeviceContext->IASetInputLayout(mModelInputLayout);
                mDeviceContext->VSSetShader(mModelViewerVS, nullptr, 0);

                ID3D11PixelShader* pixelShaders[2] = { mModelViewerPS, mModelViewerWireframePS };
                ID3D11RasterizerState* rsStates[2] = { mRasterState, mRasterStateWireframe };

                const size_t numPasses = 1 + scast<size_t>(mShowWireframe);

                for (size_t pass = 0; pass < numPasses; ++pass) {
                    mDeviceContext->RSSetState(rsStates[pass]);
                    mDeviceContext->PSSetShader(pixelShaders[pass], nullptr, 0);

                    if (mModelGeometries) {
                        for each (RenderGeometry* rg in mModelGeometries) {
                            if (!rg || (!mShowCollision && rg->isCollision)) {
                                continue;
                            }

                            ID3D11ShaderResourceView* texSRV = (rg->texture) ? rg->texture->srv : nullptr;
                            pin_ptr<ID3D11ShaderResourceView*> srvPtr(&texSRV);
                            mDeviceContext->PSSetShaderResources(0, 1, srvPtr);

                            const UINT stride = sizeof(MetroVertex);
                            const UINT offset = 0;
                            mDeviceContext->IASetVertexBuffers(0, 1, &rg->vb, &stride, &offset);
                            mDeviceContext->IASetIndexBuffer(rg->ib, DXGI_FORMAT_R16_UINT, 0);

                            mDeviceContext->DrawIndexed(scast<UINT>(rg->numFaces * 3), 0, 0);
                        }
                    }
                }
            } else if (mCubemap) {
                mDeviceContext->RSSetState(mRasterState);

                mDeviceContext->IASetInputLayout(nullptr);
                mDeviceContext->VSSetShader(mCubemapViewerVS, nullptr, 0);
                mDeviceContext->PSSetShader(mCubemapViewerPS, nullptr, 0);

                pin_ptr<ID3D11ShaderResourceView*> srvPtr(&mCubemapTexture->srv);
                mDeviceContext->PSSetShaderResources(0, 1, srvPtr);

                mDeviceContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
                mDeviceContext->IASetIndexBuffer(nullptr, scast<DXGI_FORMAT>(0), 0);

                mDeviceContext->Draw(3, 0);
            }

            mSwapChain->Present(0, 0);
        }
    }

    void RenderPanel::OnResize(System::EventArgs^ e) {
        Panel::OnResize(e);

        if (mSwapChain) {
            mRenderTargetView->Release();
            mRenderTargetView = nullptr;

            const int w = this->Size.Width;
            const int h = this->Size.Height;

            HRESULT hr = mSwapChain->ResizeBuffers(1, w, h, DXGI_FORMAT_UNKNOWN, 0);
            if (SUCCEEDED(hr)) {
                this->CreateRenderTargets();
            }
        }
    }

    void RenderPanel::OnPaint(System::Windows::Forms::PaintEventArgs^ e) {
        this->Render();
    }

    void RenderPanel::OnMouseDown(System::Windows::Forms::MouseEventArgs^ e) {
        PointF mp(scast<float>(e->X), scast<float>(e->Y));

        if (e->Button == System::Windows::Forms::MouseButtons::Left) {
            mLastLMPos = mp;
        } else if (e->Button == System::Windows::Forms::MouseButtons::Right) {
            mLastRMPos = mp;
        }

        Panel::OnMouseDown(e);
    }

    void RenderPanel::OnMouseMove(System::Windows::Forms::MouseEventArgs^ e) {
        static const float kCameraRotateSpeed = 0.2f;
        static const float kModelMoveSpeed = 0.01f;

        PointF mp(scast<float>(e->X), scast<float>(e->Y));

        if (e->Button == System::Windows::Forms::MouseButtons::Left) {
            PointF delta = PointF(mp.X - mLastLMPos.X, mp.Y - mLastLMPos.Y);
            mLastLMPos = mp;

            mCamera->Rotate(-delta.X * kCameraRotateSpeed, -delta.Y * kCameraRotateSpeed);

            this->Render();
        } else if (e->Button == System::Windows::Forms::MouseButtons::Right) {
            PointF delta = PointF(mp.X - mLastRMPos.X, mp.Y - mLastRMPos.Y);
            mLastRMPos = mp;

            if (mModel) {
                vec3 tx = mCamera->GetSide() * delta.X * kModelMoveSpeed;
                vec3 ty = mCamera->GetUp() * -delta.Y * kModelMoveSpeed;
                mConstantBufferData->matModel = MatTranslate(mConstantBufferData->matModel, tx + ty);

                this->Render();
            }
        }

        Panel::OnMouseMove(e);
    }

    void RenderPanel::OnMouseWheel(System::Windows::Forms::MouseEventArgs^ e) {
        if (e->Delta > 0) {
            mZoom = std::min<float>(mZoom + 0.1f, 5.0f);
        } else if (e->Delta < 0) {
            mZoom = std::max<float>(mZoom - 0.1f, 0.1f);
        }

        this->UpdateCamera();
        this->Render();
    }

    void RenderPanel::AnimationTimer_Tick(System::Object^, System::EventArgs^) {
        static double sLastTimeMS = -1.0;

        std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
        std::chrono::system_clock::time_point::duration epoch = tp.time_since_epoch();

        const double currentTimeMS = scast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count());

        if (sLastTimeMS < 0.0) {
            sLastTimeMS = currentTimeMS;
        }

        const double dtSeconds = (currentTimeMS - sLastTimeMS) * 0.001;
        sLastTimeMS = currentTimeMS;

        this->UpdateAnimation(scast<float>(dtSeconds));
    }
}
