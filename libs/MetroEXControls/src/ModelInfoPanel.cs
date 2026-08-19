using System;
using System.Collections;
using System.Drawing;
using System.Windows.Forms;

namespace MetroEXControls {
    public partial class ModelInfoPanel : UserControl {
        public delegate void OnButtonClicked(Object sender);
        public delegate void OnListSelectionChanged(int selection);

        private class MotionsNodeSorter : IComparer {
            public int Compare(Object x, Object y) {
                TreeNode left = (TreeNode)x;
                TreeNode right = (TreeNode)y;

                bool leftIsFolder = (left.Tag == null);
                bool rightIsFolder = (right.Tag == null);

                if (leftIsFolder != rightIsFolder) {
                    return leftIsFolder ? -1 : 1;
                }

                return String.Compare(left.Text, right.Text, StringComparison.OrdinalIgnoreCase);
            }
        }

        private int mImgIdxFolderClosed = -1;
        private int mImgIdxFolderOpen = -1;
        private int mImgIdxMotion = -1;

        public OnButtonClicked OnPlayButtonClicked {
            get;
            set;
        }

        public OnButtonClicked OnInfoButtonClicked {
            get;
            set;
        }

        public OnButtonClicked OnMotionExportButtonClicked {
            get;
            set;
        }

        public OnListSelectionChanged OnMotionsListSelectionChanged {
            get;
            set;
        }

        public OnListSelectionChanged OnLodsListSelectionChanged
        {
            get;
            set;
        }

        public ModelInfoPanel() {
            InitializeComponent();

            this.OnPlayButtonClicked = null;
            this.OnInfoButtonClicked = null;
            this.OnMotionExportButtonClicked = null;
            this.OnMotionsListSelectionChanged = null;
            this.OnLodsListSelectionChanged = null;

            this.btnModelExportMotion.Enabled = false;
            this.lstLods.Enabled = false;

            this.treeMdlPropMotions.TreeView.TreeViewNodeSorter = new MotionsNodeSorter();
            this.treeMdlPropMotions.TreeView.AfterSelect += new TreeViewEventHandler(this.treeMdlPropMotions_AfterSelect);
            this.treeMdlPropMotions.TreeView.AfterExpand += new TreeViewEventHandler(this.treeMdlPropMotions_AfterExpand);
            this.treeMdlPropMotions.TreeView.AfterCollapse += new TreeViewEventHandler(this.treeMdlPropMotions_AfterCollapse);
        }

        public String MdlPropTypeText {
            set {
                this.lblMdlPropType.Text = value;
            }
        }

        public String MdlPropVerticesText {
            set {
                this.lblMdlPropVertices.Text = value;
            }
        }

        public String MdlPropTrianglesText {
            set {
                this.lblMdlPropTriangles.Text = value;
            }
        }

        public String MdlPropJointsText {
            set {
                this.lblMdlPropJoints.Text = value;
            }
        }

        public String MdlPropNumAnimsText {
            set {
                this.lblMdlPropNumAnims.Text = value;
            }
        }

        public String MdlPropPlayStopAnimBtnText {
            set {
                this.btnMdlPropPlayStopAnim.Text = value;
            }
        }

        public int SelectedMotionIdx {
            get {
                TreeNode node = this.treeMdlPropMotions.TreeView.SelectedNode;
                return (node != null && node.Tag is int) ? (int)node.Tag : -1;
            }
        }

        public void SetMotionsImages(ImageList imageList, int folderClosedIdx, int folderOpenIdx, int motionIdx) {
            this.treeMdlPropMotions.TreeView.ImageList = imageList;

            mImgIdxFolderClosed = folderClosedIdx;
            mImgIdxFolderOpen = folderOpenIdx;
            mImgIdxMotion = motionIdx;
        }

        public void ClearMotionsList() {
            this.treeMdlPropMotions.ResetFilter();

            this.treeMdlPropMotions.TreeView.BeginUpdate();
            this.treeMdlPropMotions.TreeView.Nodes.Clear();
            this.treeMdlPropMotions.TreeView.EndUpdate();

            this.treeMdlPropMotions.Initialize();

            this.btnModelExportMotion.Enabled = false;
        }

