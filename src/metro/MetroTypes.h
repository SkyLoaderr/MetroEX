#pragma once
#include "mycommon.h"
#include "mymath.h"

PACKED_STRUCT_BEGIN
struct MetroGuid {  // 16 bytes
    uint32_t    a;
    uint16_t    b;
    uint16_t    c;
    uint16_t    d;
    uint8_t     e[6];
} PACKED_STRUCT_END;

struct MetroFile {
public:
    enum : size_t {
        Flag_Unknown4   = 4,    // patch folders have this one
        Flag_Folder     = 8,
    };

public:
    struct iterator {
        friend MetroFile;
    private:
        iterator(const size_t _idx) : idx(_idx) {}

    public:
        inline size_t operator *() const {
            return this->idx;
        }
        inline bool operator ==(const iterator& other) {
            return this->idx == other.idx;
        }
        inline bool operator !=(const iterator& other) {
            return this->idx != other.idx;
        }
        inline iterator& operator ++() {
            ++this->idx;
            return *this;
        }
        inline iterator operator ++(int) {
            iterator prev(this->idx);
            ++this->idx;
            return prev;
        }

    private:
        size_t idx;
    };

public:
    static const size_t InvalidFileIdx = kInvalidValue;

    inline bool IsFile() const {
        return !TestBit<size_t>(this->flags, Flag_Folder);
    }

    inline bool ContainsFile(const size_t fileIdx) const {
        return this->IsFile() ? false : (fileIdx >= this->firstFile && fileIdx < (this->firstFile + this->numFiles));
    }

    //#NOTE_SK: iteration over files
    //          using standard names so that modern for(i : a) syntaxis works
    iterator begin() const {
        return (this->IsFile() ? iterator(kInvalidValue) : iterator(this->firstFile));
    }
    iterator end() const {
        return (this->IsFile() ? iterator(kInvalidValue) : iterator(this->firstFile + this->numFiles));
    }

    // common fields
    size_t      idx;
    size_t      flags;
    CharString  name;

    // file fields
    size_t      pakIdx;
    size_t      offset;
    size_t      sizeUncompressed;
    size_t      sizeCompressed;

    // dir fields
    size_t      firstFile;
    size_t      numFiles;

    // duplication
    size_t      baseIdx;
    size_t      duplicates;

    // fs linkage
    size_t      overrideIdx;
};


PACKED_STRUCT_BEGIN
struct MetroFace {
    uint16_t a, b, c;
} PACKED_STRUCT_END;

PACKED_STRUCT_BEGIN
struct MetroVertex {
    vec3        pos;
    uint8_t     bones[4];
    vec4        normal;
    uint8_t     weights[4];
    vec2        uv0;
    vec2        uv1;
} PACKED_STRUCT_END;

struct MetroVertexType {
    enum VertexType : uint32_t {
        Invalid     = 0,
        Skin        = 1,
        Static      = 2,
        Level       = 3,
        LevelLegacy = 4,

        Particle    = 16,
        Soft        = 17,
        Impostor    = 19,

        SkinNB      = 25
    };
};

struct MetroMesh {
    size_t               version;
    bool                 skinned;
    bool                 isCollision;
    size_t               flags;
    uint16_t             materialFlags0;
    uint16_t             materialFlags1;
    float                vscale;
    AABBox               bbox;
    size_t               type;
    size_t               shaderId;
    StringArray          materials;
    MyArray<MetroFace>   faces;
    MyArray<MetroVertex> vertices;
    BytesArray           bonesRemap;
};


// all these vertices are 32 bytes each
PACKED_STRUCT_BEGIN
struct VertexStatic {
    vec3     pos;
    uint32_t normal;
    uint32_t aux0;
    uint32_t aux1;
    vec2     uv;
} PACKED_STRUCT_END;

PACKED_STRUCT_BEGIN
struct VertexSkinned {
    int16_t  pos[4];
    uint32_t normal;
    uint32_t aux0;
    uint32_t aux1;
    uint8_t  bones[4];
    uint8_t  weights[4];
    int16_t  uv[2];
} PACKED_STRUCT_END;

PACKED_STRUCT_BEGIN
struct VertexLevel {
    vec3     pos;
    uint32_t normal;
    uint32_t aux0;
    uint32_t aux1;
    int16_t  uv0[2];
    int16_t  uv1[2];
} PACKED_STRUCT_END;

PACKED_STRUCT_BEGIN
struct VertexSoft {
    vec3     pos;
    uint32_t aux0;
    vec3     normal;
    int16_t  uv[2];
} PACKED_STRUCT_END;

PACKED_STRUCT_BEGIN
struct VertexStaticShadow {
    vec3     pos;
    uint32_t padding;
} PACKED_STRUCT_END;

PACKED_STRUCT_BEGIN
struct VertexSkinnedShadow {
    int16_t  pos[4];
    uint8_t  bones[4];
    uint8_t  weights[4];
} PACKED_STRUCT_END;

static_assert(sizeof(VertexStatic) == 32);
static_assert(sizeof(VertexSkinned) == 32);
static_assert(sizeof(VertexLevel) == 32);
static_assert(sizeof(VertexSoft) == 32);
static_assert(sizeof(VertexStaticShadow) == 16);
static_assert(sizeof(VertexSkinnedShadow) == 16);


static vec4 DecodeNormal(const uint32_t n) {
    const float div255 = 1.0f / 255.0f;
    const float x = (scast<float>((n & 0x00FF0000) >> 16) * div255) * 2.0f - 1.0f;
    const float y = (scast<float>((n & 0x0000FF00) >>  8) * div255) * 2.0f - 1.0f;
    const float z = (scast<float>((n & 0x000000FF) >>  0) * div255) * 2.0f - 1.0f;
    const float w = (scast<float>((n & 0xFF000000) >> 24) * div255); // vao

    return vec4(x, y, z, w);
}

