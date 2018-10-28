#define xTEST_SLIDER
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using VQUI.Controls;

namespace VQUI
{
	public partial class ControlPanelForm : Form
	{
#if TEST_SLIDER
		Launcher.SliderDescData mDesc = new Launcher.SliderDescData();
#endif

		public ControlPanelForm()
		{
			InitializeComponent();

#if TEST_SLIDER
			unsafe
			{
				string text = "Test Slider 0";
				mDesc.SetLabel(text);
			}


			AddSliderF(mDesc);
#endif
		}

		public void AddSliderF(Launcher.SliderDescData desc)
		{
			this.tabPageTest.Controls.Add(new Slider(desc));
		}
	}
}
