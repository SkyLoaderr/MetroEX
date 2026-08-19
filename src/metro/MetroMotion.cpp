#include "MetroMotion.h"

enum MotionChunks {
    MC_HeaderChunk  = 0x00000000,
    MC_InfoChunk    = 0x00000001,
    MC_DataChunk    = 0x00000009,
};

static inline uint16_t ReadU16(const uint8_t* p, const bool bigEndian) {
    return bigEndian ? scast<uint16_t>((scast<uint16_t>(p[0]) << 8) | p[1])
                     : scast<uint16_t>((scast<uint16_t>(p[1]) << 8) | p[0]);
}

static inline uint32_t ReadU32(const uint8_t* p, const bool bigEndian) {
    return bigEndian ? ((scast<uint32_t>(p[0]) << 24) | (scast<uint32_t>(p[1]) << 16) | (scast<uint32_t>(p[2]) << 8) | p[3])
                     : ((scast<uint32_t>(p[3]) << 24) | (scast<uint32_t>(p[2]) << 16) | (scast<uint32_t>(p[1]) << 8) | p[0]);
}

static inline float ReadF32(const uint8_t* p, const bool bigEndian) {
    const uint32_t u = ReadU32(p, bigEndian);
    float result;
    memcpy(&result, &u, sizeof(result));
    return result;
}

static inline size_t Align4(const size_t x) {
    return (x + 3) & ~scast<size_t>(3);
}



MetroCurve::MetroCurve()
    : mFormat(Format::Unknown)
    , mDimension(0)
    , mDegree(0)
    , mNormalize(false)
    , mSizeInBytes(0)
{
}

size_t MetroCurve::CalcSizeInBytes(const uint32_t header) {
    const size_t numKnots = scast<size_t>(header & 0xFFFF);
    const size_t format = scast<size_t>((header >> 16) & 0xF);
    const size_t dimension = scast<size_t>((header >> 24) & 0xF);

    switch (scast<Format>(format)) {
        case Format::Fp32:      return 4 + numKnots * (dimension + 1) * sizeof(float);
        case Format::ConstFp32: return 4 + numKnots * dimension * sizeof(float);
        case Format::ConstU16:  return Align4(4 + numKnots * dimension * sizeof(uint16_t));
        case Format::V3U16:     return 32 + numKnots * 8;
        case Format::V4NU16:    return 8 + numKnots * 8;
        case Format::S1U16:     return 8 + numKnots * (1 + dimension) * sizeof(uint16_t);
        case Format::Ident:     return 4;
        case Format::V1U16:     return 16 + numKnots * (1 + dimension) * sizeof(uint16_t);
        default:                return 0;
    }
}

