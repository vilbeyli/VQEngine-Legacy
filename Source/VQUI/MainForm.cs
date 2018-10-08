using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace VQUI
{
    public partial class VQUIMainForm : Form
    {
        public VQUIMainForm()
        {
            InitializeComponent();
        }

        public void Shutdown()
        {
            this.Shutdown();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            Shutdown();
        }
    }
}
