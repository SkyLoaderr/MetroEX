using System;
using System.Collections.Generic;
using System.Windows.Forms;

namespace MetroEXControls {
    public partial class FilterableTreeView : UserControl {
        public string FilterPlaceholder { get; set; } = "Search here...";
        public int FilterTimeout { get; set; } = 1000;
        public TreeView TreeView { get { return this.treeView; } }
        public TextBox FilterTextBox { get { return this.filterTextBox; } }
        public bool IsFiltering { get { return mIsFiltering; } }

        // when set, the owner takes over the filtering and rebuilds the tree by itself
        public event Action<string> FilterRequested;

        private Timer mTimer;
        private TreeNode[] mOriginalRootNodes;
        private bool mIsFiltering;

        public FilterableTreeView() {
            InitializeComponent();

            //#NOTE_SK: set placeholder text for better and cooler look ;)
            WinApi.SendMessage(FilterTextBox.Handle, WinApi.EM_SETCUEBANNER, 0, FilterPlaceholder);

            mTimer = new Timer();
            mTimer.Interval = FilterTimeout;
            mTimer.Tick += new EventHandler(filterTimer_Tick);
        }

        public void Initialize() {
            if (mOriginalRootNodes == null) {
                mOriginalRootNodes = new TreeNode[this.treeView.Nodes.Count];
            } else if(mOriginalRootNodes.Length != this.treeView.Nodes.Count) {
                Array.Resize(ref mOriginalRootNodes, this.treeView.Nodes.Count);
            }

            this.treeView.Nodes.CopyTo(mOriginalRootNodes, 0);
        }

        private void filterTimer_Tick(Object sender, EventArgs e) {
            mTimer.Stop();

            Cursor.Current = Cursors.WaitCursor;

            if (FilterRequested != null) {
                mIsFiltering = !string.IsNullOrWhiteSpace(this.filterTextBox.Text);
                FilterRequested(this.filterTextBox.Text);

                Cursor.Current = Cursors.Arrow;
                return;
            }

            this.treeView.BeginUpdate();
            this.treeView.Nodes.Clear();

            if (string.IsNullOrWhiteSpace(this.filterTextBox.Text)) {
                mIsFiltering = false;
                this.treeView.Nodes.AddRange(mOriginalRootNodes);
            } else {
                mIsFiltering = true;
                for (var i = 0; i < mOriginalRootNodes.Length; ++i) {
                    var rootNode = mOriginalRootNodes[i].Clone() as TreeNode;
                    FilterTreeView(rootNode, this.filterTextBox.Text);
                    this.treeView.Nodes.Add(rootNode);
                    this.treeView.Nodes[i].ExpandAll();
                }
            }

            this.treeView.EndUpdate();

            Cursor.Current = Cursors.Arrow;
        }

        private void FilterTreeView(TreeNode node, string text) {
            var nodesToRemove = new List<TreeNode>();

            for (int i = 0; i < node.Nodes.Count; ++i) {
                if (node.Nodes[i].Nodes.Count > 0) {
                    FilterTreeView(node.Nodes[i], text);

                    if (node.Nodes[i].Nodes.Count == 0) {
                        nodesToRemove.Add(node.Nodes[i]);
                    }
                } else if (!node.Nodes[i].Text.Contains(text)) {
                    nodesToRemove.Add(node.Nodes[i]);
                }
            }

            for (int i = 0; i < nodesToRemove.Count; ++i) {
                node.Nodes.Remove(nodesToRemove[i]);
            }
        }

        private void filterText_TextChanged(object sender, EventArgs e) {
            if (FilterRequested == null) {
                if (mOriginalRootNodes == null) {
                    Initialize();
                }

                if (mOriginalRootNodes == null || mOriginalRootNodes.Length == 0) {
                    return;
                }
            }

            mTimer.Stop();
            mTimer.Start();
        }

        public bool FindAndSelect(string text, string[] extensions) {
            if (mOriginalRootNodes == null) {
                Initialize();
            }

            var textParts = text.Split('\\');

            TreeNode node = null;
            foreach(var rootNode in mOriginalRootNodes) {
                TreeNode foundNode = null;

                var nodeToSearch = rootNode;
                for (int i = 0; i < textParts.Length; i++) {
                    foundNode = this.FindNode(nodeToSearch, textParts[i]);

                    if (i == textParts.Length - 1) {
                        if(extensions != null) {
                            for (int j = 0; j < extensions.Length; j++) {
                                foundNode = this.FindNode(nodeToSearch, textParts[i] + extensions[j]);

                                if (foundNode != null) {
                                    break;
                                }
                            }
                        }
                    } else if(foundNode != null) {
                        foundNode.Expand();
                    }

                    nodeToSearch = foundNode;
                }

                if(foundNode != null)
                {
                    node = foundNode;
                    break;
                }
            }

            if (node != null) {
                this.TreeView.SelectedNode = node;

                return true;
            }

            return false;
        }

        private TreeNode FindNode(TreeNode parent, string text) {
            var term = text.ToUpper();

            for (int i = 0; i < parent.Nodes.Count; i++) {
                if (parent.Nodes[i].Text.ToUpper() == term) {
                    return parent.Nodes[i];
                }
            }

            return null;
        }
    }
}