inline vec3 MetroSwizzle(const vec3& v) {
    return vec3(v.z, v.y, v.x);
}

inline vec4 MetroSwizzle(const vec4& v) {
    return vec4(v.z, v.y, v.x, v.w);
}

inline quat MetroSwizzle(const quat& q) {
    return quat(q.w, q.z, q.y, q.x);
}

inline void MetroSwizzle(uint8_t* a) {
    std::swap(a[0], a[2]);
}

template <typename T>
inline MetroVertex ConvertVertex(const T&) {
    assert(false);
}

template <>
inline MetroVertex ConvertVertex<VertexStatic>(const VertexStatic& v) {
    MetroVertex result = {};

    result.pos = MetroSwizzle(v.pos);
    result.normal = MetroSwizzle(DecodeNormal(v.normal));
    result.uv0 = v.uv;

    return result;
}

template <>
inline MetroVertex ConvertVertex<VertexSkinned>(const VertexSkinned& v) {
    const float posDequant = 1.0f / 32767.0f;
    const float uvDequant = 1.0f / 2048.0f;

    MetroVertex result = {};

    result.pos = MetroSwizzle(vec3(scast<float>(v.pos[0]) * posDequant,
                                   scast<float>(v.pos[1]) * posDequant,
                                   scast<float>(v.pos[2]) * posDequant));
    result.bones[0] = v.bones[0] / 3;
    result.bones[1] = v.bones[1] / 3;
    result.bones[2] = v.bones[2] / 3;
    result.bones[3] = v.bones[3] / 3;
    MetroSwizzle(result.bones);
    result.normal = MetroSwizzle(DecodeNormal(v.normal));
    *rcast<uint32_t*>(result.weights) = *rcast<const uint32_t*>(v.weights);
    MetroSwizzle(result.weights);
    result.uv0 = vec2(scast<float>(v.uv[0]) * uvDequant,
                      scast<float>(v.uv[1]) * uvDequant);
    return result;
}

template <>
inline MetroVertex ConvertVertex<VertexLevel>(const VertexLevel& v) {
    const float kUV0Dequant = 1.0f / 1024.0f;
    const float kUV1Dequant = 1.0f / 32767.0f;

    MetroVertex result = {};

    result.pos = MetroSwizzle(v.pos);
    result.normal = MetroSwizzle(DecodeNormal(v.normal));
    result.uv0 = vec2(scast<float>(v.uv0[0]) * kUV0Dequant,
                      scast<float>(v.uv0[1]) * kUV0Dequant);
    result.uv1 = vec2(scast<float>(v.uv1[0]) * kUV1Dequant,
                      scast<float>(v.uv1[1]) * kUV1Dequant);
    return result;
}

template <>
inline MetroVertex ConvertVertex<VertexSoft>(const VertexSoft& v) {
    const float uvDequant = 1.0f / 2048.0f;

    MetroVertex result = {};

    result.pos = MetroSwizzle(v.pos);
    result.normal = MetroSwizzle(vec4(v.normal, 0.0f));
    result.uv0 = vec2(scast<float>(v.uv[0]) * uvDequant,
                      scast<float>(v.uv[1]) * uvDequant);
    return result;
}

template <>
inline MetroVertex ConvertVertex<VertexStaticShadow>(const VertexStaticShadow& v) {
    MetroVertex result = {};
    result.pos = MetroSwizzle(v.pos);
    return result;
}

template <>
inline MetroVertex ConvertVertex<VertexSkinnedShadow>(const VertexSkinnedShadow& v) {
    const float posDequant = 1.0f / 32767.0f;

    MetroVertex result = {};

    result.pos = MetroSwizzle(vec3(scast<float>(v.pos[0]) * posDequant,
                                   scast<float>(v.pos[1]) * posDequant,
                                   scast<float>(v.pos[2]) * posDequant));
    result.bones[0] = v.bones[0] / 3;
    result.bones[1] = v.bones[1] / 3;
    result.bones[2] = v.bones[2] / 3;
    result.bones[3] = v.bones[3] / 3;
    MetroSwizzle(result.bones);
    *rcast<uint32_t*>(result.weights) = *rcast<const uint32_t*>(v.weights);
    MetroSwizzle(result.weights);

    return result;
}


enum class MetroBodyPart : size_t {
    Invalid     = 0,
    Generic     = 1,
    Head        = 2,
    Stomach     = 3,
    Leftarm     = 4,
    Rightarm    = 5,
    Leftleg     = 6,
    Rightleg    = 7,
    Chest       = 8,
    Gear        = 9
};

enum class MetroLanguage : size_t {
    CN = 0,
    CZ,
    DE,
    ES,
    FR,
    IT,
    JP,
    KO,
    MX,
    PL,
    PT,
    RU,
    TW,
    UK,
    US,

    First = CN,
    Last = US,
    Count = Last + 1
};

static const CharString MetroLanguagesStr[scast<size_t>(MetroLanguage::Count)] = {
    "cn",
    "cz",
    "de",
    "es",
    "fr",
    "it",
    "jp",
    "ko",
    "mx",
    "pl",
    "pt",
    "ru",
    "tw",
    "uk",
    "us"
};

struct MetroSurfaceDescription {
    CharString  albedoPath;
    CharString  bumpPath;
    CharString  normalmapPath;
    CharString  detailPath;
};
