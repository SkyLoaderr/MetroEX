#include "MetroFileSystem.h"
#include "MetroCompression.h"
#include "VFXReader.h"
#include <fstream>

static const uint32_t kVFXVersionUnknown = 0;
static const uint32_t kVFXVersion2033Redux = 1;
static const uint32_t kVFXVersionArktika1 = 2;
static const uint32_t kVFXVersionExodus = 3;
static const uint32_t kVFXVersionMax = 4;

static const CharString sGameVersions[] = {
    "Unknown",
    "Redux (2033 / Last Light)",
    "Arktika.1",
    "Exodus"
};

static const CharString sVFXList[] = {
    "content.vfx",
    "patch.vfx0",
    "patch_00.vfx",
    "patch_01_shared.vfx",
    "patch_01.vfx",
    "patch_02.vfx",
    "patch_03.vfx"
};


MetroFileSystem::MetroFileSystem()
    : mCurrentVfxIdx(0)
    , mGameVersion(kVFXVersionUnknown)
{
}
MetroFileSystem::~MetroFileSystem() {

}

bool MetroFileSystem::InitFromGameFolder(const fs::path& gameFolder) {
    this->Shutdown();

    LogPrint(LogLevel::Info, "Initializing game FS (" + gameFolder.u8string() + ")");

    bool result = false;
    for (const CharString& s : sVFXList) {
        LogPrint(LogLevel::Info, "Adding (" + s + ") to FS");

        const bool mandatory = (s == sVFXList[0]);

        fs::path vfxPath = gameFolder / s;
        const bool added = this->AddVFX(vfxPath);
        if (!added) {
            LogPrint(LogLevel::Info, "Failed to add (" + s + ") to FS");
            if (mandatory) {
                break;
            }
        } else if (mandatory) {
            result = true;
        }
    }

    return result;
}

bool MetroFileSystem::InitFromSingleVFX(const fs::path& vfxPath) {
    this->Shutdown();
    const bool result = this->AddVFX(vfxPath);
    return result;
}

void MetroFileSystem::Shutdown() {
    std::for_each(mLoadedVFX.begin(), mLoadedVFX.end(), [](VFXReader* v) { delete v; });
    mLoadedVFX.clear();
    mEntries.clear();
    mGameVersion = kVFXVersionUnknown;

    // Add root
    mEntries.push_back({ kEmptyString, 0, kInvalidHandle, kInvalidValue, kInvalidValue, kInvalidValue, kInvalidValue });
}

bool MetroFileSystem::Empty() const {
    return mEntries.size() <= 1;
}

bool MetroFileSystem::IsSingleVFX() const {
    return mLoadedVFX.size() == 1;
}

const CharString& MetroFileSystem::GetVFXName(const size_t idx) const {
    if (idx < mLoadedVFX.size()) {
        return mLoadedVFX[idx]->GetSelfName();
    } else {
        return kEmptyString;
    }
}

MyHandle MetroFileSystem::GetRootFolder() const {
    return 0;
}

bool MetroFileSystem::IsFolder(const MyHandle entry) const {
    bool result = false;

    if (entry < mEntries.size()) {
        const MetroFSEntry& fsEntry = mEntries[entry];
        result = fsEntry.fileIdx == kInvalidValue;
    }

    return result;
}

bool MetroFileSystem::IsFile(const MyHandle entry) const {
    return !this->IsFolder(entry);
}

const CharString& MetroFileSystem::GetName(const MyHandle entry) const {
    if (entry < mEntries.size()) {
        const MetroFSEntry& fsEntry = mEntries[entry];
        return fsEntry.name.str;
    } else {
        return kEmptyString;
    }
}

size_t MetroFileSystem::GetGameVersion() const {
    return mGameVersion;
}

CharString MetroFileSystem::GetFullPath(const MyHandle entry) const {
    CharString result;

    if (entry < mEntries.size()) {
        const MetroFSEntry& fsEntry = mEntries[entry];
        result = fsEntry.name.str;

        MyHandle parentHandle = fsEntry.parent;
        const MyHandle rootHandle = this->GetRootFolder();
        while (parentHandle != kInvalidHandle && parentHandle != rootHandle) {
            const CharString& parentName = mEntries[parentHandle].name.str;
            result = parentName + kPathSeparator + result;

            parentHandle = mEntries[parentHandle].parent;
        }
    }

    return result;
}

size_t MetroFileSystem::GetCompressedSize(const MyHandle entry) const {
    size_t result = 0;

    if (entry < mEntries.size()) {
        const MetroFSEntry& fsEntry = mEntries[entry];
        const VFXReader* vfx = mLoadedVFX[fsEntry.vfxIdx];
        const MetroFile& mf = vfx->GetFile(fsEntry.fileIdx);
        result = mf.sizeCompressed;
    }

    return result;
}

