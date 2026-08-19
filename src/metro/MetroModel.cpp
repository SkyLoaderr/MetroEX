#include "MetroModel.h"
#include "MetroFileSystem.h"
#include "MetroTexturesDatabase.h"
#include "MetroSkeleton.h"
#include "MetroMotion.h"

#define FBXSDK_NEW_API
#define FBXSDK_SHARED
#include "fbxsdk.h"

#pragma comment (lib, "libfbxsdk.lib")

#include <fstream>
#include <sstream>
#include <functional>

enum ModelChunks {
    MC_HeaderChunk          = 0x00000001,
    MC_MaterialsChunk       = 0x00000002,
    MC_VerticesChunk        = 0x00000003,
    MC_FacesChunk           = 0x00000004,
    MC_SkinnedVerticesChunk = 0x00000005,

    MC_SubMeshesChunk       = 0x00000009,

    MC_Lod_1_Chunk          = 0x0000000B,   // 11
    MC_Lod_2_Chunk          = 0x0000000C,   // 12

    MC_MeshesInline         = 0x0000000F,   // 15
    MC_MeshesLinks          = 0x00000010,   // 16

    MC_SkeletonLink         = 0x00000014,   // 20
    MC_SkeletonInline       = 0x00000018,   // 24

    MC_Comment              = 0x00000024,   // 36
};

enum MeshesChunks {
    MC_Lod_0_MeshChunk      = 0x00000000,
    MC_Lod_1_MeshChunk      = 0x00000001,
    MC_Lod_2_MeshChunk      = 0x00000002,
};


static const size_t kModelVersionRedux      = 22;
static const size_t kModelVersionArktika1   = 32;
static const size_t kModelVersionExodus     = 42;

static const size_t kMetroModelMaxMaterials = 4;

PACKED_STRUCT_BEGIN
struct MdlHeader {          // size = 64
    enum : uint32_t {
        Flag_ModelIsDraft = 1
    };

    uint8_t     version;
    uint8_t     type;
    uint16_t    shaderId;
    AABBox      bbox;
    vec4        bsphere;
    uint32_t    checkSum;
    float       invLod;
    uint32_t    flags;
    float       vscale;
    float       texelDensity;
} PACKED_STRUCT_END;

PACKED_STRUCT_BEGIN
struct MetroOBB {           // size = 60
    mat3    matrix;
    vec3    offset;
    vec3    hsize;
} PACKED_STRUCT_END;



MetroModel::MetroModel()
    : mVersion(0)
    , mHeaderRead(false)
    , mBSphere(0.0f)
    , mSkeleton(nullptr)
    , mCurrentMesh(nullptr)
    , mThisFileIdx(MetroFile::InvalidFileIdx)
{
    mBBox.Reset();

    for (size_t i = 0; i < kMetroModelMaxLods; ++i) {
        mLodModels[i] = nullptr;
    }
}
MetroModel::~MetroModel() {
    std::for_each(mMeshes.begin(), mMeshes.end(), [](MetroMesh* mesh) { delete mesh; });
    std::for_each(mMotions.begin(), mMotions.end(), [](MetroModel::MotionInfo& mi) { MySafeDelete(mi.motion); });
    MySafeDelete(mSkeleton);
    for (size_t i = 0; i < kMetroModelMaxLods; ++i) {
        MySafeDelete(mLodModels[i]);
    }
}

bool MetroModel::LoadFromData(MemStream& stream, const size_t fileIdx) {
    bool result = false;

    mThisFileIdx = fileIdx;

    this->ReadSubChunks(stream);

    for (size_t i = 0; i < kMetroModelMaxLods; ++i) {
        if (mLodModels[i] != nullptr && mLodModels[i]->mMeshes.empty()) {
            MySafeDelete(mLodModels[i]);
        }
    }

    if (mVersion >= kModelVersionArktika1) {
        this->LoadMotions();
    }

    result = !mMeshes.empty();

    return result;
}

bool MetroModel::SaveAsOBJ(const fs::path& filePath, const bool excludeCollision) {
    bool result = false;

    std::ofstream file(filePath, std::ofstream::binary);
    if (file.good()) {
        CharString matName = filePath.filename().string();
        matName[matName.size() - 3] = 'm';
        matName[matName.size() - 2] = 't';
        matName[matName.size() - 1] = 'l';

        std::ostringstream stringBuilder;
        stringBuilder << "# Generated from Metro Exodus model file" << std::endl;
        stringBuilder << "# using MetroEX tool made by iOrange, 2019" << std::endl << std::endl;
        stringBuilder << "mtllib " << matName << std::endl << std::endl;

        size_t lastIdx = 0;
        for (size_t i = 0; i < mMeshes.size(); ++i) {
            const MetroMesh* mesh = mMeshes[i];

            // empty mesh ???
            if (mesh->vertices.empty() || mesh->faces.empty()) {
                continue;
            }

            // skip collision geometry if asked so
            if (excludeCollision && mesh->isCollision) {
                continue;
            }

            for (const MetroVertex& v : mesh->vertices) {
                stringBuilder << "v " << v.pos.x << ' ' << v.pos.y << ' ' << v.pos.z << std::endl;
            }
            stringBuilder << "# " << mesh->vertices.size() << " vertices" << std::endl << std::endl;

            for (const MetroVertex& v : mesh->vertices) {
                stringBuilder << "vt " << v.uv0.x << ' ' << (1.0f - v.uv0.y) << std::endl;
            }
            stringBuilder << "# " << mesh->vertices.size() << " texcoords" << std::endl << std::endl;

            for (const MetroVertex& v : mesh->vertices) {
                stringBuilder << "vn " << v.normal.x << ' ' << v.normal.y << ' ' << v.normal.z << std::endl;
            }
            stringBuilder << "# " << mesh->vertices.size() << " normals" << std::endl << std::endl;

            stringBuilder << "g Mesh_" << i << std::endl;
            stringBuilder << "usemtl " << "Material_" << i << std::endl;

            for (const MetroFace& f : mesh->faces) {
                const size_t a = f.c + lastIdx + 1;
                const size_t b = f.b + lastIdx + 1;
                const size_t c = f.a + lastIdx + 1;

                stringBuilder << "f " << a << '/' << a << '/' << a <<
                                    ' ' << b << '/' << b << '/' << b <<
                                    ' ' << c << '/' << c << '/' << c << std::endl;
            }
            stringBuilder << "# " << mesh->faces.size() << " faces" << std::endl << std::endl;

            lastIdx += mesh->vertices.size();
        }

        const CharString& str = stringBuilder.str();
        file.write(str.c_str(), str.length());
        file.flush();

        fs::path modelFolder = filePath.parent_path();

        CharString matPath = filePath.string();
        matPath[matPath.size() - 3] = 'm';
        matPath[matPath.size() - 2] = 't';
        matPath[matPath.size() - 1] = 'l';

        std::ofstream mtlFile(matPath, std::ofstream::binary);
        if (mtlFile.good()) {
            std::ostringstream mtlBuilder;
            mtlBuilder << "# Generated from Metro Exodus model file" << std::endl;
            mtlBuilder << "# using MetroEX tool made by iOrange, 2019" << std::endl << std::endl;

            for (size_t i = 0; i < mMeshes.size(); ++i) {
                const MetroMesh* mesh = mMeshes[i];

                // empty mesh ???
                if (mesh->vertices.empty() || mesh->faces.empty()) {
                    continue;
                }

                // skip collision geometry if asked so
                if (excludeCollision && mesh->isCollision) {
                    continue;
                }

                mtlBuilder << "newmtl " << "Material_" << i << std::endl;

                const CharString& textureName = mesh->materials.front();

                const CharString& sourceName = MetroTexturesDatabase::Get().GetSourceName(textureName);
                const CharString& bumpName = MetroTexturesDatabase::Get().GetSourceName(textureName);

                CharString textureTgaName = fs::path(sourceName).filename().string() + ".tga";

                mtlBuilder << "Kd 1 1 1" << std::endl;
                mtlBuilder << "Ke 0 0 0" << std::endl;
                mtlBuilder << "Ns 1000" << std::endl;
                mtlBuilder << "illum 2" << std::endl;
                mtlBuilder << "map_Ka " << textureTgaName << std::endl;
                mtlBuilder << "map_Kd " << textureTgaName << std::endl;

                if (!bumpName.empty()) {
                    CharString bumpTgaName = fs::path(bumpName).filename().string() + "_nm.tga";
                    mtlBuilder << "bump " << bumpTgaName << std::endl;
                    mtlBuilder << "map_bump " << bumpTgaName << std::endl;
                }

                mtlBuilder << std::endl;
            }

            const CharString& mtlStr = mtlBuilder.str();
            mtlFile.write(mtlStr.c_str(), mtlStr.length());
            mtlFile.flush();
        }

        result = true;
    }

    return result;
}

