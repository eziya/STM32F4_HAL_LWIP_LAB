using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace MyTcpClient
{
    public partial class Form1 : Form
    {

        private TextBox[] sndTextBoxes;
        private TextBox[] rcvTextBoxes;
        private TextBox[] errTextBoxes;

        private bool stopThread = false;

        private void UpdateTextBox(TextBox textBox, string data)
        {
            if (textBox.InvokeRequired)
            {
                // 작업쓰레드인 경우
                textBox.BeginInvoke(new Action(() =>
                {
                    textBox.Text = data;
                }));
            }
            else
            {
                // UI 쓰레드인 경우
                textBox.Text = data;
            }
        }


        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            try
            {
                sndTextBoxes = new TextBox[] { txtSndCnt1, txtSndCnt2, txtSndCnt3 };
                rcvTextBoxes = new TextBox[] { txtRcvCnt1, txtRcvCnt2, txtRcvCnt3 };
                errTextBoxes = new TextBox[] { txtErrCnt1, txtErrCnt2, txtErrCnt3 };
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
            }
        }

        private void btnConnect_Click(object sender, EventArgs e)
        {
            try
            {
                //Thread 를 생성해서 다중으로 전송한다.
                Thread t1 = new Thread(() => Run(1, txtIP.Text));
                Thread t2 = new Thread(() => Run(2, txtIP.Text));
                Thread t3 = new Thread(() => Run(3, txtIP.Text));

                t1.IsBackground = true;
                t2.IsBackground = true;
                t3.IsBackground = true;

                stopThread = false;

                t1.Start();

                if(radioButton1.Checked)
                {
                    //LwIP supports backlog queue but W5x00 doesn't
                    t2.Start();
                    t3.Start();
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
            }
        }

        private void Run(int idx, String ipStr)
        {
            TcpClient tcpClient;
            IPAddress ipAddr;

            byte[] sndBuffer = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77 };
            byte[] rcvBuffer = new byte[sndBuffer.Length];
            int errCnt = 0;

            if (IPAddress.TryParse(ipStr, out ipAddr))
            {
                for (int i = 0; i < 10000; i++)
                {
                    try
                    {
                        if (stopThread) break;

                        tcpClient = new TcpClient();
                        tcpClient.Connect(ipAddr, 7);

                        NetworkStream ns = tcpClient.GetStream();
                        ns.WriteTimeout = 100;
                        ns.ReadTimeout = 100;
                                                
                        ns.Write(sndBuffer, 0, sndBuffer.Length);
                        ns.Flush();
                        int nRead = ns.Read(rcvBuffer, 0, rcvBuffer.Length);
                        if (nRead != sndBuffer.Length) errCnt++;
                        
                        UpdateTextBox(sndTextBoxes[idx - 1], Convert.ToString(i));
                        UpdateTextBox(rcvTextBoxes[idx - 1], Convert.ToString(i));
                        UpdateTextBox(errTextBoxes[idx - 1], Convert.ToString(errCnt));


                        if(tcpClient.Connected)
                        {
                            ns.Close();
                            tcpClient.Close();
                        }
                        
                        Thread.Sleep(10);
                    }
                    catch (Exception)
                    {
                        errCnt++;
                    }
                }
            }
        }

        private void radioButton1_CheckedChanged(object sender, EventArgs e)
        {
            txtIP.Text = "192.168.1.179";
        }

        private void radioButton2_CheckedChanged(object sender, EventArgs e)
        {
            txtIP.Text = "192.168.1.180";
        }

        private void btnStop_Click(object sender, EventArgs e)
        {
            stopThread = true;

            Thread.Sleep(100);

            txtErrCnt1.Text = txtErrCnt2.Text = txtErrCnt3.Text = "0";
            txtRcvCnt1.Text = txtRcvCnt2.Text = txtRcvCnt3.Text = "0";
            txtSndCnt1.Text = txtSndCnt2.Text = txtSndCnt3.Text = "0";
        }
    }
}