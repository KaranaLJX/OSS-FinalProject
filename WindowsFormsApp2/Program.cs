using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Diagnostics;

namespace WindowsFormsApp2 {
    static class Program {
        /// <summary>
        /// 应用程序的主入口点。
        /// </summary>
        /// 
        public static Process sniffle;
        public static ProcessStartInfo sniffle_info;
        [STAThread]
        static void Main() {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new Form1());
        }
    }
}