bool MetroCurve::Load(const uint8_t* curveData, const size_t dataLeft, const bool bigEndian) {
    mFormat = Format::Unknown;
    mDimension = 0;
    mDegree = 0;
    mNormalize = false;
    mSizeInBytes = 0;
    mKnots.clear();
    mPoints.clear();

    if (dataLeft < 4) {
        return false;
    }

    const uint32_t header = ReadU32(curveData, bigEndian);

    const size_t numKnots = scast<size_t>(header & 0xFFFF);
    const Format format = scast<Format>((header >> 16) & 0xF);

    mDimension = scast<uint8_t>((header >> 24) & 0xF);
    mDegree = scast<uint8_t>((header >> 20) & 0xF);
    mNormalize = ((header >> 28) & 0x1) != 0;

    const size_t sizeInBytes = MetroCurve::CalcSizeInBytes(header);
    if (!sizeInBytes || sizeInBytes > dataLeft || mDimension > 4) {
        return false;
    }

    mFormat = format;
    mSizeInBytes = sizeInBytes;

    const uint8_t* ptr = curveData + 4;

    const bool needsSwizzle = (mDimension >= 3);

    switch (format) {
        case Format::Ident: {
        } break;

        case Format::Fp32: {
            mKnots.resize(numKnots);
            mPoints.resize(numKnots, vec4(0.0f));

            const uint8_t* valuesPtr = ptr + numKnots * sizeof(float);
            for (size_t i = 0; i < numKnots; ++i) {
                mKnots[i] = ReadF32(ptr + i * sizeof(float), bigEndian);
                for (size_t c = 0; c < mDimension; ++c) {
                    mPoints[i][scast<int>(c)] = ReadF32(valuesPtr + (i * mDimension + c) * sizeof(float), bigEndian);
                }

                if (needsSwizzle) {
                    mPoints[i] = MetroSwizzle(mPoints[i]);
                }
            }
        } break;

        case Format::ConstFp32: {
            mPoints.resize(1, vec4(0.0f));
            for (size_t c = 0; c < mDimension; ++c) {
                mPoints[0][scast<int>(c)] = ReadF32(ptr + c * sizeof(float), bigEndian);
            }

            if (needsSwizzle) {
                mPoints[0] = MetroSwizzle(mPoints[0]);
            }
        } break;

        case Format::ConstU16: {
            mPoints.resize(1, vec4(0.0f));
            for (size_t c = 0; c < mDimension; ++c) {
                mPoints[0][scast<int>(c)] = scast<float>(ReadU16(ptr + c * sizeof(uint16_t), bigEndian));
            }
        } break;

        case Format::V3U16: {
            const float oneOverKnotScale = ReadF32(ptr, bigEndian);
            const float knotScale = (oneOverKnotScale != 0.0f) ? (1.0f / oneOverKnotScale) : 0.0f;

            vec3 scale, offset;
            for (size_t c = 0; c < 3; ++c) {
                scale[scast<int>(c)] = ReadF32(ptr + 4 + c * sizeof(float), bigEndian);
                offset[scast<int>(c)] = ReadF32(ptr + 16 + c * sizeof(float), bigEndian);
            }

            const uint8_t* knotsPtr = curveData + 32;
            const uint8_t* valuesPtr = knotsPtr + numKnots * sizeof(uint16_t);

            mKnots.resize(numKnots);
            mPoints.resize(numKnots, vec4(0.0f));
            for (size_t i = 0; i < numKnots; ++i) {
                mKnots[i] = scast<float>(ReadU16(knotsPtr + i * sizeof(uint16_t), bigEndian)) * knotScale;

                vec3 v;
                for (size_t c = 0; c < 3; ++c) {
                    v[scast<int>(c)] = scast<float>(ReadU16(valuesPtr + (i * 3 + c) * sizeof(uint16_t), bigEndian)) * scale[scast<int>(c)] + offset[scast<int>(c)];
                }

                mPoints[i] = vec4(MetroSwizzle(v), 0.0f);
            }
        } break;

        case Format::V4NU16: {
            const float normFactor = 0.0000215782f;

            const float oneOverKnotScale = ReadF32(ptr, bigEndian);
            const float knotScale = (oneOverKnotScale != 0.0f) ? (1.0f / oneOverKnotScale) : 0.0f;

            const uint8_t* knotsPtr = curveData + 8;
            const uint8_t* valuesPtr = knotsPtr + numKnots * sizeof(uint16_t);

            mKnots.resize(numKnots);
            mPoints.resize(numKnots, vec4(0.0f));
            for (size_t i = 0; i < numKnots; ++i) {
                mKnots[i] = scast<float>(ReadU16(knotsPtr + i * sizeof(uint16_t), bigEndian)) * knotScale;

                const int16_t v0 = scast<int16_t>(ReadU16(valuesPtr + (i * 3 + 0) * sizeof(uint16_t), bigEndian));
                const int16_t v1 = scast<int16_t>(ReadU16(valuesPtr + (i * 3 + 1) * sizeof(uint16_t), bigEndian));
                const int16_t v2 = scast<int16_t>(ReadU16(valuesPtr + (i * 3 + 2) * sizeof(uint16_t), bigEndian));

                const int permutation = ((v0 & 1) << 1) | (v1 & 1);
                const bool wNegative = (v2 & 1) != 0;

                const float qa = scast<float>(v0) * normFactor;
                const float qb = scast<float>(v1) * normFactor;
                const float qc = scast<float>(v2) * normFactor;

                const float t = 1.0f - (qa * qa) - (qb * qb) - (qc * qc);
                const float qd = (t <= 0.0f) ? 0.0f : (wNegative ? -std::sqrtf(t) : std::sqrtf(t));

                vec4 value;
                switch (permutation) {
                    case 0:  value = vec4(qd, qa, qb, qc); break;
                    case 1:  value = vec4(qa, qd, qb, qc); break;
                    case 2:  value = vec4(qa, qb, qd, qc); break;
                    default: value = vec4(qa, qb, qc, qd); break;
                }

                mPoints[i] = MetroSwizzle(value);
            }
        } break;

        case Format::S1U16: {
            const float oneOverKnotScale = ReadF32(ptr, bigEndian);
            const float knotScale = (oneOverKnotScale != 0.0f) ? (1.0f / oneOverKnotScale) : 0.0f;

            const uint8_t* knotsPtr = curveData + 8;
            const uint8_t* valuesPtr = knotsPtr + numKnots * sizeof(uint16_t);

            mKnots.resize(numKnots);
            mPoints.resize(numKnots, vec4(0.0f));
            for (size_t i = 0; i < numKnots; ++i) {
                mKnots[i] = scast<float>(ReadU16(knotsPtr + i * sizeof(uint16_t), bigEndian)) * knotScale;
                for (size_t c = 0; c < mDimension; ++c) {
                    mPoints[i][scast<int>(c)] = scast<float>(ReadU16(valuesPtr + (i * mDimension + c) * sizeof(uint16_t), bigEndian));
                }
            }
        } break;

        case Format::V1U16: {
            const float oneOverKnotScale = ReadF32(ptr, bigEndian);
            const float knotScale = (oneOverKnotScale != 0.0f) ? (1.0f / oneOverKnotScale) : 0.0f;
            const float scale = ReadF32(ptr + 4, bigEndian);
            const float offset = ReadF32(ptr + 8, bigEndian);

            const uint8_t* knotsPtr = curveData + 16;
            const uint8_t* valuesPtr = knotsPtr + numKnots * sizeof(uint16_t);

            mKnots.resize(numKnots);
            mPoints.resize(numKnots, vec4(0.0f));
            for (size_t i = 0; i < numKnots; ++i) {
                mKnots[i] = scast<float>(ReadU16(knotsPtr + i * sizeof(uint16_t), bigEndian)) * knotScale;
                mPoints[i].x = scast<float>(ReadU16(valuesPtr + i * sizeof(uint16_t), bigEndian)) * scale + offset;
            }
        } break;

        default: {
            mFormat = Format::Unknown;
            return false;
        }
    }

    return true;
}

