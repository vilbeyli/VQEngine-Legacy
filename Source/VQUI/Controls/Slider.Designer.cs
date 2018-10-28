namespace VQUI.Controls
{
    partial class Slider
    {
        /// <summary> 
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary> 
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
			this.components = new System.ComponentModel.Container();
			this.numericUpDownForData = new System.Windows.Forms.NumericUpDown();
			this.trackBarDataSlider = new System.Windows.Forms.TrackBar();
			this.bindingSource1 = new System.Windows.Forms.BindingSource(this.components);
			this.groupdBoxVariable = new System.Windows.Forms.GroupBox();
			((System.ComponentModel.ISupportInitialize)(this.numericUpDownForData)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.trackBarDataSlider)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.bindingSource1)).BeginInit();
			this.groupdBoxVariable.SuspendLayout();
			this.SuspendLayout();
			// 
			// numericUpDownForData
			// 
			this.numericUpDownForData.DecimalPlaces = 4;
			this.numericUpDownForData.Increment = new decimal(new int[] {
            1,
            0,
            0,
            196608});
			this.numericUpDownForData.Location = new System.Drawing.Point(6, 33);
			this.numericUpDownForData.Name = "numericUpDownForData";
			this.numericUpDownForData.Size = new System.Drawing.Size(101, 20);
			this.numericUpDownForData.TabIndex = 2;
			this.numericUpDownForData.Value = new decimal(new int[] {
            6,
            0,
            0,
            65536});
			this.numericUpDownForData.ValueChanged += new System.EventHandler(this.numericUpDownForData_ValueChanged);
			// 
			// trackBarDataSlider
			// 
			this.trackBarDataSlider.Location = new System.Drawing.Point(113, 29);
			this.trackBarDataSlider.Name = "trackBarDataSlider";
			this.trackBarDataSlider.Size = new System.Drawing.Size(330, 45);
			this.trackBarDataSlider.TabIndex = 3;
			this.trackBarDataSlider.Scroll += new System.EventHandler(this.trackBarDataSlider_Scroll);
			// 
			// groupdBoxVariable
			// 
			this.groupdBoxVariable.Controls.Add(this.trackBarDataSlider);
			this.groupdBoxVariable.Controls.Add(this.numericUpDownForData);
			this.groupdBoxVariable.Dock = System.Windows.Forms.DockStyle.Fill;
			this.groupdBoxVariable.Location = new System.Drawing.Point(0, 0);
			this.groupdBoxVariable.Name = "groupdBoxVariable";
			this.groupdBoxVariable.Size = new System.Drawing.Size(449, 80);
			this.groupdBoxVariable.TabIndex = 4;
			this.groupdBoxVariable.TabStop = false;
			this.groupdBoxVariable.Text = "variable name";
			// 
			// Slider
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.Controls.Add(this.groupdBoxVariable);
			this.Name = "Slider";
			this.Size = new System.Drawing.Size(449, 80);
			((System.ComponentModel.ISupportInitialize)(this.numericUpDownForData)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.trackBarDataSlider)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.bindingSource1)).EndInit();
			this.groupdBoxVariable.ResumeLayout(false);
			this.groupdBoxVariable.PerformLayout();
			this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.NumericUpDown numericUpDownForData;
        private System.Windows.Forms.TrackBar trackBarDataSlider;
        private System.Windows.Forms.BindingSource bindingSource1;
        private System.Windows.Forms.GroupBox groupdBoxVariable;
    }
}
