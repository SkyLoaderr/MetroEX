#pragma once
#include "MetroTypes.h"

class MetroReflectionReader;

struct ParentMapped {   // 48 bytes
    CharString  parent_bone;
    CharString  self_bone;
    quat        q;
    vec3        t;
    vec3        s;

    void Serialize(MetroReflectionReader& s);
};

struct MetroBone {      // 38 bytes
    static const size_t InvalidIdx = kInvalidValue;

    CharString  name;
    CharString  parent;
    quat        q;
    vec3        t;
    uint8_t     bp;
    uint8_t     bpf;

    void Serialize(MetroReflectionReader& s);
};

struct MetroAuxBoneFlags {
    enum : uint8_t {
        SkipFx      = 0x01,
        QuatCalc    = 0x02,
        EditorType  = 0x04,
    };
};

struct MetroAuxBone {   // 37 bytes
    CharString  name;
    CharString  parent;
    quat        q;
    vec3        t;
    uint8_t     fl;

    void Serialize(MetroReflectionReader& s);
};

struct MetroProceduralComponent {
    enum : uint8_t {
        None        = 0,
        AxisX       = 1,
        AxisY       = 2,
        AxisZ       = 3,
        OffsetX     = 4,
        OffsetY     = 5,
        OffsetZ     = 6,
        Offset      = 7,
        AxisXNeg    = 8,
        AxisYNeg    = 9,
        AxisZNeg    = 10,
        Size        = 11
    };
};

struct MetroProceduralType {
    enum : uint16_t {
        Driven              = 0,
        PosRotConstrained   = 1,
        Dynamic             = 2,
        LookAtConstrained   = 3,
        Size                = 4
    };
};

struct MetroUpType {
    enum : uint8_t {
        LocalY              = 0,
        ObjectRotationUp    = 1,
        ObjectUp            = 2,
    };
};

struct MetroRotationOrder {
    enum : uint8_t {
        Default             = 0,
        ZYX                 = 1,
    };
};

struct MetroProceduralRef {   // 4 bytes, tells which array the bone lives in
    uint16_t    type;
    uint16_t    index_in_array;

    void Serialize(MetroReflectionReader& s);
};

struct MetroDrivenBone {
    CharString  bone;
    CharString  driver;
    CharString  driver_parent;
    uint8_t     component;
    CharString  twister;        // name of the motion to sample, not a bone
    float       value_min;
    float       value_max;
    uint8_t     refresh_kids;
    bool        use_anim_poses;

    void Serialize(MetroReflectionReader& s);
};

struct MetroParentBone {
    CharString  bone;
    float       weight;
};

struct MetroParentBones {
    static const float      kMinWeight;

    uint8_t                 axis;
    MyArray<MetroParentBone> bones;

    void Serialize(MetroReflectionReader& s);
};

struct MetroDynamicBone {
    CharString  bone;
    float       inertia;
    float       damping;
    vec3        pos_limits[2];
    vec3        rot_limits[2];
    bool        use_world_pos;
    uint8_t     refresh_kids;

    void Serialize(MetroReflectionReader& s);
};

struct MetroConstrainedBone {
    CharString          bone;
    uint8_t             look_at_axis;
    uint8_t             pos_axis;
    uint8_t             rot_axis;
    uint8_t             rotation_order;
    MetroParentBones    position;
    MetroParentBones    orientation;
    uint8_t             refresh_kids;
    bool                use_anim_poses;
    vec3                pos_limits[2];
    vec3                rot_limits[2];
    uint8_t             uptype;
    MetroParentBones    up;

    void Serialize(MetroReflectionReader& s);
};

class MetroSkeleton {
public:
    MetroSkeleton();
    ~MetroSkeleton();

    bool                    LoadFromData(MemStream& stream);

    size_t                  GetCRC() const;
    size_t                  GetNumBones() const;
    const quat&             GetBoneRotation(const size_t idx) const;
    const vec3&             GetBonePosition(const size_t idx) const;
    mat4                    GetBoneTransform(const size_t idx) const;
    mat4                    GetBoneFullTransform(const size_t idx) const;
    const size_t            GetBoneParentIdx(const size_t idx) const;
    const CharString&       GetBoneName(const size_t idx) const;
    size_t                  FindBone(const CharString& name) const;

    size_t                  GetNumLocators() const;
    const MetroAuxBone&     GetLocator(const size_t idx) const;

    size_t                  GetNumAuxBones() const;
    const MetroAuxBone&     GetAuxBone(const size_t idx) const;
    size_t                  FindAuxBone(const CharString& name) const;

    size_t                  GetNumDrivenBones() const;
    size_t                  GetNumConstrainedBones() const;
    const MetroConstrainedBone& GetConstrainedBone(const size_t idx) const;

    size_t                  GetNumProceduralRefs() const;
    const MetroProceduralRef& GetProceduralRef(const size_t idx) const;
    uint32_t                GetVersion() const;
    uint32_t                GetProceduralVersion() const;
    const MetroDrivenBone&  GetDrivenBone(const size_t idx) const;

    const CharString&       GetMotionsStr() const;

private:
    void                    DeserializeSelf(MetroReflectionReader& reader);

private:
    uint32_t                ver;
    uint32_t                crc;
    CharString              facefx;
    CharString              pfnn;
    bool                    has_as;
    bool                    bone_lods_allowed;
    CharString              motions;
    CharString              source_info;
    CharString              parent_skeleton;
    MyArray<ParentMapped>   parent_bone_maps;
    MyArray<MetroBone>      bones;
    MyArray<MetroAuxBone>   locators;
    MyArray<MetroAuxBone>   aux_bones;
    uint32_t                procedural_ver;
    MyArray<MetroProceduralRef> procedural_bones;
    MyArray<MetroDrivenBone>    driven_bones;
    MyArray<MetroDynamicBone>   dynamic_bones;
    MyArray<MetroConstrainedBone> constrained_bones;

    StringArray             mStringsDict;
};
