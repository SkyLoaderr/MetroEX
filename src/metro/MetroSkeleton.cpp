#include "MetroSkeleton.h"
#include "MetroBinArchive.h"
#include "MetroReflection.h"

enum SkeletonChunks : size_t {
    SC_SelfData             = 0x00000001,
    SC_StringsDictionary    = 0x00000002,
};

struct ReduxBoneBodyPartHelper {
    uint16_t bp;

    void Serialize(MetroReflectionReader& reader) {
        METRO_READ_MEMBER(reader, bp);
    }
};


void ParentMapped::Serialize(MetroReflectionReader& reader) {
    METRO_READ_MEMBER(reader, parent_bone);
    METRO_READ_MEMBER(reader, self_bone);
    METRO_READ_MEMBER(reader, q);
    METRO_READ_MEMBER(reader, t);
    METRO_READ_MEMBER(reader, s);
}

void MetroAuxBone::Serialize(MetroReflectionReader& reader) {
    METRO_READ_MEMBER(reader, name);
    METRO_READ_MEMBER(reader, parent);
    METRO_READ_MEMBER(reader, q);
    METRO_READ_MEMBER(reader, t);
    METRO_READ_MEMBER_CHOOSE(reader, fl);
}

void MetroProceduralRef::Serialize(MetroReflectionReader& reader) {
    METRO_READ_MEMBER(reader, type);
    METRO_READ_MEMBER(reader, index_in_array);
}

void MetroDrivenBone::Serialize(MetroReflectionReader& reader) {
    METRO_READ_MEMBER(reader, bone);
    METRO_READ_MEMBER(reader, driver);
    METRO_READ_MEMBER(reader, driver_parent);
    METRO_READ_MEMBER(reader, component);
    METRO_READ_MEMBER(reader, twister);
    METRO_READ_MEMBER(reader, value_min);
    METRO_READ_MEMBER(reader, value_max);

    const size_t proceduralVersion = reader.GetUserData() & 0xFFFF;
    if (proceduralVersion >= 1) {
        METRO_READ_MEMBER(reader, refresh_kids);
    }
    if (proceduralVersion >= 5) {
        METRO_READ_MEMBER(reader, use_anim_poses);
    }
}

void MetroDynamicBone::Serialize(MetroReflectionReader& reader) {
    const size_t proceduralVersion = reader.GetUserData() & 0xFFFF;

    METRO_READ_MEMBER(reader, bone);
    METRO_READ_MEMBER(reader, inertia);
    METRO_READ_MEMBER(reader, damping);

    bool hasUseWorldPos = true;
    if (proceduralVersion < 9) {
        vec3 constraints;
        METRO_READ_MEMBER(reader, constraints);
        this->pos_limits[0] = -constraints;
        this->pos_limits[1] = constraints;
        if (proceduralVersion >= 6) {
            vec3 rot_limits_old;
            METRO_READ_MEMBER(reader, rot_limits_old);
            this->rot_limits[0] = -rot_limits_old;
            this->rot_limits[1] = rot_limits_old;
        }
        hasUseWorldPos = (proceduralVersion >= 4);
    } else {
        vec3 pos_min_limits, pos_max_limits, rot_min_limits, rot_max_limits;
        METRO_READ_MEMBER(reader, pos_min_limits);
        METRO_READ_MEMBER(reader, pos_max_limits);
        METRO_READ_MEMBER(reader, rot_min_limits);
        METRO_READ_MEMBER(reader, rot_max_limits);
        this->pos_limits[0] = pos_min_limits;
        this->pos_limits[1] = pos_max_limits;
        this->rot_limits[0] = rot_min_limits;
        this->rot_limits[1] = rot_max_limits;
    }

    if (hasUseWorldPos) {
        METRO_READ_MEMBER(reader, use_world_pos);
    }
    if (proceduralVersion >= 10) {
        METRO_READ_MEMBER(reader, refresh_kids);
    }
}

const float MetroParentBones::kMinWeight = 0.001f;

void MetroParentBones::Serialize(MetroReflectionReader& reader) {
    METRO_READ_MEMBER(reader, axis);

    MetroReflectionReader namesReader = reader.OpenSection("bone_names");
    if (namesReader.Good()) {
        uint32_t bone_strs_size = 0, count = 0;
        METRO_READ_MEMBER(namesReader, bone_strs_size);
        METRO_READ_MEMBER(namesReader, count);

        if (count <= 64) {
            this->bones.resize(count);
            for (uint32_t i = 0; i < count; ++i) {
                namesReader >> this->bones[i].bone;
                namesReader >> this->bones[i].weight;
            }
        }
    }
    reader.CloseSection(namesReader);
}

