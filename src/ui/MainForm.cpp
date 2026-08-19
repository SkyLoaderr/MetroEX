#include "metro/MetroDatabases.h"
#include "metro/MetroTexture.h"
#include "metro/MetroModel.h"
#include "metro/MetroSound.h"
#include "metro/MetroSkeleton.h"
#include "metro/MetroMotion.h"
#include "metro/MetroLocalization.h"

#include "mex_settings.h"

#include <fstream>

#include "ChooseFolderDlg.h"

#include "MainForm.h"
#include "AboutDlg.h"
#include "TexturesDatabaseViewer.h"
#include "DlgSettings.h"

#include "ui/tools/DlgConvertTextures.h"
#include "ui/tools/DlgCreateArchive.h"

#include "UIHelpers.h"

using namespace System::Collections::Generic;

enum class eNodeEventType : size_t {
    Default,
    Open,
    Close
};

static const int    kImageIdxFolderClosed   = 0;
static const int    kImageIdxFolderOpen     = 1;
static const int    kImageIdxFile           = 2;
static const int    kImageIdxBinUnkn        = 3;
static const int    kImageIdxBinArchive     = 4;
static const int    kImageIdxBinEditable    = 5;
static const int    kImageIdxTexture        = 6;
static const int    kImageIdxMotion         = 7;
static const int    kImageIdxSound          = 8;
static const int    kImageIdxModel          = 9;
static const int    kImageIdxLocalization   = 10;

namespace MetroEX {
    ref struct FileTagData {
        FileType    fileType;       // type of file
        MyHandle    fileHandle;     // file system file handle
        size_t      subFileIdx;     // index inside .bin database
        bool        populated;      // are the real children already added ?

        FileTagData(const FileType _fileType, MyHandle _file, const size_t _subFileIdx)
            : fileType(_fileType)
            , fileHandle(_file)
            , subFileIdx(_subFileIdx)
            , populated(false)
        { }

        FileTagData(const FileType _fileType, MyHandle _file, const size_t _subFileIdx, const bool _populated)
            : fileType(_fileType)
            , fileHandle(_file)
            , subFileIdx(_subFileIdx)
            , populated(_populated)
        { }
    };

    static FileType DetectFileType(const MyHandle file) {
        FileType result = FileType::Unknown;

        String^ name = ToNetString(MetroFileSystem::Get().GetName(file));

        if (name->EndsWith(L".dds") ||
            name->EndsWith(L".512") ||
            name->EndsWith(L".1024") ||
            name->EndsWith(L".2048") ||
            name->EndsWith(L".4096")) {
            result = FileType::Texture;
        } else if (name->EndsWith(L".bin")) {
            result = FileType::Bin;
        } else if (name->EndsWith(L".model")) {
            result = FileType::Model;
        } else if (name->EndsWith(L".m2")) {
            result = FileType::Motion;
        } else if (name->EndsWith(L".vba")) {
            result = FileType::Sound;
        } else if (name->EndsWith(L"lightmaps")) {
            result = FileType::Level;
        } else if (name->EndsWith(L".lng")) {
            result = FileType::Localization;
        }

        return result;
    }

    static MyArray<MyHandle> CollectSortedChildren(const MyHandle folder, const MyHandle configBinFile) {
        MetroFileSystem& mfs = MetroFileSystem::Get();

        MyArray<MyHandle> children;
        for (MyHandle child = mfs.GetFirstChild(folder); child != kInvalidHandle; child = mfs.GetNextChild(child)) {
            children.push_back(child);
        }

        std::sort(children.begin(), children.end(), [&mfs, configBinFile](const MyHandle a, const MyHandle b)->bool {
            const bool aIsFolder = mfs.IsFolder(a) || a == configBinFile;
            const bool bIsFolder = mfs.IsFolder(b) || b == configBinFile;

            if (aIsFolder != bIsFolder) {
                return aIsFolder;
            } else {
                const CharString& aName = mfs.GetName(a);
                const CharString& bName = mfs.GetName(b);

                const int cmp = _stricmp(aName.c_str(), bName.c_str());
                return (cmp != 0) ? (cmp < 0) : (aName < bName);
            }
        });

        return children;
    }

    static void UpdateNodeIcon(TreeNode^ Node, eNodeEventType eventType = eNodeEventType::Default) {
        FileTagData^ fileData = safe_cast<FileTagData^>(Node->Tag);
        FileType fileType = fileData->fileType;

        Node->ImageIndex = kImageIdxFile;
        Node->SelectedImageIndex = kImageIdxFile;

        switch (fileType) {
            case FileType::Unknown: {
            } break;

            case FileType::Folder:
            case FileType::FolderBin: {
                if (eventType == eNodeEventType::Open) {
                    Node->ImageIndex = kImageIdxFolderOpen;
                    Node->SelectedImageIndex = kImageIdxFolderOpen;
                } else {
                    Node->ImageIndex = kImageIdxFolderClosed;
                    Node->SelectedImageIndex = kImageIdxFolderClosed;
                }
            } break;

            case FileType::Bin: {
                Node->ImageIndex = kImageIdxBinUnkn;
                Node->SelectedImageIndex = kImageIdxBinUnkn;
            } break;

            case FileType::BinArchive: {
                Node->ImageIndex = kImageIdxBinArchive;
                Node->SelectedImageIndex = kImageIdxBinArchive;
            } break;

            case FileType::BinEditable: {
                Node->ImageIndex = kImageIdxBinEditable;
                Node->SelectedImageIndex = kImageIdxBinEditable;
            } break;

            case FileType::Model: {
                Node->ImageIndex = kImageIdxModel;
                Node->SelectedImageIndex = kImageIdxModel;
            } break;

            case FileType::Texture: {
                Node->ImageIndex = kImageIdxTexture;
                Node->SelectedImageIndex = kImageIdxTexture;
            } break;

            case FileType::Sound: {
                Node->ImageIndex = kImageIdxSound;
                Node->SelectedImageIndex = kImageIdxSound;
            } break;

            case FileType::Localization: {
                Node->ImageIndex = kImageIdxLocalization;
                Node->SelectedImageIndex = kImageIdxLocalization;
            } break;
        }
    }

    void MainForm::MainForm_Load(System::Object^, System::EventArgs^) {
//#ifdef _DEBUG
//        //#NOTE_SK: for debugging purposes we might want to extract raw files
//        this->ctxMenuExportModel->Items->Add(this->extractFileToolStripMenuItem);
//        this->ctxMenuExportModel->Size.Height += this->extractFileToolStripMenuItem->Size.Height;
//        this->ctxMenuExportTexture->Items->Add(this->extractFileToolStripMenuItem);
//        this->ctxMenuExportTexture->Size.Height += this->extractFileToolStripMenuItem->Size.Height;
//        this->ctxMenuExportSound->Items->Add(this->extractFileToolStripMenuItem);
//        this->ctxMenuExportSound->Size.Height += this->extractFileToolStripMenuItem->Size.Height;
//#endif

        mImagePanel = gcnew ImagePanel();
        this->pnlViewers->Controls->Add(mImagePanel);
        mImagePanel->Dock = System::Windows::Forms::DockStyle::Fill;
        mImagePanel->Location = System::Drawing::Point(0, 0);
        mImagePanel->Name = L"mImagePanel";
        mImagePanel->Size = System::Drawing::Size(528, 386);
        mImagePanel->AutoScroll = true;


        mSoundPanel = gcnew SoundPanel();
        this->pnlViewers->Controls->Add(mSoundPanel);
        mSoundPanel->Dock = System::Windows::Forms::DockStyle::Fill;
        mSoundPanel->Location = System::Drawing::Point(0, 0);
        mSoundPanel->Name = L"mSoundPanel";
        mSoundPanel->Size = System::Drawing::Size(528, 386);


        mLocalizationPanel = gcnew LocalizationPanel();
        this->pnlViewers->Controls->Add(mLocalizationPanel);
        mLocalizationPanel->Dock = System::Windows::Forms::DockStyle::Fill;
        mLocalizationPanel->Location = System::Drawing::Point(0, 0);
        mLocalizationPanel->Name = L"mLocalizationPanel";
        mLocalizationPanel->Size = System::Drawing::Size(528, 386);


        mRenderPanel = gcnew RenderPanel();
        this->pnlViewers->Controls->Add(mRenderPanel);
        mRenderPanel->Dock = System::Windows::Forms::DockStyle::Fill;
        mRenderPanel->Location = System::Drawing::Point(0, 0);
        mRenderPanel->Name = L"mRenderPanel";
        mRenderPanel->Size = System::Drawing::Size(528, 386);

        if (!mRenderPanel->InitGraphics()) {
            MetroEX::ShowErrorMessageBox(this, L"Failed to initialize DirectX 11 graphics!\n3D viewer will be unavailable.");
        }

        // Create info panels
        mImageInfoPanel = gcnew MetroEXControls::ImageInfoPanel();
        this->pnlMetaProps->Controls->Add(this->mImageInfoPanel);
        mImageInfoPanel->Dock = System::Windows::Forms::DockStyle::None;
        mImageInfoPanel->Location = System::Drawing::Point(0, 0);
        mImageInfoPanel->Name = L"mImageInfoPanel";
        mImageInfoPanel->Size = System::Drawing::Size(481, 72);

        mModelInfoPanel = gcnew MetroEXControls::ModelInfoPanel();
        this->pnlMetaProps->Controls->Add(this->mModelInfoPanel);
        mModelInfoPanel->Dock = System::Windows::Forms::DockStyle::None;
        mModelInfoPanel->Location = System::Drawing::Point(0, 0);
        mModelInfoPanel->Name = L"mModelInfoPanel";
        mModelInfoPanel->Size = System::Drawing::Size(481, 72);
        mModelInfoPanel->OnMotionsListSelectionChanged += gcnew MetroEXControls::ModelInfoPanel::OnListSelectionChanged(this, &MainForm::lstMdlPropMotions_SelectedIndexChanged);
        mModelInfoPanel->OnPlayButtonClicked += gcnew MetroEXControls::ModelInfoPanel::OnButtonClicked(this, &MainForm::btnMdlPropPlayStopAnim_Click);
        mModelInfoPanel->OnInfoButtonClicked += gcnew MetroEXControls::ModelInfoPanel::OnButtonClicked(this, &MainForm::btnModelInfo_Click);
        mModelInfoPanel->OnMotionExportButtonClicked += gcnew MetroEXControls::ModelInfoPanel::OnButtonClicked(this, &MainForm::btnModelExportMotion_Click);
        mModelInfoPanel->OnLodsListSelectionChanged += gcnew MetroEXControls::ModelInfoPanel::OnListSelectionChanged(this, &MainForm::lstLods_SelectedIndexChanged);
        ////

        this->SwitchViewPanel(PanelType::Texture);
        this->SwitchInfoPanel(PanelType::Sound);
    }

