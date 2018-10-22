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
        public ControlPanelForm()
        {
            InitializeComponent();
        }

        public void AddSliderF(Launcher.SliderDescData desc)
        {
            this.tabPageTest.Controls.Add(new Slider(desc));
        }
    }
}