size_t MetroFileSystem::GetUncompressedSize(const MyHandle entry) const {
    size_t result = 0;

    if (entry < mEntries.size()) {
        const MetroFSEntry& fsEntry = mEntries[entry];
        const VFXReader* vfx = mLoadedVFX[fsEntry.vfxIdx];
        const MetroFile& mf = vfx->GetFile(fsEntry.fileIdx);
        result = mf.sizeUncompressed;
    }

    return result;
}

size_t MetroFileSystem::CountFilesInFolder(const MyHandle entry) const {
    size_t result = 0;

    if (entry < mEntries.size()) {
        const bool isFolder = this->IsFolder(entry);
        if (isFolder) {
            for (MyHandle child = this->GetFirstChild(entry); child != kInvalidHandle; child = this->GetNextChild(child)) {
                if (this->IsFolder(child)) {
                    result += this->CountFilesInFolder(child);
                } else {
                    ++result;
                }
            }
        }
    }

    return result;
}

MyHandle MetroFileSystem::GetParentFolder(const MyHandle entry) const {
    MyHandle parent = this->GetRootFolder();

    if (entry < mEntries.size()) {
        const MetroFSEntry& fsEntry = mEntries[entry];
        parent = fsEntry.parent;
    }

    return parent;
}

MyHandle MetroFileSystem::FindFile(const CharString& fileName, const MyHandle inFolder) const {
    MyHandle result = kInvalidHandle;

    MyHandle folder = (inFolder == kInvalidHandle) ? this->GetRootFolder() : inFolder;

    CharString::size_type lastSlashPos = fileName.find_last_of('\\');
    if (lastSlashPos != CharString::npos) {
        lastSlashPos++;
        folder = this->FindFolder(fileName.substr(0, lastSlashPos), inFolder);
    } else {
        lastSlashPos = 0;
    }

    if (folder) {
        CharString name = fileName.substr(lastSlashPos);

        for (MyHandle child = this->GetFirstChild(folder); child != kInvalidHandle; child = this->GetNextChild(child)) {
            const CharString& childName = this->GetName(child);
            if (name == childName) {
                result = child;
                break;
            }
        }
    }

    return result;
}

MyHandle MetroFileSystem::FindFolder(const CharString& folderPath, const MyHandle inFolder) const {
    CharString::size_type slashPos = folderPath.find_first_of('\\'), lastSlashPos = 0;
    MyHandle folder = (inFolder == kInvalidHandle) ? this->GetRootFolder() : inFolder;
    while (slashPos != CharString::npos) {
        MyHandle folderTestHandle = kInvalidHandle;

        CharString name = folderPath.substr(lastSlashPos, slashPos - lastSlashPos);
        for (MyHandle child = this->GetFirstChild(folder); child != kInvalidHandle; child = this->GetNextChild(child)) {
            const CharString& childName = this->GetName(child);
            if (name == childName) {
                folderTestHandle = child;
                break;
            }
        }

        if (folderTestHandle == kInvalidHandle) { // failed to find
            return folderTestHandle;
        }

        lastSlashPos = slashPos + 1;
        slashPos = folderPath.find_first_of('\\', lastSlashPos);
        folder = folderTestHandle;
    }

    return folder;
}

MyArray<MyHandle> MetroFileSystem::FindFilesInFolder(const CharString& folder, const CharString& extension, const bool withSubfolders) const {
    MyArray<MyHandle> result;

    const MyHandle folderHandle = this->FindFolder(folder);
    if (kInvalidHandle != folderHandle) {
        result = this->FindFilesInFolder(folderHandle, extension, withSubfolders);
    }

    return std::move(result);
}

MyArray<MyHandle> MetroFileSystem::FindFilesInFolder(const MyHandle folder, const CharString& extension, const bool withSubfolders) const {
    MyArray<MyHandle> result;

    const bool isFolder = this->IsFolder(folder);
    if (isFolder) { // sanity check
        for (MyHandle child = this->GetFirstChild(folder); child != kInvalidHandle; child = this->GetNextChild(child)) {
            const bool isSubFolder = this->IsFolder(child);
            if (isSubFolder) {
                const MyArray<MyHandle>& v = this->FindFilesInFolder(child, extension, withSubfolders);
                result.insert(result.end(), v.begin(), v.end());
            } else {
                const CharString& childName = this->GetName(child);
                if (StrEndsWith(childName, extension)) {
                    result.push_back(child);
                }
            }
        }
    }

    return std::move(result);
}

MyHandle MetroFileSystem::GetFirstChild(const MyHandle parentEntry) const {
    MyHandle result = kInvalidHandle;

    if (parentEntry < mEntries.size()) {
        const MetroFSEntry& parent = mEntries[parentEntry];
        result = parent.firstChild;
    }

    return result;
}

MyHandle MetroFileSystem::GetNextChild(const MyHandle currentChild) const {
    MyHandle result = kInvalidHandle;

    if (currentChild < mEntries.size()) {
        const MetroFSEntry& child = mEntries[currentChild];
        result = child.nextSibling;
    }

    return result;
}