    // toolstrip buttons
    void MainForm::openGameFolderToolStripMenuItem_Click(System::Object^ sender, System::EventArgs^ e) {
        fs::path folderPath = ChooseFolderDialog::ChooseFolder("Choose game directory...", this->Handle.ToPointer());
        if (!folderPath.empty()) {
            System::Windows::Forms::Cursor::Current = System::Windows::Forms::Cursors::WaitCursor;

            if (MetroFileSystem::Get().InitFromGameFolder(folderPath)) {
                MetroConfigsDatabase* cfgDb;
                LoadDatabasesFromFile(cfgDb);
                mConfigsDatabase = cfgDb;

                this->UpdateFilesList();
            }

            this->toolBtnTexturesDatabase->Enabled = true;
            System::Windows::Forms::Cursor::Current = System::Windows::Forms::Cursors::Arrow;
        }
    }

    void MainForm::openSingleVFXArchiveToolStripMenuItem_Click(System::Object^ sender, System::EventArgs^ e) {
        OpenFileDialog ofd;
        ofd.Title = L"Open Metro Exodus vfx file...";
        ofd.Filter = L"VFX files (*.vfx)|*.vfx";
        ofd.FilterIndex = 0;
        ofd.RestoreDirectory = true;
        if (ofd.ShowDialog(this) == System::Windows::Forms::DialogResult::OK) {
            System::Windows::Forms::Cursor::Current = System::Windows::Forms::Cursors::WaitCursor;

            if (MetroFileSystem::Get().InitFromSingleVFX(StringToPath(ofd.FileName))) {
                MetroConfigsDatabase* cfgDb;
                LoadDatabasesFromFile(cfgDb);
                mConfigsDatabase = cfgDb;

                this->UpdateFilesList();
            }

            this->toolBtnTexturesDatabase->Enabled = true;
            System::Windows::Forms::Cursor::Current = System::Windows::Forms::Cursors::Arrow;
        }
    }

    void MainForm::toolBtnAbout_Click(System::Object^, System::EventArgs^) {
        AboutDlg dlg;
        dlg.Icon = this->Icon;
        dlg.Text = this->Text;
        dlg.ShowDialog(this);
    }

    void MainForm::toolBtnImgEnableAlpha_Click(System::Object^, System::EventArgs^) {
        if (mImagePanel) {
            this->toolBtnImgEnableAlpha->Checked = !this->toolBtnImgEnableAlpha->Checked;
            mImagePanel->EnableTransparency(toolBtnImgEnableAlpha->Checked);
        }
    }

    void MainForm::toolBtnMdlShowWireframe_Click(System::Object^, System::EventArgs^) {
        if (mRenderPanel) {
            this->toolBtnMdlShowWireframe->Checked = !this->toolBtnMdlShowWireframe->Checked;
            mRenderPanel->SetShowWireframe(this->toolBtnMdlShowWireframe->Checked);
        }
    }

    void MainForm::toolBtnMdlShowCollision_Click(System::Object^, System::EventArgs^) {
        if (mRenderPanel) {
            this->toolBtnMdlShowCollision->Checked = !this->toolBtnMdlShowCollision->Checked;
            mRenderPanel->SetShowCollision(this->toolBtnMdlShowCollision->Checked);
        }
    }

    void MainForm::toolBtnMdlResetCamera_Click(System::Object^, System::EventArgs^) {
        if (mRenderPanel) {
            mRenderPanel->ResetCamera(true);
        }
    }

    void MainForm::toolBtnTexturesDatabase_Click(System::Object^, System::EventArgs^) {
        if (MetroTexturesDatabase::Get().Good()) {
            TexturesDatabaseViewer wnd(this, this->imageListMain);
            wnd.Icon = this->Icon;
            wnd.ShowDialog(this);
        }
    }

    // settings
    void MainForm::toolBtnSettings_Click(System::Object^ sender, System::EventArgs^ e) {
        DlgSettings dlg;
        dlg.Icon = this->Icon;
        dlg.ShowDialog(this);
    }

    static TreeNode^ MakeDummyNode() {
        TreeNode^ node = gcnew TreeNode(L"...");
        node->Tag = gcnew FileTagData(FileType::Unknown, kInvalidHandle, kInvalidValue);
        UpdateNodeIcon(node);
        return node;
    }

    static String^ MakeBinChunkName(const MetroConfigsDatabase::ConfigInfo& ci) {
        return ci.nameStr.empty() ? String::Format("unknCRC32_0x{0:X}.bin", ci.nameCRC) : ToNetString(ci.nameStr);
    }

    static int CompareNodesByText(TreeNode^ a, TreeNode^ b) {
        return String::Compare(a->Text, b->Text, StringComparison::OrdinalIgnoreCase);
    }

    static TreeNode^ FindChildNodeByText(TreeNode^ parent, String^ text) {
        for (int i = 0; i < parent->Nodes->Count; ++i) {
            TreeNode^ child = parent->Nodes[i];
            if (String::Equals(child->Text, text, StringComparison::OrdinalIgnoreCase)) {
                return child;
            }
        }

        return nullptr;
    }

    // treeview
    void MainForm::ResetTreeView() {
        if (mIsTreeFiltering) {
            this->UpdateFilesList();
        }
    }

    bool MainForm::FindAndSelect(String^ text, array<String^>^ extensions) {
        TreeView^ treeView = this->filterableTreeView->TreeView;
        if (treeView == nullptr || treeView->Nodes->Count == 0) {
            return false;
        }

        array<String^>^ pathParts = text->Split(L'\\');

        TreeNode^ node = treeView->Nodes[0];
        for (int i = 0; i < pathParts->Length; ++i) {
            // expanding populates the node's children if they aren't there yet
            node->Expand();

            TreeNode^ childNode = FindChildNodeByText(node, pathParts[i]);
            if (childNode == nullptr && extensions != nullptr && i == (pathParts->Length - 1)) {
                for (int j = 0; j < extensions->Length && childNode == nullptr; ++j) {
                    childNode = FindChildNodeByText(node, pathParts[i] + extensions[j]);
                }
            }

            if (childNode == nullptr) {
                return false;
            }

            node = childNode;
        }

        treeView->SelectedNode = node;
        node->EnsureVisible();

        return true;
    }

    void MainForm::filterableTreeView_AfterSelect(System::Object^, System::Windows::Forms::TreeViewEventArgs^ e) {
        if (e->Node == nullptr) {
            return;
        }

        FileTagData^ fileData = safe_cast<FileTagData^>(e->Node->Tag);
        if (fileData == nullptr || fileData->fileHandle == kInvalidHandle) {
            return;
        }

        MyHandle file = fileData->fileHandle;
        const bool isSubFile = fileData->subFileIdx != kInvalidValue;

        const MetroFileSystem& mfs = MetroFileSystem::Get();
        if (!mfs.Empty()) {
            if (isSubFile) {
                const MetroConfigsDatabase::ConfigInfo& ci = mConfigsDatabase->GetFileByIdx(fileData->subFileIdx);

                this->statusLabel1->Text = L"config.bin";
                this->statusLabel2->Text = fileData->subFileIdx.ToString();
                this->statusLabel3->Text = ci.offset.ToString();
                this->statusLabel4->Text = ci.length.ToString();
            } else {
                const bool isFolder = mfs.IsFolder(file);
                if (isFolder) {
                    this->statusLabel1->Text = String::Empty;
                    this->statusLabel2->Text = String::Empty;
                    this->statusLabel3->Text = String::Empty;
                    this->statusLabel4->Text = String::Empty;
                } else {
                    this->statusLabel1->Text = L"0";//mf.pakIdx.ToString();
                    this->statusLabel2->Text = L"0";//mf.offset.ToString();
                    this->statusLabel3->Text = mfs.GetCompressedSize(file).ToString();
                    this->statusLabel4->Text = mfs.GetUncompressedSize(file).ToString();

                    this->DetectFileAndShow(file);
                }
            }
        }
    }

