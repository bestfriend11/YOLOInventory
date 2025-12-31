/********************************************************************
	created:	2002/09/20
	created:	20:9:2002   10:41
	filename: 	c:\documents and settings\mbiddlecombe\my documents\visual studio projects\DSTKRegMod\form1.cs
	file path:	c:\documents and settings\mbiddlecombe\my documents\visual studio projects\DSTKRegMod
	file base:	form1
	file ext:	cs
	author:		biddle
	
	purpose:	Provides a tool for manipulating the DSTK registry settings
	
*********************************************************************/

using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Data;
using System.IO;
using Microsoft.Win32;

using System.Runtime.InteropServices;
using System.Text;

namespace Microbion.SHBrowseFolders
{
	/// <summary>
	/// Class to call the Shell API function SHBrowseForFolder.
	/// </summary>
	/// <remarks>This class uses pointers and therefore is marked as unsafe. Compile with the /unsafe
	/// compiler option.
	/// <para>This class is copyright (c) Steve Pedler and Microbion Software, but may be freely used
	///  and distributed.</para>
	/// <para>For more details see the Microbion web site at http://www.microbion.co.uk
	/// </para></remarks>
	unsafe public class SHBrowser
	{
		// declarations for API functions
		[DllImport("shell32.dll")]
		static extern int SHBrowseForFolder(browseInfo* br);
		[DllImport("shell32.dll")]
		static extern bool SHGetPathFromIDList(int pidl, byte* pszPath);
		[DllImport("ole32.dll")]
		static extern void CoTaskMemFree(void* pv);

		// declares an API structure - element names are from the Windows SDK
		private struct browseInfo 
		{
			public int hwndOwner;
			public int pidlRoot;
			public byte* pszDisplayName;
			public byte* lpszTitle;
			public uint ulFlags;
			public int lpfn;
			public int lparam;
			public int imImage;
		}

		// class-level fields
		private browseInfo brinfo;
		private string brtitle;

		/// <summary>
		/// The constructor for the class.
		/// </summary>
		/// <remarks>The constructor initialises the required API structure with default values.</remarks>
		public SHBrowser()
		{
			brinfo.hwndOwner = 0;
			brinfo.pidlRoot = 0;
			brinfo.pszDisplayName = (byte*)0;
			brinfo.lpszTitle = (byte*)0;
			brinfo.ulFlags = 0;
			brinfo.lpfn = 0;
			brinfo.lparam = 0;
			brinfo.imImage = 0;
		}

		/// <summary>
		/// This method is called to browse for a folder (plus 3 overloads).
		/// </summary>
		/// <returns>Returns a string with a fully-qualified path name on success, or an empty 
		/// string on failure; an empty string is also returned if the user clicks the Cancel button.</returns>
		/// <remarks>Overloaded. This version uses default values; the dialog title is set to
		/// "Choose a folder:"</remarks>
		public string DoBrowse() 
		{				// no parameters, so provide defaults
			brtitle = "Choose a folder:";
			return BrowseIt();
		}

		/// <summary>
		/// Overloaded method to browse for a folder.
		/// </summary>
		/// <param name="title">The title to be displayed in the dialog box.</param>
		/// <returns>Returns a string with a fully-qualified path name on success, or an empty 
		/// string on failure; an empty string is also returned if the user clicks the Cancel button.</returns>
		public string DoBrowse(string title) 
		{	// title only
			brtitle = title;
			return BrowseIt();
		}

		/// <summary>
		/// Overloaded method to browse for a folder.
		/// </summary>
		/// <param name="title">The title to be displayed in the dialog box.</param>
		/// <param name="flags">A bit field giving the flags to be set which govern the behaviour
		/// of the dialog box. Set to zero by default and ignored in this version of the class.</param>
		/// <returns>Returns a string with a fully-qualified path name on success, or an empty 
		/// string on failure; an empty string is also returned if the user clicks the Cancel button.</returns>
		public string DoBrowse(string title, int flags) 
		{	// owner handle and flags
			brtitle = title;
			// brinfo.ulFlags = (uint)flags;
			return BrowseIt();
		}

		/// <summary>
		/// Overloaded method to browse for a folder.
		/// </summary>
		/// <param name="title">The title to be displayed in the dialog box.</param>
		/// <param name="flags">A bit field giving the flags to be set which govern the behaviour
		/// of the dialog box. Set to zero by default and ignored in this version of the class.</param>
		/// <param name="handle">The handle of the window owning the dialog box. Set to zero by default.</param>
		/// <returns>Returns a string with a fully-qualified path name on success, or an empty 
		/// string on failure; an empty string is also returned if the user clicks the Cancel button.</returns>
		public string DoBrowse(string title, uint flags, IntPtr handle) 
		{	// title, flags, and owner handle
			brtitle = title;
			// brinfo.ulFlags = (uint)flags;
			brinfo.hwndOwner = (int)handle;
			return BrowseIt();
		}