struct ClusterInfo {
    MyArray<int>      vertexIdxs;
    MyArray<float>    weigths;
};
void CollectClusters(const MetroMesh* mesh, const MetroSkeleton* skeleton, MyArray<ClusterInfo>& clusters) {
    const size_t numBones = skeleton->GetNumBones();
    clusters.resize(numBones);

    for (size_t i = 0; i < numBones; ++i) {
        ClusterInfo& cluster = clusters[i];

        for (size_t j = 0; j < mesh->vertices.size(); ++j) {
            const MetroVertex& v = mesh->vertices[j];

            for (size_t k = 0; k < 4; ++k) {
                const size_t boneIdx = v.bones[k];
                if (boneIdx == i && v.weights[k]) {
                    cluster.vertexIdxs.push_back(scast<int>(j));
                    cluster.weigths.push_back(scast<float>(v.weights[k]) * (1.0f / 255.0f));
                }
            }
        }
    }
}


static FbxVector4 MetroVecToFbxVec(const vec3& v) {
    return FbxVector4(v.x, v.y, v.z);
}

static FbxVector4 MetroRotToFbxRot(const quat& q) {
    vec3 euler = QuatToEuler(q);
    return FbxVector4(Rad2Deg(euler.x), Rad2Deg(euler.y), Rad2Deg(euler.z));
}


static FbxNode* CreateFBXSkeleton(FbxScene* scene, const MetroSkeleton* skeleton, MyArray<FbxNode*>& boneNodes) {
    const size_t numBones = skeleton->GetNumBones();
    boneNodes.reserve(numBones);

    for (size_t i = 0; i < numBones; ++i) {
        const size_t parentIdx = skeleton->GetBoneParentIdx(i);
        const CharString& name = skeleton->GetBoneName(i);

        FbxSkeleton* attribute = FbxSkeleton::Create(scene, name.c_str());
        if (MetroBone::InvalidIdx == parentIdx) {
            attribute->SetSkeletonType(FbxSkeleton::eRoot);
        } else {
            attribute->SetSkeletonType(FbxSkeleton::eLimbNode);
        }

        FbxNode* node = FbxNode::Create(scene, name.c_str());
        node->SetNodeAttribute(attribute);

        boneNodes.push_back(node);
    }

    FbxNode* rootNode = nullptr;
    for (size_t i = 0; i < numBones; ++i) {
        FbxNode* node = boneNodes[i];
        const size_t parentIdx = skeleton->GetBoneParentIdx(i);

        const quat& bindQ = skeleton->GetBoneRotation(i);
        const vec3& bindT = skeleton->GetBonePosition(i);

        node->LclTranslation.Set(MetroVecToFbxVec(bindT));
        node->LclRotation.Set(MetroRotToFbxRot(bindQ));

        if (MetroBone::InvalidIdx != parentIdx) {
            boneNodes[parentIdx]->AddChild(node);
        } else {
            rootNode = node;
        }
    }

    return rootNode;
}

// The curve code doesn't differentiate between angles and other data, so an interpolation from 179 to -179
// will cause the bone to rotate all the way around through 0 degrees.  So here we make a second pass over the
// rotation tracks to convert the angles into a more interpolation-friendly format.
static void CorrectAnimTrackInterpolation(MyArray<FbxNode*>& boneNodes, FbxAnimLayer* animLayer) {
    for (FbxNode* bone : boneNodes) {
        FbxAnimCurveNode* rotCurveNode = bone->LclRotation.GetCurveNode(animLayer);
        if (rotCurveNode) {
            //#NOTE_SK: just because fucking FBX doesn't allow us to use quaternions for rotations
            //          we'll get angles "clicking" issues
            //          this is the only way I found to fight this issue
            FbxAnimCurveFilterUnroll unrollFilter;
            unrollFilter.SetForceAutoTangents(true);
            unrollFilter.Apply(*rotCurveNode);
        }
    }
}

static void AddAnimTrackToScene(FbxScene* scene, MetroModel* model, const MetroMotion* motion, const CharString& animName, MyArray<FbxNode*>& skelNodes) {
    FbxAnimStack* animStack = FbxAnimStack::Create(scene, animName.c_str());
    FbxAnimLayer* animLayer = FbxAnimLayer::Create(scene->GetFbxManager(), "Base_Layer");
    animStack->AddMember(animLayer);

    const double animFPS = 30.0f;

    FbxTime startTime, stopTime;
    startTime.SetGlobalTimeMode(FbxTime::eFrames30);
    stopTime.SetGlobalTimeMode(FbxTime::eFrames30);

    startTime.SetSecondDouble(0.0);

    // kinda hack to get animation duration
    const double animDuration = scast<double>(motion->GetMotionTimeInSeconds());
    stopTime.SetSecondDouble(animDuration);

    FbxTimeSpan animTimeSpan;
    animTimeSpan.Set(startTime, stopTime);
    animStack->SetLocalTimeSpan(animTimeSpan);


    FbxTime keyTime;
    int keyIndex;

    const size_t numFrames = std::max<size_t>(motion->GetNumFrames(), 1);

    MyArray<MyArray<quat>> poseQ(numFrames);
    MyArray<MyArray<vec3>> poseT(numFrames);
    for (size_t f = 0; f < numFrames; ++f) {
        model->CalcPose(motion, scast<float>(f) / scast<float>(animFPS), poseQ[f], poseT[f]);
    }

    MyArray<bool> boneHasTrack(skelNodes.size(), false);
    for (size_t i = 0; i < skelNodes.size(); ++i) {
        boneHasTrack[i] = motion->IsBoneAnimated(i);
    }
    if (model->GetSkeleton() && MetroModel::GetApplyProceduralBones()) {
        const MetroSkeleton* skel = model->GetSkeleton();
        for (size_t i = 0; i < skel->GetNumDrivenBones(); ++i) {
            const size_t b = skel->FindBone(skel->GetDrivenBone(i).bone);
            if (b != MetroBone::InvalidIdx && b < boneHasTrack.size()) {
                boneHasTrack[b] = true;
            }
        }
    }

    for (size_t i = 0; i < skelNodes.size(); ++i) {
        FbxNode* boneNode = skelNodes[i];

        if (boneHasTrack[i]) {
            const MetroCurve& posCurve = motion->GetBonePositionCurve(i);
            const MetroCurve& rotCurve = motion->GetBoneRotationCurve(i);

            const bool isDrivenOnly = !motion->IsBoneAnimated(i);
            const bool posIsConst = !isDrivenOnly && (posCurve.GetNumPoints() < 2);
            const bool rotIsConst = !isDrivenOnly && (rotCurve.GetNumPoints() < 2);

            const size_t numPosKeys = posIsConst ? 1 : numFrames;
            const size_t numRotKeys = rotIsConst ? 1 : numFrames;

            boneNode->LclRotation.GetCurveNode(animLayer, true);
            boneNode->LclTranslation.GetCurveNode(animLayer, true);

            FbxAnimCurve* offsetCurve[3] = {
                boneNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X, true),
                boneNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y, true),
                boneNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z, true)
            };

            offsetCurve[0]->KeyModifyBegin();
            offsetCurve[1]->KeyModifyBegin();
            offsetCurve[2]->KeyModifyBegin();

            for (size_t f = 0; f < numPosKeys; ++f) {
                const double t = scast<double>(f) / animFPS;
                const FbxVector4 fv = MetroVecToFbxVec(poseT[f][i]);

                keyTime.SetSecondDouble(t);

                for (int k = 0; k < 3; ++k) {
                    keyIndex = offsetCurve[k]->KeyAdd(keyTime);
                    offsetCurve[k]->KeySetValue(keyIndex, scast<float>(fv[k]));
                    offsetCurve[k]->KeySetInterpolation(keyIndex, FbxAnimCurveDef::eInterpolationLinear);
                }
            }

            offsetCurve[0]->KeyModifyEnd();
            offsetCurve[1]->KeyModifyEnd();
            offsetCurve[2]->KeyModifyEnd();

            FbxAnimCurve* rotationCurve[3] = {
                boneNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X, true),
                boneNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y, true),
                boneNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z, true)
            };

            rotationCurve[0]->KeyModifyBegin();
            rotationCurve[1]->KeyModifyBegin();
            rotationCurve[2]->KeyModifyBegin();

            for (size_t f = 0; f < numRotKeys; ++f) {
                const double t = scast<double>(f) / animFPS;
                const FbxVector4 fv = MetroRotToFbxRot(poseQ[f][i]);

                keyTime.SetSecondDouble(t);

                for (int k = 0; k < 3; ++k) {
                    keyIndex = rotationCurve[k]->KeyAdd(keyTime);
                    rotationCurve[k]->KeySetValue(keyIndex, scast<float>(fv[k]));
                    rotationCurve[k]->KeySetInterpolation(keyIndex, FbxAnimCurveDef::eInterpolationLinear);
                }
            }

            rotationCurve[0]->KeyModifyEnd();
            rotationCurve[1]->KeyModifyEnd();
            rotationCurve[2]->KeyModifyEnd();
        } else {
            const FbxVector4 bindT = boneNode->LclTranslation.Get();
            const FbxVector4 bindR = boneNode->LclRotation.Get();

            keyTime.SetSecondDouble(0.0);

            boneNode->LclRotation.GetCurveNode(animLayer, true);
            boneNode->LclTranslation.GetCurveNode(animLayer, true);

            FbxAnimCurve* offsetCurve[3] = {
                boneNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X, true),
                boneNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y, true),
                boneNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z, true)
            };

            FbxAnimCurve* rotationCurve[3] = {
                boneNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X, true),
                boneNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y, true),
                boneNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z, true)
            };

            for (int k = 0; k < 3; ++k) {
                offsetCurve[k]->KeyModifyBegin();
                keyIndex = offsetCurve[k]->KeyAdd(keyTime);
                offsetCurve[k]->KeySetValue(keyIndex, scast<float>(bindT[k]));
                offsetCurve[k]->KeySetInterpolation(keyIndex, FbxAnimCurveDef::eInterpolationConstant);
                offsetCurve[k]->KeyModifyEnd();

                rotationCurve[k]->KeyModifyBegin();
                keyIndex = rotationCurve[k]->KeyAdd(keyTime);
                rotationCurve[k]->KeySetValue(keyIndex, scast<float>(bindR[k]));
                rotationCurve[k]->KeySetInterpolation(keyIndex, FbxAnimCurveDef::eInterpolationConstant);
                rotationCurve[k]->KeyModifyEnd();
            }
        }
    }

    CorrectAnimTrackInterpolation(skelNodes, animLayer);
}

