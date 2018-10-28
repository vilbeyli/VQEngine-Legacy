using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Runtime.InteropServices;

namespace VQUI.Controls
{
	public unsafe partial class Slider : UserControl
	{
		IntPtr pData = IntPtr.Zero;

		public Slider(Launcher.SliderDescData desc)
		{
			InitializeComponent();

			unsafe
			{
				pData = desc.pData;
				groupdBoxVariable.Text = new String(desc.name);
				numericUpDownForData.Value = desc.pData == IntPtr.Zero 
					? 0
					: new Decimal(*(float*)desc.pData);
				trackBarDataSlider.Value = (int)numericUpDownForData.Value;
			}
		}

		private void numericUpDownForData_ValueChanged(object sender, EventArgs e)
		{
			trackBarDataSlider.Value = (int)numericUpDownForData.Value;
			unsafe
			{
				*(float*)pData = (float)numericUpDownForData.Value;
			}
		}

		private void trackBarDataSlider_Scroll(object sender, EventArgs e)
		{
			numericUpDownForData.Value = trackBarDataSlider.Value;
			unsafe
			{
				*(float*)pData = (float)numericUpDownForData.Value;
			}
		}
	}
}
