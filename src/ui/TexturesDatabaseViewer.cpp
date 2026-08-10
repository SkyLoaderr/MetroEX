#include "TexturesDatabaseViewer.h"
#include "ui/MainForm.h"
#include "ui/NodeSorter.h"
#include "ui/UIHelpers.h"


static const int kImageIdxFile = 6;
static const int kImageIdxFolderClosed = 0;
static const int kImageIdxFolderOpen = 1;

namespace MetroEX {
    TexturesDatabaseViewer::TexturesDatabaseViewer(MainForm^ form, System::Windows::Forms::ImageList^ imageList) {
        InitializeComponent();

        mMainForm = form;
        mOriginalRootNode = nullptr;
        mPropertiesViewer = nullptr;

        mFileExtensions = gcnew array<String^>(4);
        mFileExtensions[0] = L".4096";
        mFileExtensions[1] = L".2048";
        mFileExtensions[2] = L".1024";
        mFileExtensions[3] = L".512";

        this->filterableTreeView->TreeView->ImageList = imageList;

        this->FillWithData();
    }

    TexturesDatabaseViewer::~TexturesDatabaseViewer() {
        this->filterableTreeView->TreeView->Nodes->Clear();

        MySafeDelete(mFileExtensions);
        MySafeDelete(mPropertiesViewer);

        MySafeDelete(components);
    }

    void TexturesDatabaseViewer::FillWithData() {
        if (!MetroTexturesDatabase::Get().Good()) {
            return;
        }

        System::Windows::Forms::Cursor::Current = System::Windows::Forms::Cursors::WaitCursor;

        this->filterableTreeView->TreeView->BeginUpdate();
        this->filterableTreeView->TreeView->Nodes->Clear();

        mOriginalRootNode = this->filterableTreeView->TreeView->Nodes->Add(L"content");
        mOriginalRootNode->ImageIndex = kImageIdxFolderClosed;
        mOriginalRootNode->SelectedImageIndex = kImageIdxFolderClosed;

        for (size_t i = 0, end = MetroTexturesDatabase::Get().GetNumTextures(); i < end; ++i) {
            const MetroTextureInfo& texInfo = MetroTexturesDatabase::Get().GetTextureInfo(i);
            String^ name = ToNetString(texInfo.name);
            array<String^>^ parts = name->Split(L'\\');

            TreeNode^ parentNode = mOriginalRootNode;
            for (int i = 0; i < parts->Length - 1; ++i) {
                TreeNode^ foundNode = FindNode(parentNode, parts[i]);

                if (foundNode == nullptr) {
                    parentNode = parentNode->Nodes->Add(parts[i]);
                    parentNode->ImageIndex = kImageIdxFolderClosed;
                    parentNode->SelectedImageIndex = kImageIdxFolderClosed;
                } else {
                    parentNode = foundNode;
                }
            }

            TreeNode^ node = parentNode->Nodes->Add(parts[parts->Length - 1]);
            node->Tag = i;
            node->ImageIndex = kImageIdxFile;
            node->SelectedImageIndex = kImageIdxFile;
        }

        NodeSorter^ sorter = gcnew NodeSorter();
        this->SortNodesRecursively(mOriginalRootNode, sorter);
        delete sorter;

        mOriginalRootNode->Expand();
        this->filterableTreeView->TreeView->EndUpdate();

        this->filterableTreeView->TreeView->NodeMouseDoubleClick += (gcnew TreeNodeMouseClickEventHandler(this, &TexturesDatabaseViewer::filterableTreeView_NodeMouseDoubleClick));
        this->filterableTreeView->TreeView->AfterCollapse += (gcnew TreeViewEventHandler(this, &TexturesDatabaseViewer::filterableTreeView_AfterCollapse));
        this->filterableTreeView->TreeView->AfterExpand += (gcnew TreeViewEventHandler(this, &TexturesDatabaseViewer::filterableTreeView_AfterExpand));
        this->filterableTreeView->TreeView->AfterSelect += (gcnew TreeViewEventHandler(this, &TexturesDatabaseViewer::filterableTreeView_AfterSelect));
        this->filterableTreeView->TreeView->KeyUp += (gcnew KeyEventHandler(this, &TexturesDatabaseViewer::filterableTreeView_KeyUp));

        System::Windows::Forms::Cursor::Current = System::Windows::Forms::Cursors::Arrow;
    }