static bool SaveFBXScene(FbxManager* mgr, FbxScene* scene, FbxIOSettings* ios, const fs::path& path, const bool saveMesh, const bool saveAnim) {
    // now export all this
    FbxGlobalSettings& settings = scene->GetGlobalSettings();
    settings.SetAxisSystem(FbxAxisSystem(FbxAxisSystem::eOpenGL));
    settings.SetOriginalUpAxis(FbxAxisSystem(FbxAxisSystem::eOpenGL));
    settings.SetSystemUnit(FbxSystemUnit(1.0f));

    // export
    FbxExporter* exp = FbxExporter::Create(mgr, "");
    const int format = mgr->GetIOPluginRegistry()->GetNativeWriterFormat();

    exp->SetFileExportVersion(FBX_2011_00_COMPATIBLE);

    ios->SetBoolProp(EXP_FBX_MATERIAL, saveMesh);
    ios->SetBoolProp(EXP_FBX_TEXTURE, saveMesh);
    ios->SetBoolProp(EXP_FBX_EMBEDDED, false);
    ios->SetBoolProp(EXP_FBX_SHAPE, saveMesh);
    ios->SetBoolProp(EXP_FBX_GOBO, saveMesh);
    ios->SetBoolProp(EXP_FBX_MODEL, saveMesh);
    ios->SetBoolProp(EXP_FBX_ANIMATION, saveAnim);
    ios->SetBoolProp(EXP_FBX_GLOBAL_SETTINGS, true);

    if (exp->Initialize(path.u8string().c_str(), format, ios)) {
        if (!exp->Export(scene)) {
            exp->Destroy(true);
            return false;
        }
    }

    exp->Destroy(true);

    return true;
}