void MetroConstrainedBone::Serialize(MetroReflectionReader& reader) {
    const size_t userData = reader.GetUserData();
    const size_t proceduralVersion = userData & 0xFFFF;
    const size_t skeletonVersion = userData >> 16;

    METRO_READ_MEMBER(reader, bone);

    METRO_READ_MEMBER(reader, look_at_axis);
    if (skeletonVersion >= 10) {
        METRO_READ_MEMBER(reader, pos_axis);
        METRO_READ_MEMBER(reader, rot_axis);
    }

    if (proceduralVersion >= 3) {
        METRO_READ_MEMBER(reader, rotation_order);
    }

    METRO_READ_STRUCT_MEMBER(reader, position);
    METRO_READ_STRUCT_MEMBER(reader, orientation);

    if (proceduralVersion >= 1) {
        METRO_READ_MEMBER(reader, refresh_kids);
    }
    if (proceduralVersion < 5) {
        return;
    }
    METRO_READ_MEMBER(reader, use_anim_poses);
    if (proceduralVersion < 7) {
        return;
    }

    if (proceduralVersion < 8) {
        vec3 pos_limits_old, rot_limits_old;
        METRO_READ_MEMBER(reader, pos_limits_old);
        METRO_READ_MEMBER(reader, rot_limits_old);
        this->pos_limits[0] = -pos_limits_old;
        this->pos_limits[1] = pos_limits_old;
        this->rot_limits[0] = -rot_limits_old;
        this->rot_limits[1] = rot_limits_old;
    } else {
        vec3 pos_min_limits, pos_max_limits, rot_min_limits, rot_max_limits;
        METRO_READ_MEMBER(reader, pos_min_limits);
        METRO_READ_MEMBER(reader, pos_max_limits);
        METRO_READ_MEMBER(reader, rot_min_limits);
        METRO_READ_MEMBER(reader, rot_max_limits);
        this->pos_limits[0] = pos_min_limits;
        this->pos_limits[1] = pos_max_limits;
        this->rot_limits[0] = rot_min_limits;
        this->rot_limits[1] = rot_max_limits;
    }

    METRO_READ_MEMBER(reader, uptype);
    METRO_READ_STRUCT_MEMBER(reader, up);
}

void MetroBone::Serialize(MetroReflectionReader& reader) {
    METRO_READ_MEMBER(reader, name);
    METRO_READ_MEMBER(reader, parent);
    METRO_READ_MEMBER(reader, q);
    METRO_READ_MEMBER(reader, t);

    const size_t skeletonVersion = reader.GetUserData();
    if (skeletonVersion > 18) {
        METRO_READ_MEMBER(reader, bp);
        METRO_READ_MEMBER(reader, bpf);
    } else {
        //#NOTE_SK: using a hack to read old (Redux) bones
        ReduxBoneBodyPartHelper helper;
        reader >> helper;

        this->bp = scast<uint8_t>(helper.bp & 0xff);
    }
}


MetroSkeleton::MetroSkeleton()
    : ver(0)
    , crc(0)
    , has_as(false)
    , bone_lods_allowed(false)
    , procedural_ver(0)
{
}
MetroSkeleton::~MetroSkeleton() {

}

bool MetroSkeleton::LoadFromData(MemStream& stream) {
    bool result = false;

    MetroBinArchive bin(kEmptyString, stream, MetroBinArchive::kHeaderNotExist);
    MetroReflectionReader reader = bin.ReflectionReader();
    if (reader.Good()) {
        this->DeserializeSelf(reader);
        result = !this->bones.empty();
    }

    return result;
}

size_t MetroSkeleton::GetCRC() const {
    return scast<size_t>(crc);
}

size_t MetroSkeleton::GetNumBones() const {
    return this->bones.size();
}

const quat& MetroSkeleton::GetBoneRotation(const size_t idx) const {
    return this->bones[idx].q;
}

const vec3& MetroSkeleton::GetBonePosition(const size_t idx) const {
    return this->bones[idx].t;
}

mat4 MetroSkeleton::GetBoneTransform(const size_t idx) const {
    mat4 result = MatFromQuat(this->bones[idx].q);
    result[3] = vec4(this->bones[idx].t, 1.0f);

    return result;
}

mat4 MetroSkeleton::GetBoneFullTransform(const size_t idx) const {
    const size_t parentIdx = this->GetBoneParentIdx(idx);
    if (parentIdx == MetroBone::InvalidIdx) {
        return this->GetBoneTransform(idx);
    } else {
        return this->GetBoneFullTransform(parentIdx) * this->GetBoneTransform(idx);
    }
}

const size_t MetroSkeleton::GetBoneParentIdx(const size_t idx) const {
    size_t result = MetroBone::InvalidIdx;

    const CharString& parentName = this->bones[idx].parent;
    for (size_t i = 0; i < this->bones.size(); ++i) {
        if (this->bones[i].name == parentName) {
            result = i;
            break;
        }
    }

    return result;
}

const CharString& MetroSkeleton::GetBoneName(const size_t idx) const {
    return this->bones[idx].name;
}

size_t MetroSkeleton::FindBone(const CharString& name) const {
    for (size_t i = 0; i < this->bones.size(); ++i) {
        if (this->bones[i].name == name) {
            return i;
        }
    }
    return MetroBone::InvalidIdx;
}