		/// <summary>
		/// Private method which actually calls the Shell API function.
		/// </summary>
		/// <returns>Returns a string with a fully-qualified path name on success, or an empty 
		/// string on failure; an empty string is also returned if the user clicks the Cancel button.</returns>
		private string BrowseIt() 
		{
			string showPath;			// the fully-qualified path is returned in this string
			int pIDL;					// the PIDL structure returned from SHBrowseForFolders API call
			bool bSuccess = false;		// indicates if we successfully got a folder
			ASCIIEncoding enc = new ASCIIEncoding();	// converts Unicode to 8-bit ASCII and vice-versa
			byte[] dialogTitle;							// buffer for the dialog title
			byte[] pathName = new byte[256];			// buffer to hold the display name (folder name)
			byte[] pathBuffer = new byte[256];			// buffer to hold the fully-qualified path

			showPath = "";
			dialogTitle = enc.GetBytes(brtitle);		// convert .NET string to API buffer
			fixed (byte* pname = pathName)				// get a pointer to this buffer which we must supply
			{
				fixed(byte* pTitle = dialogTitle)		// and a pointer to the title buffer
				{
					try 
					{
						// complete the browseInfo structure initialisation
						brinfo.pszDisplayName = pname;
						brinfo.lpszTitle = pTitle;

						fixed (browseInfo* brp = &brinfo)		// get a pointer to the browseInfo
							pIDL = SHBrowseForFolder(brp);		// call the shell and return a PIDL
						fixed (byte* pbuffer = pathBuffer) 
						{		// get a pointer to the buffer for the path
							if (pIDL != 0) 
							{
								bSuccess = SHGetPathFromIDList(pIDL, pbuffer);		// get the path name from the ID list
								CoTaskMemFree((void*)pIDL);				// free memory used by the shell
								showPath = enc.GetString(pathBuffer);	// convert to a string from the byte array
							}
						}
					}
					catch (System.Exception e) 
					{
						MessageBox.Show("Exception in SHBrowser.cs: " + e.Message, "Browse for Folders exception",
							MessageBoxButtons.OK, MessageBoxIcon.Stop);
					}
				}
			}
			if (bSuccess)
				return showPath;
			else
				return "";
		}
	}
}

namespace DSTKRegMod
{
	/// <summary>
	/// Summary description for Form1.
	/// </summary>
	public class DSTK_settings : System.Windows.Forms.Form
	{
		private System.Windows.Forms.TextBox outPathTextBox;
		private System.Windows.Forms.Button browseOutPathButton;

		private System.Windows.Forms.Label animViewerLabel;
		private System.Windows.Forms.TextBox animViewerTextBox;
		private System.Windows.Forms.Button browseAnimViewerButton;

		private System.Windows.Forms.Label nodeViewerLabel;
		private System.Windows.Forms.TextBox nodeViewerTextBox;
		private System.Windows.Forms.Button browseNodeViewerButton;

		private System.Windows.Forms.Button cancelButton;
		private System.Windows.Forms.Button saveExit;
		private System.Windows.Forms.Label installDirectory;
		private System.Windows.Forms.Label outputPathLabel;

