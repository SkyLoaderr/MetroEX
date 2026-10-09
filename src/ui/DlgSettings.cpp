#include "mex_settings.h"

#include "DlgSettings.h"

namespace MetroEX {
    void DlgSettings::DlgSettings_Load(System::Object^ sender, System::EventArgs^ e) {
        const MEXSettings& s = MEXSettings::Get();

        // textures
        switch (s.extraction.textureFormat) {
            case MEXSettings::Extraction::TexFormat::Dds: {
                this->radioTexAsDDS->Checked = true;
            } break;
            case MEXSettings::Extraction::TexFormat::LegacyDds: {
                this->radioTexAsLegacyDDS->Checked = true;
            } break;
            case MEXSettings::Extraction::TexFormat::Tga: {
                this->radioTexAsTGA->Checked = true;
            } break;
            case MEXSettings::Extraction::TexFormat::Png: {
                this->radioTexAsPNG->Checked = true;
            } break;
            case MEXSettings::Extraction::TexFormat::DdsAllMips: {
                this->radioTexAsDDSAllMips->Checked = true;
            } break;
            case MEXSettings::Extraction::TexFormat::LegacyDdsAllMips: {
                this->radioTexAsLegacyDDSAllMips->Checked = true;
            } break;
        }

        // models
        switch (s.extraction.modelFormat) {
            case MEXSettings::Extraction::MdlFormat::Obj: {
                this->radioMdlAsOBJ->Checked = true;
            } break;
            case MEXSettings::Extraction::MdlFormat::Fbx: {
                this->radioMdlAsFBX->Checked = true;
            } break;
        }

        this->chkMdlExportAnims->Checked = s.extraction.modelSaveWithAnims;
        this->chkMdlExportAnimsSeparate->Checked = s.extraction.modelAnimsSeparate;
        this->chkMdlSaveWithTextures->Checked = s.extraction.modelSaveWithTextures;
        this->chkMdlExcludeCollision->Checked = s.extraction.modelExcludeCollision;
        this->chkMdlSaveSurfaceTextures->Checked = s.extraction.modelSaveSurfaceSet;

        // sounds
        switch (s.extraction.soundFormat) {
            case MEXSettings::Extraction::SndFormat::Ogg: {
                this->radioSndAsOGG->Checked = true;
            } break;
            case MEXSettings::Extraction::SndFormat::Wav: {
                this->radioSndAsWAV->Checked = true;
            } break;
        }

        // stuff
        this->chkExportAskEveryTime->Checked = s.extraction.askEveryTime;
    }

    void DlgSettings::btnCancel_Click(System::Object^ sender, System::EventArgs^ e) {
        this->DialogResult = System::Windows::Forms::DialogResult::Cancel;
        this->Close();
    }

    void DlgSettings::btnOK_Click(System::Object^ sender, System::EventArgs^ e) {
        MEXSettings& s = MEXSettings::Get();

        // textures
        if (this->radioTexAsDDS->Checked) {
            s.extraction.textureFormat = MEXSettings::Extraction::TexFormat::Dds;
        } else if (this->radioTexAsLegacyDDS->Checked) {
            s.extraction.textureFormat = MEXSettings::Extraction::TexFormat::LegacyDds;
        } else if (this->radioTexAsDDSAllMips->Checked) {
            s.extraction.textureFormat = MEXSettings::Extraction::TexFormat::DdsAllMips;
        } else if (this->radioTexAsLegacyDDSAllMips->Checked) {
            s.extraction.textureFormat = MEXSettings::Extraction::TexFormat::LegacyDdsAllMips;
        } else if (this->radioTexAsTGA->Checked) {
            s.extraction.textureFormat = MEXSettings::Extraction::TexFormat::Tga;
        } else {
            s.extraction.textureFormat = MEXSettings::Extraction::TexFormat::Png;
        }

        // models
        if (this->radioMdlAsOBJ->Checked) {
            s.extraction.modelFormat = MEXSettings::Extraction::MdlFormat::Obj;
        } else {
            s.extraction.modelFormat = MEXSettings::Extraction::MdlFormat::Fbx;
        }

        s.extraction.modelSaveWithAnims = this->chkMdlExportAnims->Checked;
        s.extraction.modelAnimsSeparate = this->chkMdlExportAnimsSeparate->Checked;
        s.extraction.modelSaveWithTextures = this->chkMdlSaveWithTextures->Checked;
        s.extraction.modelExcludeCollision = this->chkMdlExcludeCollision->Checked;
        s.extraction.modelSaveSurfaceSet = this->chkMdlSaveSurfaceTextures->Checked;
        s.extraction.modelSaveLods = this->chkMdlExportLods->Checked;

        // sounds
        if (this->radioSndAsOGG->Checked) {
            s.extraction.soundFormat = MEXSettings::Extraction::SndFormat::Ogg;
        } else {
            s.extraction.soundFormat = MEXSettings::Extraction::SndFormat::Wav;
        }

        // stuff
        s.extraction.askEveryTime = this->chkExportAskEveryTime->Checked;

        s.Save();

        this->DialogResult = System::Windows::Forms::DialogResult::OK;
        this->Close();
    }
}