bool MetroModel::SaveAsFBX(const fs::path& filePath, const size_t options, const size_t motionIdx) {
    FbxManager* mgr = FbxManager::Create();
    if (!mgr) {
        return false;
    }

    const bool exportMesh = TestBit(options, MetroModel::FBX_Export_Mesh);
    const bool exportSkeleton = TestBit(options, MetroModel::FBX_Export_Skeleton);
    const bool exportAnimation = TestBit(options, MetroModel::FBX_Export_Animation);
    const bool excludeCollision = TestBit(options, MetroModel::FBX_Export_ExcludeCollision);

    fs::path modelFolder = filePath.parent_path();

    FbxIOSettings* ios = FbxIOSettings::Create(mgr, IOSROOT);
    mgr->SetIOSettings(ios);

    FbxScene* scene = FbxScene::Create(mgr, "Metro model");
    FbxDocumentInfo* info = scene->GetSceneInfo();
    if (info) {
        info->Original_ApplicationVendor = FbxString("iOrange");
        info->Original_ApplicationName = FbxString("MetroEX");
        info->mTitle = FbxString("Metro Exodus model");
        info->mComment = FbxString("Exported using MetroEX created by iOrange");
    }

    MyDict<CharString, FbxSurfacePhong*> fbxMaterials;
    if (exportMesh) {
        for (size_t i = 0; i < mMeshes.size(); ++i) {
            const MetroMesh* mesh = mMeshes[i];

            // empty mesh ???
            if (mesh->vertices.empty() || mesh->faces.empty()) {
                continue;
            }

            // skip collision geometry if asked so
            if (excludeCollision && mesh->isCollision) {
                continue;
            }

            const CharString& textureName = mesh->materials.front();

            auto it = fbxMaterials.find(textureName);
            if (it == fbxMaterials.end()) {
                const CharString& sourceName = MetroTexturesDatabase::Get().GetSourceName(textureName);
                const CharString& bumpName = MetroTexturesDatabase::Get().GetSourceName(textureName);

                CharString textureTgaName = fs::path(sourceName).filename().u8string() + ".tga";
                CharString texturePath = (modelFolder / textureTgaName).u8string();

                FbxFileTexture* texture = FbxFileTexture::Create(mgr, textureName.c_str());
                texture->SetFileName(texturePath.c_str());
                texture->SetTextureUse(FbxTexture::eStandard);
                texture->SetMappingType(FbxTexture::eUV);
                texture->SetMaterialUse(FbxFileTexture::eModelMaterial);
                texture->UVSwap = false;
                texture->SetTranslation(0.0, 0.0);
                texture->SetScale(1.0, 1.0);
                texture->SetRotation(0.0, 0.0);
                texture->SetAlphaSource(FbxTexture::eBlack);

                FbxFileTexture* bump = nullptr;
                if (!bumpName.empty()) {
                    CharString bumpTgaName = fs::path(bumpName).filename().u8string() + "_nm.tga";
                    CharString bumpPath = (modelFolder / bumpTgaName).u8string();

                    bump = FbxFileTexture::Create(mgr, bumpName.c_str());
                    bump->SetFileName(bumpPath.c_str());
                    bump->SetTextureUse(FbxTexture::eBumpNormalMap);
                    bump->SetMappingType(FbxTexture::eUV);
                    bump->SetMaterialUse(FbxFileTexture::eModelMaterial);
                    bump->UVSwap = false;
                    bump->SetTranslation(0.0, 0.0);
                    bump->SetScale(1.0, 1.0);
                    bump->SetRotation(0.0, 0.0);
                }

                FbxSurfacePhong* material = FbxSurfacePhong::Create(mgr, textureName.c_str());
                material->Emissive = FbxDouble3(0.0, 0.0, 0.0);
                material->Diffuse.ConnectSrcObject(texture);
                material->Specular = FbxDouble3(1.0, 1.0, 1.0);
                material->SpecularFactor = 0.0;
                material->Shininess = 0.0; // simple diffuse

                if (bump) {
                    material->Bump.ConnectSrcObject(bump);
                    material->BumpFactor = 1.0;
                }

                fbxMaterials[textureName] = material;
            }
        }
    }

    MyArray<FbxNode*> meshNodes;
    MyArray<FbxMesh*> fbxMeshes;
    if (exportMesh) {
        for (size_t i = 0; i < mMeshes.size(); ++i) {
            const MetroMesh* mesh = this->GetMesh(i);

            // empty mesh ???
            if (mesh->vertices.empty() || mesh->faces.empty()) {
                continue;
            }

            // skip collision geometry if asked so
            if (excludeCollision && mesh->isCollision) {
                continue;
            }

            CharString meshName = CharString("mesh_") + std::to_string(i);

            FbxMesh* fbxMesh = FbxMesh::Create(scene, meshName.c_str());

            // assign vertices
            fbxMesh->InitControlPoints(scast<int>(mesh->vertices.size()));
            FbxVector4* ptrCtrlPoints = fbxMesh->GetControlPoints();

            FbxGeometryElementNormal* normalElement = fbxMesh->CreateElementNormal();
            normalElement->SetMappingMode(FbxGeometryElement::eByControlPoint);
            normalElement->SetReferenceMode(FbxGeometryElement::eDirect);

            FbxGeometryElementUV* uvElement = fbxMesh->CreateElementUV("uv0");
            uvElement->SetMappingMode(FbxGeometryElement::eByControlPoint);
            uvElement->SetReferenceMode(FbxGeometryElement::eDirect);

            for (const MetroVertex& v : mesh->vertices) {
                *ptrCtrlPoints = MetroVecToFbxVec(v.pos);
                normalElement->GetDirectArray().Add(MetroVecToFbxVec(v.normal));
                uvElement->GetDirectArray().Add(FbxVector2(v.uv0.x, 1.0f - v.uv0.y));

                ++ptrCtrlPoints;
            }

            FbxGeometryElementMaterial* materialElement = fbxMesh->CreateElementMaterial();
            materialElement->SetMappingMode(FbxGeometryElement::eAllSame);
            materialElement->SetReferenceMode(FbxGeometryElement::eIndexToDirect);
            materialElement->GetIndexArray().Add(0);

            // build polygons
            for (const MetroFace& face : mesh->faces) {
                fbxMesh->BeginPolygon();
                fbxMesh->AddPolygon(scast<int>(face.c));
                fbxMesh->AddPolygon(scast<int>(face.b));
                fbxMesh->AddPolygon(scast<int>(face.a));
                fbxMesh->EndPolygon();
            }

            FbxNode* meshNode = FbxNode::Create(scene, meshName.c_str());
            meshNode->SetNodeAttribute(fbxMesh);
            scene->GetRootNode()->AddChild(meshNode);

            const CharString& textureName = mesh->materials.front();
            auto it = fbxMaterials.find(textureName);
            if (it != fbxMaterials.end()) {
                meshNode->SetShadingMode(FbxNode::eTextureShading);
                meshNode->AddMaterial(it->second);
            }

            fbxMeshes.push_back(fbxMesh);
            meshNodes.push_back(meshNode);
        }
    }

    MyArray<FbxNode*> boneNodes;
    if (mSkeleton && (exportSkeleton || exportAnimation)) {
        FbxNode* rootBoneNode = CreateFBXSkeleton(scene, mSkeleton, boneNodes);
        scene->GetRootNode()->AddChild(rootBoneNode);

        FbxPose* bindPose = FbxPose::Create(scene, "BindPose");
        bindPose->SetIsBindPose(true);

        for (FbxNode* node : boneNodes) {
            bindPose->Add(node, node->EvaluateGlobalTransform());
        }

        for (size_t i = 0; i < fbxMeshes.size(); ++i) {
            FbxMesh* fbxMesh = fbxMeshes[i];
            FbxNode* meshNode = meshNodes[i];

            const MetroMesh* mesh = mMeshes[i];

            FbxAMatrix meshXMatrix = meshNode->EvaluateGlobalTransform();

            MyArray<ClusterInfo> clusters;
            CollectClusters(mesh, mSkeleton, clusters);

            FbxSkin* skin = FbxSkin::Create(scene, "");
            for (size_t i = 0; i < clusters.size(); ++i) {
                const ClusterInfo& cluster = clusters[i];
                if (!cluster.vertexIdxs.empty()) {
                    FbxCluster* fbxCluster = FbxCluster::Create(scene, "");
                    FbxNode* linkNode = boneNodes[i];
                    fbxCluster->SetLink(linkNode);
                    fbxCluster->SetLinkMode(FbxCluster::eTotalOne);
                    for (size_t j = 0; j < cluster.vertexIdxs.size(); ++j) {
                        fbxCluster->AddControlPointIndex(cluster.vertexIdxs[j], cluster.weigths[j]);
                    }
                    fbxCluster->SetTransformMatrix(meshXMatrix);
                    fbxCluster->SetTransformLinkMatrix(linkNode->EvaluateGlobalTransform());
                    skin->AddCluster(fbxCluster);
                }
            }

            fbxMesh->AddDeformer(skin);
            bindPose->Add(meshNode, meshXMatrix);
        }

        scene->AddPose(bindPose);
    }

    if (exportAnimation) {
        if (motionIdx != kInvalidValue) {
            const MetroMotion* motion = this->GetMotion(motionIdx);
            AddAnimTrackToScene(scene, this, motion, motion->GetName(), boneNodes);
        } else {
            for (size_t i = 0; i < this->GetNumMotions(); ++i) {
                const MetroMotion* motion = this->GetMotion(i);
                AddAnimTrackToScene(scene, this, motion, motion->GetName(), boneNodes);
            }
        }
    }

    if (!SaveFBXScene(mgr, scene, ios, filePath, exportMesh, exportSkeleton || exportAnimation)) {
        return false;
    }

    mgr->Destroy();
    return true;
}

bool MetroModel::IsAnimated() const {
    return mSkeleton != nullptr;
}

bool MetroModel::HasLodModel(const size_t idx) const {
    return idx < kMetroModelMaxLods && mLodModels[idx] != nullptr;
}

const AABBox& MetroModel::GetBBox() const {
    return mBBox;
}

const vec4& MetroModel::GetBSphere() const {
    return mBSphere;
}

size_t MetroModel::GetNumMeshes() const {
    return mMeshes.size();
}

const MetroMesh* MetroModel::GetMesh(const size_t idx) const {
    return mMeshes[idx];
}

const CharString& MetroModel::GetSkeletonPath() const {
    return mSkeletonPath;
}

const MetroSkeleton* MetroModel::GetSkeleton() const {
    return mSkeleton;
}

MetroModel* MetroModel::GetLodModel(const size_t idx) const {
    return mLodModels[idx];
}

size_t MetroModel::GetNumMotions() const {
    return mMotions.size();
}

CharString MetroModel::GetMotionName(const size_t idx) const {
    const MyHandle file = mMotions[idx].file;
    const CharString& fileName = MetroFileSystem::Get().GetName(file);

    CharString name = fileName.substr(0, fileName.length() - 3);
    return name;
}

const CharString& MetroModel::GetMotionPath(const size_t idx) const {
    return mMotions[idx].path;
}

float MetroModel::GetMotionDuration(const size_t idx) const {
    return scast<float>(mMotions[idx].numFrames) / scast<float>(MetroMotion::kFrameRate);
}

const MetroMotion* MetroModel::GetMotion(const size_t idx) {
    MetroMotion* motion = mMotions[idx].motion;

    if (!motion) {
        const CharString& name = this->GetMotionName(idx);
        const MyHandle file = mMotions[idx].file;

        motion = new MetroMotion(name);
        MemStream stream = MetroFileSystem::Get().OpenFileStream(file);
        motion->LoadFromData(stream);

        mMotions[idx].motion = motion;
    }

    return motion;
}

const MetroMotion* MetroModel::FindMotionByName(const CharString& name) {
    if (name.empty()) {
        return nullptr;
    }

    for (size_t i = 0; i < mMotions.size(); ++i) {
        if (this->GetMotionName(i) == name) {
            return this->GetMotion(i);
        }
    }

    return nullptr;
}

