using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Data.SQLite;
using System.Diagnostics;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;
using System.Collections.Concurrent;
using System.Windows.Forms;
using System.IO;
using System.IO.Compression;
using System.Runtime.InteropServices;
using System.Windows.Forms.DataVisualization.Charting;

namespace WindowsFormsApp2 {



    public partial class Form2 : Form {

        private List<byte[]> data_Raw;

        //private List<PacketData> data_Buffer;

        private List<ListViewItem> data_listViews;
        private int tot;
        private int db_tot;
        private bool is_paused = false;
        private Mutex lock_db = new Mutex();

        SQLiteConnection sql_con;
        SQLiteCommand sql_cmd;

        public Form2() {

            data_Raw = new List<byte[]>();
            data_listViews = new List<ListViewItem>();
            InitializeComponent();
            textBox1.Text = "src port 53 or dst port 53";

            string executable = System.Reflection.Assembly.GetExecutingAssembly().Location;
            string path = (System.IO.Path.GetDirectoryName(executable));
            string buffer_db = $"URI=file:{path}\\sniffle_data.db";

            sql_con = new SQLiteConnection(buffer_db);
            sql_con.Open();
            sql_cmd = new SQLiteCommand(sql_con);

            // 清空之前的数据库
            sql_cmd.CommandText = @"drop table if exists DATA";
            sql_cmd.ExecuteNonQuery();
            sql_cmd.CommandText =
            @"create table DATA (
                Timestamp   TEXT,
                Type        TEXT,
                Source      TEXT,
                Destination TEXT,
                Res         TEXT,
                Raw         BLOB
            );";

            sql_cmd.ExecuteNonQuery();

            listView1.VirtualMode = true;
            listView1.VirtualListSize = 0;

        }
        public static void SetDoubleBuffered(System.Windows.Forms.Control c) {
            //Taxes: Remote Desktop Connection and painting
            //http://blogs.msdn.com/oldnewthing/archive/2006/01/03/508694.aspx
            if (System.Windows.Forms.SystemInformation.TerminalServerSession)
                return;

            System.Reflection.PropertyInfo aProp =
                  typeof(System.Windows.Forms.Control).GetProperty(
                        "DoubleBuffered",
                        System.Reflection.BindingFlags.NonPublic |
                        System.Reflection.BindingFlags.Instance);

            aProp.SetValue(c, true, null);
        }

        private void Form2_Load(object sender, EventArgs e) {

            chart2.Series[0].Points.Clear();

            chart2.ChartAreas[0].AxisX.MajorGrid.LineColor = Color.AntiqueWhite;
            chart2.ChartAreas[0].AxisY.MajorGrid.LineColor = Color.AntiqueWhite;

            chart2.ChartAreas[0].AxisX.LineColor = Color.AntiqueWhite;
            chart2.ChartAreas[0].AxisX.LineWidth = 2;

            chart2.ChartAreas[0].AxisY.LineColor = Color.AntiqueWhite;

            chart2.ChartAreas[0].AxisY.MajorGrid.LineWidth = 2;
            chart2.ChartAreas[0].AxisX.MajorGrid.LineWidth = 2;

            chart2.Series[0].Color = Color.DarkOrange;
            chart2.Series[0].BorderWidth = 2;

            listView1.Columns.Add("时间戳");
            listView1.Columns.Add("类型");
            listView1.Columns.Add("源地址");
            listView1.Columns.Add("目的地址");
            listView1.Columns.Add("信息");
            listView1.AutoResizeColumns(ColumnHeaderAutoResizeStyle.ColumnContent);
            listView1.AutoResizeColumns(ColumnHeaderAutoResizeStyle.HeaderSize);


            for (int i = 0; i < 15; ++i)
                chart2.Series[0].Points.AddXY(i, 0);

        }