		private string origOutPath;
		private string origAnimViewer;
		private string origNodeViewer;
		private System.Windows.Forms.GroupBox groupBox1;
		private System.Windows.Forms.GroupBox groupBox2;

		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		public DSTK_settings()
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			//
			// TODO: Add any constructor code after InitializeComponent call
			//
		}

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if (components != null) 
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.outPathTextBox = new System.Windows.Forms.TextBox();
			this.browseOutPathButton = new System.Windows.Forms.Button();
			this.cancelButton = new System.Windows.Forms.Button();
			this.saveExit = new System.Windows.Forms.Button();
			this.animViewerLabel = new System.Windows.Forms.Label();
			this.animViewerTextBox = new System.Windows.Forms.TextBox();
			this.browseAnimViewerButton = new System.Windows.Forms.Button();
			this.browseNodeViewerButton = new System.Windows.Forms.Button();
			this.nodeViewerTextBox = new System.Windows.Forms.TextBox();
			this.nodeViewerLabel = new System.Windows.Forms.Label();
			this.outputPathLabel = new System.Windows.Forms.Label();
			this.installDirectory = new System.Windows.Forms.Label();
			this.groupBox1 = new System.Windows.Forms.GroupBox();
			this.groupBox2 = new System.Windows.Forms.GroupBox();
			this.SuspendLayout();
			// 
			// outPathTextBox
			// 
			this.outPathTextBox.Location = new System.Drawing.Point(96, 108);
			this.outPathTextBox.Name = "outPathTextBox";
			this.outPathTextBox.Size = new System.Drawing.Size(192, 22);
			this.outPathTextBox.TabIndex = 2;
			this.outPathTextBox.Text = "";
			this.outPathTextBox.TextChanged += new System.EventHandler(this.outPathTextBox_TextChanged);
			// 
			// browseOutPathButton
			// 
			this.browseOutPathButton.Location = new System.Drawing.Point(296, 108);
			this.browseOutPathButton.Name = "browseOutPathButton";
			this.browseOutPathButton.Size = new System.Drawing.Size(72, 24);
			this.browseOutPathButton.TabIndex = 3;
			this.browseOutPathButton.Text = "Browse";
			this.browseOutPathButton.Click += new System.EventHandler(this.browseOutPathButton_Click);
			// 
			// cancelButton
			// 
			this.cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.cancelButton.Location = new System.Drawing.Point(16, 228);
			this.cancelButton.Name = "cancelButton";
			this.cancelButton.Size = new System.Drawing.Size(72, 24);
			this.cancelButton.TabIndex = 4;
			this.cancelButton.Text = "Cancel";
			this.cancelButton.Click += new System.EventHandler(this.cancelButton_Click);
			// 
			// saveExit
			// 
			this.saveExit.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.saveExit.Location = new System.Drawing.Point(272, 228);
			this.saveExit.Name = "saveExit";
			this.saveExit.Size = new System.Drawing.Size(96, 24);
			this.saveExit.TabIndex = 5;
			this.saveExit.Text = "Save and Exit";
			this.saveExit.Click += new System.EventHandler(this.saveAndExitButton_Click);
			// 
			// animViewerLabel
			// 
			this.animViewerLabel.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
			this.animViewerLabel.Location = new System.Drawing.Point(16, 148);
			this.animViewerLabel.Name = "animViewerLabel";
			this.animViewerLabel.Size = new System.Drawing.Size(104, 22);
			this.animViewerLabel.TabIndex = 7;
			this.animViewerLabel.Text = "Anim Previewer:";
			// 
			// animViewerTextBox
			// 
			this.animViewerTextBox.Location = new System.Drawing.Point(120, 148);
			this.animViewerTextBox.Name = "animViewerTextBox";
			this.animViewerTextBox.Size = new System.Drawing.Size(168, 22);
			this.animViewerTextBox.TabIndex = 8;
			this.animViewerTextBox.Text = "";
			this.animViewerTextBox.TextChanged += new System.EventHandler(this.animViewerTextBox_TextChanged);
			// 
			// browseAnimViewerButton
			// 
			this.browseAnimViewerButton.Location = new System.Drawing.Point(296, 148);
			this.browseAnimViewerButton.Name = "browseAnimViewerButton";
			this.browseAnimViewerButton.Size = new System.Drawing.Size(72, 24);
			this.browseAnimViewerButton.TabIndex = 3;
			this.browseAnimViewerButton.Text = "Browse";
			this.browseAnimViewerButton.Click += new System.EventHandler(this.browseAnimViewerButton_Click);
			// 
			// browseNodeViewerButton
			// 
			this.browseNodeViewerButton.Location = new System.Drawing.Point(296, 188);
			this.browseNodeViewerButton.Name = "browseNodeViewerButton";
			this.browseNodeViewerButton.Size = new System.Drawing.Size(72, 24);
			this.browseNodeViewerButton.TabIndex = 3;
			this.browseNodeViewerButton.Text = "Browse";
			this.browseNodeViewerButton.Click += new System.EventHandler(this.browseNodeViewerButton_Click);
			// 
			// nodeViewerTextBox
			// 
			this.nodeViewerTextBox.Location = new System.Drawing.Point(128, 188);
			this.nodeViewerTextBox.Name = "nodeViewerTextBox";
			this.nodeViewerTextBox.Size = new System.Drawing.Size(160, 22);
			this.nodeViewerTextBox.TabIndex = 8;
			this.nodeViewerTextBox.Text = "";
			this.nodeViewerTextBox.TextChanged += new System.EventHandler(this.nodeViewerTextBox_TextChanged);
			// 
			// nodeViewerLabel
			// 
			this.nodeViewerLabel.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
			this.nodeViewerLabel.Location = new System.Drawing.Point(16, 188);
			this.nodeViewerLabel.Name = "nodeViewerLabel";
			this.nodeViewerLabel.Size = new System.Drawing.Size(104, 24);
			this.nodeViewerLabel.TabIndex = 7;
			this.nodeViewerLabel.Text = "Node Previewer:";
			// 
			// outputPathLabel
			// 
			this.outputPathLabel.Location = new System.Drawing.Point(16, 108);
			this.outputPathLabel.Name = "outputPathLabel";
			this.outputPathLabel.Size = new System.Drawing.Size(80, 21);
			this.outputPathLabel.TabIndex = 6;
			this.outputPathLabel.Text = "Output Path:";
			this.outputPathLabel.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
			// 
			// installDirectory
			// 
			this.installDirectory.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
			this.installDirectory.Location = new System.Drawing.Point(24, 34);
			this.installDirectory.Name = "installDirectory";
			this.installDirectory.Size = new System.Drawing.Size(336, 22);
			this.installDirectory.TabIndex = 9;
			this.installDirectory.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
			// 
			// groupBox1
			// 
			this.groupBox1.Location = new System.Drawing.Point(8, 8);
			this.groupBox1.Name = "groupBox1";
			this.groupBox1.Size = new System.Drawing.Size(368, 64);
			this.groupBox1.TabIndex = 10;
			this.groupBox1.TabStop = false;
			this.groupBox1.Text = "The DSTK Tools were installed in:";
			// 
			// groupBox2
			// 
			this.groupBox2.Location = new System.Drawing.Point(8, 80);
			this.groupBox2.Name = "groupBox2";
			this.groupBox2.Size = new System.Drawing.Size(368, 184);
			this.groupBox2.TabIndex = 11;
			this.groupBox2.TabStop = false;
			this.groupBox2.Text = "Adjustable Settings:";
			// 
			// DSTK_settings
			// 
			this.AcceptButton = this.cancelButton;
			this.AutoScaleBaseSize = new System.Drawing.Size(6, 15);
			this.CancelButton = this.saveExit;
			this.ClientSize = new System.Drawing.Size(384, 273);
			this.Controls.AddRange(new System.Windows.Forms.Control[] {
																		  this.animViewerTextBox,
																		  this.animViewerLabel,
																		  this.saveExit,
																		  this.cancelButton,
																		  this.browseOutPathButton,
																		  this.outPathTextBox,
																		  this.browseAnimViewerButton,
																		  this.browseNodeViewerButton,
																		  this.nodeViewerTextBox,
																		  this.nodeViewerLabel,
																		  this.outputPathLabel,
																		  this.installDirectory,
																		  this.groupBox1,
																		  this.groupBox2});
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedToolWindow;
			this.Name = "DSTK_settings";
			this.Text = "Modify DSTK settings";
			this.Load += new System.EventHandler(this.Form1_Load);
			this.ResumeLayout(false);

		}
		#endregion

		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main() 
		{
			Application.Run(new DSTK_settings());
		}

		private void Form1_Load(object sender, System.EventArgs e)
		{
			origOutPath = null;
			origAnimViewer = null;
			origNodeViewer = null;

			string DSTK = @"Software\Gas Powered Games\DSTK\1.0";

			RegistryKey regKey;

			regKey = Registry.LocalMachine.OpenSubKey(DSTK);

			if (regKey != null)
			{
				installDirectory.Text =  (string)regKey.GetValue("Install Directory","Not Installed");
				origAnimViewer = (string)regKey.GetValue("AnimPreviewer",null);
				origNodeViewer = (string)regKey.GetValue("NodePreviewer",null);
				animViewerTextBox.Text = origAnimViewer;
				nodeViewerTextBox.Text = origNodeViewer;
				regKey.Close();
			}
			else
			{
				MessageBox.Show(
					"Unable to locate the DSTK install directory in the registry\n\nThe DSTK does not seem to be properly installed on this system",
					"DSTK not found",
					MessageBoxButtons.OK,
					MessageBoxIcon.Error);

				Application.Exit();
			}

			regKey = Registry.CurrentUser.OpenSubKey(DSTK+@"\Paths");

			if (regKey != null)
			{
				origOutPath = (string)regKey.GetValue("Out",null);
				outPathTextBox.Text = origOutPath;
				regKey.Close();
			}

			if (origOutPath == null)
			{
				outPathTextBox.Text = "";
			}

			if (origAnimViewer == null)
			{
				animViewerTextBox.Text = "";
			}

			if (origNodeViewer == null)
			{
				nodeViewerTextBox.Text = "";
			}
		}

		private void browseOutPathButton_Click(object sender, System.EventArgs e)
		{
			Microbion.SHBrowseFolders.SHBrowser myBrowser = new Microbion.SHBrowseFolders.SHBrowser();
			string newPath = myBrowser.DoBrowse("Select an \"Out\" directory");

			if ( newPath != null)
			{
				outPathTextBox.Text = newPath;
			}
		}

		private void outPathTextBox_TextChanged(object sender, System.EventArgs e)
		{
		}

		private void nodeViewerTextBox_TextChanged(object sender, System.EventArgs e)
		{
		}

		private void animViewerTextBox_TextChanged(object sender, System.EventArgs e)
		{		
		}

		private void label1_Click(object sender, System.EventArgs e)
		{
		}

		private string GetViewer(string initFilter,string initDir,string initEXE)
		{
			OpenFileDialog dlg = new OpenFileDialog();
			dlg.Filter = initFilter + "|*.exe";
			dlg.InitialDirectory = installDirectory.Text;
			dlg.FileName = nodeViewerTextBox.Text;

			DialogResult ret = dlg.ShowDialog();

			if (ret == DialogResult.OK)
			{
				string newPath = Path.GetFullPath(dlg.FileName);
				
				if ( Path.GetDirectoryName(newPath) == Path.GetFullPath(installDirectory.Text) )
				{
					return Path.GetFileName(newPath);
				}
				else
				{
					MessageBox.Show("All viewer executables must be located in the DSTK Tools directory" );
				}
			}
			return null;
		}

		private void browseAnimViewerButton_Click(object sender, System.EventArgs e)
		{
			string newviewer = GetViewer("Anim Viewers",installDirectory.Text,animViewerTextBox.Text);
			if (newviewer != null)
			{
				animViewerTextBox.Text = newviewer;
			}				
		}

		private void browseNodeViewerButton_Click(object sender, System.EventArgs e)
		{
			string newviewer = GetViewer("Node Viewers",installDirectory.Text,nodeViewerTextBox.Text);
			if (newviewer != null)
			{
				nodeViewerTextBox.Text = newviewer;
			}				
		}

		private void ApplyCurrentSettings()
		{
			RegistryKey regKey;
			string DSTK = @"Software\Gas Powered Games\DSTK\1.0";

			regKey = Registry.CurrentUser.CreateSubKey(DSTK+@"\Paths");

			if (regKey != null)
			{
				if (outPathTextBox.Text.Length>0)
				{
					regKey.SetValue("out",outPathTextBox.Text);
				}
				regKey.Close();
			}

			regKey = Registry.LocalMachine.OpenSubKey(DSTK,true);

			if (regKey != null)
			{
				if ( animViewerTextBox.Text.Length>0 )
				{
					regKey.SetValue("AnimPreviewer",animViewerTextBox.Text);
				}
				if ( nodeViewerTextBox.Text.Length>0 )
				{
					regKey.SetValue("NodePreviewer",nodeViewerTextBox.Text);
				}
				regKey.Close();
			}
		}

		private void ApplyOriginalSettings()
		{
			RegistryKey regKey;
			string DSTK = @"Software\Gas Powered Games\DSTK\1.0";

			regKey = Registry.CurrentUser.OpenSubKey(DSTK+@"\Paths",true);

			if (regKey != null)
			{
				if (origOutPath != null)
				{
					regKey.SetValue("out",origOutPath);
				}
				regKey.Close();
			}

			regKey = Registry.LocalMachine.OpenSubKey(DSTK,true);

			if (regKey != null)
			{
				if (origAnimViewer != null)
				{
					regKey.SetValue("AnimPreviewer",origAnimViewer);
				}
				if (origNodeViewer != null)
				{
					regKey.SetValue( "NodePreviewer",origNodeViewer );
				}
				regKey.Close();
			}
		}

		private void applyButton_Click(object sender, System.EventArgs e)
		{
			ApplyCurrentSettings();
		}

		private void saveAndExitButton_Click(object sender, System.EventArgs e)
		{
			ApplyCurrentSettings();
			Application.Exit();
		}

		private void cancelButton_Click(object sender, System.EventArgs e)
		{
			ApplyOriginalSettings();
			Application.Exit();
		}

		private void DSTKInstallLabel_Click(object sender, System.EventArgs e)
		{
		
		}
	}
}