        public void AddMotionToList(String path, int motionIdx) {
            String[] parts = path.Split(new char[] { '\\', '/' }, StringSplitOptions.RemoveEmptyEntries);
            if (parts.Length == 0) {
                return;
            }

            TreeNodeCollection nodes = this.treeMdlPropMotions.TreeView.Nodes;
            for (int i = 0; i + 1 < parts.Length; ++i) {
                nodes = this.GetOrAddFolderNode(nodes, parts[i]).Nodes;
            }

            TreeNode motionNode = new TreeNode(parts[parts.Length - 1]);
            motionNode.ImageIndex = mImgIdxMotion;
            motionNode.SelectedImageIndex = mImgIdxMotion;
            motionNode.Tag = motionIdx;

            nodes.Add(motionNode);
        }

        public void FinishMotionsList() {
            this.treeMdlPropMotions.TreeView.BeginUpdate();
            this.treeMdlPropMotions.TreeView.Sort();
            this.treeMdlPropMotions.TreeView.EndUpdate();

            this.treeMdlPropMotions.Initialize();
        }

        private TreeNode GetOrAddFolderNode(TreeNodeCollection nodes, String name) {
            for (int i = 0; i < nodes.Count; ++i) {
                if (nodes[i].Tag == null && String.Equals(nodes[i].Text, name, StringComparison.OrdinalIgnoreCase)) {
                    return nodes[i];
                }
            }

            TreeNode folderNode = new TreeNode(name);
            folderNode.ImageIndex = mImgIdxFolderClosed;
            folderNode.SelectedImageIndex = mImgIdxFolderClosed;

            nodes.Add(folderNode);

            return folderNode;
        }

        private void btnMdlPropPlayStopAnim_Click(object sender, EventArgs e) {
            this.OnPlayButtonClicked?.Invoke(sender);
        }

        private void btnModelInfo_Click(object sender, EventArgs e) {
            this.OnInfoButtonClicked?.Invoke(sender);
        }

        public void ClearLodsList()
        {
            this.lstLods.Items.Clear();
            this.lstLods.Enabled = false;
        }

        public void AddLodIdToList(int lodId)
        {
            this.lstLods.Items.Add("Lod " + lodId.ToString());
            if (this.lstLods.Items.Count > 1) {
                this.lstLods.Enabled = true;
            }
        }

        public void SelectLod(int lodId)
        {
            this.lstLods.SelectedIndex = lodId;
        }

        private void lstMdlLods_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (this.lstLods.SelectedIndex >= 0) {
                this.OnLodsListSelectionChanged?.Invoke(this.lstLods.SelectedIndex);
            }
        }

        private void treeMdlPropMotions_AfterSelect(object sender, TreeViewEventArgs e) {
            int motionIdx = this.SelectedMotionIdx;

            this.btnModelExportMotion.Enabled = (motionIdx >= 0);

            if (motionIdx >= 0) {
                this.OnMotionsListSelectionChanged?.Invoke(motionIdx);
            }
        }

        private void treeMdlPropMotions_AfterExpand(object sender, TreeViewEventArgs e) {
            if (e.Node.Tag == null) {
                e.Node.ImageIndex = mImgIdxFolderOpen;
                e.Node.SelectedImageIndex = mImgIdxFolderOpen;
            }
        }

        private void treeMdlPropMotions_AfterCollapse(object sender, TreeViewEventArgs e) {
            if (e.Node.Tag == null) {
                e.Node.ImageIndex = mImgIdxFolderClosed;
                e.Node.SelectedImageIndex = mImgIdxFolderClosed;
            }
        }

        private void btnModelExportMotion_Click(object sender, EventArgs e) {
            if (this.SelectedMotionIdx >= 0) {
                this.OnMotionExportButtonClicked(sender);
            }
        }
    }
}