static mat4 MetroMakeLocalMat(const quat& q, const vec3& t) {
    mat4 result = MatFromQuat(q);
    result[3] = vec4(t, 1.0f);
    return result;
}

static quat MetroLookAtRotation(const vec3& aimLocal, const vec3& aimWorld, const vec3& upWorld) {
    vec3 side = Cross(aimWorld, upWorld);
    if (Length(side) < MM_Epsilon) {
        return quat(1.0f, 0.0f, 0.0f, 0.0f);
    }
    side = Normalize(side);

    mat3 target;
    target[0] = aimWorld;
    target[1] = Cross(side, aimWorld);
    target[2] = side;

    const vec3 upLocal(0.0f, 1.0f, 0.0f);
    vec3 sideLocal = Cross(aimLocal, upLocal);
    if (Length(sideLocal) < MM_Epsilon) {
        sideLocal = Cross(aimLocal, vec3(0.0f, 0.0f, 1.0f));
        if (Length(sideLocal) < MM_Epsilon) {
            return quat(1.0f, 0.0f, 0.0f, 0.0f);
        }
    }
    sideLocal = Normalize(sideLocal);

    mat3 local;
    local[0] = aimLocal;
    local[1] = Cross(sideLocal, aimLocal);
    local[2] = sideLocal;

    return Normalize(quat(target * glm::transpose(local)));
}

static float MetroCalcDrivenValue(const mat4& driver, const mat4& driverParent, const uint8_t component) {
    static const float kRad2Deg = 57.29578f;

    const mat3 rel = glm::transpose(mat3(driverParent)) * mat3(driver);

    auto at = [&rel](const int row, const int col) { return rel[col][row]; };

    switch (component) {
        case MetroProceduralComponent::AxisX:
        case MetroProceduralComponent::AxisXNeg: {
            const float v = -std::atan2f(-at(1, 2), at(1, 1)) * kRad2Deg;
            return (component == MetroProceduralComponent::AxisXNeg) ? -v : v;
        }

        case MetroProceduralComponent::AxisY:
        case MetroProceduralComponent::AxisYNeg: {
            const float v = -std::atan2f(-at(2, 0), at(0, 0)) * kRad2Deg;
            return (component == MetroProceduralComponent::AxisYNeg) ? -v : v;
        }

        case MetroProceduralComponent::AxisZ:
        case MetroProceduralComponent::AxisZNeg: {
            const float v = -std::asinf(Clamp(at(1, 0), -1.0f, 1.0f)) * kRad2Deg;
            return (component == MetroProceduralComponent::AxisZNeg) ? -v : v;
        }

        case MetroProceduralComponent::OffsetX: return driver[3].x - driverParent[3].x;
        case MetroProceduralComponent::OffsetY: return driver[3].y - driverParent[3].y;
        case MetroProceduralComponent::OffsetZ: return driver[3].z - driverParent[3].z;
        case MetroProceduralComponent::Offset:  return Length(vec3(driver[3] - driverParent[3]));

        default: return 0.0f;
    }
}

static bool sApplyProceduralBones = true;

static const bool sApplyConstrainedBones = true;

bool MetroModel::GetApplyProceduralBones() {
    return sApplyProceduralBones;
}

void MetroModel::SetApplyProceduralBones(const bool apply) {
    sApplyProceduralBones = apply;
}

void MetroModel::BuildProceduralCache() {
    if (mProceduralCache.built || !mSkeleton) {
        return;
    }
    mProceduralCache.built = true;

    const size_t numBones = mSkeleton->GetNumBones();

    MyArray<size_t>& order = mProceduralCache.boneOrder;
    order.reserve(numBones);
    MyArray<bool> placed(numBones, false);
    std::function<void(const size_t)> place = [&](const size_t idx) {
        if (placed[idx]) {
            return;
        }
        placed[idx] = true;
        const size_t parentIdx = mSkeleton->GetBoneParentIdx(idx);
        if (parentIdx != MetroBone::InvalidIdx) {
            place(parentIdx);
        }
        order.push_back(idx);
    };
    for (size_t i = 0; i < numBones; ++i) {
        place(i);
    }

    mProceduralCache.children.resize(numBones);
    for (size_t i = 0; i < numBones; ++i) {
        const size_t parentIdx = mSkeleton->GetBoneParentIdx(i);
        if (parentIdx != MetroBone::InvalidIdx) {
            mProceduralCache.children[parentIdx].push_back(i);
        }
    }

    std::function<ProceduralSource(const CharString&)> resolve = [&](const CharString& name) -> ProceduralSource {
        ProceduralSource src = { MetroBone::InvalidIdx, mat4(1.0f), mat4(1.0f), false };

        const size_t boneIdx = mSkeleton->FindBone(name);
        if (boneIdx != MetroBone::InvalidIdx) {
            src.boneIdx = boneIdx;
            src.bindGlobal = mSkeleton->GetBoneFullTransform(boneIdx);
            src.valid = true;
            return src;
        }

        const size_t auxIdx = mSkeleton->FindAuxBone(name);
        if (auxIdx == MetroBone::InvalidIdx) {
            return src;
        }

        const MetroAuxBone& aux = mSkeleton->GetAuxBone(auxIdx);
        const mat4 local = MetroMakeLocalMat(aux.q, aux.t);
        if (aux.parent.empty()) {
            src.localChain = local;
            src.bindGlobal = local;
            src.valid = true;
            return src;
        }

        const ProceduralSource parent = resolve(aux.parent);
        if (!parent.valid) {
            return src;
        }

        src.boneIdx = parent.boneIdx;
        src.localChain = parent.localChain * local;
        src.bindGlobal = parent.bindGlobal * local;
        src.valid = true;
        return src;
    };

    const size_t numDriven = mSkeleton->GetNumDrivenBones();
    mProceduralCache.driven.resize(numDriven);
    for (size_t i = 0; i < numDriven; ++i) {
        const MetroDrivenBone& rule = mSkeleton->GetDrivenBone(i);
        DrivenRuleCache& c = mProceduralCache.driven[i];

        c.targetIdx = mSkeleton->FindBone(rule.bone);
        c.driver = resolve(rule.driver);
        c.driverParent = resolve(rule.driver_parent);
        c.twister = this->FindMotionByName(rule.twister);
        c.span = 0;
        c.additive = false;

        if (c.twister && c.twister->GetNumFrames()) {
            const size_t numFrames = c.twister->GetNumFrames();
            c.span = c.twister->IsLooped() ? numFrames : std::max<size_t>(numFrames - 1, 1);
            c.additive = c.twister->IsAdditive();
        }
    }

    auto cacheSet = [&](const MetroParentBones& set, const size_t selfIdx,
                        MyArray<ProceduralWeight>& out, bool& outDriven) {
        outDriven = false;
        for (const MetroParentBone& p : set.bones) {
            if (p.weight < MetroParentBones::kMinWeight) {
                continue;
            }
            const ProceduralSource src = resolve(p.bone);
            if (!src.valid) {
                continue;
            }
            out.push_back({ src, p.weight });
            if (src.boneIdx != selfIdx) {
                outDriven = true;
            }
        }
    };

    const size_t numConstrained = mSkeleton->GetNumConstrainedBones();
    mProceduralCache.constrained.resize(numConstrained);
    for (size_t i = 0; i < numConstrained; ++i) {
        const MetroConstrainedBone& rule = mSkeleton->GetConstrainedBone(i);
        ConstrainedCache& c = mProceduralCache.constrained[i];

        c.targetIdx = mSkeleton->FindBone(rule.bone);
        bool upDriven = false;
        cacheSet(rule.position, c.targetIdx, c.position, c.posDriven);
        cacheSet(rule.orientation, c.targetIdx, c.orientation, c.oriDriven);
        cacheSet(rule.up, c.targetIdx, c.up, upDriven);
    }
}