    void MainForm::filterableTreeView_AfterCollapse(System::Object^, System::Windows::Forms::TreeViewEventArgs^ e) {
        UpdateNodeIcon(e->Node, eNodeEventType::Close);
    }

    void MainForm::filterableTreeView_BeforeExpand(System::Object^, System::Windows::Forms::TreeViewCancelEventArgs^ e) {
        if (e->Node == nullptr) {
            return;
        }

        FileTagData^ fileData = safe_cast<FileTagData^>(e->Node->Tag);
        if (fileData == nullptr || fileData->populated) {
            return;
        }

        System::Windows::Forms::Cursor::Current = System::Windows::Forms::Cursors::WaitCursor;

        this->filterableTreeView->TreeView->BeginUpdate();
        this->PopulateNode(e->Node);
        this->filterableTreeView->TreeView->EndUpdate();

        System::Windows::Forms::Cursor::Current = System::Windows::Forms::Cursors::Arrow;
    }

    void MainForm::filterableTreeView_AfterExpand(System::Object^, System::Windows::Forms::TreeViewEventArgs^ e) {
        if (e->Node != nullptr) {
            UpdateNodeIcon(e->Node, eNodeEventType::Open);
        }
    }

    void MainForm::filterableTreeView_NodeMouseClick(System::Object^, System::Windows::Forms::TreeNodeMouseClickEventArgs^ e) {
        if (e->Button == System::Windows::Forms::MouseButtons::Right) {
            FileTagData^ fileData = safe_cast<FileTagData^>(e->Node->Tag);
            const bool isSubFile = fileData->subFileIdx != kInvalidValue;

            MyHandle file = fileData->fileHandle;

            const FileType fileType = isSubFile ? fileData->fileType : DetectFileType(file);

            *mExtractionCtx = {};
            mExtractionCtx->file = file;
            mExtractionCtx->type = fileType;
            mExtractionCtx->customOffset = kInvalidValue;
            mExtractionCtx->customLength = kInvalidValue;
            mExtractionCtx->customFileName = "";

            const bool isFolder = MetroFileSystem::Get().IsFolder(file);
            if (isFolder) {
                this->ctxMenuExportFolder->Show(this->filterableTreeView->TreeView, e->X, e->Y);
            } else {
                switch (fileType) {
                    case FileType::Texture: {
                        //#NOTE_SK: if this is an albedo texture - enable whole set extraction option
                        const bool isAlbedo = MetroTexturesDatabase::Get().IsAlbedo(file);
                        this->saveSurfaceSetToolStripMenuItem->Enabled = isAlbedo;

                        this->ctxMenuExportTexture->Show(this->filterableTreeView->TreeView, e->X, e->Y);
                    } break;

                    case FileType::Model: {
                        this->ctxMenuExportModel->Show(this->filterableTreeView->TreeView, e->X, e->Y);
                    } break;

                    case FileType::Sound: {
                        this->ctxMenuExportSound->Show(this->filterableTreeView->TreeView, e->X, e->Y);
                    } break;

                    case FileType::Localization: {
                        this->ctxMenuExportLocalization->Show(this->filterableTreeView->TreeView, e->X, e->Y);
                    } break;

                    case FileType::Bin: {
                        if (isSubFile) {
                            const MetroConfigsDatabase::ConfigInfo& ci = mConfigsDatabase->GetFileByIdx(fileData->subFileIdx);

                            mExtractionCtx->customOffset = ci.offset;
                            mExtractionCtx->customLength = ci.length;
                            mExtractionCtx->customFileName = NetToCharStr(e->Node->Text);
                            this->ctxMenuExportBin->Show(this->filterableTreeView->TreeView, e->X, e->Y);
                        } else {
                            this->ctxMenuExportRaw->Show(this->filterableTreeView->TreeView, e->X, e->Y);
                        }
                    } break;

                    case FileType::FolderBin: {
                    } break;

                    default: {
                        this->ctxMenuExportRaw->Show(this->filterableTreeView->TreeView, e->X, e->Y);
                    } break;
                }
            }
        }
    }

    //
    void MainForm::extractFileToolStripMenuItem_Click(System::Object^, System::EventArgs^) {
        if (!this->ExtractFile(*mExtractionCtx, fs::path())) {
            MetroEX::ShowErrorMessageBox(this, L"Failed to extract file!");
        }
    }

    void MainForm::saveAsDDSToolStripMenuItem_Click(System::Object^, System::EventArgs^) {
        mExtractionCtx->txSaveAsDds = true;
        mExtractionCtx->txUseBC3 = false;

        if (!this->ExtractTexture(*mExtractionCtx, fs::path())) {
            MetroEX::ShowErrorMessageBox(this, L"Failed to extract texture!");
        }
    }

    void MainForm::saveAsLegacyDDSToolStripMenuItem_Click(System::Object^, System::EventArgs^) {
        mExtractionCtx->txSaveAsDds = true;
        mExtractionCtx->txUseBC3 = true;

        if (!this->ExtractTexture(*mExtractionCtx, fs::path())) {
            MetroEX::ShowErrorMessageBox(this, L"Failed to extract texture!");
        }
    }

    void MainForm::saveAsTGAToolStripMenuItem_Click(System::Object^, System::EventArgs^) {
        mExtractionCtx->txSaveAsTga = true;

        if (!this->ExtractTexture(*mExtractionCtx, fs::path())) {
            MetroEX::ShowErrorMessageBox(this, L"Failed to extract texture!");
        }
    }

    void MainForm::saveAsPNGToolStripMenuItem_Click(System::Object^, System::EventArgs^) {
        mExtractionCtx->txSaveAsPng = true;

        if (!this->ExtractTexture(*mExtractionCtx, fs::path())) {
            MetroEX::ShowErrorMessageBox(this, L"Failed to extract texture!");
        }
    }

    void MainForm::saveSurfaceSetToolStripMenuItem_Click(System::Object^ sender, System::EventArgs^ e) {
        MetroSurfaceDescription surface = MetroTexturesDatabase::Get().GetSurfaceSet(mExtractionCtx->file);

        this->EnsureExtractionOptions();
        mExtractionCtx->batch = false;
        mExtractionCtx->raw = false;

        this->ExtractSurfaceSet(*mExtractionCtx, surface, fs::path());
    }

    void MainForm::saveAsOBJToolStripMenuItem_Click(System::Object^, System::EventArgs^) {
        this->EnsureExtractionOptions();
        mExtractionCtx->mdlSaveAsObj = true;
        mExtractionCtx->mdlSaveAsFbx = false;

        if (!this->ExtractModel(*mExtractionCtx, fs::path())) {
            MetroEX::ShowErrorMessageBox(this, L"Failed to extract model!");
        }
    }

    void MainForm::saveAsFBXToolStripMenuItem_Click(System::Object^, System::EventArgs^) {
        this->EnsureExtractionOptions();
        mExtractionCtx->mdlSaveAsObj = false;
        mExtractionCtx->mdlSaveAsFbx = true;

        if (!this->ExtractModel(*mExtractionCtx, fs::path())) {
            MetroEX::ShowErrorMessageBox(this, L"Failed to extract model!");
        }
    }

    void MainForm::saveAsOGGToolStripMenuItem_Click(System::Object^, System::EventArgs^) {
        mExtractionCtx->sndSaveAsOgg = true;

        if (!this->ExtractSound(*mExtractionCtx, fs::path())) {
            MetroEX::ShowErrorMessageBox(this, L"Failed to extract sound!");
        }
    }

    void MainForm::saveAsWAVToolStripMenuItem_Click(System::Object^, System::EventArgs^) {
        mExtractionCtx->sndSaveAsWav = true;

        if (!this->ExtractSound(*mExtractionCtx, fs::path())) {
            MetroEX::ShowErrorMessageBox(this, L"Failed to extract sound!");
        }
    }

    void MainForm::saveAsExcel2003XMLToolStripMenuItem_Click(System::Object^, System::EventArgs^) {
        if (!this->ExtractLocalization(*mExtractionCtx, fs::path())) {
            MetroEX::ShowErrorMessageBox(this, L"Failed to extract localization!");
        }
    }

    void MainForm::extractBinRootToolStripMenuItem_Click(System::Object^, System::EventArgs^) {
        mExtractionCtx->customOffset = kInvalidValue;
        mExtractionCtx->customLength = kInvalidValue;
        mExtractionCtx->customFileName = "";

        this->extractFileToolStripMenuItem_Click(nullptr, nullptr);
    }