    TreeNode^ TexturesDatabaseViewer::FindNode(TreeNode^ parent, String^ text) {
        String^ term = text->ToUpper();

        for (int i = 0; i < parent->Nodes->Count; ++i) {
            if (parent->Nodes[i]->Text->ToUpper() == term) {
                return parent->Nodes[i];
            }
        }

        return nullptr;
    }

    void TexturesDatabaseViewer::SortNodesRecursively(TreeNode^ parent, NodeSorter^ sorter) {
        array<TreeNode^>^ nodes = gcnew array<TreeNode^>(parent->Nodes->Count);
        parent->Nodes->CopyTo(nodes, 0);
        System::Array::Sort(nodes, sorter);
        parent->Nodes->Clear();
        parent->Nodes->AddRange(nodes);

        delete nodes;

        for (int i = 0; i < parent->Nodes->Count; ++i) {
            if (parent->Nodes[i]->Nodes->Count > 0) {
                this->SortNodesRecursively(parent->Nodes[i], sorter);
            }
        }
    }

    void TexturesDatabaseViewer::filterableTreeView_NodeMouseDoubleClick(System::Object^ sender, System::Windows::Forms::TreeNodeMouseClickEventArgs^ e) {
        if (e->Button == System::Windows::Forms::MouseButtons::Left) {
            this->SelectTexture(e->Node);
        }
    }

    String^ TexturesDatabaseViewer::GetRealPath(const size_t index) {
        const MetroTextureInfo& texInfo = MetroTexturesDatabase::Get().GetTextureInfo(index);
        const CharString& sourceName = MetroTexturesDatabase::Get().GetSourceName(texInfo.name);
        return ToNetString(sourceName);
    }

    void TexturesDatabaseViewer::filterableTreeView_AfterCollapse(System::Object^ sender, System::Windows::Forms::TreeViewEventArgs^ e) {
        e->Node->ImageIndex = kImageIdxFolderClosed;
        e->Node->SelectedImageIndex = kImageIdxFolderClosed;
    }

    void TexturesDatabaseViewer::filterableTreeView_AfterExpand(System::Object^ sender, System::Windows::Forms::TreeViewEventArgs^ e) {
        e->Node->ImageIndex = kImageIdxFolderOpen;
        e->Node->SelectedImageIndex = kImageIdxFolderOpen;
    }

    void TexturesDatabaseViewer::filterableTreeView_AfterSelect(System::Object^ sender, System::Windows::Forms::TreeViewEventArgs^ e) {
        this->SelectNode(e->Node);
    }

    void TexturesDatabaseViewer::filterableTreeView_KeyUp(System::Object^ sender, System::Windows::Forms::KeyEventArgs^ e) {
        if (e->KeyCode == Keys::Enter) {
            SelectTexture(this->filterableTreeView->TreeView->SelectedNode);
        }
    }

    bool TexturesDatabaseViewer::FindAndSelect(String^ text) {
        return this->filterableTreeView->FindAndSelect(text, nullptr);
    }

    void TexturesDatabaseViewer::SelectNode(TreeNode^ node) {
        if (node == nullptr || node->Tag == nullptr || node->Nodes->Count > 0) return;

        if (mPropertiesViewer == nullptr) {
            mPropertiesViewer = gcnew TexturePropertiesViewer();
        }

        const size_t index = safe_cast<size_t>(node->Tag);
        const MetroTextureInfo& texInfo = MetroTexturesDatabase::Get().GetTextureInfo(index);
        String^ realPath = this->GetRealPath(index);

        mPropertiesViewer->SetTextureInfo(&texInfo);
        mPropertiesViewer->SetRealPath(realPath);
        this->propertyGrid->SelectedObject = mPropertiesViewer;
    }

    void TexturesDatabaseViewer::SelectTexture(TreeNode^ node) {
        if (node == nullptr || node->Tag == nullptr || node->Nodes->Count > 0) return;

        size_t index = safe_cast<size_t>(node->Tag);
        String^ path = this->GetRealPath(index);

        mMainForm->ResetTreeView();
        if (!mMainForm->FindAndSelect(L"content\\textures\\" + path, mFileExtensions)) {
            MetroEX::ShowErrorMessageBox(this, L"Couldn't find texture!");
        }
    }

}