MyHandle MetroFileSystem::FindChild(const MyHandle parentEntry, const HashString& childName) const {
    MyHandle result = kInvalidHandle;

    if (parentEntry < mEntries.size()) {
        const MetroFSEntry& parent = mEntries[parentEntry];
        if (parent.firstChild != kInvalidValue) {
            const MetroFSEntry* sibling = &mEntries[parent.firstChild];
            do {
                if (sibling->name == childName) {
                    result = sibling->idx;
                    break;
                }
                sibling = (sibling->nextSibling == kInvalidValue) ? nullptr : &mEntries[sibling->nextSibling];
            } while (sibling != nullptr);
        }
    }

    return result;
}


MemStream MetroFileSystem::OpenFileStream(const MyHandle entry, const size_t subOffset, const size_t subLength) const {
    MemStream result;

    if (entry < mEntries.size()) {
        const MetroFSEntry& file = mEntries[entry];
        const VFXReader* vfx = mLoadedVFX[file.vfxIdx];
        result = vfx->ExtractFile(file.fileIdx, subOffset, subLength);
    }

    return std::move(result);
}



bool MetroFileSystem::AddVFX(const fs::path& vfxPath) {
    bool result = false;

    LogPrint(LogLevel::Info, "Adding vfx to FS...");
    LogPrint(LogLevel::Info, "    " + vfxPath.generic_u8string());

    std::error_code ec;
    fs::path absoluteVfxPath = fs::absolute(vfxPath, ec);
    
    auto it = std::find_if(mLoadedVFX.begin(), mLoadedVFX.end(), [&absoluteVfxPath](const VFXReader* v) {
        return v->GetAbsolutePath() == absoluteVfxPath;
    });
    if (it != mLoadedVFX.end()) {
        LogPrint(LogLevel::Warning, "Such vfx was already added to FS, ignoring.");
    } else {
        VFXReader* vfxReader = new VFXReader();
        if (vfxReader->LoadFromFile(vfxPath)) {
            mCurrentVfxIdx = mLoadedVFX.size();
            mGameVersion = vfxReader->GetVersion();

            const MetroFile& rootDir = vfxReader->GetRootFolder();
            this->MergeFolderRecursive(this->GetRootFolder(), rootDir, *vfxReader);

            mLoadedVFX.push_back(vfxReader);
            result = true;
        } else {
            delete vfxReader;
        }
    }

    return result;
}

void MetroFileSystem::MergeFolderRecursive(MyHandle parentEntry, const MetroFile& folder, const VFXReader& vfxReader) {
    MyHandle newFolder = parentEntry;

    //#NOTE_SK: this is to skip root folder that some of the game packs have
    if (!folder.name.empty()) {
        StringArray parts = StrSplit(folder.name, kPathSeparator);
        for (const CharString& s : parts) {
            newFolder = this->FindChild(parentEntry, s);
            if (newFolder == kInvalidHandle) {
                newFolder = this->AddEntryFolder(parentEntry, s);
            }
            parentEntry = newFolder;
        }
    }

    for (const size_t idx : folder) {
        const MetroFile& mf = vfxReader.GetFile(idx);
        if (mf.IsFile()) {
            this->AddEntryFile(newFolder, mf);
        } else {
            this->MergeFolderRecursive(newFolder, mf, vfxReader);
        }
    }
}

MyHandle MetroFileSystem::AddEntryFolder(const MyHandle parentEntry, const HashString& name) {
    MyHandle result = this->FindChild(parentEntry, name);

    if (result == kInvalidHandle) {
        const size_t newIdx = mEntries.size();

        MetroFSEntry newEntry = {
            name,
            newIdx,
            parentEntry,
            kInvalidValue,
            kInvalidValue,
            kInvalidValue,
            kInvalidValue
        };

        result = this->AddEntryCommon(parentEntry, newEntry);
    }

    return result;
}

MyHandle MetroFileSystem::AddEntryFile(const MyHandle parentEntry, const MetroFile& file) {
    MyHandle result = this->FindChild(parentEntry, file.name);

    if (result == kInvalidHandle) {
        const size_t newIdx = mEntries.size();

        MetroFSEntry newEntry = {
            file.name,
            newIdx,
            parentEntry,
            kInvalidValue,
            kInvalidValue,
            mCurrentVfxIdx,
            file.idx
        };

        result = this->AddEntryCommon(parentEntry, newEntry);
    }

    return result;
}

MyHandle MetroFileSystem::AddEntryCommon(const MyHandle parentEntry, const MetroFSEntry& entry) {
    mEntries.push_back(entry);

    // sorry :(
    MetroFSEntry& parent = mEntries[parentEntry];
    if (parent.firstChild == kInvalidValue) {
        parent.firstChild = entry.idx;
    } else {
        MetroFSEntry* lastSibling = &mEntries[parent.firstChild];
        while (lastSibling->nextSibling != kInvalidValue) {
            lastSibling = &mEntries[lastSibling->nextSibling];
        }

        lastSibling->nextSibling = entry.idx;
    }

    return entry.idx;
}
