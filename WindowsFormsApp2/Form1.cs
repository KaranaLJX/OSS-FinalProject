using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;
using System.IO;
using System.Windows.Forms;
using System.Diagnostics;
using System.Data.SQLite;

namespace WindowsFormsApp2 {
    public partial class Form1 : Form {


        Form2 form2;

        public Form1() {
            form2 = new Form2();
            form2.Visible = false;

            InitializeComponent();
        }

        private void Form1_Shown(object sender, EventArgs e) {
            JobManager manager = new JobManager();

            Program.sniffle_info = new ProcessStartInfo("./core/core.exe");
            Program.sniffle_info.RedirectStandardOutput =
            Program.sniffle_info.RedirectStandardError =
            Program.sniffle_info.RedirectStandardInput = true;

            Program.sniffle_info.StandardErrorEncoding = new UTF8Encoding();
            Program.sniffle_info.StandardOutputEncoding = new UTF8Encoding();

            Program.sniffle_info.UseShellExecute = false;
            Program.sniffle_info.CreateNoWindow = true;
            

            Program.sniffle = new Process();
            
            Program.sniffle.StartInfo = Program.sniffle_info;

            Program.sniffle.EnableRaisingEvents = true;
            Program.sniffle.Exited += Sniffle_Exited;


            Program.sniffle.OutputDataReceived += Sniffle_OutputDataReceived;
            Program.sniffle.ErrorDataReceived += Sniffle_ErrorDataReceived;

            Program.sniffle.Start();

            manager.AddProcess(Program.sniffle.Id);


            Program.sniffle.BeginErrorReadLine();
            Program.sniffle.BeginOutputReadLine();


            listView1.Columns.Add("类型");
            listView1.Columns.Add("详细");
            //listView1.Columns.Add("描述");
            //listView1.Columns.Add("回环");
            listView1.AutoResizeColumns(ColumnHeaderAutoResizeStyle.ColumnContent);
            listView1.AutoResizeColumns(ColumnHeaderAutoResizeStyle.HeaderSize);




        }

        private void Sniffle_Exited(object sender, EventArgs e) {
            this.Invoke((Action)delegate {
                MessageBox.Show("Subprocess exited. Now exiting\nPlease handle a bug report.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                this.Close();
            });
        }

        private void Sniffle_ErrorDataReceived(object sender, DataReceivedEventArgs e) {
            //Console.WriteLine(e.Data);
            Debug.WriteLine(e.Data);
            if (null == e.Data) return;

            //label1.Invoke((Action) delegate {
            //});

            var devicesWrapper = JsonSerializer.Deserialize<DevicesWrapper>(e.Data);
            listView1.Invoke((Action)delegate {

                List<int> possible = new List<int>();

                int possibleItemID = -1;
                devicesWrapper.devices_list.ForEach((DeviceData x) => {
                    ListViewGroup listViewGroup = new ListViewGroup(string.Format("设备 {0}", x.id));
                    listViewGroup.Name = x.id.ToString();

                    listView1.Groups.Add(listViewGroup);


                    listView1.Items.Add(new ListViewItem(new string[] { "名称", x.device_name }, listViewGroup));
                    listView1.Items.Add(new ListViewItem(new string[] { "描述", x.description }, listViewGroup));

                    listView1.Items.Add(new ListViewItem(new string[] { "环回", x.loopback.ToString() }, listViewGroup));

                    bool vis = false;
                    x.details.ForEach((DeviceDetail y) => {
                        //string text = string.Format("Family Name: {0}\nAddress: {1}\nNetmask: {2}\n", y.address_family_name, y.address, y.netmask);
                        //ListViewGroup listView11 = new ListViewGroup(string.Format("地址"));
                        if (y.address != "(none)" && !vis) {
                            possible.Add(x.id);
                            vis = true;
                            possibleItemID = listView1.Items.Count + 1;
                        }

                        listView1.Items.Add(new ListViewItem(new string[] { "Family name", y.address_family_name }, listViewGroup));
                        listView1.Items.Add(new ListViewItem(new string[] { "Address", y.address }, listViewGroup));

                        listView1.Items.Add(new ListViewItem(new string[] { "Netmask", y.netmask }, listViewGroup));

                        listView1.Items.Add(new ListViewItem(new string[] { "", "" }, listViewGroup));

                    });

                    //var item = new ListViewItem(new string[] {x.id.ToString(), x.device_name, x.description, x.loopback.ToString()});
                    //listView1.Items.Add(item);


                });
                listView1.ShowGroups = true;
                if (possible.Count > 0) {
                    string text = "。设备 ";
                    possible.ForEach((int x) => {
                        text += x.ToString() + ", ";
                    });
                    char[] arr = { ',', ' ' };
                    text = text.TrimEnd(arr);
                    text += " 可能是你想要的。";
                    toolStripStatusLabel1.Text += text;
                    listView1.Items[possibleItemID].Selected = true;
                    listView1.EnsureVisible(possibleItemID);
                }

            });
        }

        private void Sniffle_OutputDataReceived(object sender, DataReceivedEventArgs e) {
            Debug.WriteLine(e.Data);
        }

        private void Form1_Load(object sender, EventArgs e) {

        }

        private async void listView1_DoubleClick(object sender, EventArgs e) {
            int n = listView1.SelectedItems.Count;

            if (n > 0) {

                int select = int.Parse(listView1.SelectedItems[0].Group.Name);
                //int select = listView1.SelectedIndices[0];
                var istream = Program.sniffle.StandardInput;

                listView1.Enabled = false;
                Program.sniffle.OutputDataReceived -= Sniffle_OutputDataReceived;
                Program.sniffle.ErrorDataReceived -= Sniffle_ErrorDataReceived;

                Program.sniffle.OutputDataReceived += form2.Sniffle_OutputDataReceived;
                Program.sniffle.ErrorDataReceived += form2.Sniffle_ErrorDataReceived;

                await istream.WriteAsync(select.ToString() + "\n");
                await istream.FlushAsync();
                listView1.Enabled = true;
                form2.Visible = true;
                this.Visible = false;
            }
        }

        private void listView1_MouseClick(object sender, MouseEventArgs e) {
        }
    }
    public class DeviceDetail {
        public string address_family_name { get; set; }
        public string address { get; set; }
        public string netmask { get; set; }
        public string broadcast_address { get; set; }

    }
    public class DeviceData {
        public int id { get; set; }
        public string device_name { get; set; }
        public string description { get; set; }
        public bool loopback { get; set; }

        public List<DeviceDetail> details { get; set; }
    }
    public class DevicesWrapper {
        public List<DeviceData> devices_list { get; set; }
    }
}