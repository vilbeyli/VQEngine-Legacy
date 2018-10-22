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

        [DllExport("CreateControlPanel", CallingConvention = CallingConvention.Cdecl)]
        static public int CreateControlPanel(int val)
        {
            ControlPanelFormList.Add(new ControlPanelForm());
            return ControlPanelFormList.Count - 1 + 1024;
        }

        [STAThread]
        [DllExport("ShowControlPanel", CallingConvention = CallingConvention.Cdecl)]
        static public void ShowControlPanel(int windowHandle)
        {
            Application.Run(ControlPanelFormList[GetFormIndex(windowHandle)]);
        }


        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
        public unsafe struct SliderDescData
        {
            public IntPtr pData;
            //[MarshalAs(UnmanagedType.LPStr)] public string name;
            //[MarshalAs(UnmanagedType.LPArray, SizeConst = 256)] public char[] name;
            public unsafe fixed char name[256];
        };

        [DllExport("AddSliderFToControlPanel", CallingConvention = CallingConvention.Cdecl)]
        public static unsafe void AddSliderFToControlPanel(int hPanel, SliderDescData* pDesc)
        {
            ControlPanelFormList[GetFormIndex(hPanel)].AddSliderF(*pDesc);
        }
    }
}