        public string byte2String(byte[] bytes) {
            return Encoding.Unicode.GetString(bytes);
        }
        public byte[] compressString(string str) {
            MemoryStream ms = new MemoryStream();
            GZipStream gZipCompress = new GZipStream(ms, CompressionLevel.Fastest);
            gZipCompress.Write(Encoding.Unicode.GetBytes(str), 0, str.Length);

            return ms.ToArray();
        }
        public string decompressString(string str) {
            return byte2String(Decompress(Encoding.Unicode.GetBytes(str)));
        }
        public static byte[] Compress(byte[] data) {
            MemoryStream output = new MemoryStream();
            using (GZipStream dstream = new GZipStream(output, CompressionLevel.Optimal)) {
                dstream.Write(data, 0, data.Length);
            }
            return output.ToArray();
        }
        public static byte[] Decompress(byte[] data) {
            MemoryStream input = new MemoryStream(data);
            MemoryStream output = new MemoryStream();
            using (GZipStream dstream = new GZipStream(input, CompressionMode.Decompress)) {
                dstream.CopyTo(output);
            }
            Debug.Assert(output.ToArray().Length > 0);
            return output.ToArray();
        }
        public async void Sniffle_OutputDataReceived(object sender, DataReceivedEventArgs e) {
            if (is_paused) {
                Debug.WriteLine(e.Data);
                return;
            }
            if (e.Data is null) return;
            if (e.Data.Length == 0 || e.Data.ElementAt(0) != '{') return;
            await Task.Run(() => {
                try {
                    var packetData = JsonSerializer.Deserialize<PacketData>(e.Data);
                    ++tot;
                    lock (data_listViews) {
                        data_listViews.Add(new ListViewItem(new string[] {
                                                packetData.Timestamp,
                                        packetData.Type, packetData.Src, packetData.Dest,
                                        packetData.Msg }));

                        
                        data_Raw.Add(Compress(Encoding.Unicode.GetBytes(packetData.Raw)));

                    }

                } catch (JsonException eee) {
                    Console.WriteLine(eee.ToString());
                }
            });
            listView1.Invoke((Action)delegate {
                if (data_Raw.Count <= 200)
                    listView1.VirtualListSize += 1;
            });

            //    listView1.Items.Add(new ListViewItem(new string[] {
            //                        packetData.Timestamp,
            //                        packetData.Type, packetData.Src, packetData.Dest,
            //                        packetData.Msg}));
            //    data_Raw.Add(packetData.Raw);
            //});
        }

