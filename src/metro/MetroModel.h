#pragma once
#include "MetroTypes.h"

class MetroSkeleton;
class MetroMotion;

class MetroModel {
public:
    static const size_t FBX_Export_None             = 0;
    static const size_t FBX_Export_Mesh             = 1;
    static const size_t FBX_Export_Skeleton         = 2;
    static const size_t FBX_Export_Animation        = 4;
    static const size_t FBX_Export_ExcludeCollision = 8;

    static const size_t kMetroModelMaxLods          = 2;

public:
    MetroModel();
    ~MetroModel();

    bool                    LoadFromData(MemStream& stream, const size_t fileIdx);
    bool                    SaveAsOBJ(const fs::path& filePath, const bool excludeCollision);
    bool                    SaveAsFBX(const fs::path& filePath, const size_t options, const size_t motionIdx = kInvalidValue);

    bool                    IsAnimated() const;
    bool                    HasLodModel(const size_t lodId) const;
    const AABBox&           GetBBox() const;
    const vec4&             GetBSphere() const;
    size_t                  GetNumMeshes() const;
    const MetroMesh*        GetMesh(const size_t idx) const;

    const CharString&       GetSkeletonPath() const;
    const MetroSkeleton*    GetSkeleton() const;
    MetroModel*             GetLodModel(const size_t lodId) const;
    size_t                  GetNumMotions() const;
    CharString              GetMotionName(const size_t idx) const;
    const CharString&       GetMotionPath(const size_t idx) const;
    float                   GetMotionDuration(const size_t idx) const;
    const MetroMotion*      GetMotion(const size_t idx);
    const MetroMotion*      FindMotionByName(const CharString& name);

    void                    CalcPose(const MetroMotion* motion, const float time,
                                     MyArray<quat>& outLocalQ, MyArray<vec3>& outLocalT);

    static bool             GetApplyProceduralBones();
    static void             SetApplyProceduralBones(const bool apply);

    const CharString&       GetComment() const;

private:
    void                    ApplyDrivenBones(const MetroMotion* motion, MyArray<quat>& localQ, MyArray<vec3>& localT);
    void                    BuildProceduralCache();
    void                    ReadSubChunks(MemStream& stream);
    void                    LoadLinkedMeshes(const StringArray& links);
    void                    LoadInlineMeshes(MemStream& stream);
    MetroModel*             GetOrCreateLodModel(const size_t lodId);
    void                    LoadMotions();

private:
    struct MotionInfo {
        MyHandle        file;
        size_t          numFrames;
        CharString      path;
        MetroMotion*    motion;
    };

    struct ProceduralSource {
        size_t          boneIdx;
        mat4            localChain;
        mat4            bindGlobal;
        bool            valid;
    };

    struct ProceduralWeight {
        ProceduralSource source;
        float            weight;
    };

    struct DrivenRuleCache {
        size_t              targetIdx;
        ProceduralSource    driver;
        ProceduralSource    driverParent;
        const MetroMotion*  twister;
        size_t              span;
        bool                additive;
    };

    struct ConstrainedCache {
        size_t                      targetIdx;
        MyArray<ProceduralWeight>   position;
        MyArray<ProceduralWeight>   orientation;
        MyArray<ProceduralWeight>   up;
        vec3                        bindPosBlend;
        quat                        bindOriBlend;
        bool                        posDriven;
        bool                        oriDriven;
    };

    struct ProceduralCache {
        bool                        built = false;
        MyArray<size_t>             boneOrder;
        MyArray<MyArray<size_t>>    children;
        MyArray<DrivenRuleCache>    driven;
        MyArray<ConstrainedCache>   constrained;
    };

    size_t                  mVersion;
    bool                    mHeaderRead;
    AABBox                  mBBox;
    vec4                    mBSphere;
    MyArray<MetroMesh*>     mMeshes;
    MetroModel*             mLodModels[kMetroModelMaxLods];
    CharString              mSkeletonPath;
    MetroSkeleton*          mSkeleton;
    MyArray<MotionInfo>     mMotions;
    ProceduralCache         mProceduralCache;
    CharString              mComment;

    // these are temp pointers, invalid after loading
    MetroMesh*              mCurrentMesh;
    size_t                  mThisFileIdx;
};