void MetroModel::ApplyDrivenBones(const MetroMotion* motion, MyArray<quat>& localQ, MyArray<vec3>& localT) {
    const size_t numRefs = mSkeleton ? mSkeleton->GetNumProceduralRefs() : 0;
    if (!numRefs || !sApplyProceduralBones) {
        return;
    }

    this->BuildProceduralCache();

    const size_t numBones = mSkeleton->GetNumBones();
    const MyArray<size_t>& order = mProceduralCache.boneOrder;

    MyArray<mat4> full(numBones);
    for (const size_t idx : order) {
        const mat4 m = MetroMakeLocalMat(localQ[idx], localT[idx]);
        const size_t parentIdx = mSkeleton->GetBoneParentIdx(idx);
        full[idx] = (parentIdx == MetroBone::InvalidIdx) ? m : (full[parentIdx] * m);
    }

    MyArray<size_t> refreshStack;
    auto refreshKids = [&](const size_t root) {
        refreshStack.clear();
        refreshStack.push_back(root);
        while (!refreshStack.empty()) {
            const size_t idx = refreshStack.back();
            refreshStack.pop_back();

            const mat4 m = MetroMakeLocalMat(localQ[idx], localT[idx]);
            const size_t parentIdx = mSkeleton->GetBoneParentIdx(idx);
            full[idx] = (parentIdx == MetroBone::InvalidIdx) ? m : (full[parentIdx] * m);

            for (const size_t kid : mProceduralCache.children[idx]) {
                refreshStack.push_back(kid);
            }
        }
    };

    auto globalOf = [&](const ProceduralSource& src) {
        return (src.boneIdx == MetroBone::InvalidIdx) ? src.localChain : (full[src.boneIdx] * src.localChain);
    };

    struct DrivenResult { quat q; vec3 t; bool valid; bool additive; };
    MyArray<DrivenResult> results(numBones, { quat(1.0f, 0.0f, 0.0f, 0.0f), vec3(0.0f), false, false });

    auto applyDrivenRule = [&](const size_t ruleIdx) {
        const DrivenRuleCache& cached = mProceduralCache.driven[ruleIdx];
        const size_t targetIdx = cached.targetIdx;

        if (targetIdx == MetroBone::InvalidIdx || !cached.twister || !cached.span) {
            return;
        }
        if (motion && motion->IsBoneAnimated(targetIdx)) {
            return;
        }
        if (!cached.driver.valid || !cached.driverParent.valid) {
            return;
        }

        const MetroDrivenBone& driven = mSkeleton->GetDrivenBone(ruleIdx);
        const float value = MetroCalcDrivenValue(globalOf(cached.driver), globalOf(cached.driverParent), driven.component);

        const float range = driven.value_max - driven.value_min;
        const float k = (range != 0.0f) ? Clamp((value - driven.value_min) / range, 0.0f, 1.0f) : 0.0f;
        const float time = k * scast<float>(cached.span) / scast<float>(MetroMotion::kFrameRate);

        quat q;
        vec3 t;
        if (cached.twister->IsBoneAnimated(targetIdx)) {
            q = cached.twister->GetBoneRotationAtTime(targetIdx, time);
            t = cached.twister->GetBonePositionAtTime(targetIdx, time);
        } else if (cached.additive) {
            return;
        } else {
            q = Normalize(mSkeleton->GetBoneRotation(targetIdx));
            t = mSkeleton->GetBonePosition(targetIdx);
        }

        DrivenResult& r = results[targetIdx];
        if (!r.valid) {
            r.q = q;
            r.t = t;
            r.valid = true;
            r.additive = cached.additive;
        } else if (r.additive == cached.additive) {
            r.t = r.t + QuatRotate(r.q, t);
            r.q = r.q * q;
        } else {
            const quat baseQ = r.additive ? q : r.q;
            const vec3 baseT = r.additive ? t : r.t;
            const quat deltaQ = r.additive ? r.q : q;
            const vec3 deltaT = r.additive ? r.t : t;

            r.q = Normalize(baseQ * deltaQ);
            r.t = baseT + deltaT;
            r.additive = false;
        }
    };

    auto blendSet = [&](const MyArray<ProceduralWeight>& set, const size_t selfIdx, const bool atBind,
                        quat& outQ, vec3& outT) {
        outQ = quat(1.0f, 0.0f, 0.0f, 0.0f);
        outT = vec3(0.0f);

        const mat4& selfMat = atBind ? mSkeleton->GetBoneFullTransform(selfIdx) : full[selfIdx];
        const quat ownQ = Normalize(QuatFromMat(selfMat));

        float total = 0.0f;
        for (const ProceduralWeight& p : set) {
            const mat4 m = atBind ? p.source.bindGlobal : globalOf(p.source);

            quat q = Normalize(QuatFromMat(m));
            if (Dot(vec4(q.x, q.y, q.z, q.w), vec4(ownQ.x, ownQ.y, ownQ.z, ownQ.w)) < 0.0f) {
                q = quat(-q.w, -q.x, -q.y, -q.z);
            }

            total += p.weight;
            float f = p.weight / total;
            if (Dot(vec4(q.x, q.y, q.z, q.w), vec4(outQ.x, outQ.y, outQ.z, outQ.w)) < 0.0f) {
                f = -f;
            }

            const vec4 mixed = vec4(outQ.x, outQ.y, outQ.z, outQ.w) * (1.0f - fabsf(f)) +
                               vec4(q.x, q.y, q.z, q.w) * f;
            outQ = Normalize(quat(mixed.w, mixed.x, mixed.y, mixed.z));
            outT = outT + (vec3(m[3]) - outT) * (p.weight / total);
        }
    };

    auto axisVector = [](const uint8_t component) -> vec3 {
        switch (component) {
            case MetroProceduralComponent::AxisY:    return vec3(0.0f, 1.0f, 0.0f);
            case MetroProceduralComponent::AxisZ:    return vec3(1.0f, 0.0f, 0.0f);
            case MetroProceduralComponent::AxisXNeg: return vec3(0.0f, 0.0f, -1.0f);
            case MetroProceduralComponent::AxisYNeg: return vec3(0.0f, -1.0f, 0.0f);
            case MetroProceduralComponent::AxisZNeg: return vec3(-1.0f, 0.0f, 0.0f);
            default:                                 return vec3(0.0f, 0.0f, 1.0f);
        }
    };

    auto applyConstrained = [&](const size_t constrainedIdx, const bool lookAt) {
        const ConstrainedCache& cached = mProceduralCache.constrained[constrainedIdx];
        const size_t targetIdx = cached.targetIdx;

        if (targetIdx == MetroBone::InvalidIdx || (!cached.posDriven && !cached.oriDriven)) {
            return;
        }
        if (motion && motion->IsBoneAnimated(targetIdx)) {
            return;
        }

        quat posQnow, oriQnow, upQnow, posQbind, oriQbind;
        vec3 posTnow, oriTnow, upTnow, posTbind, oriTbind;
        blendSet(cached.position, targetIdx, false, posQnow, posTnow);
        blendSet(cached.orientation, targetIdx, false, oriQnow, oriTnow);
        blendSet(cached.up, targetIdx, false, upQnow, upTnow);
        blendSet(cached.position, targetIdx, true, posQbind, posTbind);
        blendSet(cached.orientation, targetIdx, true, oriQbind, oriTbind);

        const MetroConstrainedBone& rule = mSkeleton->GetConstrainedBone(constrainedIdx);
        const mat4 bindMat = mSkeleton->GetBoneFullTransform(targetIdx);
        const vec3 worldPos = vec3(full[targetIdx][3]);
        quat worldRot;

        if (lookAt) {
            worldRot = Normalize(QuatFromMat(full[targetIdx]));

            const vec3 aimWorld = posTnow - worldPos;
            if (cached.posDriven && Length(aimWorld) > MM_Epsilon) {
                const quat upFrame = (rule.uptype == MetroUpType::ObjectRotationUp) ? upQnow : oriQnow;

                vec3 upWorld;
                if (rule.uptype == MetroUpType::ObjectUp) {
                    upWorld = upTnow - worldPos;
                } else {
                    upWorld = QuatRotate(upFrame, vec3(0.0f, 1.0f, 0.0f));
                }

                if (Length(upWorld) > MM_Epsilon) {
                    const quat aimed = MetroLookAtRotation(axisVector(rule.look_at_axis),
                                                           Normalize(aimWorld), Normalize(upWorld));
                    if (aimed.w != 1.0f || aimed.x != 0.0f || aimed.y != 0.0f || aimed.z != 0.0f) {
                        worldRot = aimed;
                    }
                }
            }
        } else {
            worldRot = Normalize(QuatFromMat(full[targetIdx]));
            if (cached.oriDriven) {
                worldRot = Normalize((oriQnow * QuatConjugate(oriQbind)) * Normalize(QuatFromMat(bindMat)));
            }
        }

        const size_t parentIdx = mSkeleton->GetBoneParentIdx(targetIdx);
        const mat4 parentMat = (parentIdx == MetroBone::InvalidIdx) ? mat4(1.0f) : full[parentIdx];

        mat4 desired = MatFromQuat(worldRot);
        desired[3] = vec4(worldPos, 1.0f);

        const mat4 local = MatInverse(parentMat) * desired;
        localQ[targetIdx] = Normalize(QuatFromMat(local));
        localT[targetIdx] = vec3(local[3]);
        refreshKids(targetIdx);
    };

    size_t pendingTarget = MetroBone::InvalidIdx;
    auto flushDriven = [&]() {
        if (pendingTarget == MetroBone::InvalidIdx) {
            return;
        }
        const DrivenResult& r = results[pendingTarget];
        if (r.valid) {
            localQ[pendingTarget] = Normalize(r.q);
            localT[pendingTarget] = r.t;
            refreshKids(pendingTarget);
        }
        pendingTarget = MetroBone::InvalidIdx;
    };

    for (size_t i = 0; i < numRefs; ++i) {
        const MetroProceduralRef& ref = mSkeleton->GetProceduralRef(i);

        if (ref.type == MetroProceduralType::Driven) {
            if (ref.index_in_array >= mProceduralCache.driven.size()) {
                continue;
            }
            const size_t targetIdx = mProceduralCache.driven[ref.index_in_array].targetIdx;
            if (targetIdx != pendingTarget) {
                flushDriven();
                pendingTarget = targetIdx;
                if (targetIdx != MetroBone::InvalidIdx) {
                    results[targetIdx] = { quat(1.0f, 0.0f, 0.0f, 0.0f), vec3(0.0f), false, false };
                }
            }
            applyDrivenRule(ref.index_in_array);
        } else if (ref.type == MetroProceduralType::PosRotConstrained ||
                   ref.type == MetroProceduralType::LookAtConstrained) {
            flushDriven();
            if (sApplyConstrainedBones && ref.index_in_array < mProceduralCache.constrained.size()) {
                applyConstrained(ref.index_in_array, ref.type == MetroProceduralType::LookAtConstrained);
            }
        }
    }
    flushDriven();
}