int MetroCurve::FindKnot(const float time) const {
    const int numKnots = scast<int>(mKnots.size());

    int idx = 0;
    while (idx < numKnots && mKnots[idx] <= time) {
        ++idx;
    }

    if (idx == numKnots && idx != 0) {
        --idx;
    }

    return idx;
}

bool MetroCurve::ConstructBuffers(const int knotIdx, float(&knots)[4], vec4(&points)[3]) const {
    const int numKnots = scast<int>(mKnots.size());
    const int start = knotIdx - scast<int>(mDegree);
    const bool clamped = (start < 0) || ((knotIdx + scast<int>(mDegree)) > numKnots);

    for (int i = 0; i < 4; ++i) {
        const int idx = std::min<int>(std::max<int>(start + i, 0), numKnots - 1);
        knots[i] = mKnots[scast<size_t>(idx)];
        if (i < 3) {
            points[i] = mPoints[scast<size_t>(idx)];
        }
    }

    return clamped;
}

static inline float SafeRatio(const float num, const float den) {
    return (den != 0.0f) ? (num / den) : 0.0f;
}

vec4 MetroCurve::Evaluate(const float time, const vec4& defaultValue) const {
    if (mFormat == Format::Ident || mPoints.empty()) {
        return defaultValue;
    }

    if (mFormat == Format::ConstFp32 || mFormat == Format::ConstU16 || mKnots.empty()) {
        return mPoints.front();
    }

    if (mFormat == Format::S1U16) {
        const int idx = std::min<int>(std::max<int>(this->FindKnot(time), 0), scast<int>(mPoints.size()) - 1);
        return mPoints[scast<size_t>(idx)];
    }

    int knotIdx = this->FindKnot(time);
    if (knotIdx >= scast<int>(mKnots.size())) {
        knotIdx = scast<int>(mKnots.size()) - 1;
    }

    float knots[4];
    vec4 points[3];
    this->ConstructBuffers(knotIdx, knots, points);

    const float u = SafeRatio(time - knots[0], knots[2] - knots[0]);
    const float v = SafeRatio(time - knots[1], knots[2] - knots[1]);
    const float s = SafeRatio(time - knots[1], knots[3] - knots[1]);

    const float b = (u + v) - (u * v);
    const float w2 = s * v;
    const float w1 = b - w2;
    const float w0 = 1.0f - b;

    return points[0] * w0 + points[1] * w1 + points[2] * w2;
}

