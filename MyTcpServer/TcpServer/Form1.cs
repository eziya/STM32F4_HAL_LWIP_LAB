using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace TcpServer
{
    public partial class Form1 : Form
    {
        private const int LISTEN_PORT = 1234;

        private TcpListener server;
        private Thread serverThread;
        private int connectionCnt;

        void AddListBox(ListBox listbox, String text)
        {
            if(listbox.InvokeRequired)
            {
                listbox.BeginInvoke(new Action(() => { 
                    if(listbox.Items.Count > 1000)
                    {
                        listbox.Items.Clear();
                    }
                    listbox.Items.Add(text);
                    listbox.SelectedIndex = listbox.Items.Count - 1;
                }));
            }
            else
            {
                if (listbox.Items.Count > 1000)
                {
                    listbox.Items.Clear();
                }
                listbox.Items.Add(text);
                listbox.SelectedIndex = listbox.Items.Count - 1;
            }
        }

        public Form1()
        {
            InitializeComponent();
        }
       
        private void Form1_FormClosing(object sender, FormClosingEventArgs e)
        {
            if(server != null)
            {
                server.Stop();
            }

            Application.Exit();
        }

        private void btnStart_Click(object sender, EventArgs e)
        {
            serverThread = new Thread(delegate () {

                try
                {
                    connectionCnt = 0;
                    server = new TcpListener(IPAddress.Any, LISTEN_PORT);
                    server.Server.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReuseAddress, true);
                    server.Start();

                    AddListBox(lbxMessage, "Server started to listen at port " + LISTEN_PORT);

                    while (true)
                    {
                        byte[] buffer = new byte[256];
                        int nRead = 0;                        

                        AddListBox(lbxMessage, "Wait to accept the client...");
                        TcpClient client = server.AcceptTcpClient();
                        connectionCnt++;
                        AddListBox(lbxMessage, connectionCnt + ": Client accepted...");
                        
                        NetworkStream ns = client.GetStream();

                        nRead = ns.Read(buffer, 0, buffer.Length);
                        AddListBox(lbxMessage, connectionCnt + ": Read " + nRead + " bytes from the client");

                        if (nRead == 256)
                        {
                            if ((buffer[0] == 0xAE) && buffer[255] == 0xEA)
                            {
                                DateTime now = DateTime.Now;
                                buffer[1] = 1;   //RESP
                                buffer[2] = (byte)(now.Year - 2000); //year
                                buffer[3] = (byte)now.Month; //month
                                buffer[4] = (byte)now.Day; //day
                                buffer[5] = (byte)now.Hour; //hour
                                buffer[6] = (byte)now.Minute; //minute
                                buffer[7] = (byte)now.Second; //second

                                ns.Write(buffer, 0, buffer.Length);
                                AddListBox(lbxMessage, connectionCnt + ": Write " + buffer.Length + " bytes to the client");
                            }
                        }                        

                        client.Close();
                        AddListBox(lbxMessage, connectionCnt + ": Close the session...");
                    }
                }
                catch (Exception ex)
                {
                    Debug.WriteLine(ex.Message);
                }            
            
            });
            serverThread.IsBackground = true;
            serverThread.Start();
        }

        private void btnStop_Click(object sender, EventArgs e)
        {
            try
            {
                if(serverThread != null)
                {
                    server.Stop();
                    serverThread.Join();
                    serverThread = null;
                    AddListBox(lbxMessage, "Stop to listen at " + LISTEN_PORT);
                }
            }
            catch(Exception ex)
            {
                Debug.WriteLine(ex.Message);
            }
        }

        private void btnClear_Click(object sender, EventArgs e)
        {
            lbxMessage.Items.Clear();
        }
    }
}
