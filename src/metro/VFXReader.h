#pragma once
#include "MetroTypes.h"

struct Package {
    CharString      name;
    StringArray     levels;
    size_t          chunk;
};

class VFXReader {
public:
    static const size_t kVFXVersionUnknown      = 0;
    static const size_t kVFXVersion2033Redux    = 1;
    static const size_t kVFXVersionArktika1     = 2;
    static const size_t kVFXVersionExodus       = 3;
    static const size_t kVFXVersionMax          = 4;

    IMPL_SINGLETON(VFXReader)

    //#NOTE_SK: not the best design choice, but I need to be able to instantiate additional vfx readers as I need
public:
    VFXReader();
    ~VFXReader();

public:
    bool                        LoadFromFile(const fs::path& filePath, const VFXReader* baseVfx = nullptr);
    bool                        SaveToFile(const fs::path& filePath) const;
    void                        Close();

    const CharString&           GetContentVersion() const;
    size_t                      GetVersion() const;

    MemStream                   ExtractFile(const size_t fileIdx, const size_t subOffset = kInvalidValue, const size_t subLength = kInvalidValue) const;

    bool                        Good() const;

    const CharString&           GetSelfName() const;
    const fs::path&             GetAbsolutePath() const;

    const MyArray<Package>&     GetAllPacks() const;
    const MyArray<MetroFile>&   GetAllFiles() const;
    const MyArray<size_t>&      GetAllFolders() const;

    const MetroFile&            GetRootFolder() const;
    const MetroFile&            GetFile(const size_t idx) const;

    // modification
    void                        AddPackage(const Package& pak);
    void                        ReplaceFileInfo(const size_t idx, const MetroFile& newFile);
    void                        AppendFolder(const MetroFile& folder);

private:
    void                        ReadFileDescription(MetroFile& mf, MemStream& stream, const bool isDuplicate = false);
    void                        LoadBasePackages(const fs::path& contentVfxPath);

private:
    size_t                      mVersion;
    size_t                      mCompressionType;
    CharString                  mContentVersion;
    MetroGuid                   mGUID;
    CharString                  mFileName;
    fs::path                    mBasePath;
    fs::path                    mAbsolutePath;
    MyArray<Package>            mPaks;
    MyArray<Package>            mBasePaks;
    MyArray<MetroFile>          mFiles;
    MyArray<size_t>             mFolders;
    MyArray<MetroFile>          mDuplicates;
};