quat MetroCurve::EvaluateQuat(const float time, const quat& defaultValue) const {
    if (mFormat == Format::Ident || mPoints.empty()) {
        return defaultValue;
    }

    if (mFormat == Format::ConstFp32 || mFormat == Format::ConstU16 || mKnots.empty()) {
        const vec4& v = mPoints.front();
        return quat(v.w, v.x, v.y, v.z);
    }

    int knotIdx = this->FindKnot(time);
    if (knotIdx >= scast<int>(mKnots.size())) {
        knotIdx = scast<int>(mKnots.size()) - 1;
    }

    float knots[4];
    vec4 points[3];
    const bool clamped = this->ConstructBuffers(knotIdx, knots, points);

    if (clamped && mNormalize) {
        vec4 prev(0.0f);
        for (int i = 0; i < 3; ++i) {
            if (Dot(points[i], prev) < 0.0f) {
                points[i] = -points[i];
            }
            prev = points[i];
        }
    }

    const float u = SafeRatio(time - knots[0], knots[2] - knots[0]);
    const float v = SafeRatio(time - knots[1], knots[2] - knots[1]);
    const float s = SafeRatio(time - knots[1], knots[3] - knots[1]);

    const float b = (u + v) - (u * v);
    const float w2 = s * v;
    const float w1 = b - w2;
    const float w0 = 1.0f - b;

    const vec4 result = points[0] * w0 + points[1] * w1 + points[2] * w2;

    return Normalize(quat(result.w, result.x, result.y, result.z));
}

uint16_t MetroCurve::EvaluateU16(const float time, const uint16_t defaultValue) const {
    if (mFormat == Format::Ident || mPoints.empty()) {
        return defaultValue;
    }

    if (mFormat == Format::ConstU16 || mFormat == Format::ConstFp32 || mKnots.empty()) {
        return scast<uint16_t>(mPoints.front().x);
    }

    const int idx = std::min<int>(std::max<int>(this->FindKnot(time), 0), scast<int>(mPoints.size()) - 1);
    return scast<uint16_t>(mPoints[scast<size_t>(idx)].x);
}



MetroMotion::MetroMotion(const CharString& name)
    : mName(name)
    // header
    , mVersion(0)
    , mBonesCRC(0)
    , mNumBones(0)
    , mNumLocators(0)
    // info
    , mFlags(0)
    , mSpeed(0.0f)
    , mAccrue(0.0f)
    , mFalloff(0.0f)
    , mNumFrames(0)
    , mJumpFrame(0)
    , mLandFrame(0)
    , mAffectedBones()
    , mMotionsDataSize(0)
    , mMotionsOffsetsSize(0)
    , mHighQualityBones()
    , mAnimatedBones()
    , mDataNumLocators(0)
    , mDataXformPresent(0)
{

}
MetroMotion::~MetroMotion() {

}