void MetroModel::CalcPose(const MetroMotion* motion, const float time,
                          MyArray<quat>& outLocalQ, MyArray<vec3>& outLocalT) {
    if (!mSkeleton) {
        outLocalQ.clear();
        outLocalT.clear();
        return;
    }

    const size_t numBones = mSkeleton->GetNumBones();
    outLocalQ.resize(numBones);
    outLocalT.resize(numBones);

    for (size_t i = 0; i < numBones; ++i) {
        if (motion && motion->IsBoneAnimated(i)) {
            outLocalQ[i] = motion->GetBoneRotationAtTime(i, time);
            outLocalT[i] = motion->GetBonePositionAtTime(i, time);
        } else {
            outLocalQ[i] = Normalize(mSkeleton->GetBoneRotation(i));
            outLocalT[i] = mSkeleton->GetBonePosition(i);
        }
    }

    this->ApplyDrivenBones(motion, outLocalQ, outLocalT);
}

const CharString& MetroModel::GetComment() const {
    return mComment;
}


static void RemapBones(MetroVertex& v, const BytesArray& remap) {
    v.bones[0] = remap[v.bones[0]];
    v.bones[1] = remap[v.bones[1]];
    v.bones[2] = remap[v.bones[2]];
    v.bones[3] = remap[v.bones[3]];
}

