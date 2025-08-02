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

        // Thread들을 관리하기 위한 List
        private List<Thread> activeThreads = new List<Thread>();
        private bool stopThread = false;

        /// <summary>
        /// UI 스레드와 작업 스레드 간의 안전한 TextBox 업데이트를 위한 메서드
        /// </summary>
        /// <param name="textBox">업데이트할 TextBox 컨트롤</param>
        /// <param name="data">표시할 문자열 데이터</param>
        private void UpdateTextBox(TextBox textBox, string data)
        {
            if (textBox.InvokeRequired)
            {
                // 작업 스레드인 경우, UI 스레드에 업데이트를 요청
                textBox.BeginInvoke(new Action(() =>
                {
                    textBox.Text = data;
                }));
            }
            else
            {
                // UI 스레드인 경우, 직접 업데이트
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
                // UI 컨트롤 배열 초기화
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
                // 이전 스레드들을 중지하도록 플래그 설정
                stopThread = false;

                // 기존에 실행 중인 스레드가 있으면 정리
                if (activeThreads.Count > 0)
                {
                    // 스레드 중지 요청 후 정리
                    stopThread = true;
                    foreach (Thread t in activeThreads)
                    {
                        if (t.IsAlive) t.Join(1000); // 1초 대기 후 종료
                    }
                    activeThreads.Clear();
                    stopThread = false;
                }

                // 요청에 따라 3개의 스레드를 생성하여 실행
                Thread t1 = new Thread(() => Run(1, txtIP.Text));
                Thread t2 = new Thread(() => Run(2, txtIP.Text));
                Thread t3 = new Thread(() => Run(3, txtIP.Text));

                t1.IsBackground = true;
                t2.IsBackground = true;
                t3.IsBackground = true;

                activeThreads.Add(t1);
                activeThreads.Add(t2);
                activeThreads.Add(t3);

                t1.Start();

                if (radioButton1.Checked)
                {
                    t2.Start();
                    t3.Start();
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
            }
        }

        /// <summary>
        /// TCP 클라이언트 연결, 송수신, 종료 로직을 담은 메인 작업 메서드
        /// 요청에 따라 100번의 연결-종료, 각 연결에서 100번의 송수신을 수행
        /// </summary>
        /// <param name="idx">스레드 인덱스</param>
        /// <param name="ipStr">서버 IP 주소 문자열</param>
        private void Run(int idx, String ipStr)
        {
            IPAddress ipAddr;
            int payloadSize = 256; // 요청에 따라 500바이트로 변경
            byte[] sndBuffer = new byte[payloadSize];
            //sndBuffer는 더미 데이터이므로 초기화하지 않고 그대로 사용
            byte[] rcvBuffer = new byte[payloadSize];
            int errCnt = 0;

            if (IPAddress.TryParse(ipStr, out ipAddr))
            {
                // 외부 루프: 연결-종료 사이클을 100번 반복
                for (int connectCycle = 0; connectCycle < 100; connectCycle++)
                {
                    if (stopThread) break;
                    TcpClient tcpClient = null;
                    NetworkStream ns = null;
                    try
                    {
                        tcpClient = new TcpClient();
                        tcpClient.Connect(ipAddr, 7);
                        ns = tcpClient.GetStream();

                        // 타임아웃을 넉넉하게 설정
                        ns.WriteTimeout = 100;
                        ns.ReadTimeout = 100;

                        // 내부 루프: 한 번 연결한 후 100번의 송수신을 반복
                        for (int sendRcvCycle = 0; sendRcvCycle < 100; sendRcvCycle++)
                        {
                            if (stopThread) break;

                            // 500바이트 더미 데이터 송신
                            ns.Write(sndBuffer, 0, sndBuffer.Length);
                            ns.Flush();

                            // 수신 데이터 읽기
                            int nRead = 0;
                            while (nRead < sndBuffer.Length)
                            {
                                int size = ns.Read(rcvBuffer, nRead, rcvBuffer.Length - nRead);
                                if (size == 0)
                                {
                                    // 서버에서 연결이 끊긴 경우 예외 발생
                                    throw new IOException("Connection closed by remote host.");
                                }
                                nRead += size;
                            }

                            // UI 업데이트
                            // connectCycle과 sendRcvCycle을 모두 표시하여 진행 상황 확인 용이하게 변경
                            UpdateTextBox(sndTextBoxes[idx - 1], $"C: {connectCycle + 1}, S: {sendRcvCycle + 1}");
                            UpdateTextBox(rcvTextBoxes[idx - 1], $"C: {connectCycle + 1}, S: {sendRcvCycle + 1}");

                            // 각 송수신 사이클 간 딜레이
                            Thread.Sleep(20);
                        }
                    }
                    catch (Exception ex)
                    {
                        errCnt++;
                        Debug.WriteLine($"Thread {idx} Error: {ex.Message}");
                        // UI에 에러 카운트 업데이트
                        UpdateTextBox(errTextBoxes[idx - 1], Convert.ToString(errCnt));

                        // 예외 발생 시, 다음 연결 사이클로 진행
                    }
                    finally
                    {
                        // 연결 종료
                        if (ns != null)
                        {
                            ns.Close();
                        }
                        if (tcpClient != null)
                        {
                            tcpClient.Close();
                        }
                    }
                    // 연결-종료 사이클 간 약간의 지연
                    Thread.Sleep(100);
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

            // 모든 스레드가 종료될 때까지 대기
            foreach (Thread t in activeThreads)
            {
                if (t.IsAlive)
                {
                    t.Join(2000); // 2초 대기
                }
            }
            activeThreads.Clear();

            // UI 초기화
            txtErrCnt1.Text = txtErrCnt2.Text = txtErrCnt3.Text = "0";
            txtRcvCnt1.Text = txtRcvCnt2.Text = txtRcvCnt3.Text = "0";
            txtSndCnt1.Text = txtSndCnt2.Text = txtSndCnt3.Text = "0";

            stopThread = false; // 플래그 초기화
        }

        private void txtErrCnt1_TextChanged(object sender, EventArgs e)
        {

        }
    }
}