using System;
using System.Windows.Forms;
using RGiesecke.DllExport;
using System.Runtime.InteropServices;
using System.Collections.Generic;

namespace VQUI
{
    class Launcher
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

        static List<VQUIMainForm> UIWindowList = new List<VQUIMainForm>();

        [DllExport("CreateControlPanel", CallingConvention = CallingConvention.Cdecl)]
        static public int CreateControlPanel(int val)
        {
            UIWindowList.Add(new VQUIMainForm(val));
            return UIWindowList.Count - 1;
        }

        [DllExport("ShowControlPanel", CallingConvention = CallingConvention.Cdecl)]
        static public void ShowControlPanel(int windowHandle)
        {
            Application.Run(UIWindowList[windowHandle]);
        }
    }
}
