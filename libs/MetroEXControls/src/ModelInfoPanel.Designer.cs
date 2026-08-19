namespace MetroEXControls
{
    partial class ModelInfoPanel
    {
        /// <summary> 
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary> 
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing) {
            if (disposing && (components != null)) {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent() {
            this.pnlMdlProps = new System.Windows.Forms.Panel();
            this.btnModelExportMotion = new System.Windows.Forms.Button();
            this.lblMdlPropNumAnims = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.btnModelInfo = new System.Windows.Forms.Button();
            this.btnMdlPropPlayStopAnim = new System.Windows.Forms.Button();
            this.treeMdlPropMotions = new MetroEXControls.FilterableTreeView();
            this.lblMdlPropJoints = new System.Windows.Forms.Label();
            this.label9 = new System.Windows.Forms.Label();
            this.lblMdlPropTriangles = new System.Windows.Forms.Label();
            this.label8 = new System.Windows.Forms.Label();
            this.lblMdlPropVertices = new System.Windows.Forms.Label();
            this.label7 = new System.Windows.Forms.Label();
            this.lblMdlPropType = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.labelLods = new System.Windows.Forms.Label();
            this.lstLods = new System.Windows.Forms.ComboBox();
            this.pnlMdlProps.SuspendLayout();
            this.SuspendLayout();
            // 
            // pnlMdlProps
            // 
            this.pnlMdlProps.Controls.Add(this.lstLods);
            this.pnlMdlProps.Controls.Add(this.labelLods);
            this.pnlMdlProps.Controls.Add(this.btnModelExportMotion);
            this.pnlMdlProps.Controls.Add(this.lblMdlPropNumAnims);
            this.pnlMdlProps.Controls.Add(this.label1);
            this.pnlMdlProps.Controls.Add(this.btnModelInfo);
            this.pnlMdlProps.Controls.Add(this.btnMdlPropPlayStopAnim);
            this.pnlMdlProps.Controls.Add(this.treeMdlPropMotions);
            this.pnlMdlProps.Controls.Add(this.lblMdlPropJoints);
            this.pnlMdlProps.Controls.Add(this.label9);
            this.pnlMdlProps.Controls.Add(this.lblMdlPropTriangles);
            this.pnlMdlProps.Controls.Add(this.label8);
            this.pnlMdlProps.Controls.Add(this.lblMdlPropVertices);
            this.pnlMdlProps.Controls.Add(this.label7);
            this.pnlMdlProps.Controls.Add(this.lblMdlPropType);
            this.pnlMdlProps.Controls.Add(this.label3);
            this.pnlMdlProps.Dock = System.Windows.Forms.DockStyle.Fill;
            this.pnlMdlProps.Location = new System.Drawing.Point(0, 0);
            this.pnlMdlProps.Margin = new System.Windows.Forms.Padding(0);
            this.pnlMdlProps.Name = "pnlMdlProps";
            this.pnlMdlProps.Size = new System.Drawing.Size(680, 160);
            this.pnlMdlProps.TabIndex = 2;
            // 
            // btnModelExportMotion
            // 
            this.btnModelExportMotion.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnModelExportMotion.Location = new System.Drawing.Point(601, 81);
            this.btnModelExportMotion.Name = "btnModelExportMotion";
            this.btnModelExportMotion.Size = new System.Drawing.Size(75, 23);
            this.btnModelExportMotion.TabIndex = 13;
            this.btnModelExportMotion.Text = "Export...";
            this.btnModelExportMotion.UseVisualStyleBackColor = true;
            this.btnModelExportMotion.Click += new System.EventHandler(this.btnModelExportMotion_Click);
            // 
            // lblMdlPropNumAnims
            // 
            this.lblMdlPropNumAnims.AutoSize = true;
            this.lblMdlPropNumAnims.Location = new System.Drawing.Point(68, 63);
            this.lblMdlPropNumAnims.Name = "lblMdlPropNumAnims";
            this.lblMdlPropNumAnims.Size = new System.Drawing.Size(43, 13);
            this.lblMdlPropNumAnims.TabIndex = 12;
            this.lblMdlPropNumAnims.Text = "100500";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(4, 63);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(61, 13);
            this.label1.TabIndex = 11;
            this.label1.Text = "Animations:";
            // 
            // btnModelInfo
            // 
            this.btnModelInfo.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnModelInfo.Location = new System.Drawing.Point(601, 55);
            this.btnModelInfo.Name = "btnModelInfo";
            this.btnModelInfo.Size = new System.Drawing.Size(75, 23);
            this.btnModelInfo.TabIndex = 10;
            this.btnModelInfo.Text = "Info";
            this.btnModelInfo.UseVisualStyleBackColor = true;
            this.btnModelInfo.Click += new System.EventHandler(this.btnModelInfo_Click);
            // 
            // btnMdlPropPlayStopAnim
            // 
            this.btnMdlPropPlayStopAnim.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnMdlPropPlayStopAnim.Location = new System.Drawing.Point(601, 29);
            this.btnMdlPropPlayStopAnim.Name = "btnMdlPropPlayStopAnim";
            this.btnMdlPropPlayStopAnim.Size = new System.Drawing.Size(75, 23);
            this.btnMdlPropPlayStopAnim.TabIndex = 9;
            this.btnMdlPropPlayStopAnim.Text = "Play";
            this.btnMdlPropPlayStopAnim.UseVisualStyleBackColor = true;
            this.btnMdlPropPlayStopAnim.Click += new System.EventHandler(this.btnMdlPropPlayStopAnim_Click);
            // 
            // treeMdlPropMotions
            // 
            this.treeMdlPropMotions.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
            | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.treeMdlPropMotions.FilterPlaceholder = "Search animation...";
            this.treeMdlPropMotions.FilterTimeout = 300;
            this.treeMdlPropMotions.Location = new System.Drawing.Point(125, 1);
            this.treeMdlPropMotions.Margin = new System.Windows.Forms.Padding(0);
            this.treeMdlPropMotions.Name = "treeMdlPropMotions";
            this.treeMdlPropMotions.Size = new System.Drawing.Size(470, 156);
            this.treeMdlPropMotions.TabIndex = 8;
            // 
            // lblMdlPropJoints
            // 
            this.lblMdlPropJoints.AutoSize = true;
            this.lblMdlPropJoints.Location = new System.Drawing.Point(68, 48);
            this.lblMdlPropJoints.Name = "lblMdlPropJoints";
            this.lblMdlPropJoints.Size = new System.Drawing.Size(43, 13);
            this.lblMdlPropJoints.TabIndex = 7;
            this.lblMdlPropJoints.Text = "100500";
            // 
            // label9
            // 
            this.label9.AutoSize = true;
            this.label9.Location = new System.Drawing.Point(4, 48);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(37, 13);
            this.label9.TabIndex = 6;
            this.label9.Text = "Joints:";
            // 
            // lblMdlPropTriangles
            // 
            this.lblMdlPropTriangles.AutoSize = true;
            this.lblMdlPropTriangles.Location = new System.Drawing.Point(68, 32);
            this.lblMdlPropTriangles.Name = "lblMdlPropTriangles";
            this.lblMdlPropTriangles.Size = new System.Drawing.Size(43, 13);
            this.lblMdlPropTriangles.TabIndex = 5;
            this.lblMdlPropTriangles.Text = "100500";
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.Location = new System.Drawing.Point(4, 32);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(53, 13);
            this.label8.TabIndex = 4;
            this.label8.Text = "Triangles:";
            // 
            // lblMdlPropVertices
            // 
            this.lblMdlPropVertices.AutoSize = true;
            this.lblMdlPropVertices.Location = new System.Drawing.Point(68, 17);
            this.lblMdlPropVertices.Name = "lblMdlPropVertices";
            this.lblMdlPropVertices.Size = new System.Drawing.Size(43, 13);
            this.lblMdlPropVertices.TabIndex = 3;
            this.lblMdlPropVertices.Text = "100500";
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.Location = new System.Drawing.Point(4, 17);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(48, 13);
            this.label7.TabIndex = 2;
            this.label7.Text = "Vertices:";
            // 
            // lblMdlPropType
            // 
            this.lblMdlPropType.AutoSize = true;
            this.lblMdlPropType.Location = new System.Drawing.Point(68, 2);
            this.lblMdlPropType.Name = "lblMdlPropType";
            this.lblMdlPropType.Size = new System.Drawing.Size(51, 13);
            this.lblMdlPropType.TabIndex = 1;
            this.lblMdlPropType.Text = "Animated";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(4, 2);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(34, 13);
            this.label3.TabIndex = 0;
            this.label3.Text = "Type:";
            // 
            // labelLods
            // 
            this.labelLods.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.labelLods.AutoSize = true;
            this.labelLods.Location = new System.Drawing.Point(561, 5);
            this.labelLods.Name = "labelLods";
            this.labelLods.Size = new System.Drawing.Size(54, 13);
            this.labelLods.TabIndex = 14;
            this.labelLods.Text = "Show lod:";
            // 
            // lstLods
            // 
            this.lstLods.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.lstLods.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.lstLods.Enabled = false;
            this.lstLods.FormattingEnabled = true;
            this.lstLods.Location = new System.Drawing.Point(621, 1);
            this.lstLods.Name = "lstLods";
            this.lstLods.Size = new System.Drawing.Size(56, 21);
            this.lstLods.TabIndex = 15;
            this.lstLods.SelectedIndexChanged += new System.EventHandler(this.lstMdlLods_SelectedIndexChanged);
            // 
            // ModelInfoPanel
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.pnlMdlProps);
            this.Margin = new System.Windows.Forms.Padding(0);
            this.Name = "ModelInfoPanel";
            this.Size = new System.Drawing.Size(680, 160);
            this.pnlMdlProps.ResumeLayout(false);
            this.pnlMdlProps.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Panel pnlMdlProps;
        private System.Windows.Forms.Button btnModelInfo;
        private System.Windows.Forms.Button btnMdlPropPlayStopAnim;
        private MetroEXControls.FilterableTreeView treeMdlPropMotions;
        private System.Windows.Forms.Label lblMdlPropJoints;
        private System.Windows.Forms.Label label9;
        private System.Windows.Forms.Label lblMdlPropTriangles;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.Label lblMdlPropVertices;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.Label lblMdlPropType;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label lblMdlPropNumAnims;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Button btnModelExportMotion;
        private System.Windows.Forms.ComboBox lstLods;
        private System.Windows.Forms.Label labelLods;
    }
}