void MetroMotion::CollectChunks(MemStream& stream, MyArray<MetroMotion::ChunkInfo>& chunks) {
    const size_t startCursor = stream.GetCursor();

    while (stream.Remains() >= 8) {
        const size_t chunkId = stream.ReadTyped<uint32_t>();
        const size_t chunkSize = stream.ReadTyped<uint32_t>();
        const size_t chunkOffset = stream.GetCursor();

        if (chunkOffset + chunkSize > stream.Length()) {
            break;
        }

        chunks.push_back({ chunkId, chunkOffset, chunkSize });
        stream.SetCursor(chunkOffset + chunkSize);
    }

    stream.SetCursor(startCursor);
}

const MetroMotion::ChunkInfo* MetroMotion::FindChunk(const MyArray<MetroMotion::ChunkInfo>& chunks, const size_t id) {
    for (const ChunkInfo& c : chunks) {
        if (c.id == id) {
            return &c;
        }
    }
    return nullptr;
}

bool MetroMotion::ReadInfoChunk(MemStream& stream, const ChunkInfo& info, const size_t dataChunkSize) {
    stream.SetCursor(info.offset);

    mFlags = stream.ReadTyped<uint16_t>();

    mSpeed = stream.ReadTyped<float>();
    mAccrue = stream.ReadTyped<float>();
    mFalloff = stream.ReadTyped<float>();

    mNumFrames = stream.ReadTyped<uint32_t>();
    mJumpFrame = stream.ReadTyped<uint16_t>();
    mLandFrame = stream.ReadTyped<uint16_t>();

    const size_t kFixedSizeOld = 22;    // flags, speed, accrue, falloff, frames, jump, land
    const size_t kFixedSizeNew = 26;    // ... + separate jump/land for rotation

    size_t candidates[2];
    if (mVersion >= 19) {
        candidates[0] = kFixedSizeNew; candidates[1] = kFixedSizeOld;
    } else {
        candidates[0] = kFixedSizeOld; candidates[1] = kFixedSizeNew;
    }

    size_t fixedSize = 0, bitsetDwords = 0;
    for (const size_t candidate : candidates) {
        if (info.size < candidate + 8) {
            continue;
        }
        const size_t rest = info.size - candidate - 8;
        if ((rest % 8) != 0) {
            continue;
        }
        const size_t dwords = rest / 8;

        stream.SetCursor(info.offset + candidate + dwords * sizeof(uint32_t));
        const size_t dataSize = stream.ReadTyped<uint32_t>();
        if (dataChunkSize && dataSize != dataChunkSize) {
            continue;
        }

        fixedSize = candidate;
        bitsetDwords = dwords;
        break;
    }

    if (!fixedSize) {
        return false;
    }

    stream.SetCursor(info.offset + fixedSize);

    mAffectedBones.Resize(bitsetDwords);
    stream.ReadToBuffer(mAffectedBones.GetData(), mAffectedBones.GetSizeInBytes());

    mMotionsDataSize = stream.ReadTyped<uint32_t>();
    mMotionsOffsetsSize = stream.ReadTyped<uint32_t>();

    mHighQualityBones.Resize(bitsetDwords);
    stream.ReadToBuffer(mHighQualityBones.GetData(), mHighQualityBones.GetSizeInBytes());

    return true;
}

bool MetroMotion::LoadHeader(MemStream& stream) {
    MyArray<ChunkInfo> chunks;
    MetroMotion::CollectChunks(stream, chunks);

    const ChunkInfo* hdr = MetroMotion::FindChunk(chunks, MC_HeaderChunk);
    const ChunkInfo* info = MetroMotion::FindChunk(chunks, MC_InfoChunk);
    const ChunkInfo* data = MetroMotion::FindChunk(chunks, MC_DataChunk);

    if (!hdr || !info || hdr->size < 12) {
        return false;
    }

    stream.SetCursor(hdr->offset);
    mVersion = stream.ReadTyped<uint32_t>();
    mBonesCRC = stream.ReadTyped<uint32_t>();
    mNumBones = stream.ReadTyped<uint16_t>();
    mNumLocators = stream.ReadTyped<uint16_t>();

    return this->ReadInfoChunk(stream, *info, data ? data->size : 0);
}