    void MainForm::extractBinChunkToolStripMenuItem_Click(System::Object^, System::EventArgs^) {
        if (!this->ExtractFile(*mExtractionCtx, fs::path())) {
            MetroEX::ShowErrorMessageBox(this, L"Failed to extract bin file chunk!");
        }
    }

    void MainForm::extractFolderToolStripMenuItem_Click(System::Object^ sender, System::EventArgs^ e) {
        fs::path folderPath = ChooseFolderDialog::ChooseFolder("Choose output directory...", this->Handle.ToPointer());
        if (!folderPath.empty()) {
            mExtractionCtx->batch = true;
            mExtractionCtx->raw = true;
            mExtractionCtx->numFilesTotal = MetroFileSystem::Get().CountFilesInFolder(mExtractionCtx->file);
            mExtractionCtx->progress = 0;

            pin_ptr<IProgressDialog*> ipdPtr(&mExtractionProgressDlg);
            HRESULT hr = ::CoCreateInstance(CLSID_ProgressDialog, NULL, CLSCTX_INPROC_SERVER, __uuidof(IProgressDialog), (void**)ipdPtr);
            if (SUCCEEDED(hr)) {
                mExtractionProgressDlg->SetTitle(L"Extracting files...");
                mExtractionProgressDlg->SetLine(0, L"Please wait while your files are being extracted...", FALSE, nullptr);
                mExtractionProgressDlg->StartProgressDialog(rcast<HWND>(this->Handle.ToPointer()), nullptr,
                                                            PROGDLG_NORMAL | PROGDLG_MODAL | PROGDLG_AUTOTIME | PROGDLG_NOMINIMIZE,
                                                            nullptr);
            }

            mExtractionThread = gcnew System::Threading::Thread(gcnew System::Threading::ParameterizedThreadStart(this, &MainForm::ExtractionProcessFunc));
            mExtractionThread->Start(PathToString(folderPath));
        }
    }

    void MainForm::extractFolderWithConversionToolStripMenuItem_Click(System::Object^ sender, System::EventArgs^ e) {
        fs::path folderPath = ChooseFolderDialog::ChooseFolder("Choose output directory...", this->Handle.ToPointer());
        if (!folderPath.empty() && this->EnsureExtractionOptions()) {
            mExtractionCtx->batch = true;
            mExtractionCtx->raw = false;

            mExtractionCtx->numFilesTotal = MetroFileSystem::Get().CountFilesInFolder(mExtractionCtx->file);
            mExtractionCtx->progress = 0;

            pin_ptr<IProgressDialog*> ipdPtr(&mExtractionProgressDlg);
            HRESULT hr = ::CoCreateInstance(CLSID_ProgressDialog, NULL, CLSCTX_INPROC_SERVER, __uuidof(IProgressDialog), (void**)ipdPtr);
            if (SUCCEEDED(hr)) {
                mExtractionProgressDlg->SetTitle(L"Extracting files...");
                mExtractionProgressDlg->SetLine(0, L"Please wait while your files are being extracted...", FALSE, nullptr);
                mExtractionProgressDlg->StartProgressDialog(rcast<HWND>(this->Handle.ToPointer()), nullptr,
                    PROGDLG_NORMAL | PROGDLG_MODAL | PROGDLG_AUTOTIME | PROGDLG_NOMINIMIZE | PROGDLG_NOCANCEL,
                    nullptr);
            }

            mExtractionThread = gcnew System::Threading::Thread(gcnew System::Threading::ParameterizedThreadStart(this, &MainForm::ExtractionProcessFunc));
            mExtractionThread->Start(PathToString(folderPath));
        }
    }

    void MainForm::UpdateFilesList() {
        MetroFileSystem& mfs = MetroFileSystem::Get();

        mIsTreeFiltering = false;

        this->filterableTreeView->TreeView->BeginUpdate();
        this->filterableTreeView->TreeView->Nodes->Clear();

        if (!mfs.Empty()) {
            this->filterableTreeView->FilterTextBox->Text = String::Empty;

            // Get idx of config.bin
            mConfigBinFile = mfs.FindFile("content\\config.bin");

            String^ rootName = "FileSystem";
            if (mfs.IsSingleVFX()) {
                rootName = ToNetString(mfs.GetVFXName(0));
            }

            TreeNode^ rootNode = this->filterableTreeView->TreeView->Nodes->Add(rootName);
            rootNode->Tag = gcnew FileTagData(FileType::Folder, mfs.GetRootFolder(), kInvalidValue);
            UpdateNodeIcon(rootNode);

            this->PopulateNode(rootNode);
        }

        this->filterableTreeView->TreeView->EndUpdate();
    }

    TreeNode^ MainForm::MakeFileSystemNode(const MyHandle entry) {
        MetroFileSystem& mfs = MetroFileSystem::Get();

        const bool isFolder = mfs.IsFolder(entry);
        const bool isBinArchive = !isFolder && entry == mConfigBinFile && mConfigsDatabase != nullptr;

        FileType fileType = FileType::Folder;
        if (!isFolder) {
            fileType = isBinArchive ? FileType::BinArchive : DetectFileType(entry);
        }

        TreeNode^ node = gcnew TreeNode(ToNetString(mfs.GetName(entry)));
        FileTagData^ fileData = gcnew FileTagData(fileType, entry, kInvalidValue);
        node->Tag = fileData;
        UpdateNodeIcon(node);

        if (isBinArchive || (isFolder && mfs.GetFirstChild(entry) != kInvalidHandle)) {
            node->Nodes->Add(MakeDummyNode());
        } else {
            fileData->populated = true;
        }

        return node;
    }

    void MainForm::PopulateNode(TreeNode^ node) {
        FileTagData^ fileData = safe_cast<FileTagData^>(node->Tag);
        if (fileData == nullptr || fileData->populated) {
            return;
        }

        node->Nodes->Clear();

        switch (fileData->fileType) {
            case FileType::Folder: {
                for (const MyHandle child : CollectSortedChildren(fileData->fileHandle, mConfigBinFile)) {
                    node->Nodes->Add(this->MakeFileSystemNode(child));
                }
            } break;

            case FileType::BinArchive:
            case FileType::FolderBin: {
                this->PopulateBinArchiveNode(node, fileData->fileHandle);
            } break;
        }

        fileData->populated = true;
    }

    void MainForm::PopulateBinArchiveNode(TreeNode^ node, const MyHandle file) {
        if (mConfigsDatabase == nullptr) {
            return;
        }

        // path of the node inside the .bin database, empty for the database root
        String^ prefix = (node->Name == nullptr) ? String::Empty : node->Name;

        List<String^>^ folderNames = gcnew List<String^>();
        Dictionary<String^, int>^ knownFolders = gcnew Dictionary<String^, int>(StringComparer::OrdinalIgnoreCase);
        List<TreeNode^>^ chunkNodes = gcnew List<TreeNode^>();

        for (size_t idx = 0, numFiles = mConfigsDatabase->GetNumFiles(); idx < numFiles; ++idx) {
            const MetroConfigsDatabase::ConfigInfo& ci = mConfigsDatabase->GetFileByIdx(idx);

            String^ fullName = MakeBinChunkName(ci);
            if (prefix->Length > 0 && !fullName->StartsWith(prefix, StringComparison::OrdinalIgnoreCase)) {
                continue;
            }

            String^ restOfPath = fullName->Substring(prefix->Length);
            const int separator = restOfPath->IndexOf(L'\\');
            if (separator < 0) {
                TreeNode^ chunkNode = gcnew TreeNode(restOfPath);
                chunkNode->Tag = gcnew FileTagData(FileType::Bin, file, idx, true);
                UpdateNodeIcon(chunkNode);
                chunkNodes->Add(chunkNode);
            } else {
                String^ folderName = restOfPath->Substring(0, separator);
                if (!knownFolders->ContainsKey(folderName)) {
                    knownFolders->Add(folderName, 0);
                    folderNames->Add(folderName);
                }
            }
        }

        folderNames->Sort(StringComparer::OrdinalIgnoreCase);
        chunkNodes->Sort(gcnew Comparison<TreeNode^>(&CompareNodesByText));

        for each (String^ folderName in folderNames) {
            TreeNode^ folderNode = gcnew TreeNode(folderName);
            folderNode->Name = prefix + folderName + L"\\";
            folderNode->Tag = gcnew FileTagData(FileType::FolderBin, file, 0);
            UpdateNodeIcon(folderNode);
            folderNode->Nodes->Add(MakeDummyNode());
            node->Nodes->Add(folderNode);
        }

        for each (TreeNode^ chunkNode in chunkNodes) {
            node->Nodes->Add(chunkNode);
        }
    }

    void MainForm::OnTreeFilterChanged(String^ text) {
        if (MetroFileSystem::Get().Empty()) {
            return;
        }

        if (String::IsNullOrWhiteSpace(text)) {
            if (mIsTreeFiltering) {
                this->UpdateFilesList();
            }
        } else {
            this->BuildFilteredTree(text);
        }
    }

