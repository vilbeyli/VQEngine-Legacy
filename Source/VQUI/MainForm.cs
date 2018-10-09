using RGiesecke.DllExport;
using System;
using System.Runtime.InteropServices;
using System.Windows.Forms;

// -------------------------------------------------------------------------------------------------
// DllExport Fix:
//
// download MSBuild tools 2015: https://www.microsoft.com/en-us/download/confirmation.aspx?id=48159
// otherwise nuget package dependency fails to build.
// https://github.com/3F/DllExport/issues/29#issuecomment-284259126
// -------------------------------------------------------------------------------------------------

namespace VQUI
{
    
    /*static*/ public partial class VQUIMainForm : Form
    {
        //VQUIMainForm Get();

        public VQUIMainForm()
        {
            InitializeComponent();
        }

        //[DllExport("Shutdown", CallingConvention = CallingConvention.Cdecl)]
        /*static*/ public void Shutdown()
        {
            this.Shutdown();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            Shutdown();
        }

        [DllExport("TestFn", CallingConvention = CallingConvention.Cdecl)]
        static public void TestFn() { MessageBox.Show("Test: Hi"); }
    }
}