bool MetroMotion::LoadFromData(MemStream& stream) {
    const size_t startCursor = stream.GetCursor();

    if (!this->LoadHeader(stream)) {
        return false;
    }

    MyArray<ChunkInfo> chunks;
    stream.SetCursor(startCursor);
    MetroMotion::CollectChunks(stream, chunks);

    const ChunkInfo* data = MetroMotion::FindChunk(chunks, MC_DataChunk);

    bool dataFound = false;
    if (data && data->size == mMotionsDataSize) {
        mMotionsData.resize(mMotionsDataSize);
        stream.SetCursor(data->offset);
        stream.ReadToBuffer(mMotionsData.data(), mMotionsData.size());
        dataFound = true;
    }

    const bool result = dataFound && this->LoadInternal();

    mMotionsData.clear();
    mMotionsData.shrink_to_fit();

    return result;
}

const CharString& MetroMotion::GetName() const {
    return mName;
}

size_t MetroMotion::GetBonesCRC() const {
    return mBonesCRC;
}

size_t MetroMotion::GetNumBones() const {
    return mNumBones;
}

size_t MetroMotion::GetNumLocators() const {
    return mNumLocators;
}

size_t MetroMotion::GetNumFrames() const {
    return mNumFrames;
}

bool MetroMotion::IsLooped() const {
    return !!(mFlags & MetroMotionFlags::Looped);
}

bool MetroMotion::IsAdditive() const {
    return !!(mFlags & MetroMotionFlags::Additive);
}

float MetroMotion::GetMotionTimeInSeconds() const {
    return scast<float>(mNumFrames) / scast<float>(kFrameRate);
}

bool MetroMotion::IsBoneAnimated(const size_t boneIdx) const {
    return mAnimatedBones.IsPresent(boneIdx);
}

const MetroCurve& MetroMotion::GetBoneRotationCurve(const size_t boneIdx) const {
    static const MetroCurve kEmptyCurve;
    return (boneIdx < mBonesRotations.size()) ? mBonesRotations[boneIdx] : kEmptyCurve;
}

const MetroCurve& MetroMotion::GetBonePositionCurve(const size_t boneIdx) const {
    static const MetroCurve kEmptyCurve;
    return (boneIdx < mBonesPositions.size()) ? mBonesPositions[boneIdx] : kEmptyCurve;
}

const MetroCurve& MetroMotion::GetBoneScaleCurve(const size_t boneIdx) const {
    static const MetroCurve kEmptyCurve;
    return (boneIdx < mBonesScales.size()) ? mBonesScales[boneIdx] : kEmptyCurve;
}

vec3 MetroMotion::GetBoneScaleAtTime(const size_t boneIdx, const float time) const {
    const vec4 v = this->GetBoneScaleCurve(boneIdx).Evaluate(time, vec4(1.0f, 1.0f, 1.0f, 0.0f));
    return vec3(v);
}

quat MetroMotion::GetBoneRotationAtTime(const size_t boneIdx, const float time) const {
    static const quat kIdentity(1.0f, 0.0f, 0.0f, 0.0f);
    return this->GetBoneRotationCurve(boneIdx).EvaluateQuat(time, kIdentity);
}

vec3 MetroMotion::GetBonePositionAtTime(const size_t boneIdx, const float time) const {
    const vec4 v = this->GetBonePositionCurve(boneIdx).Evaluate(time, vec4(0.0f));
    return vec3(v);
}

quat MetroMotion::GetBoneRotation(const size_t boneIdx, const size_t key) const {
    return this->GetBoneRotationAtTime(boneIdx, scast<float>(key) / scast<float>(kFrameRate));
}