    void MainForm::BuildFilteredTree(String^ text) {
        MetroFileSystem& mfs = MetroFileSystem::Get();

        this->filterableTreeView->TreeView->BeginUpdate();
        this->filterableTreeView->TreeView->Nodes->Clear();

        String^ rootName = "FileSystem";
        if (mfs.IsSingleVFX()) {
            rootName = ToNetString(mfs.GetVFXName(0));
        }

        TreeNode^ rootNode = gcnew TreeNode(rootName);
        rootNode->Tag = gcnew FileTagData(FileType::Folder, mfs.GetRootFolder(), kInvalidValue, true);
        UpdateNodeIcon(rootNode);

        this->AddFilteredChildren(rootNode, mfs.GetRootFolder(), text);

        this->filterableTreeView->TreeView->Nodes->Add(rootNode);
        rootNode->ExpandAll();

        this->filterableTreeView->TreeView->EndUpdate();

        mIsTreeFiltering = true;
    }

    void MainForm::AddFilteredChildren(TreeNode^ parentNode, const MyHandle folder, String^ text) {
        MetroFileSystem& mfs = MetroFileSystem::Get();

        for (const MyHandle child : CollectSortedChildren(folder, mConfigBinFile)) {
            if (mfs.IsFolder(child)) {
                TreeNode^ folderNode = gcnew TreeNode(ToNetString(mfs.GetName(child)));
                folderNode->Tag = gcnew FileTagData(FileType::Folder, child, kInvalidValue, true);
                UpdateNodeIcon(folderNode);

                this->AddFilteredChildren(folderNode, child, text);

                if (folderNode->Nodes->Count > 0) {
                    parentNode->Nodes->Add(folderNode);
                }
            } else if (child == mConfigBinFile && mConfigsDatabase != nullptr) {
                TreeNode^ binNode = gcnew TreeNode(ToNetString(mfs.GetName(child)));
                binNode->Tag = gcnew FileTagData(FileType::BinArchive, child, kInvalidValue, true);
                UpdateNodeIcon(binNode);

                this->AddFilteredBinChildren(binNode, child, text);

                if (binNode->Nodes->Count > 0) {
                    parentNode->Nodes->Add(binNode);
                }
            } else if (ToNetString(mfs.GetName(child))->Contains(text)) {
                parentNode->Nodes->Add(this->MakeFileSystemNode(child));
            }
        }
    }

    void MainForm::AddFilteredBinChildren(TreeNode^ binNode, const MyHandle file, String^ text) {
        Dictionary<String^, TreeNode^>^ folderNodes = gcnew Dictionary<String^, TreeNode^>(StringComparer::OrdinalIgnoreCase);

        for (size_t idx = 0, numFiles = mConfigsDatabase->GetNumFiles(); idx < numFiles; ++idx) {
            const MetroConfigsDatabase::ConfigInfo& ci = mConfigsDatabase->GetFileByIdx(idx);

            array<String^>^ pathParts = MakeBinChunkName(ci)->Split(L'\\');
            String^ fileName = pathParts[pathParts->Length - 1];
            if (!fileName->Contains(text)) {
                continue;
            }

            TreeNode^ parentNode = binNode;
            String^ curPath = String::Empty;
            for (int i = 0; i < (pathParts->Length - 1); ++i) {
                curPath += pathParts[i] + L"\\";

                TreeNode^ folderNode = nullptr;
                if (!folderNodes->TryGetValue(curPath, folderNode)) {
                    folderNode = gcnew TreeNode(pathParts[i]);
                    folderNode->Name = curPath;
                    folderNode->Tag = gcnew FileTagData(FileType::FolderBin, file, 0, true);
                    UpdateNodeIcon(folderNode);
                    parentNode->Nodes->Add(folderNode);
                    folderNodes->Add(curPath, folderNode);
                }

                parentNode = folderNode;
            }

            TreeNode^ chunkNode = gcnew TreeNode(fileName);
            chunkNode->Tag = gcnew FileTagData(FileType::Bin, file, idx, true);
            UpdateNodeIcon(chunkNode);
            parentNode->Nodes->Add(chunkNode);
        }
    }

    void MainForm::DetectFileAndShow(MyHandle file) {
        const bool isFolder = MetroFileSystem::Get().IsFolder(file);
        if (isFolder) {
            return;
        }

        const FileType fileType = DetectFileType(file);

        switch (fileType) {
            case FileType::Texture: {
                this->ShowTexture(file);
            } break;

            case FileType::Model: {
                this->ShowModel(file);
            } break;

            case FileType::Sound: {
                this->ShowSound(file);
            } break;

        //case FileType::Level: {
        //    this->ShowLevel(file);
        //} break;

            case FileType::Localization: {
                this->ShowLocalization(file);
            } break;
        }
    }

    void MainForm::ShowTexture(MyHandle file) {
        MemStream stream = MetroFileSystem::Get().OpenFileStream(file);
        if (stream) {
            MetroTexture texture;
            if (texture.LoadFromData(stream, file)) {
                if (texture.IsCubemap()) {
                    this->SwitchViewPanel(PanelType::Model);
                    mRenderPanel->SetCubemap(&texture);
                } else {
                    this->SwitchViewPanel(PanelType::Texture);
                    mImagePanel->SetTexture(&texture);
                }

                this->SwitchInfoPanel(PanelType::Texture);

                mImageInfoPanel->ImgPropCompressionText = texture.IsCubemap() ? L"BC6H" : L"BC7";;
                mImageInfoPanel->ImgPropWidthText = texture.GetWidth().ToString();
                mImageInfoPanel->ImgPropHeightText = texture.GetHeight().ToString();
                mImageInfoPanel->ImgPropsMipsText = texture.GetNumMips().ToString();
            }
        }
    }

    void MainForm::ShowModel(MyHandle file) {
        this->SwitchViewPanel(PanelType::Model);
        this->SwitchInfoPanel(PanelType::Model);

        MemStream stream = MetroFileSystem::Get().OpenFileStream(file);
        if (stream) {
            MetroModel* mdl = new MetroModel();
            if (mdl->LoadFromData(stream, file)) {
                mRenderPanel->SetModel(nullptr);

                mModelInfoPanel->ClearLodsList();
                mModelInfoPanel->AddLodIdToList(0);
                if (mdl->HasLodModel(0)) {
                    mModelInfoPanel->AddLodIdToList(1);
                    if (mdl->HasLodModel(1)) {
                        mModelInfoPanel->AddLodIdToList(2);
                    }
                }
                mModelInfoPanel->SelectLod(0);

                mRenderPanel->SetModel(mdl);

                mModelInfoPanel->ClearMotionsList();
                if (mdl->IsAnimated()) {
                    const size_t numMotions = mdl->GetNumMotions();
                    for (size_t i = 0; i < numMotions; ++i) {
                        const CharString& motionName = mdl->GetMotionName(i);
                        mModelInfoPanel->AddMotionToList(ToNetString(motionName));
                    }

                    mModelInfoPanel->MdlPropTypeText = L"Animated";
                    mModelInfoPanel->MdlPropJointsText = mdl->GetSkeleton()->GetNumBones().ToString();
                    mModelInfoPanel->MdlPropNumAnimsText = numMotions.ToString();
                } else {
                    mModelInfoPanel->MdlPropTypeText = L"Static";
                    mModelInfoPanel->MdlPropJointsText = L"0";
                    mModelInfoPanel->MdlPropNumAnimsText = L"0";
                }

                size_t numVertices = 0, numTriangles = 0;
                const size_t numMeshes = mdl->GetNumMeshes();
                for (size_t i = 0; i < numMeshes; ++i) {
                    const MetroMesh* mesh = mdl->GetMesh(i);
                    numVertices += mesh->vertices.size();
                    numTriangles += mesh->faces.size();
                }

                mModelInfoPanel->MdlPropVerticesText = numVertices.ToString();
                mModelInfoPanel->MdlPropTrianglesText = numTriangles.ToString();

                mModelInfoPanel->MdlPropPlayStopAnimBtnText = L"Play";

                if (mDlgModelInfo) {
                    mDlgModelInfo->SetModel(mdl);
                }
            } else {
                MySafeDelete(mdl);
            }
        }
    }

    void MainForm::ShowSound(MyHandle file) {
        this->SwitchViewPanel(PanelType::Sound);
        this->SwitchInfoPanel(PanelType::Sound);

        MemStream stream = MetroFileSystem::Get().OpenFileStream(file);
        if (stream) {
            MetroSound* snd = new MetroSound();
            if (snd->LoadFromData(stream)) {
                mSoundPanel->SetSound(snd);
            } else {
                MySafeDelete(snd);
            }
        }
    }

    void MainForm::ShowLocalization(MyHandle file) {
        this->SwitchViewPanel(PanelType::Localization);
        this->SwitchInfoPanel(PanelType::Localization);

        MemStream stream = MetroFileSystem::Get().OpenFileStream(file);
        if (stream) {
            MetroLocalization loc;
            if (loc.LoadFromData(stream)) {
                mLocalizationPanel->SetLocalizationTable(&loc);
            }
        }
    }

