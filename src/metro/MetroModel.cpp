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
    : mSkeleton(nullptr)
    , mCurrentMesh(nullptr)
    , mThisFileIdx(MetroFile::InvalidFileIdx)
{
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

static void AddAnimTrackToScene(FbxScene* scene, const MetroMotion* motion, const CharString& animName, MyArray<FbxNode*>& skelNodes) {
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

    for (size_t i = 0; i < skelNodes.size(); ++i) {
        FbxNode* boneNode = skelNodes[i];

        if (motion->IsBoneAnimated(i)) {
            const MetroCurve& posCurve = motion->GetBonePositionCurve(i);
            const MetroCurve& rotCurve = motion->GetBoneRotationCurve(i);

            const bool posIsConst = (posCurve.GetNumPoints() < 2);
            const bool rotIsConst = (rotCurve.GetNumPoints() < 2);

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
                const FbxVector4 fv = MetroVecToFbxVec(motion->GetBonePositionAtTime(i, scast<float>(t)));

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
                const FbxVector4 fv = MetroRotToFbxRot(motion->GetBoneRotationAtTime(i, scast<float>(t)));

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
            AddAnimTrackToScene(scene, motion, motion->GetName(), boneNodes);
        } else {
            for (size_t i = 0; i < this->GetNumMotions(); ++i) {
                const MetroMotion* motion = this->GetMotion(i);
                AddAnimTrackToScene(scene, motion, motion->GetName(), boneNodes);
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
                } else {
                    mVersion = hdr.version;
                    mBBox = hdr.bbox;
                    mBSphere = hdr.bsphere;
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

            case MC_Lod_1_Chunk: {
                if (mLodModels[0] != nullptr) {
                    MySafeDelete(mLodModels[0]);
                }
                MetroModel* model = new MetroModel();
                if (model->LoadFromData(stream, mThisFileIdx)) {
                    mLodModels[0] = model;
                } else {
                    MySafeDelete(model);
                }
            } break;

            case MC_Lod_2_Chunk: {
                if (mLodModels[1] != nullptr) {
                    MySafeDelete(mLodModels[1]);
                }
                MetroModel* model = new MetroModel();
                if (model->LoadFromData(stream, mThisFileIdx)) {
                    mLodModels[1] = model;
                } else {
                    MySafeDelete(model);
                }
            } break;

            case MC_MeshesInline: {
                stream.SkipBytes(16); // wtf ???
                MemStream subStream = stream.Substream(chunkSize - 16);
                this->ReadSubChunks(subStream);
            } break;

            case MC_MeshesLinks: {
                const size_t numStrings = stream.ReadTyped<uint32_t>();
                StringArray links, linksLod1, linksLod2;
                for (size_t i = 0; i < numStrings; ++i) {
                    CharString linksString = stream.ReadStringZ();
                    if (!linksString.empty()) {
                        StringArray splittedLinks = StrSplit(linksString, ',');
                        if (this->IsAnimated() && numStrings <= 3) { // is it correct to find lods of dynamic models this way?
                            if (i == 1) {
                                linksLod1.insert(linksLod1.end(), splittedLinks.begin(), splittedLinks.end());
                            } else if (i == 2) {
                                linksLod2.insert(linksLod2.end(), splittedLinks.begin(), splittedLinks.end());
                            } else {
                                links.insert(links.end(), splittedLinks.begin(), splittedLinks.end());
                            }
                        } else {
                            links.insert(links.end(), splittedLinks.begin(), splittedLinks.end());
                        }
                    }
                }

                if (!links.empty()) {
                    this->LoadLinkedMeshes(links);
                }
                if (!linksLod1.empty()) {
                    if (mLodModels[0] == nullptr) {
                        mLodModels[0] = new MetroModel();
                        mLodModels[0]->mThisFileIdx = this->mThisFileIdx;
                    }
                    mLodModels[0]->LoadLinkedMeshes(linksLod1);
                }
                if (!linksLod2.empty()) {
                    if (mLodModels[1] == nullptr) {
                        mLodModels[1] = new MetroModel();
                        mLodModels[1]->mThisFileIdx = this->mThisFileIdx;
                    }
                    mLodModels[1]->LoadLinkedMeshes(linksLod2);
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
