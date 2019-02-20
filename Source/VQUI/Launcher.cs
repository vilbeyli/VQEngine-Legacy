using System;
using System.Windows.Forms;
using RGiesecke.DllExport;
using System.Runtime.InteropServices;
using System.Collections.Generic;

namespace VQUI
{
	public class Launcher
	{
		[DllExport("TestFn", CallingConvention = CallingConvention.Cdecl)]
		static public void TestFn() { MessageBox.Show("Test: Hi"); }

		[STAThread]
		[DllExport("LaunchWindow", CallingConvention = CallingConvention.Cdecl)]
		static public void LaunchWindow()
		{
			Application.EnableVisualStyles();
			Application.SetCompatibleTextRenderingDefault(false);
			Application.Run(new VQUIMainForm());
		}


		// CONTROL PANELS
		//
		static List<ControlPanelForm> ControlPanelFormList = new List<ControlPanelForm>();

		static int GetFormIndex(int VQHandle) { return VQHandle-1024; }

		[DllExport("CreateWindow", CallingConvention = CallingConvention.Cdecl)]
		static public int CreateWindow(int val)
		{
			ControlPanelFormList.Add(new ControlPanelForm());
			int retHandle = ControlPanelFormList.Count - 1 + 1024;

			//Application.Run(ControlPanelFormList[ControlPanelFormList.Count - 1]);
			return retHandle;
		}

		[STAThread]
		[DllExport("HideWindow", CallingConvention = CallingConvention.Cdecl)]
		static public void HideWindow(int VQHandle)
		{
			int i = GetFormIndex(VQHandle);

			// Note:
			// This actually disposes the form and clears the containers. Hence,
			// it can't be used. The design should be an MVC where the UI can be 
			// destroyed but data should remain and can be used to re-initialize 
			// or create a new UI form.
#if false
			ControlPanelFormList[i].Hide();
#endif
			ControlPanelFormList[i].Visible = false;
		}

		[STAThread]
		[DllExport("ShowWindow", CallingConvention = CallingConvention.Cdecl)]
		static public void ShowWindow(int windowHandle)
		{
			var panel = ControlPanelFormList[GetFormIndex(windowHandle)];
			if (!panel.HasRun)
			{
				panel.HasRun = true;
				panel.Show();
				//Application.Run(panel);
			}
			else
			{
				panel.Visible = true;
				//panel.Show();
			}
		}

		[DllExport("ShutdownWindows", CallingConvention = CallingConvention.Cdecl)]
		static public void ShutdownWindows()
		{
			Application.Exit();
		}


		[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
		public unsafe struct SliderDescData
		{
			const int CHAR_BUFFER_SIZE = 256;
			public void SetLabel(string text)
			{
				fixed (char* pCh = this.name)
				{
					int i = 0;
					while (i < text.Length && i < CHAR_BUFFER_SIZE-1)
					{
						*(pCh + i) = text[i];
						++i;
					}
				}
			}

			public IntPtr pData;
			public unsafe fixed char name[CHAR_BUFFER_SIZE];
		};




		[DllExport("AddSliderFToControlPanel", CallingConvention = CallingConvention.Cdecl)]
		public static unsafe void AddSliderFToControlPanel(int hPanel, SliderDescData desc)
		{
			ControlPanelFormList[GetFormIndex(hPanel)].AddSliderF(desc);

			//MessageBox.Show(new string(desc.name));
			//MessageBox.Show(new string(&desc.name[0]));
			//MessageBox.Show(new string(&desc.name[1]));
			//MessageBox.Show(new string(&desc.name[2]));
			//MessageBox.Show(new string(&desc.name[3]));
			//MessageBox.Show(new string(&desc.name[4]));
			//MessageBox.Show(new string(&desc.name[5]));
			//MessageBox.Show(new string(&desc.name[6]));
			//MessageBox.Show(new string(&desc.name[7]));
			//MessageBox.Show(new string(&desc.name[8]));
			//MessageBox.Show(desc.pData.ToString());
			//if(desc.pData != IntPtr.Zero)
			//{
			//	*((float*)desc.pData) = 5.0f;
			//}
			//MessageBox.Show(sizeof(SliderDescData).ToString());
		}
	}
}