    void MainForm::SwitchViewPanel(PanelType t) {
        switch (t) {
            case PanelType::Texture: {
                mRenderPanel->Hide();
                mSoundPanel->Hide();
                mLocalizationPanel->Hide();
                mImagePanel->Show();
            } break;

            case PanelType::Model: {
                mImagePanel->Hide();
                mSoundPanel->Hide();
                mLocalizationPanel->Hide();
                mRenderPanel->Show();
            } break;

            case PanelType::Sound: {
                mImagePanel->Hide();
                mRenderPanel->Hide();
                mLocalizationPanel->Hide();
                mSoundPanel->Show();
            } break;

            case PanelType::Localization: {
                mImagePanel->Hide();
                mRenderPanel->Hide();
                mSoundPanel->Hide();
                mLocalizationPanel->Show();
            } break;
        }
    }

    void MainForm::SwitchInfoPanel(PanelType t) {
        switch (t) {
            case PanelType::Texture: {
                mModelInfoPanel->Dock = System::Windows::Forms::DockStyle::None;
                mModelInfoPanel->Hide();

                mImageInfoPanel->Location = System::Drawing::Point(0, 0);
                mImageInfoPanel->Dock = System::Windows::Forms::DockStyle::Fill;
                mImageInfoPanel->Show();
            } break;

            case PanelType::Model: {
                mImageInfoPanel->Dock = System::Windows::Forms::DockStyle::None;
                mImageInfoPanel->Hide();

                mModelInfoPanel->Location = System::Drawing::Point(0, 0);
                mModelInfoPanel->Dock = System::Windows::Forms::DockStyle::Fill;
                mModelInfoPanel->Show();
            } break;

            case PanelType::Sound:
            case PanelType::Localization: {
                mModelInfoPanel->Dock = System::Windows::Forms::DockStyle::None;
                mModelInfoPanel->Hide();

                mImageInfoPanel->Dock = System::Windows::Forms::DockStyle::None;
                mImageInfoPanel->Hide();
            } break;
        }
    }

    // extraction
    bool MainForm::EnsureExtractionOptions() {
        bool result = true;

        MEXSettings& s = MEXSettings::Get();

        if (s.extraction.askEveryTime) {
            DlgSettings dlg;
            dlg.Icon = this->Icon;
            auto dlgResult = dlg.ShowDialog(this);
            if (dlgResult == System::Windows::Forms::DialogResult::Cancel) {
                result = false;
            }
        }

        // models
        mExtractionCtx->mdlSaveAsObj = (s.extraction.modelFormat == MEXSettings::Extraction::MdlFormat::Obj);
        mExtractionCtx->mdlSaveAsFbx = (s.extraction.modelFormat == MEXSettings::Extraction::MdlFormat::Fbx);
        mExtractionCtx->mdlSaveWithAnims = s.extraction.modelSaveWithAnims;
        mExtractionCtx->mdlAnimsSeparate = s.extraction.modelAnimsSeparate;
        mExtractionCtx->mdlSaveWithTextures = s.extraction.modelSaveWithTextures;
        mExtractionCtx->mdlExcludeCollision = s.extraction.modelExcludeCollision;
        mExtractionCtx->mdlSaveLods = s.extraction.modelSaveLods;
        // textures
        mExtractionCtx->txSaveAsDds = (s.extraction.textureFormat == MEXSettings::Extraction::TexFormat::Dds || s.extraction.textureFormat == MEXSettings::Extraction::TexFormat::LegacyDds);
        mExtractionCtx->txUseBC3 = (s.extraction.textureFormat == MEXSettings::Extraction::TexFormat::LegacyDds);
        mExtractionCtx->txSaveAsTga = (s.extraction.textureFormat == MEXSettings::Extraction::TexFormat::Tga);
        mExtractionCtx->txSaveAsPng = (s.extraction.textureFormat == MEXSettings::Extraction::TexFormat::Png);
        // sounds
        mExtractionCtx->sndSaveAsOgg = (s.extraction.soundFormat == MEXSettings::Extraction::SndFormat::Ogg);
        mExtractionCtx->sndSaveAsWav = (s.extraction.soundFormat == MEXSettings::Extraction::SndFormat::Wav);

        return result;
    }

    CharString MainForm::MakeFileOutputName(MyHandle file, const FileExtractionCtx& ctx) {
        CharString name = MetroFileSystem::Get().GetName(file);

        switch (ctx.type) {
            case FileType::Texture: {
                const CharString::size_type dotPos = name.find_last_of('.');
                const size_t replaceLen = name.size() - dotPos;

                if (ctx.txSaveAsDds) {
                    name = name.replace(dotPos, replaceLen, ".dds");
                } else if (ctx.txSaveAsTga) {
                    name = name.replace(dotPos, replaceLen, ".tga");
                } else if (ctx.txSaveAsPng) {
                    name = name.replace(dotPos, replaceLen, ".png");
                }
            } break;

            case FileType::Model: {
                const CharString::size_type dotPos = name.find_last_of('.');
                const size_t replaceLen = name.size() - dotPos;

                if (ctx.mdlSaveAsObj) {
                    name = name.replace(dotPos, replaceLen, ".obj");
                } else {
                    name = name.replace(dotPos, replaceLen, ".fbx");
                }
            } break;

            case FileType::Sound: {
                if (ctx.sndSaveAsOgg) {
                    name[name.size() - 3] = 'o';
                    name[name.size() - 2] = 'g';
                    name[name.size() - 1] = 'g';
                } else {
                    name[name.size() - 3] = 'w';
                    name[name.size() - 2] = 'a';
                    name[name.size() - 1] = 'v';
                }
            } break;

            case FileType::Localization: {
                name[name.size() - 3] = 'x';
                name[name.size() - 2] = 'm';
                name[name.size() - 1] = 'l';
            } break;
        }

        return name;
    }

    void MainForm::TextureSaveHelper(const fs::path& folderPath, const FileExtractionCtx& ctx, const CharString& name) {
        CharString textureName = CharString("content\\textures\\") + name;

        const MetroFileSystem& mfs = MetroFileSystem::Get();

        CharString textureNameSrc = textureName + ".4096";
        MyHandle textureHandle = mfs.FindFile(textureNameSrc);
        if (textureHandle == kInvalidHandle) {
            textureNameSrc = textureName + ".2048";
            textureHandle = mfs.FindFile(textureNameSrc);
        }
        if (textureHandle == kInvalidHandle) {
            textureNameSrc = textureName + ".1024";
            textureHandle = mfs.FindFile(textureNameSrc);
        }
        if (textureHandle == kInvalidHandle) {
            textureNameSrc = textureName + ".512";
            textureHandle = mfs.FindFile(textureNameSrc);
        }

        if (textureHandle == kInvalidHandle) {
            // last try - Redux .bin
            textureNameSrc = textureName + ".bin";
            textureHandle = mfs.FindFile(textureNameSrc);
        }

        if (textureHandle == kInvalidHandle) {
            FileExtractionCtx tmpCtx = ctx;
            tmpCtx.type = FileType::Texture;
            tmpCtx.file = textureHandle;
            tmpCtx.txSaveAsDds = false;
            tmpCtx.txSaveAsTga = true;
            tmpCtx.txSaveAsPng = false;

            CharString outName = this->MakeFileOutputName(textureHandle, tmpCtx);
            this->ExtractTexture(tmpCtx, folderPath / outName);
        }
    }

    bool MainForm::ExtractFile(const FileExtractionCtx& ctx, const fs::path& outPath) {
        bool result = false;

        const CharString& fileName = MetroFileSystem::Get().GetName(ctx.file);
        String^ name = ctx.customFileName.empty() ?
            ToNetString(fileName) :
            ToNetString(ctx.customFileName);

        fs::path resultPath = outPath;
        if (resultPath.empty()) {
            SaveFileDialog sfd;
            sfd.Title = L"Save file...";
            sfd.Filter = L"All files (*.*)|*.*";
            sfd.FileName = name;
            sfd.RestoreDirectory = true;
            sfd.OverwritePrompt = true;

            if (sfd.ShowDialog(this) == System::Windows::Forms::DialogResult::OK) {
                resultPath = StringToPath(sfd.FileName);
            } else {
                return true;
            }
        }

        if (!resultPath.empty()) {
            MemStream stream = MetroFileSystem::Get().OpenFileStream(ctx.file);
            if (stream) {
                std::ofstream file(resultPath, std::ofstream::binary);
                if (file.good()) {
                    const bool hasCustomLength = ctx.customLength != kInvalidValue;
                    const bool hasCustomOffset = ctx.customOffset != kInvalidValue;

                    size_t lengthToWrite = hasCustomLength ? ctx.customLength : stream.Remains();

                    if (hasCustomOffset) {
                        stream.SetCursor(ctx.customOffset);

                        if (hasCustomLength == false) {
                            lengthToWrite = stream.Length() - ctx.customOffset;
                        }
                    }

                    file.write(rcast<const char*>(stream.GetDataAtCursor()), lengthToWrite);
                    file.flush();

                    result = true;
                }
            }
        }

        return result;
    }

