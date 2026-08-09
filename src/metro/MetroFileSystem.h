#pragma once
#include "MetroTypes.h"

class VFXReader;

class MetroFileSystem {
    IMPL_SINGLETON(MetroFileSystem)

private:
    struct MetroFSEntry {
        HashString  name;
        size_t      idx;
        MyHandle    parent;
        size_t      firstChild;
        size_t      nextSibling;
        size_t      vfxIdx;
        size_t      fileIdx;
    };

public:
    MetroFileSystem();
    ~MetroFileSystem();

    bool                    InitFromGameFolder(const fs::path& gameFolder);
    bool                    InitFromSingleVFX(const fs::path& vfxPath);
    void                    Shutdown();
    bool                    Empty() const;
    bool                    IsSingleVFX() const;
    const CharString&       GetVFXName(const size_t idx) const;
    size_t                  GetGameVersion() const;

    MyHandle                GetRootFolder() const;

    bool                    IsFolder(const MyHandle entry) const;
    bool                    IsFile(const MyHandle entry) const;
    const CharString&       GetName(const MyHandle entry) const;
    CharString              GetFullPath(const MyHandle entry) const;
    size_t                  GetCompressedSize(const MyHandle entry) const;
    size_t                  GetUncompressedSize(const MyHandle entry) const;

    size_t                  CountFilesInFolder(const MyHandle entry) const;

    MyHandle                GetParentFolder(const MyHandle entry) const;
    MyHandle                FindFile(const CharString& fileName, const MyHandle inFolder = kInvalidHandle) const;
    MyHandle                FindFolder(const CharString& folderPath, const MyHandle inFolder = kInvalidHandle) const;
    MyArray<MyHandle>       FindFilesInFolder(const CharString& folder, const CharString& extension, const bool withSubfolders = true) const;
    MyArray<MyHandle>       FindFilesInFolder(const MyHandle folder, const CharString& extension, const bool withSubfolders = true) const;

    MyHandle                GetFirstChild(const MyHandle parentEntry) const;
    MyHandle                GetNextChild(const MyHandle currentChild) const;
    MyHandle                FindChild(const MyHandle parentEntry, const HashString& childName) const;

    MemStream               OpenFileStream(const MyHandle entry, const size_t subOffset = kInvalidValue, const size_t subLength = kInvalidValue) const;

private:
    bool                    AddVFX(const fs::path& vfxPath);
    void                    MergeFolderRecursive(MyHandle parentEntry, const MetroFile& folder, const VFXReader& vfxReader);
    MyHandle                AddEntryFolder(const MyHandle parentEntry, const HashString& name);
    MyHandle                AddEntryFile(const MyHandle parentEntry, const MetroFile& file);
    MyHandle                AddEntryCommon(const MyHandle parentEntry, const MetroFSEntry& entry);

private:
    MyArray<VFXReader*>     mLoadedVFX;
    MyArray<MetroFSEntry>   mEntries;
    size_t                  mCurrentVfxIdx;
    size_t                  mGameVersion;
};