size_t MetroSkeleton::GetNumLocators() const {
    return this->locators.size();
}

const MetroAuxBone& MetroSkeleton::GetLocator(const size_t idx) const {
    return this->locators[idx];
}

size_t MetroSkeleton::GetNumAuxBones() const {
    return this->aux_bones.size();
}

const MetroAuxBone& MetroSkeleton::GetAuxBone(const size_t idx) const {
    return this->aux_bones[idx];
}

size_t MetroSkeleton::FindAuxBone(const CharString& name) const {
    for (size_t i = 0; i < this->aux_bones.size(); ++i) {
        if (this->aux_bones[i].name == name) {
            return i;
        }
    }
    return MetroBone::InvalidIdx;
}

size_t MetroSkeleton::GetNumDrivenBones() const {
    return this->driven_bones.size();
}

const MetroDrivenBone& MetroSkeleton::GetDrivenBone(const size_t idx) const {
    return this->driven_bones[idx];
}

size_t MetroSkeleton::GetNumProceduralRefs() const {
    return this->procedural_bones.size();
}

const MetroProceduralRef& MetroSkeleton::GetProceduralRef(const size_t idx) const {
    return this->procedural_bones[idx];
}

uint32_t MetroSkeleton::GetVersion() const {
    return this->ver;
}

uint32_t MetroSkeleton::GetProceduralVersion() const {
    return this->procedural_ver;
}

size_t MetroSkeleton::GetNumConstrainedBones() const {
    return this->constrained_bones.size();
}

const MetroConstrainedBone& MetroSkeleton::GetConstrainedBone(const size_t idx) const {
    return this->constrained_bones[idx];
}

const CharString& MetroSkeleton::GetMotionsStr() const {
    return this->motions;
}


void MetroSkeleton::DeserializeSelf(MetroReflectionReader& reader) {
    MetroReflectionReader skeletonReader = reader.OpenSection("skeleton");
    if (skeletonReader.Good()) {
        METRO_READ_MEMBER(skeletonReader, ver);
        METRO_READ_MEMBER(skeletonReader, crc);

        skeletonReader.SetUserData(this->ver);

        if (this->ver < 15) {
            METRO_READ_MEMBER(skeletonReader, facefx);
        } else {
            METRO_READ_MEMBER(skeletonReader, pfnn); // if version > 16
        }
        if (this->ver > 20) {
            METRO_READ_MEMBER(skeletonReader, has_as); // if version > 20
        }
        METRO_READ_MEMBER(skeletonReader, motions);
        if (this->ver > 12) {
            METRO_READ_MEMBER(skeletonReader, source_info); // if version > 12
            if (this->ver > 13) {
                METRO_READ_MEMBER(skeletonReader, parent_skeleton); // if version > 13
                METRO_READ_STRUCT_ARRAY_MEMBER(skeletonReader, parent_bone_maps); // if version > 13
            }
        }
        METRO_READ_STRUCT_ARRAY_MEMBER(skeletonReader, bones);

        if (!skeletonReader.HasDebugInfo()) {

        METRO_READ_STRUCT_ARRAY_MEMBER(skeletonReader, locators);
        if (this->ver > 5) {
            METRO_READ_STRUCT_ARRAY_MEMBER(skeletonReader, aux_bones);
        }

        if (this->ver > 10) {
            MetroReflectionReader proceduralReader = skeletonReader.OpenSection("procedural");
            if (proceduralReader.Good()) {
                uint32_t ver = 0;
                METRO_READ_MEMBER(proceduralReader, ver);
                this->procedural_ver = ver;

                proceduralReader.SetUserData(this->procedural_ver | (scast<size_t>(this->ver) << 16));

                if (this->procedural_ver >= 1) {
                    METRO_READ_STRUCT_ARRAY_MEMBER(proceduralReader, procedural_bones);
                }
                if (this->ver > 6) {
                    METRO_READ_STRUCT_ARRAY_MEMBER(proceduralReader, driven_bones);
                }
                if (this->ver > 7) {
                    METRO_READ_STRUCT_ARRAY_MEMBER(proceduralReader, dynamic_bones);
                }
                if (this->ver > 8) {
                    METRO_READ_STRUCT_ARRAY_MEMBER(proceduralReader, constrained_bones);
                }
                //#TODO_SK: param_bones (ver > 19)
            }
            skeletonReader.CloseSection(proceduralReader);
        }

        } // !HasDebugInfo
    }
    reader.CloseSection(skeletonReader);

    //#NOTE_SK: fix-up bones transforms by swizzling them back
    for (auto& b : bones) {
        b.q = MetroSwizzle(b.q);
        b.t = MetroSwizzle(b.t);
    }
    for (auto& b : locators) {
        b.q = MetroSwizzle(b.q);
        b.t = MetroSwizzle(b.t);
    }
    for (auto& b : aux_bones) {
        b.q = MetroSwizzle(b.q);
        b.t = MetroSwizzle(b.t);
    }
}