    bool MainForm::ExtractTexture(const FileExtractionCtx& ctx, const fs::path& outPath) {
        bool result = false;

        fs::path resultPath = outPath;
        if (resultPath.empty()) {
            String^ title;
            String^ filter;
            if (ctx.txSaveAsDds) {
                title = L"Save DDS texture...";
                filter = L"DirectDraw Surface (*.dds)|*.dds";
            } else if (ctx.txSaveAsTga) {
                title = L"Save TGA texture...";
                filter = L"Targa images (*.tga)|*.tga";
            } else {
                title = L"Save PNG texture...";
                filter = L"PNG images (*.png)|*.png";
            }

            CharString nameWithExt = this->MakeFileOutputName(ctx.file, ctx);

            SaveFileDialog sfd;
            sfd.Title = title;
            sfd.Filter = filter;
            sfd.FileName = ToNetString(nameWithExt);
            sfd.RestoreDirectory = true;
            sfd.OverwritePrompt = true;

            if (sfd.ShowDialog(this) == System::Windows::Forms::DialogResult::OK) {
                resultPath = StringToPath(sfd.FileName);
            } else {
                return true;
            }
        }

        if (!resultPath.empty()) {
            MemStream stream = MetroFileSystem::Get().OpenFileStream(ctx.file);
            if (stream) {
                MetroTexture texture;
                if (texture.LoadFromData(stream, ctx.file)) {
                    if (ctx.txSaveAsDds) {
                        if (ctx.txUseBC3) {
                            result = texture.SaveAsLegacyDDS(resultPath);
                        } else {
                            result = texture.SaveAsDDS(resultPath);
                        }
                    } else if (ctx.txSaveAsTga) {
                        result = texture.SaveAsTGA(resultPath);
                    } else {
                        result = texture.SaveAsPNG(resultPath);
                    }
                }
            }
        }

        return result;
    }