void MetroModel::ReadSubChunks(MemStream& stream) {
    while (!stream.Ended()) {
        const size_t chunkId = stream.ReadTyped<uint32_t>();
        const size_t chunkSize = stream.ReadTyped<uint32_t>();
        const size_t chunkEnd = stream.GetCursor() + chunkSize;

        switch (chunkId) {
            case MC_HeaderChunk: {
                MdlHeader hdr;
                stream.ReadStruct(hdr);

                //#NOTE_SK: versions prior to Redux are not supported
                if (hdr.version < kModelVersionRedux) {
                    assert(false);
                    return;
                }

                if (hdr.vscale <= MM_Epsilon) {
                    hdr.vscale = 1.0f;
                }

                if (mCurrentMesh) {
                    mCurrentMesh->version = hdr.version;
                    mCurrentMesh->flags = hdr.flags;
                    mCurrentMesh->vscale = hdr.vscale;
                    mCurrentMesh->bbox = hdr.bbox;
                    mCurrentMesh->type = hdr.type;
                    mCurrentMesh->shaderId = hdr.shaderId;
                } else if (!mHeaderRead) {
                    mVersion = hdr.version;
                    mBBox = hdr.bbox;
                    mBSphere = hdr.bsphere;
                    mHeaderRead = true;
                } else {
                    mBBox.Absorb(hdr.bbox.minimum);
                    mBBox.Absorb(hdr.bbox.maximum);
                }
            } break;

            case MC_MaterialsChunk: {
                if (!mCurrentMesh) {
                    return;
                }

                mCurrentMesh->materials.resize(kMetroModelMaxMaterials);
                for (auto& s : mCurrentMesh->materials) {
                    s = stream.ReadStringZ();
                }

                mCurrentMesh->materialFlags0 = stream.ReadTyped<uint16_t>();
                mCurrentMesh->materialFlags1 = stream.ReadTyped<uint16_t>();
                if (TestBit<size_t>(mCurrentMesh->materialFlags0, 8)) {
                    mCurrentMesh->isCollision = true;
                }

                //#NOTE_SK: seems like meshes with either "invalid" texture, and/or "collision" source materials
                //          are additional collision geometry, invisible during drawing
                const CharString& textureName = mCurrentMesh->materials[0];
                const CharString& shaderName = mCurrentMesh->materials[1];
                const CharString& srcMatName = mCurrentMesh->materials[3];
                if (StrEndsWith(textureName, "invalid") ||
                    StrContains(shaderName, "invisible") ||
                    StrContains(srcMatName, "collision") ||
                    StrContains(srcMatName, "colision")) {
                    mCurrentMesh->isCollision = true;
                }

            } break;

            case MC_VerticesChunk: {
                if (mCurrentMesh) {
                    mCurrentMesh->skinned = false;

                    const size_t vertexType = stream.ReadTyped<uint32_t>();
                    const size_t numVertices = stream.ReadTyped<uint32_t>();
                    const size_t numShadowVertices = mCurrentMesh->version >= kModelVersionArktika1 ? stream.ReadTyped<uint16_t>() : 0;

                    mCurrentMesh->vertices.resize(numVertices);

                    const VertexStatic* srcVerts = rcast<const VertexStatic*>(stream.GetDataAtCursor());
                    MetroVertex* dstVerts = mCurrentMesh->vertices.data();

                    for (size_t i = 0; i < numVertices; ++i) {
                        *dstVerts = ConvertVertex(*srcVerts);
                        dstVerts->pos *= mCurrentMesh->vscale;
                        ++srcVerts;
                        ++dstVerts;
                    }
                }
            } break;

            case MC_SkinnedVerticesChunk: {
                if (mCurrentMesh) {
                    mCurrentMesh->skinned = true;

                    const size_t numBones = stream.ReadTyped<uint8_t>();

                    mCurrentMesh->bonesRemap.resize(numBones);
                    stream.ReadToBuffer(mCurrentMesh->bonesRemap.data(), numBones);

                    //std::vector<MetroOBB> obbs(numBones);
                    //stream.ReadToBuffer(obbs.data(), obbs.size() * sizeof(MetroOBB));
                    stream.SkipBytes(numBones * sizeof(MetroOBB));

                    size_t numVertices = 0, numShadowVertices = 0;

                    numVertices = stream.ReadTyped<uint32_t>();
                    if (mCurrentMesh->version >= kModelVersionArktika1) {
                        numShadowVertices = stream.ReadTyped<uint16_t>();
                    } else {
                        mCurrentMesh->vscale = 12.0f;   //#NOTE_SK: Redux versions are scaled down by 12
                    }

                    mCurrentMesh->vertices.resize(numVertices);

                    const VertexSkinned* srcVerts = rcast<const VertexSkinned*>(stream.GetDataAtCursor());
                    MetroVertex* dstVerts = mCurrentMesh->vertices.data();

                    for (size_t i = 0; i < numVertices; ++i) {
                        *dstVerts = ConvertVertex(*srcVerts);
                        dstVerts->pos *= mCurrentMesh->vscale;
                        RemapBones(*dstVerts, mCurrentMesh->bonesRemap);

                        ++srcVerts;
                        ++dstVerts;
                    }
                }
            } break;

            case MC_FacesChunk: {
                if (mCurrentMesh) {
                    size_t numFaces = 0, numShadowFaces = 0;

                    if (!mCurrentMesh->skinned) {
                        numFaces = stream.ReadTyped<uint32_t>();
                        if (mCurrentMesh->version >= kModelVersionArktika1) {
                            numShadowFaces = stream.ReadTyped<uint16_t>();
                        } else {
                            numFaces /= 3;  //#NOTE_SK: Redux models store number of indices, not faces
                        }
                    } else {
                        numFaces = stream.ReadTyped<uint16_t>();
                        numShadowFaces = stream.ReadTyped<uint16_t>();
                    }

                    mCurrentMesh->faces.resize(numFaces);
                    stream.ReadToBuffer(mCurrentMesh->faces.data(), numFaces * sizeof(MetroFace));
                }
            } break;

            case MC_SubMeshesChunk: {
                MemStream meshesStream = stream.Substream(chunkSize);
                size_t nextMeshId = 0;
                while (!meshesStream.Ended()) {
                    const size_t subMeshId = meshesStream.ReadTyped<uint32_t>();
                    const size_t subMeshSize = meshesStream.ReadTyped<uint32_t>();

                    if (subMeshId == nextMeshId) {
                        mCurrentMesh = new MetroMesh();
                        mMeshes.push_back(mCurrentMesh);

                        MemStream subStream = meshesStream.Substream(subMeshSize);
                        this->ReadSubChunks(subStream);
                        ++nextMeshId;

                        meshesStream.SkipBytes(subMeshSize);
                    } else {
                        break;
                    }
                }
                mCurrentMesh = nullptr;
            } break;

            case MC_Lod_1_Chunk:
            case MC_Lod_2_Chunk: {
                const size_t lodId = (MC_Lod_1_Chunk == chunkId) ? 0 : 1;

                MySafeDelete(mLodModels[lodId]);

                MemStream lodStream = stream.Substream(chunkSize);
                MetroModel* model = new MetroModel();
                if (model->LoadFromData(lodStream, mThisFileIdx)) {
                    mLodModels[lodId] = model;
                } else {
                    MySafeDelete(model);
                }
            } break;

            case MC_MeshesInline: {
                MemStream lodsStream = stream.Substream(chunkSize);
                while (!lodsStream.Ended()) {
                    const size_t lodId = lodsStream.ReadTyped<uint32_t>();
                    const size_t lodSize = lodsStream.ReadTyped<uint32_t>();

                    if (lodId > kMetroModelMaxLods) {
                        break;
                    }

                    MemStream meshesStream = lodsStream.Substream(lodSize);
                    MetroModel* target = (0 == lodId) ? this : this->GetOrCreateLodModel(lodId - 1);
                    target->LoadInlineMeshes(meshesStream);

                    lodsStream.SkipBytes(lodSize);
                }
            } break;

            case MC_MeshesLinks: {
                MemStream linksStream = stream.Substream(chunkSize);
                linksStream.SkipBytes(sizeof(uint32_t));
                for (size_t lodId = 0; lodId <= kMetroModelMaxLods && !linksStream.Ended(); ++lodId) {
                    CharString linksString = linksStream.ReadStringZ();
                    if (linksString.empty()) {
                        continue;
                    }

                    const StringArray links = StrSplit(linksString, ',');
                    if (links.empty()) {
                        continue;
                    }

                    MetroModel* target = (0 == lodId) ? this : this->GetOrCreateLodModel(lodId - 1);
                    target->LoadLinkedMeshes(links);
                }
            } break;

            case MC_SkeletonLink: {
                CharString skeletonRef = stream.ReadStringZ();
                mSkeletonPath = "content\\meshes\\" + skeletonRef + ".skeleton.bin";
                const MyHandle file = MetroFileSystem::Get().FindFile(mSkeletonPath);
                if (kInvalidHandle != file) {
                    MemStream stream = MetroFileSystem::Get().OpenFileStream(file);
                    if (stream) {
                        mSkeleton = new MetroSkeleton();
                        if (!mSkeleton->LoadFromData(stream)) {
                            MySafeDelete(mSkeleton);
                        }
                    }
                }
            } break;

            case MC_SkeletonInline: {
                mSkeleton = new MetroSkeleton();
                if (!mSkeleton->LoadFromData(stream.Substream(chunkSize))) {
                    MySafeDelete(mSkeleton);
                }
            } break;

            case MC_Comment: {
                if (chunkSize > 16) {
                    stream.SkipBytes(16); // wtf ???
                    mComment = stream.ReadStringZ();
                }
            } break;
        }

        stream.SetCursor(chunkEnd);
    }
}

void MetroModel::LoadLinkedMeshes(const StringArray& links) {
    mCurrentMesh = nullptr;

    const MetroFileSystem& mfs = MetroFileSystem::Get();
    for (const CharString& lnk : links) {
        MyHandle file = kInvalidHandle;

        if (lnk[0] == '.' && lnk[1] == kPathSeparator) { // relative path
            const MyHandle folder = mfs.GetParentFolder(mThisFileIdx);
            file = mfs.FindFile(lnk.substr(2) + ".mesh", folder);
        } else {
            CharString meshFilePath = R"(content\meshes\)" + lnk + ".mesh";
            file = mfs.FindFile(meshFilePath);
        }
        if (kInvalidHandle != file) {
            MemStream stream = mfs.OpenFileStream(file);
            if (stream) {
                this->ReadSubChunks(stream);
            }

            mCurrentMesh = nullptr;
        }
    }
}

void MetroModel::LoadInlineMeshes(MemStream& stream) {
    mCurrentMesh = nullptr;

    size_t nextMeshId = 0;
    while (!stream.Ended()) {
        const size_t meshId = stream.ReadTyped<uint32_t>();
        const size_t meshSize = stream.ReadTyped<uint32_t>();

        if (meshId != nextMeshId) {
            break;
        }

        MemStream meshStream = stream.Substream(meshSize);
        this->ReadSubChunks(meshStream);
        mCurrentMesh = nullptr;

        stream.SkipBytes(meshSize);
        ++nextMeshId;
    }
}

MetroModel* MetroModel::GetOrCreateLodModel(const size_t lodId) {
    if (lodId >= kMetroModelMaxLods) {
        return this;
    }

    if (nullptr == mLodModels[lodId]) {
        mLodModels[lodId] = new MetroModel();
        mLodModels[lodId]->mThisFileIdx = mThisFileIdx;
    }

    return mLodModels[lodId];
}

void MetroModel::LoadMotions() {
    CharString motionsStr;
    if (mSkeleton) {
        motionsStr = mSkeleton->GetMotionsStr();
    }

    if (motionsStr.empty()) {
        return;
    }

    const MetroFileSystem& mfs = MetroFileSystem::Get();

    MyArray<size_t> motionFiles;

    StringArray motionFolders = StrSplit(motionsStr, ',');
    StringArray motionPaths;
    for (const CharString& f : motionFolders) {
        CharString fullFolderPath = "content\\motions\\" + f + "\\";

        const auto& files = mfs.FindFilesInFolder(fullFolderPath, ".m2");

        for (const MyHandle file : files) {
            motionPaths.push_back(fullFolderPath + mfs.GetName(file));
        }

        motionFiles.insert(motionFiles.end(), files.begin(), files.end());
    }

    const size_t numBones = mSkeleton->GetNumBones();

    mMotions.reserve(motionFiles.size());
    size_t i = 0;
    for (const size_t idx : motionFiles) {
        MemStream stream = mfs.OpenFileStream(idx);
        if (stream) {
            MetroMotion motion(kEmptyString);
            if (motion.LoadHeader(stream) && motion.GetNumBones() == numBones) {
                mMotions.push_back({idx, motion.GetNumFrames(), motionPaths[i], nullptr});
            }
        }
        ++i;
    }
}





