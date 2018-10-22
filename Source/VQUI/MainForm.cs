using System;
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

    /*static*/
    public partial class VQUIMainForm : Form
    {
        //VQUIMainForm Get();

        int val;

        public VQUIMainForm()
        {
            InitializeComponent();
        }
        public VQUIMainForm(int v)
        {
            val = v;
            InitializeComponent();
            labelData.Text = val.ToString();
        }

        //[DllExport("Shutdown", CallingConvention = CallingConvention.Cdecl)]
        /*static*/
        public void Shutdown()
        {
            //this.Shutdown();
            //Application.Exit();
            Close();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            //MessageBox.Show("Shutting Down UI");
            Shutdown();
        }

    }

}