    bool MainForm::ExtractSurfaceSet(const FileExtractionCtx& ctx, const MetroSurfaceDescription& surface, const fs::path& outFolder) {
        bool result = false;

        fs::path folderPath = outFolder;
        if (folderPath.empty()) {
            folderPath = ChooseFolderDialog::ChooseFolder("Choose output directory...", this->Handle.ToPointer());
        }

        if (!folderPath.empty()) {
            const MetroFileSystem& mfs = MetroFileSystem::Get();

            FileExtractionCtx setCtx = ctx;
            setCtx.type = FileType::Texture;

#define EXTRACT_SURFACE_TEXTURE(tex_name)                                                       \
            if (!surface.tex_name##Path.empty()) {                                              \
                const MyHandle file = mfs.FindFile(surface.tex_name##Path);                     \
                if (file != kInvalidHandle) {                                                   \
                    setCtx.file = file;                                                         \
                    CharString nameWithExt = this->MakeFileOutputName(file, setCtx);            \
                    result = this->ExtractTexture(setCtx, folderPath / nameWithExt) && result;  \
                }                                                                               \
            }

            EXTRACT_SURFACE_TEXTURE(albedo);
            EXTRACT_SURFACE_TEXTURE(bump);
            EXTRACT_SURFACE_TEXTURE(normalmap);
            EXTRACT_SURFACE_TEXTURE(detail);

#undef EXTRACT_SURFACE_TEXTURE
        }

        return result;
    }

    bool MainForm::ExtractModel(const FileExtractionCtx& ctx, const fs::path& outPath) {
        bool result = false;

        fs::path resultPath = outPath;
        if (resultPath.empty()) {
            String^ title;
            String^ filter;
            if (ctx.mdlSaveAsObj) {
                title = L"Save OBJ model...";
                filter = L"OBJ model (*.obj)|*.obj";
            } else {
                title = L"Save FBX model...";
                filter = L"FBX model (*.fbx)|*.fbx";
            }

            CharString nameWithExt = this->MakeFileOutputName(ctx.file, ctx);

            SaveFileDialog sfd;
            sfd.Title = title;
            sfd.Filter = filter;
            sfd.FileName = ToNetString(nameWithExt);
            sfd.RestoreDirectory = true;
            sfd.OverwritePrompt = true;

            if (sfd.ShowDialog(this) == System::Windows::Forms::DialogResult::OK) {
                resultPath = StringToPath(sfd.FileName);
            } else {
                return true;
            }
        }

        const MEXSettings& settings = MEXSettings::Get();

        if (!resultPath.empty()) {
            MemStream& stream = MetroFileSystem::Get().OpenFileStream(ctx.file);
            if (stream) {
                MetroModel mdl;
                if (mdl.LoadFromData(stream, ctx.file)) {
                    if (ctx.mdlSaveAsObj) {
                        mdl.SaveAsOBJ(resultPath, ctx.mdlExcludeCollision);
                        if (ctx.mdlSaveLods) {
                            fs::path resultLodPath;
                            if (mdl.HasLodModel(0)) {
                                resultLodPath = resultPath.replace_extension("").native() + L"_lod1.obj";
                                mdl.GetLodModel(0)->SaveAsOBJ(resultLodPath, ctx.mdlExcludeCollision);
                            }
                            if (mdl.HasLodModel(1)) {
                                resultLodPath = resultPath.replace_extension("").native() + L"_lod2.obj";
                                mdl.GetLodModel(1)->SaveAsOBJ(resultLodPath, ctx.mdlExcludeCollision);
                            }
                        }
                    } else {
                        size_t fbxOptions = MetroModel::FBX_Export_Mesh | MetroModel::FBX_Export_Skeleton;
                        if (ctx.mdlExcludeCollision) {
                            fbxOptions |= MetroModel::FBX_Export_ExcludeCollision;
                        }
                        if (settings.extraction.modelSaveWithAnims && !settings.extraction.modelAnimsSeparate) {
                            fbxOptions |= MetroModel::FBX_Export_Animation;
                        }

                        mdl.SaveAsFBX(resultPath, fbxOptions);

                        if (ctx.mdlSaveLods) {
                            fs::path resultLodPath;
                            if (mdl.HasLodModel(0)) {
                                resultLodPath = resultPath.replace_extension("").native() + L"_lod1.fbx";
                                mdl.GetLodModel(0)->SaveAsFBX(resultLodPath, ctx.mdlExcludeCollision);
                            }
                            if (mdl.HasLodModel(1)) {
                                resultLodPath = resultPath.replace_extension("").native() + L"_lod2.fbx";
                                mdl.GetLodModel(1)->SaveAsFBX(resultLodPath, ctx.mdlExcludeCollision);
                            }
                        }

                        if (settings.extraction.modelSaveWithAnims && settings.extraction.modelAnimsSeparate) {
                            fbxOptions = MetroModel::FBX_Export_Skeleton | MetroModel::FBX_Export_Animation;

                            fs::path modelBasePath = resultPath.parent_path() / resultPath.stem();
                            for (size_t motionIdx = 0; motionIdx != mdl.GetNumMotions(); ++motionIdx) {
                                const MetroMotion* motion = mdl.GetMotion(motionIdx);
                                fs::path animPath = modelBasePath.native() + fs::path("@" + motion->GetName()).native() + L".fbx";
                                mdl.SaveAsFBX(animPath, fbxOptions, motionIdx);
                            }
                        }
                    }

                    if (!ctx.batch && ctx.mdlSaveWithTextures) {
                        fs::path folderPath = resultPath.parent_path();
                        for (size_t i = 0; i < mdl.GetNumMeshes(); ++i) {
                            const MetroMesh* mesh = mdl.GetMesh(i);
                            if (!mesh->materials.empty()) {
                                const CharString& textureName = mesh->materials.front();

                                if (settings.extraction.modelSaveSurfaceSet) {
                                    MetroSurfaceDescription surface = MetroTexturesDatabase::Get().GetSurfaceSet(textureName);
                                    this->ExtractSurfaceSet(ctx, surface, folderPath);
                                } else {
                                    const CharString& sourceName = MetroTexturesDatabase::Get().GetSourceName(textureName);
                                    this->TextureSaveHelper(folderPath, ctx, sourceName);
                                }
                            }
                        }
                    }

                    result = true;
                }
            }
        }

        return result;
    }

    bool MainForm::ExtractMotion(const FileExtractionCtx& ctx, const fs::path& outPath) {
        bool result = false;

        MetroModel* model = mRenderPanel->GetModel();
        const int motionIdx = mModelInfoPanel->SelectedMotionIdx;

        if (!model || motionIdx < 0) {
            return false;
        }

        fs::path resultPath = outPath;
        if (resultPath.empty()) {
            String^ name = ToNetString(model->GetMotionName(scast<size_t>(motionIdx)));

            SaveFileDialog sfd;
            sfd.Title = L"Save FBX animation...";
            sfd.Filter = L"FBX animation (*.fbx)|*.fbx";
            sfd.FileName = name + L".fbx";
            sfd.RestoreDirectory = true;
            sfd.OverwritePrompt = true;

            if (sfd.ShowDialog(this) == System::Windows::Forms::DialogResult::OK) {
                resultPath = StringToPath(sfd.FileName);
            } else {
                return true;
            }
        }

        if (!resultPath.empty()) {
            System::Windows::Forms::Cursor::Current = System::Windows::Forms::Cursors::WaitCursor;
            const size_t fbxOptions = MetroModel::FBX_Export_Skeleton | MetroModel::FBX_Export_Animation;

            result = model->SaveAsFBX(resultPath, fbxOptions, scast<size_t>(motionIdx));

            System::Windows::Forms::Cursor::Current = System::Windows::Forms::Cursors::Arrow;
        }

        return result;
    }

    bool MainForm::ExtractSound(const FileExtractionCtx& ctx, const fs::path& outPath) {
        bool result = false;

        fs::path resultPath = outPath;
        if (resultPath.empty()) {
            String^ title;
            String^ filter;
            if (ctx.sndSaveAsOgg) {
                title = L"Save Ogg sound...";
                filter = L"Ogg Vorbis (*.ogg)|*.ogg";
            } else {
                title = L"Save WAV sound...";
                filter = L"Wave sounds (*.wav)|*.wav";
            }

            CharString nameWithExt = this->MakeFileOutputName(ctx.file, ctx);

            SaveFileDialog sfd;
            sfd.Title = title;
            sfd.Filter = filter;
            sfd.FileName = ToNetString(nameWithExt);
            sfd.RestoreDirectory = true;
            sfd.OverwritePrompt = true;

            if (sfd.ShowDialog(this) == System::Windows::Forms::DialogResult::OK) {
                resultPath = StringToPath(sfd.FileName);
            } else {
                return true;
            }
        }

        if (!resultPath.empty()) {
            MemStream stream = MetroFileSystem::Get().OpenFileStream(ctx.file);
            if (stream) {
                MetroSound sound;
                if (sound.LoadFromData(stream)) {
                    if (ctx.sndSaveAsOgg) {
                        result = sound.SaveAsOGG(resultPath);
                    } else {
                        result = sound.SaveAsWAV(resultPath);
                    }
                }
            }
        }

        return result;
    }

    bool MainForm::ExtractLocalization(const FileExtractionCtx& ctx, const fs::path& outPath) {
        bool result = false;

        fs::path resultPath = outPath;
        if (resultPath.empty()) {
            CharString nameWithExt = this->MakeFileOutputName(ctx.file, ctx);

            SaveFileDialog sfd;
            sfd.Title = L"Save Excel 2003 XML...";
            sfd.Filter = L"Excel 2003 XML (*.xml)|*.xml";
            sfd.FileName = ToNetString(nameWithExt);
            sfd.RestoreDirectory = true;
            sfd.OverwritePrompt = true;

            if (sfd.ShowDialog(this) == System::Windows::Forms::DialogResult::OK) {
                resultPath = StringToPath(sfd.FileName);
            } else {
                return true;
            }
        }

        if (!resultPath.empty()) {
            MemStream stream = MetroFileSystem::Get().OpenFileStream(ctx.file);
            if (stream) {
                MetroLocalization loc;
                if (loc.LoadFromData(stream)) {
                    result = loc.SaveToExcel2003(resultPath);
                }
            }
        }

        return result;
    }

    bool MainForm::ExtractFolderComplete(const FileExtractionCtx& ctx, const fs::path& outPath) {
        bool result = false;

        const MetroFileSystem& mfs = MetroFileSystem::Get();
        const CharString& folderName = mfs.GetName(ctx.file);

        fs::path curPath = outPath / folderName;
        fs::create_directories(curPath);

        FileExtractionCtx tmpCtx = ctx;
        for (MyHandle child = mfs.GetFirstChild(ctx.file); child != kInvalidHandle; child = mfs.GetNextChild(child)) {
            tmpCtx.file = child;
            tmpCtx.type = DetectFileType(child);

            const bool isFolder = mfs.IsFolder(child);
            if (isFolder) {
                this->ExtractFolderComplete(tmpCtx, curPath);
            } else {
                if (ctx.raw) {
                    const CharString& childName = mfs.GetName(child);
                    fs::path filePath = curPath / childName;
                    this->ExtractFile(tmpCtx, filePath);
                } else {
                    fs::path filePath = curPath / this->MakeFileOutputName(child, tmpCtx);
                    switch (tmpCtx.type) {
                        case FileType::Texture: {
                            this->ExtractTexture(tmpCtx, filePath);
                        } break;

                        case FileType::Model: {
                            this->ExtractModel(tmpCtx, filePath);
                        } break;

                        case FileType::Sound: {
                            this->ExtractSound(tmpCtx, filePath);
                        } break;

                        case FileType::Localization: {
                            this->ExtractLocalization(tmpCtx, filePath);
                        } break;

                        default: {
                            this->ExtractFile(tmpCtx, filePath);
                        } break;
                    }
                }

                mExtractionCtx->progress++;
                if (mExtractionProgressDlg) {
                    mExtractionProgressDlg->SetProgress64(mExtractionCtx->progress, mExtractionCtx->numFilesTotal);
                }
            }
        }

        result = true;

        return result;
    }

    void MainForm::ExtractionProcessFunc(Object^ folderPath) {
        this->ExtractFolderComplete(*mExtractionCtx, StringToPath(folderPath->ToString()));

        if (mExtractionProgressDlg) {
            mExtractionProgressDlg->StopProgressDialog();
            MySafeRelease(mExtractionProgressDlg);
        }
    }

    // property panels
    // model props
    void MainForm::lstMdlPropMotions_SelectedIndexChanged(int selection) {
        if (selection >= 0) {
            mRenderPanel->SwitchMotion(scast<size_t>(selection));
        }
    }

    void MainForm::lstLods_SelectedIndexChanged(int selection) {
        if (selection >= 0 && selection <= 2) {
            mRenderPanel->SetLod(selection);
        }
    }

    void MainForm::btnMdlPropPlayStopAnim_Click(System::Object^) {
        mRenderPanel->PlayAnim(!mRenderPanel->IsPlayingAnim());
        mModelInfoPanel->MdlPropPlayStopAnimBtnText = mRenderPanel->IsPlayingAnim() ? L"Stop" : L"Play";
    }

    void MainForm::btnModelInfo_Click(System::Object^) {
        if (!mDlgModelInfo) {
            mDlgModelInfo = gcnew MetroEX::DlgModelInfo();
            mDlgModelInfo->Closed += gcnew System::EventHandler(this, &MetroEX::MainForm::OnDlgModelInfo_Closed);

            mDlgModelInfo->Icon = this->Icon;
            mDlgModelInfo->SetModel(mRenderPanel->GetModel());
            mDlgModelInfo->Show();
        }
    }

    void MainForm::btnModelExportMotion_Click(System::Object^) {
        if (!this->ExtractMotion(*mExtractionCtx, fs::path())) {
            MetroEX::ShowErrorMessageBox(this, L"Failed to extract motion!");
        }
    }

    void MainForm::OnDlgModelInfo_Closed(System::Object^, System::EventArgs^) {
        MySafeDelete(mDlgModelInfo);
    }

    // patch creation
    void MainForm::archiveToolToolStripMenuItem_Click(System::Object^, System::EventArgs^) {
        DlgCreateArchive dlg;
        dlg.Icon = this->Icon;
        dlg.ShowDialog(this);
    }

    void MainForm::texturesConverterToolStripMenuItem_Click(System::Object^, System::EventArgs^) {
        DlgConvertTextures dlg;
        dlg.Icon = this->Icon;
        dlg.ShowDialog(this);
    }

    void MainForm::localizationConversionToolStripMenuItem_Click(System::Object^ sender, System::EventArgs^ e) {
        OpenFileDialog ofd;
        ofd.Title = L"Choose localization file to convert";
        ofd.Filter = L"Excel 2003 XML files (*.xml)|*.xml";
        ofd.FilterIndex = 0;
        ofd.CheckFileExists = true;
        ofd.RestoreDirectory = true;
        if (ofd.ShowDialog(this) == System::Windows::Forms::DialogResult::OK) {
            fs::path srcPath = StringToPath(ofd.FileName);

            MetroLocalization loc;
            if (loc.LoadFromExcel2003(srcPath)) {
                String^ fileName = PathToString(srcPath.stem().native() + L".lng");

                SaveFileDialog sfd;
                sfd.Title = L"Where to save Metro localization...";
                sfd.Filter = L"Metro localization (*.lng)|*.lng";
                sfd.FileName = fileName;
                sfd.RestoreDirectory = true;
                sfd.OverwritePrompt = true;
                if (sfd.ShowDialog(this) == System::Windows::Forms::DialogResult::OK) {
                    if (loc.Save(StringToPath(sfd.FileName))) {
                        MetroEX::ShowInfoMessageBox(this, L"Conversion succeeded!");
                    } else {
                        MetroEX::ShowErrorMessageBox(this, L"Failed to save " + sfd.FileName);
                    }
                }
            } else {
                MetroEX::ShowErrorMessageBox(this, L"Failed to open " + ofd.FileName);
            }
        }
    }
}