        public void Sniffle_ErrorDataReceived(object sender, DataReceivedEventArgs e) {
            MessageBox.Show("Unhandled exception: \n" + e.Data + "\nPlease handle a bug report.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            Application.Exit();
        }

        private void Form2_FormClosed(object sender, FormClosedEventArgs e) {
            Application.Exit();
        }

        private void listView1_ItemSelectionChanged(object sender, ListViewItemSelectionChangedEventArgs e) {
            //if (listView1.SelectedItems.Count != 1) return;
            int now = listView1.SelectedIndices[0];

            int _base = Math.Max(data_Raw.Count - listView1.VirtualListSize, 0);

            richTextBox1.Text = byte2String(Decompress(data_Raw[now + _base]));
        }

        private void timer1_Tick(object sender, EventArgs e) {
            for (int i = 0; i < 14; ++i)
                chart2.Series[0].Points[i].SetValueY(chart2.Series[0].Points[i + 1].YValues[0]);

            chart2.Series[0].Points[14].SetValueY(tot);
            chart2.ChartAreas[0].RecalculateAxesScale();

            tot = 0;
        }

        private void Form2_Shown(object sender, EventArgs e) {
            timer1.Enabled = true;
            timer2.Enabled = true;
            StopCaptureToolStripMenuItem_Click(sender, e);
        }

        private async void StopCaptureToolStripMenuItem_Click(object sender, EventArgs e) {
            // CTRL_C_EVENT 0
            string name = StopCaptureToolStripMenuItem.Text;
            if (name.StartsWith("停")) {
                bool res = JobManager.AttachConsole((uint)Program.sniffle.Id);
                // Ignore ^C signal
                res = JobManager.SetConsoleCtrlHandler(null, true);
                Debug.Assert(res);
                is_paused = true;
                res = JobManager.GenerateConsoleCtrlEvent(JobManager.CTRL_C_EVENT, 0);
                Debug.Print(Marshal.GetLastWin32Error().ToString());
                    
                Thread.Sleep(10);
                // resume
                JobManager.FreeConsole();
                JobManager.SetConsoleCtrlHandler(null, false);
                StopCaptureToolStripMenuItem.Text = "开始捕获(&S)";
                toolStripStatusLabel1.Text = $"暂停。目前共有 {data_Raw.Count + db_tot} 项，显示最新 {listView1.VirtualListSize} 项。";
                timer1.Enabled = false;
                timer2.Enabled = false;

                textBox1.ReadOnly = false;
                ImportToolStripMenuItem2.Enabled = true;


            } else {
                is_paused = false;
                var istream = Program.sniffle.StandardInput;
                await istream.WriteAsync(textBox1.Text + "\n");
                await istream.FlushAsync();

                menuStrip1.Invoke((Action)delegate {
                    StopCaptureToolStripMenuItem.Text = "停止捕获(&S)";
                    toolStripStatusLabel1.Text = "捕获中";
                    timer1.Enabled = true;
                    timer2.Enabled = true;
                    textBox1.ReadOnly = true;
                    ImportToolStripMenuItem2.Enabled = false;

                    listView1.VirtualListSize = 0;
                    data_listViews.Clear();
                    data_Raw.Clear();
                    var Path = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);

                    doClearDB(Path + "\\sniffle_data.db");

                    data_listViews.Capacity = 0;
                    data_Raw.Capacity = 0;

                    GC.Collect();

                    label2.Text = $"瞬时速率（个 / 秒）";
                    chart1.Visible = false;
                    chart3.Visible = false;
                    chart2.Visible = true;


                });
            }
        }
        private void doClearDB(string filePath) {
            string buffer_db = $"URI=file:{filePath}";

            if (sql_con.State != ConnectionState.Open) {
                sql_con = new SQLiteConnection(buffer_db);
                sql_con.Open();
            }
            sql_cmd = new SQLiteCommand(sql_con);
            db_tot = 0;

            // 清空之前的数据库
            try {
                sql_cmd.CommandText = @"drop table if exists DATA";
                sql_cmd.ExecuteNonQuery();
                sql_cmd.CommandText =
                        @"create table DATA (
                                    Timestamp   TEXT,
                                    Type        TEXT,
                                    Source      TEXT,
                                    Destination TEXT,
                                    Res         TEXT,
                                    Raw         BLOB
                                );";
                sql_cmd.ExecuteNonQuery();
            } catch (SQLiteException exception) {
                sql_con.Close();
                Console.WriteLine($"导出失败：{exception.Message}");
                //MessageBox.Show($"打开数据库失败!\n\n{exception.Message}", "Sniffle", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            } finally {
                sql_con.Close();
            }
        }
        private async void timer2_Tick(object sender, EventArgs e) {
            // 大于 1000，尝试把前 900 个内容缓存到硬盘
            listView1.Update();
            const int take = 900;

            if (data_listViews.Count > 1000 && tot < 100) {

                toolStripProgressBar1.Visible = true;
                timer2.Enabled = false;

                await Task.Run(() => {

                    lock_db.WaitOne();

                    var copy_raw = new List<byte[]>(data_Raw.Take(take));
                    var copy_data = new List<ListViewItem>(data_listViews.Take(take));

                    var Path = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
                    doExportDB(ref copy_raw, ref copy_data, Path + "\\sniffle_data.db", false);

                    copy_data.Clear();
                    copy_raw.Clear();
                    copy_raw.Capacity = copy_raw.Capacity = 0;

                    listView1.Invoke((Action)delegate {
                        if (listView1.VirtualListSize >= data_Raw.Count)
                            listView1.VirtualListSize -= take;
                    });

                    lock (data_Raw) {
                        data_Raw.RemoveRange(0, take);
                        data_listViews.RemoveRange(0, take);
                        data_listViews.Capacity -= take;
                        data_Raw.Capacity -= take;
                    }
                    GC.Collect();
                    Console.WriteLine($"缓存成功! 剩余: {data_Raw.Count} 项");

                    lock_db.ReleaseMutex();

                });

                toolStripProgressBar1.Visible = false;
                timer2.Enabled = true;
                db_tot += take;

            }
        }

        private void doExportDB(ref List<byte[]> data_Raw, ref List<ListViewItem> data_listViews,
                                string filePath, bool clearifExists = true) {
            string buffer_db = $"URI=file:{filePath}";

            if (sql_con.State != ConnectionState.Open) {
                sql_con = new SQLiteConnection(buffer_db);
                sql_con.Open();
            }
            sql_cmd = new SQLiteCommand(sql_con);

            if (clearifExists) {
                // 清空之前的数据库
                doClearDB(filePath);
            }

            int n = data_listViews.Count;
            sql_cmd.CommandText =
                    @"INSERT INTO DATA (Timestamp, Type, Source, Destination, Res, Raw) 
                              Values(@time, @type, @source, @dest, @res, @raw)";

            for (int i = 0; i < n; ++i) {
                sql_cmd.Parameters.AddWithValue("@time", data_listViews[i].SubItems[0].Text);
                sql_cmd.Parameters.AddWithValue("@type", data_listViews[i].SubItems[1].Text);
                sql_cmd.Parameters.AddWithValue("@source", data_listViews[i].SubItems[2].Text);
                sql_cmd.Parameters.AddWithValue("@dest", data_listViews[i].SubItems[3].Text);
                sql_cmd.Parameters.AddWithValue("@res", data_listViews[i].SubItems[4].Text);
                sql_cmd.Parameters.AddWithValue("@raw", data_Raw[i]);
                sql_cmd.Prepare();
                sql_cmd.ExecuteNonQuery();
            }
            sql_con.Close();
        }
        private async void ExporttToolStripMenuItem1_Click(object sender, EventArgs e) {
            if (data_Raw.Count == 0) {
                return;
            }
            string filePath = null;
            bool auto_save = false;
            ExporttToolStripMenuItem1.Enabled = false;
            SaveFileDialog saveFileDialog = null;
            if (e is FileEvent) {
                auto_save = true;
                filePath = (e as FileEvent).FilePath;
            } else {
                saveFileDialog = new SaveFileDialog();
                saveFileDialog.InitialDirectory = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
                saveFileDialog.RestoreDirectory = true;
                saveFileDialog.Filter = "数据文件 (*.db) | *.db";

            }
            if (auto_save || saveFileDialog.ShowDialog() == DialogResult.OK) {
                // 写入数据库
                string old = toolStripStatusLabel1.Text;
                toolStripProgressBar1.Visible = true;
                toolStripStatusLabel1.Text = "（已暂停）保存到数据库...";
                bool need_resume = false;
                if (!is_paused) {
                    need_resume = true;
                    // 先暂停
                    StopCaptureToolStripMenuItem_Click(sender, e);
                }
                if (!auto_save) {
                    filePath = saveFileDialog.FileName;
                }

                await Task.Run(() => {

                    // 等自动保存写完
                    lock_db.WaitOne();
                    // 复制过来覆盖同名文件
                    File.Copy(saveFileDialog.InitialDirectory + "\\sniffle_data.db",
                                filePath, true);

                    // 写进去
                    doExportDB(ref data_Raw, ref data_listViews, filePath, false);

                    // 清理当前的数据
                    listView1.Invoke((Action)delegate {
                        listView1.VirtualListSize = 0;
                    });

                    data_Raw.Clear();
                    data_listViews.Clear();

                    data_Raw.Capacity = data_listViews.Capacity = 0;
                    GC.Collect();

                    lock_db.ReleaseMutex();

                });
                // listView1.Items.Clear();
                toolStripProgressBar1.Visible = false;
                toolStripStatusLabel1.Text = "就绪";

                MessageBox.Show("导出成功！", "提示", MessageBoxButtons.OK, MessageBoxIcon.Information);

                if (need_resume) {
                    // 重新恢复
                    StopCaptureToolStripMenuItem_Click(sender, e);

                }
            }
            ExporttToolStripMenuItem1.Enabled = true;

        }
        private void selectFileDialog() {

        }
        private async void ImportToolStripMenuItem2_Click(object sender, EventArgs e) {
            ImportToolStripMenuItem2.Enabled = false;
            int tot;
            OpenFileDialog openFileDialog = new OpenFileDialog();
            openFileDialog.InitialDirectory = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
            openFileDialog.Filter = "数据文件 (*.db) | *.db";
            openFileDialog.RestoreDirectory = true;
            if (openFileDialog.ShowDialog() == DialogResult.OK) {
                string filePath = openFileDialog.FileName;
                string buffer_db = $"URI=file:{filePath}";
                try {
                    sql_con.Close();
                    sql_con = new SQLiteConnection(buffer_db);
                    sql_con.Open();
                    sql_cmd = new SQLiteCommand(sql_con);



                    sql_cmd.CommandText = @"select count(*) from DATA;";

                    var reader = sql_cmd.ExecuteReader();
                    reader.Read();
                    tot = reader.GetInt32(0);
                    toolStripStatusLabel1.Text = $"正在加载 { tot } 项...";
                    reader.Close();

                    sql_cmd.CommandText = @"select * from DATA;";
                } catch (SQLiteException exception) {
                    MessageBox.Show($"打开数据库失败!\n\n{exception.Message}", "Sniffle", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    return;
                }
                int v4_tot = 0, v6_tot = 0;
                int udp_tot = 0, tcp_tot = 0, arp_tot = 0;
                listView1.VirtualListSize = 0;

                await Task.Run(() => {
                    var reader = sql_cmd.ExecuteReader();
                    lock (data_listViews) {
                        data_Raw.Clear();
                        data_listViews.Clear();
                        data_listViews.Capacity = data_Raw.Capacity = 0;
                        GC.Collect();

                        while (reader.Read()) {
                            data_listViews.Add(new ListViewItem(new string[] {
                                                reader["Timestamp"].ToString(),
                                                reader["Type"].ToString(), reader["Source"].ToString(), reader["Destination"].ToString(),
                                                reader["Res"].ToString() }));

                            data_Raw.Add(reader["Raw"] as byte[]);
                            string type = reader["type"].ToString(), addr = reader["Destination"].ToString();
                            if (type == "TCP") tcp_tot++;
                            if (type == "UDP" || type == "DNS" || type == "QQ") udp_tot++;
                            if (type == "ARP") arp_tot++;

                            if (addr.Contains('.')) v4_tot++;
                            else v6_tot++;
                        }
                        reader.Close();
                        listView1.Invoke((Action)delegate {
                            listView1.BeginUpdate();
                            // listView1.Items.Clear();
                            // listView1.Items.AddRange(data_listViews.ToArray());
                            listView1.EndUpdate();

                        });

                    }
                });
                menuStrip1.Invoke((Action)delegate {
                    ImportToolStripMenuItem2.Enabled = true;
                    toolStripStatusLabel1.Text = $"{tot} 项";
                    listView1.VirtualListSize = data_Raw.Count;

                    label2.Text = $"统计信息";
                    chart2.Visible = false;

                    // 画饼状图
                    // ipv4, ipv6
                    if (v4_tot != 0 || v6_tot != 0) {
                        chart1.Visible = true;
                        chart1.Series[0].Points[1].IsVisibleInLegend =
                            chart1.Series[0].Points[0].IsVisibleInLegend = true;

                        chart1.Series[0].Points[0].YValues[0] = v4_tot;
                        chart1.Series[0].Points[1].YValues[0] = v6_tot;

                        if (v6_tot == 0)
                            chart1.Series[0].Points[1].IsVisibleInLegend = false;

                        if (v4_tot == 0)
                            chart1.Series[0].Points[0].IsVisibleInLegend = false;

                    }
                    // tcp udp arp
                    if (tcp_tot != 0 || udp_tot != 0 || arp_tot != 0) {
                        chart3.Visible = true;
                        chart3.Series[0].Points[0].YValues[0] = tcp_tot;
                        chart3.Series[1].Points[0].YValues[0] = udp_tot;
                        chart3.Series[2].Points[0].YValues[0] = arp_tot;
                        if (tcp_tot == 0)
                            chart3.Series[0].Points[0].IsVisibleInLegend = false;
                        if (udp_tot == 0)
                            chart3.Series[1].Points[0].IsVisibleInLegend = false;
                        if (arp_tot == 0)
                            chart3.Series[2].Points[0].IsVisibleInLegend = false;
                    }

                });
            }
            ImportToolStripMenuItem2.Enabled = true;

        }
        // 启用 listView 的 Virtual Mode
        private void listView1_RetrieveVirtualItem(object sender, RetrieveVirtualItemEventArgs e) {
            int n = data_listViews.Count;
            int _base = Math.Max(n - listView1.VirtualListSize, 0);
            if (_base + e.ItemIndex >= n) return;
            e.Item = data_listViews[_base + e.ItemIndex];
        }
    }
    public class FileEvent : EventArgs {
        private readonly string filePath;
        public FileEvent(string filePath) {
            this.filePath = filePath;
        }
        public string FilePath {
            get { return this.filePath; }
        }
    }
    public class PacketData {
        public string Timestamp { get; set; }
        public string Type { get; set; }
        public string Src { get; set; }
        public string Dest { get; set; }

        public string Msg { get; set; }

        public string Raw { get; set; }


    }
}
