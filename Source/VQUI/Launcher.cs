using System;
using System.Windows.Forms;
using RGiesecke.DllExport;
using System.Runtime.InteropServices;

namespace VQUI
{
    static class Launcher
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
    }
}