vec3 MetroMotion::GetBonePosition(const size_t boneIdx, const size_t key) const {
    return this->GetBonePositionAtTime(boneIdx, scast<float>(key) / scast<float>(kFrameRate));
}

bool MetroMotion::ReadMotionDataHeader(size_t& outHeaderSize, size_t& outNumOffsets, size_t& outNumSwapped) {
    const size_t bitsetBytes = mAffectedBones.GetSizeInBytes();
    const size_t headerSize = bitsetBytes + 16;
    const size_t numDwords = bitsetBytes / sizeof(uint32_t);

    if (!bitsetBytes || mMotionsData.size() < headerSize) {
        return false;
    }

    const uint8_t* ptr = mMotionsData.data();

    const bool endiannessOrder[2] = { false, true };
    for (const bool bigEndian : endiannessOrder) {
        mAnimatedBones.Resize(numDwords);
        for (size_t i = 0; i < numDwords; ++i) {
            mAnimatedBones.GetData()[i] = ReadU32(ptr + i * sizeof(uint32_t), bigEndian);
        }

        mDataNumLocators = ReadU16(ptr + bitsetBytes + 0, bigEndian);
        mDataXformPresent = ReadU16(ptr + bitsetBytes + 2, bigEndian);

        const size_t numAnimatedBones = mAnimatedBones.CountOnes();
        const size_t xform = mDataXformPresent ? 1 : 0;

        // uskeleton::motion_data::compute_offsets_size
        const size_t numOffsets = numAnimatedBones * 3 + mDataNumLocators * 4 + xform * 3;

        if (mMotionsOffsetsSize != headerSize + numOffsets * sizeof(uint32_t) ||
            mMotionsData.size() < mMotionsOffsetsSize) {
            continue;
        }

        if (numOffsets && ReadU32(ptr + headerSize, bigEndian) != mMotionsOffsetsSize) {
            continue;
        }

        outNumOffsets = numOffsets;
        outHeaderSize = headerSize;
        outNumSwapped = bigEndian ? (numAnimatedBones * 2 + mDataNumLocators * 3 + xform * 2) : 0;
        return true;
    }

    return false;
}

bool MetroMotion::LoadInternal() {
    size_t headerSize = 0, numOffsets = 0, numSwapped = 0;
    if (!this->ReadMotionDataHeader(headerSize, numOffsets, numSwapped)) {
        return false;
    }

    const uint8_t* ptr = mMotionsData.data();
    const uint8_t* offsetsTable = ptr + headerSize;
    const size_t dataSize = mMotionsData.size();

    mBonesRotations.clear();
    mBonesPositions.clear();
    mBonesScales.clear();
    mBonesRotations.resize(mNumBones);
    mBonesPositions.resize(mNumBones);
    mBonesScales.resize(mNumBones);

    auto loadCurve = [&](const size_t curveIdx, MetroCurve& curve) -> bool {
        if (curveIdx >= numOffsets) {
            return false;
        }

        const bool bigEndian = (curveIdx < numSwapped);
        const size_t offset = ReadU32(offsetsTable + curveIdx * sizeof(uint32_t), bigEndian);
        if (offset < mMotionsOffsetsSize || offset >= dataSize) {
            return false;
        }

        return curve.Load(ptr + offset, dataSize - offset, bigEndian);
    };

    bool result = true;

    for (size_t boneIdx = 0; boneIdx < mNumBones; ++boneIdx) {
        if (!mAnimatedBones.IsPresent(boneIdx)) {
            continue;
        }

        // uskeleton::motion_data::bone_quat_idx / bone_pos_idx / bone_scale_idx
        const size_t flatIdx = mAnimatedBones.CountOnesBefore(boneIdx);

        result &= loadCurve(flatIdx * 3 + 0, mBonesRotations[boneIdx]);
        result &= loadCurve(flatIdx * 3 + 1, mBonesPositions[boneIdx]);
        result &= loadCurve(flatIdx * 3 + 2, mBonesScales[boneIdx]);
    }

    return result;
}
