#pragma once
#include "MetroTypes.h"

class MotionBitset {
public:
    void                    Resize(const size_t numDwords) { mDwords.resize(numDwords, 0); }
    size_t                  GetSizeInBytes() const { return mDwords.size() * sizeof(uint32_t); }
    uint32_t*               GetData() { return mDwords.data(); }
    const uint32_t*         GetData() const { return mDwords.data(); }

    inline bool IsPresent(const size_t idx) const {
        const size_t i = idx >> 5;
        if (i >= mDwords.size()) {
            return false;
        }
        const uint32_t mask = 1u << (idx & 0x1F);
        return (mDwords[i] & mask) == mask;
    }

    inline size_t CountOnes() const {
        size_t result = 0;
        for (const uint32_t x : mDwords) {
            result += CountBitsU32(x);
        }
        return result;
    }

    inline size_t CountOnesBefore(const size_t idx) const {
        size_t result = 0;
        const size_t lastDword = idx >> 5;
        for (size_t i = 0; i < mDwords.size(); ++i) {
            if (i < lastDword) {
                result += CountBitsU32(mDwords[i]);
            } else if (i == lastDword) {
                const uint32_t mask = (1u << (idx & 0x1F)) - 1u;
                result += CountBitsU32(mDwords[i] & mask);
            } else {
                break;
            }
        }
        return result;
    }

private:
    MyArray<uint32_t>       mDwords;
};


class MetroCurve {
public:
    enum class Format : uint8_t {
        Unknown     = 0,
        Fp32        = 1,    // raw floats, knots then values
        ConstFp32   = 2,    // single constant float value, no knots
        ConstU16    = 3,    // single constant uint16 value, no knots
        V3U16       = 4,    // quantized vec3, scale + offset + u16 knots and values
        V4NU16      = 5,    // quantized quaternion, s16 xyz (w restored), u16 knots
        S1U16       = 6,    // stepped uint16 scalar, u16 knots
        Ident       = 7,    // no data at all, evaluates to the caller's default
        V1U16       = 8     // quantized scalar, scale + offset, u16 knots and values
    };

public:
    MetroCurve();

    bool                    Load(const uint8_t* curveData, const size_t dataLeft, const bool bigEndian);

    Format                  GetFormat() const { return mFormat; }
    size_t                  GetDimension() const { return mDimension; }
    size_t                  GetDegree() const { return mDegree; }
    size_t                  GetNumPoints() const { return mPoints.size(); }
    size_t                  GetSizeInBytes() const { return mSizeInBytes; }
    bool                    IsEmpty() const { return mFormat == Format::Unknown || mFormat == Format::Ident; }

    const MyArray<float>&   GetKnots() const { return mKnots; }
    const MyArray<vec4>&    GetPoints() const { return mPoints; }

    // evaluation, mimics curve::evaluate_at3 / evaluate_at4 / evaluate_at
    vec4                    Evaluate(const float time, const vec4& defaultValue) const;
    quat                    EvaluateQuat(const float time, const quat& defaultValue) const;
    uint16_t                EvaluateU16(const float time, const uint16_t defaultValue) const;

    // returns the amount of bytes a curve with the given header occupies
    static size_t           CalcSizeInBytes(const uint32_t header);

private:
    int                     FindKnot(const float time) const;
    bool                    ConstructBuffers(const int knotIdx, float(&knots)[4], vec4(&points)[3]) const;

private:
    Format                  mFormat;
    uint8_t                 mDimension;
    uint8_t                 mDegree;
    bool                    mNormalize;
    size_t                  mSizeInBytes;
    MyArray<float>          mKnots;
    MyArray<vec4>           mPoints;
};


struct MetroMotionFlags {
    enum : size_t {
        Looped      = 0x0002,
        Additive    = 0x0040,
    };
};

class MetroMotion {
public:
    static const size_t kFrameRate = 30;

public:
    MetroMotion(const CharString& name = "");
    ~MetroMotion();

    bool                    LoadHeader(MemStream& stream);
    bool                    LoadFromData(MemStream& stream);

    const CharString&       GetName() const;

    size_t                  GetBonesCRC() const;
    size_t                  GetNumBones() const;
    size_t                  GetNumLocators() const;
    size_t                  GetNumFrames() const;
    bool                    IsLooped() const;
    bool                    IsAdditive() const;
    float                   GetMotionTimeInSeconds() const;

    bool                    IsBoneAnimated(const size_t boneIdx) const;
    quat                    GetBoneRotation(const size_t boneIdx, const size_t key) const;
    vec3                    GetBonePosition(const size_t boneIdx, const size_t key) const;
    quat                    GetBoneRotationAtTime(const size_t boneIdx, const float time) const;
    vec3                    GetBonePositionAtTime(const size_t boneIdx, const float time) const;

    const MetroCurve&       GetBoneRotationCurve(const size_t boneIdx) const;
    const MetroCurve&       GetBonePositionCurve(const size_t boneIdx) const;
    const MetroCurve&       GetBoneScaleCurve(const size_t boneIdx) const;
    vec3                    GetBoneScaleAtTime(const size_t boneIdx, const float time) const;

private:
    struct ChunkInfo {
        size_t id;
        size_t offset;
        size_t size;
    };

    static void             CollectChunks(MemStream& stream, MyArray<ChunkInfo>& chunks);
    static const ChunkInfo* FindChunk(const MyArray<ChunkInfo>& chunks, const size_t id);

    bool                    ReadInfoChunk(MemStream& stream, const ChunkInfo& info, const size_t dataChunkSize);
    bool                    LoadInternal();
    bool                    ReadMotionDataHeader(size_t& outHeaderSize, size_t& outNumOffsets, size_t& outNumSwapped);

private:
    CharString              mName;

    // header
    size_t                  mVersion;
    size_t                  mBonesCRC;
    size_t                  mNumBones;
    size_t                  mNumLocators;
    // info
    size_t                  mFlags;
    float                   mSpeed;
    float                   mAccrue;
    float                   mFalloff;
    size_t                  mNumFrames;
    size_t                  mJumpFrame;
    size_t                  mLandFrame;
    MotionBitset            mAffectedBones;
    size_t                  mMotionsDataSize;
    size_t                  mMotionsOffsetsSize;
    MotionBitset            mHighQualityBones;
    // data
    BytesArray              mMotionsData;
    MotionBitset            mAnimatedBones;     // motion_data::animated_bones
    size_t                  mDataNumLocators;
    size_t                  mDataXformPresent;
    // curves, indexed by the skeleton bone index
    MyArray<MetroCurve>     mBonesRotations;
    MyArray<MetroCurve>     mBonesPositions;
    MyArray<MetroCurve>     mBonesScales;
};
