#pragma once

namespace MetroEX {
    using namespace System;
    using namespace System::ComponentModel;
    using namespace System::Collections;
    using namespace System::Windows::Forms;
    using namespace System::Data;
    using namespace System::Drawing;

    /// <summary>
    /// Summary for DlgSettings
    /// </summary>
    public ref class DlgSettings : public System::Windows::Forms::Form {
    public:
        DlgSettings() {
            InitializeComponent();
        }

    protected:
        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        ~DlgSettings() {
            if (components) {
                delete components;
            }
        }
    private: System::Windows::Forms::TableLayoutPanel^  tableLayoutPanel1;
    private: System::Windows::Forms::TabControl^  tabControl1;
    private: System::Windows::Forms::TabPage^  tabPageExport;
    private: System::Windows::Forms::Panel^  panel1;
    private: System::Windows::Forms::Button^  btnOK;

    private: System::Windows::Forms::Label^  label1;
    private: System::Windows::Forms::Button^  btnCancel;

    private: System::Windows::Forms::GroupBox^  groupBox3;
    private: System::Windows::Forms::RadioButton^  radioSndAsWAV;
    private: System::Windows::Forms::RadioButton^  radioSndAsOGG;
    private: System::Windows::Forms::GroupBox^  groupBox2;
    private: System::Windows::Forms::CheckBox^  chkMdlExportAnimsSeparate;
    private: System::Windows::Forms::CheckBox^  chkMdlExportAnims;
    private: System::Windows::Forms::RadioButton^  radioMdlAsFBX;
    private: System::Windows::Forms::RadioButton^  radioMdlAsOBJ;
    private: System::Windows::Forms::GroupBox^  groupBox1;
    private: System::Windows::Forms::RadioButton^  radioTexAsPNG;
    private: System::Windows::Forms::RadioButton^  radioTexAsTGA;
    private: System::Windows::Forms::RadioButton^  radioTexAsLegacyDDS;
    private: System::Windows::Forms::RadioButton^  radioTexAsDDSAllMips;
    private: System::Windows::Forms::RadioButton^  radioTexAsLegacyDDSAllMips;
    private: System::Windows::Forms::RadioButton^  radioTexAsDDS;
    private: System::Windows::Forms::CheckBox^  chkExportAskEveryTime;
    private: System::Windows::Forms::CheckBox^  chkMdlExcludeCollision;
    private: System::Windows::Forms::CheckBox^  chkMdlSaveWithTextures;
    private: System::Windows::Forms::Label^  label2;
    private: System::Windows::Forms::CheckBox^  chkMdlSaveSurfaceTextures;
    private: System::Windows::Forms::CheckBox^  chkMdlExportLods;


    protected:

    private:
        /// <summary>
        /// Required designer variable.
        /// </summary>
        System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        void InitializeComponent(void)
        {
            this->tableLayoutPanel1 = (gcnew System::Windows::Forms::TableLayoutPanel());
            this->tabControl1 = (gcnew System::Windows::Forms::TabControl());
            this->tabPageExport = (gcnew System::Windows::Forms::TabPage());
            this->chkExportAskEveryTime = (gcnew System::Windows::Forms::CheckBox());
            this->groupBox3 = (gcnew System::Windows::Forms::GroupBox());
            this->radioSndAsWAV = (gcnew System::Windows::Forms::RadioButton());
            this->radioSndAsOGG = (gcnew System::Windows::Forms::RadioButton());
            this->groupBox2 = (gcnew System::Windows::Forms::GroupBox());
            this->chkMdlSaveSurfaceTextures = (gcnew System::Windows::Forms::CheckBox());
            this->chkMdlExcludeCollision = (gcnew System::Windows::Forms::CheckBox());
            this->chkMdlSaveWithTextures = (gcnew System::Windows::Forms::CheckBox());
            this->label2 = (gcnew System::Windows::Forms::Label());
            this->chkMdlExportAnimsSeparate = (gcnew System::Windows::Forms::CheckBox());
            this->chkMdlExportAnims = (gcnew System::Windows::Forms::CheckBox());
            this->radioMdlAsFBX = (gcnew System::Windows::Forms::RadioButton());
            this->radioMdlAsOBJ = (gcnew System::Windows::Forms::RadioButton());
            this->groupBox1 = (gcnew System::Windows::Forms::GroupBox());
            this->radioTexAsPNG = (gcnew System::Windows::Forms::RadioButton());
            this->radioTexAsTGA = (gcnew System::Windows::Forms::RadioButton());
            this->radioTexAsLegacyDDS = (gcnew System::Windows::Forms::RadioButton());
            this->radioTexAsDDSAllMips = (gcnew System::Windows::Forms::RadioButton());
            this->radioTexAsLegacyDDSAllMips = (gcnew System::Windows::Forms::RadioButton());
            this->radioTexAsDDS = (gcnew System::Windows::Forms::RadioButton());
            this->panel1 = (gcnew System::Windows::Forms::Panel());
            this->btnOK = (gcnew System::Windows::Forms::Button());
            this->label1 = (gcnew System::Windows::Forms::Label());
            this->btnCancel = (gcnew System::Windows::Forms::Button());
            this->chkMdlExportLods = (gcnew System::Windows::Forms::CheckBox());
            this->tableLayoutPanel1->SuspendLayout();
            this->tabControl1->SuspendLayout();
            this->tabPageExport->SuspendLayout();
            this->groupBox3->SuspendLayout();
            this->groupBox2->SuspendLayout();
            this->groupBox1->SuspendLayout();
            this->panel1->SuspendLayout();
            this->SuspendLayout();
            // 
            // tableLayoutPanel1
            // 
            this->tableLayoutPanel1->ColumnCount = 1;
            this->tableLayoutPanel1->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
                100)));
            this->tableLayoutPanel1->Controls->Add(this->tabControl1, 0, 0);
            this->tableLayoutPanel1->Controls->Add(this->panel1, 0, 1);
            this->tableLayoutPanel1->Dock = System::Windows::Forms::DockStyle::Fill;
            this->tableLayoutPanel1->Location = System::Drawing::Point(0, 0);
            this->tableLayoutPanel1->Margin = System::Windows::Forms::Padding(0);
            this->tableLayoutPanel1->Name = L"tableLayoutPanel1";
            this->tableLayoutPanel1->RowCount = 2;
            this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 100)));
            this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 25)));
            this->tableLayoutPanel1->Size = System::Drawing::Size(494, 413);
            this->tableLayoutPanel1->TabIndex = 0;
            // 
            // tabControl1
            // 
            this->tabControl1->Controls->Add(this->tabPageExport);
            this->tabControl1->Dock = System::Windows::Forms::DockStyle::Fill;
            this->tabControl1->Location = System::Drawing::Point(0, 0);
            this->tabControl1->Margin = System::Windows::Forms::Padding(0);
            this->tabControl1->Name = L"tabControl1";
            this->tabControl1->SelectedIndex = 0;
            this->tabControl1->Size = System::Drawing::Size(494, 388);
            this->tabControl1->TabIndex = 1;
            // 
            // tabPageExport
            // 
            this->tabPageExport->Controls->Add(this->chkExportAskEveryTime);
            this->tabPageExport->Controls->Add(this->groupBox3);
            this->tabPageExport->Controls->Add(this->groupBox2);
            this->tabPageExport->Controls->Add(this->groupBox1);
            this->tabPageExport->Location = System::Drawing::Point(4, 22);
            this->tabPageExport->Name = L"tabPageExport";
            this->tabPageExport->Padding = System::Windows::Forms::Padding(3);
            this->tabPageExport->Size = System::Drawing::Size(486, 362);
            this->tabPageExport->TabIndex = 0;
            this->tabPageExport->Text = L"Export options";
            this->tabPageExport->UseVisualStyleBackColor = true;
            // 
            // chkExportAskEveryTime
            // 
            this->chkExportAskEveryTime->AutoSize = true;
            this->chkExportAskEveryTime->Location = System::Drawing::Point(15, 336);
            this->chkExportAskEveryTime->Name = L"chkExportAskEveryTime";
            this->chkExportAskEveryTime->Size = System::Drawing::Size(168, 17);
            this->chkExportAskEveryTime->TabIndex = 6;
            this->chkExportAskEveryTime->Text = L"Show this dialog before export";
            this->chkExportAskEveryTime->UseVisualStyleBackColor = true;
            // 
            // groupBox3
            // 
            this->groupBox3->Controls->Add(this->radioSndAsWAV);
            this->groupBox3->Controls->Add(this->radioSndAsOGG);
            this->groupBox3->Location = System::Drawing::Point(8, 256);
            this->groupBox3->Name = L"groupBox3";
            this->groupBox3->Size = System::Drawing::Size(467, 71);
            this->groupBox3->TabIndex = 5;
            this->groupBox3->TabStop = false;
            this->groupBox3->Text = L"Sounds options:";
            // 
            // radioSndAsWAV
            // 
            this->radioSndAsWAV->AutoSize = true;
            this->radioSndAsWAV->Location = System::Drawing::Point(7, 44);
            this->radioSndAsWAV->Name = L"radioSndAsWAV";
            this->radioSndAsWAV->Size = System::Drawing::Size(97, 17);
            this->radioSndAsWAV->TabIndex = 1;
            this->radioSndAsWAV->TabStop = true;
            this->radioSndAsWAV->Text = L"Export as WAV";
            this->radioSndAsWAV->UseVisualStyleBackColor = true;
            // 
            // radioSndAsOGG
            // 
            this->radioSndAsOGG->AutoSize = true;
            this->radioSndAsOGG->Checked = true;
            this->radioSndAsOGG->Location = System::Drawing::Point(7, 20);
            this->radioSndAsOGG->Name = L"radioSndAsOGG";
            this->radioSndAsOGG->Size = System::Drawing::Size(96, 17);
            this->radioSndAsOGG->TabIndex = 0;
            this->radioSndAsOGG->TabStop = true;
            this->radioSndAsOGG->Text = L"Export as OGG";
            this->radioSndAsOGG->UseVisualStyleBackColor = true;
            // 
            // groupBox2
            // 
            this->groupBox2->Controls->Add(this->chkMdlExportLods);
            this->groupBox2->Controls->Add(this->chkMdlSaveSurfaceTextures);
            this->groupBox2->Controls->Add(this->chkMdlExcludeCollision);
            this->groupBox2->Controls->Add(this->chkMdlSaveWithTextures);
            this->groupBox2->Controls->Add(this->label2);
            this->groupBox2->Controls->Add(this->chkMdlExportAnimsSeparate);
            this->groupBox2->Controls->Add(this->chkMdlExportAnims);
            this->groupBox2->Controls->Add(this->radioMdlAsFBX);
            this->groupBox2->Controls->Add(this->radioMdlAsOBJ);
            this->groupBox2->Location = System::Drawing::Point(9, 130);
            this->groupBox2->Name = L"groupBox2";
            this->groupBox2->Size = System::Drawing::Size(467, 120);
            this->groupBox2->TabIndex = 4;
            this->groupBox2->TabStop = false;
            this->groupBox2->Text = L"Models options:";
            // 
            // chkMdlSaveSurfaceTextures
            // 
            this->chkMdlSaveSurfaceTextures->AutoSize = true;
            this->chkMdlSaveSurfaceTextures->Location = System::Drawing::Point(242, 68);
            this->chkMdlSaveSurfaceTextures->Name = L"chkMdlSaveSurfaceTextures";
            this->chkMdlSaveSurfaceTextures->Size = System::Drawing::Size(142, 17);
            this->chkMdlSaveSurfaceTextures->TabIndex = 7;
            this->chkMdlSaveSurfaceTextures->Text = L"Save all surface textures";
            this->chkMdlSaveSurfaceTextures->UseVisualStyleBackColor = true;
            // 
            // chkMdlExcludeCollision
            // 
            this->chkMdlExcludeCollision->AutoSize = true;
            this->chkMdlExcludeCollision->Location = System::Drawing::Point(242, 44);
            this->chkMdlExcludeCollision->Name = L"chkMdlExcludeCollision";
            this->chkMdlExcludeCollision->Size = System::Drawing::Size(150, 17);
            this->chkMdlExcludeCollision->TabIndex = 6;
            this->chkMdlExcludeCollision->Text = L"Exclude collision geometry";
            this->chkMdlExcludeCollision->UseVisualStyleBackColor = true;
            // 
            // chkMdlSaveWithTextures
            // 
            this->chkMdlSaveWithTextures->AutoSize = true;
            this->chkMdlSaveWithTextures->Checked = true;
            this->chkMdlSaveWithTextures->CheckState = System::Windows::Forms::CheckState::Checked;
            this->chkMdlSaveWithTextures->Location = System::Drawing::Point(242, 20);
            this->chkMdlSaveWithTextures->Name = L"chkMdlSaveWithTextures";
            this->chkMdlSaveWithTextures->Size = System::Drawing::Size(113, 17);
            this->chkMdlSaveWithTextures->TabIndex = 5;
            this->chkMdlSaveWithTextures->Text = L"Save with textures";
            this->chkMdlSaveWithTextures->UseVisualStyleBackColor = true;
            // 
            // label2
            // 
            this->label2->BorderStyle = System::Windows::Forms::BorderStyle::Fixed3D;
            this->label2->Location = System::Drawing::Point(233, 9);
            this->label2->Name = L"label2";
            this->label2->Size = System::Drawing::Size(2, 106);
            this->label2->TabIndex = 4;
            // 
            // chkMdlExportAnimsSeparate
            // 
            this->chkMdlExportAnimsSeparate->AutoSize = true;
            this->chkMdlExportAnimsSeparate->Location = System::Drawing::Point(7, 92);
            this->chkMdlExportAnimsSeparate->Name = L"chkMdlExportAnimsSeparate";
            this->chkMdlExportAnimsSeparate->Size = System::Drawing::Size(154, 17);
            this->chkMdlExportAnimsSeparate->TabIndex = 3;
            this->chkMdlExportAnimsSeparate->Text = L"Animations to separate files";
            this->chkMdlExportAnimsSeparate->UseVisualStyleBackColor = true;
            // 
            // chkMdlExportAnims
            // 
            this->chkMdlExportAnims->AutoSize = true;
            this->chkMdlExportAnims->Checked = true;
            this->chkMdlExportAnims->CheckState = System::Windows::Forms::CheckState::Checked;
            this->chkMdlExportAnims->Location = System::Drawing::Point(7, 68);
            this->chkMdlExportAnims->Name = L"chkMdlExportAnims";
            this->chkMdlExportAnims->Size = System::Drawing::Size(109, 17);
            this->chkMdlExportAnims->TabIndex = 2;
            this->chkMdlExportAnims->Text = L"Export animations";
            this->chkMdlExportAnims->UseVisualStyleBackColor = true;
            // 
            // radioMdlAsFBX
            // 
            this->radioMdlAsFBX->AutoSize = true;
            this->radioMdlAsFBX->Location = System::Drawing::Point(7, 44);
            this->radioMdlAsFBX->Name = L"radioMdlAsFBX";
            this->radioMdlAsFBX->Size = System::Drawing::Size(92, 17);
            this->radioMdlAsFBX->TabIndex = 1;
            this->radioMdlAsFBX->TabStop = true;
            this->radioMdlAsFBX->Text = L"Export as FBX";
            this->radioMdlAsFBX->UseVisualStyleBackColor = true;
            // 
            // radioMdlAsOBJ
            // 
            this->radioMdlAsOBJ->AutoSize = true;
            this->radioMdlAsOBJ->Checked = true;
            this->radioMdlAsOBJ->Location = System::Drawing::Point(7, 20);
            this->radioMdlAsOBJ->Name = L"radioMdlAsOBJ";
            this->radioMdlAsOBJ->Size = System::Drawing::Size(92, 17);
            this->radioMdlAsOBJ->TabIndex = 0;
            this->radioMdlAsOBJ->TabStop = true;
            this->radioMdlAsOBJ->Text = L"Export as OBJ";
            this->radioMdlAsOBJ->UseVisualStyleBackColor = true;
            // 
            // groupBox1
            // 
            this->groupBox1->Controls->Add(this->radioTexAsLegacyDDSAllMips);
            this->groupBox1->Controls->Add(this->radioTexAsDDSAllMips);
            this->groupBox1->Controls->Add(this->radioTexAsPNG);
            this->groupBox1->Controls->Add(this->radioTexAsTGA);
            this->groupBox1->Controls->Add(this->radioTexAsLegacyDDS);
            this->groupBox1->Controls->Add(this->radioTexAsDDS);
            this->groupBox1->Location = System::Drawing::Point(8, 6);
            this->groupBox1->Name = L"groupBox1";
            this->groupBox1->Size = System::Drawing::Size(468, 117);
            this->groupBox1->TabIndex = 3;
            this->groupBox1->TabStop = false;
            this->groupBox1->Text = L"Textures options:";
            // 
            // radioTexAsPNG
            // 
            this->radioTexAsPNG->AutoSize = true;
            this->radioTexAsPNG->Location = System::Drawing::Point(7, 92);
            this->radioTexAsPNG->Name = L"radioTexAsPNG";
            this->radioTexAsPNG->Size = System::Drawing::Size(95, 17);
            this->radioTexAsPNG->TabIndex = 3;
            this->radioTexAsPNG->Text = L"Export as PNG";
            this->radioTexAsPNG->UseVisualStyleBackColor = true;
            // 
            // radioTexAsTGA
            // 
            this->radioTexAsTGA->AutoSize = true;
            this->radioTexAsTGA->Location = System::Drawing::Point(7, 68);
            this->radioTexAsTGA->Name = L"radioTexAsTGA";
            this->radioTexAsTGA->Size = System::Drawing::Size(94, 17);
            this->radioTexAsTGA->TabIndex = 2;
            this->radioTexAsTGA->Text = L"Export as TGA";
            this->radioTexAsTGA->UseVisualStyleBackColor = true;
            // 
            // radioTexAsLegacyDDS
            // 
            this->radioTexAsLegacyDDS->AutoSize = true;
            this->radioTexAsLegacyDDS->Location = System::Drawing::Point(7, 44);
            this->radioTexAsLegacyDDS->Name = L"radioTexAsLegacyDDS";
            this->radioTexAsLegacyDDS->Size = System::Drawing::Size(129, 17);
            this->radioTexAsLegacyDDS->TabIndex = 1;
            this->radioTexAsLegacyDDS->Text = L"Export as legacy DDS";
            this->radioTexAsLegacyDDS->UseVisualStyleBackColor = true;
            //
            // radioTexAsDDSAllMips
            //
            this->radioTexAsDDSAllMips->AutoSize = true;
            this->radioTexAsDDSAllMips->Location = System::Drawing::Point(242, 20);
            this->radioTexAsDDSAllMips->Name = L"radioTexAsDDSAllMips";
            this->radioTexAsDDSAllMips->Size = System::Drawing::Size(147, 17);
            this->radioTexAsDDSAllMips->TabIndex = 4;
            this->radioTexAsDDSAllMips->Text = L"Export as DDS (all mips)";
            this->radioTexAsDDSAllMips->UseVisualStyleBackColor = true;
            //
            // radioTexAsLegacyDDSAllMips
            //
            this->radioTexAsLegacyDDSAllMips->AutoSize = true;
            this->radioTexAsLegacyDDSAllMips->Location = System::Drawing::Point(242, 44);
            this->radioTexAsLegacyDDSAllMips->Name = L"radioTexAsLegacyDDSAllMips";
            this->radioTexAsLegacyDDSAllMips->Size = System::Drawing::Size(181, 17);
            this->radioTexAsLegacyDDSAllMips->TabIndex = 5;
            this->radioTexAsLegacyDDSAllMips->Text = L"Export as legacy DDS (all mips)";
            this->radioTexAsLegacyDDSAllMips->UseVisualStyleBackColor = true;
            // 
            // radioTexAsDDS
            // 
            this->radioTexAsDDS->AutoSize = true;
            this->radioTexAsDDS->Checked = true;
            this->radioTexAsDDS->Location = System::Drawing::Point(7, 20);
            this->radioTexAsDDS->Name = L"radioTexAsDDS";
            this->radioTexAsDDS->Size = System::Drawing::Size(95, 17);
            this->radioTexAsDDS->TabIndex = 0;
            this->radioTexAsDDS->TabStop = true;
            this->radioTexAsDDS->Text = L"Export as DDS";
            this->radioTexAsDDS->UseVisualStyleBackColor = true;
            // 
            // panel1
            // 
            this->panel1->Controls->Add(this->btnOK);
            this->panel1->Controls->Add(this->label1);
            this->panel1->Controls->Add(this->btnCancel);
            this->panel1->Dock = System::Windows::Forms::DockStyle::Fill;
            this->panel1->Location = System::Drawing::Point(0, 388);
            this->panel1->Margin = System::Windows::Forms::Padding(0, 0, 3, 0);
            this->panel1->Name = L"panel1";
            this->panel1->Size = System::Drawing::Size(491, 25);
            this->panel1->TabIndex = 2;
            // 
            // btnOK
            // 
            this->btnOK->Dock = System::Windows::Forms::DockStyle::Right;
            this->btnOK->Location = System::Drawing::Point(335, 0);
            this->btnOK->Margin = System::Windows::Forms::Padding(3, 3, 5, 3);
            this->btnOK->Name = L"btnOK";
            this->btnOK->Size = System::Drawing::Size(75, 25);
            this->btnOK->TabIndex = 1;
            this->btnOK->Text = L"OK";
            this->btnOK->UseVisualStyleBackColor = true;
            this->btnOK->Click += gcnew System::EventHandler(this, &DlgSettings::btnOK_Click);
            // 
            // label1
            // 
            this->label1->Dock = System::Windows::Forms::DockStyle::Right;
            this->label1->Location = System::Drawing::Point(410, 0);
            this->label1->Name = L"label1";
            this->label1->Size = System::Drawing::Size(6, 25);
            this->label1->TabIndex = 2;
            this->label1->Text = L"label1";
            // 
            // btnCancel
            // 
            this->btnCancel->Dock = System::Windows::Forms::DockStyle::Right;
            this->btnCancel->Location = System::Drawing::Point(416, 0);
            this->btnCancel->Name = L"btnCancel";
            this->btnCancel->Size = System::Drawing::Size(75, 25);
            this->btnCancel->TabIndex = 0;
            this->btnCancel->Text = L"Cancel";
            this->btnCancel->UseVisualStyleBackColor = true;
            this->btnCancel->Click += gcnew System::EventHandler(this, &DlgSettings::btnCancel_Click);
            // 
            // chkMdlExportLods
            // 
            this->chkMdlExportLods->AutoSize = true;
            this->chkMdlExportLods->Location = System::Drawing::Point(242, 91);
            this->chkMdlExportLods->Name = L"chkMdlExportLods";
            this->chkMdlExportLods->Size = System::Drawing::Size(78, 17);
            this->chkMdlExportLods->TabIndex = 8;
            this->chkMdlExportLods->Text = L"Export lods";
            this->chkMdlExportLods->UseVisualStyleBackColor = true;
            // 
            // DlgSettings
            // 
            this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
            this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
            this->ClientSize = System::Drawing::Size(494, 413);
            this->Controls->Add(this->tableLayoutPanel1);
            this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedSingle;
            this->Name = L"DlgSettings";
            this->StartPosition = System::Windows::Forms::FormStartPosition::CenterParent;
            this->Text = L"MetroEX: Settings";
            this->Load += gcnew System::EventHandler(this, &DlgSettings::DlgSettings_Load);
            this->tableLayoutPanel1->ResumeLayout(false);
            this->tabControl1->ResumeLayout(false);
            this->tabPageExport->ResumeLayout(false);
            this->tabPageExport->PerformLayout();
            this->groupBox3->ResumeLayout(false);
            this->groupBox3->PerformLayout();
            this->groupBox2->ResumeLayout(false);
            this->groupBox2->PerformLayout();
            this->groupBox1->ResumeLayout(false);
            this->groupBox1->PerformLayout();
            this->panel1->ResumeLayout(false);
            this->ResumeLayout(false);

        }
#pragma endregion

    private:
        void DlgSettings_Load(System::Object^ sender, System::EventArgs^ e);
        void btnCancel_Click(System::Object^ sender, System::EventArgs^ e);
        void btnOK_Click(System::Object^ sender, System::EventArgs^ e);
    };
}
