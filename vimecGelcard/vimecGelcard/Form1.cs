using System;
using System.Drawing;
using System.IO;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using AForge.Video;
using MySqlX.XDevAPI;
using uPLibrary.Networking.M2Mqtt;
using uPLibrary.Networking.M2Mqtt.Messages;
using System.Security.Cryptography.X509Certificates;
using uPLibrary.Networking.M2Mqtt.Exceptions;

namespace vimecGelcard
{
    public partial class Form1 : Form
    {
        private HttpClient httpClient = new HttpClient();
        private string esp32CamIP = "192.168.1.184"; // Địa chỉ IP mặc định

        string broker = "f42bfb9b.ala.us-east-1.emqxsl.com";
        int port = 8883;
        string topic1 = "barcode/request";
        string topic2 = "barcode/response";
        string topic3 = "wifi/request";
        string clientId = Guid.NewGuid().ToString();
        string username = "gelcard";
        string password = "gelcard";

        private string ca_cert = @"-----BEGIN CERTIFICATE-----
        MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh
        MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
        d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD
        QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT
        MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
        b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG
        9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB
        CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97
        nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt
        43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P
        T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4
        gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO
        BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR
        TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw
        DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr
        hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg
        06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF
        PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls
        YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk
        CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=
        -----END CERTIFICATE-----";

        private MqttClient client;

        public Form1()
        {
            InitializeComponent();
            // Khởi tạo đối tượng client và cấu hình

            X509Certificate caCert = new X509Certificate(Encoding.UTF8.GetBytes(ca_cert));

            client = ConnectMQTT(broker, port, clientId, username, password, caCert);
            client.MqttMsgPublishReceived += client_MqttMsgPublishReceived;
            client.Subscribe(new string[] { topic2 }, new byte[] { MqttMsgBase.QOS_LEVEL_AT_MOST_ONCE });
            client.Subscribe(new string[] { topic3 }, new byte[] { MqttMsgBase.QOS_LEVEL_AT_MOST_ONCE });
            if (client.IsConnected)
            {
                richTextBox1.AppendText("Connected to MQTT Broker" + Environment.NewLine);
            }
        }

        private void client_MqttMsgPublishReceived(object sender, MqttMsgPublishEventArgs e)
        {
            var message = Encoding.UTF8.GetString(e.Message);
            if (e.Topic == topic3)
            {
                // Cập nhật địa chỉ IP mới từ message
                esp32CamIP = message;
                richTextBox1.Invoke((MethodInvoker)(() => richTextBox1.AppendText($"Received new ESP32 Cam IP: {esp32CamIP}" + Environment.NewLine)));
            }
            listBox1.Invoke((MethodInvoker)(() => listBox1.Items.Add(message)));
            BeginInvoke(new Action(TakePhotoAndDisplay));
        }

        private MqttClient ConnectMQTT(string broker, int port, string clientId, string username, string password, X509Certificate caCert)
        {

            MqttClient client = new MqttClient(broker, port, true, null, null, MqttSslProtocols.TLSv1_2);
            client.Connect(clientId, username, password, true, 60);
            if (client.IsConnected)
            {
                Console.WriteLine("Connected to MQTT Broker");
            }
            else
            {
                Console.WriteLine("Failed to connect");
            }
            return client;
        }

        private async void TakePhotoAndDisplay()
        {
            try
            {
                string esp32CamURL = $"http://{esp32CamIP}/capture";
                HttpResponseMessage response = await httpClient.GetAsync(esp32CamURL);
                if (response.IsSuccessStatusCode)
                {
                    byte[] imageBytes = await response.Content.ReadAsByteArrayAsync();
                    using (var stream = new MemoryStream(imageBytes))
                    {
                        Bitmap bitmap = new Bitmap(stream);
                        pictureBox1.Image = new Bitmap(bitmap);

                        // Lưu ảnh vào thư mục
                        string savePath = Path.Combine("D:\\Data_Image", DateTime.Now.ToString("yyyyMMddHHmmss") + ".jpg");
                        bitmap.Save(savePath);
                    }
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine("Error: " + ex.Message);
            }
        }

        private async Task SetFrameSize(string frameSize)
        {
            string setFrameSizeURL = $"http://192.168.1.184/set_frame_size?size={frameSize}";

            try
            {
                HttpResponseMessage response = await httpClient.GetAsync(setFrameSizeURL);
                if (response.IsSuccessStatusCode)
                {
                    Console.WriteLine($"Frame size set to {frameSize}");
                }
                else
                {
                    Console.WriteLine($"Failed to set frame size to {frameSize}");
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine("Error: " + ex.Message);
            }
        }

        private void button1_Click(object sender, EventArgs e)
        {
            client.Publish(topic1, Encoding.UTF8.GetBytes("barcode"));
            Invoke(new Action(() => richTextBox1.AppendText("read barcode:" + Environment.NewLine)));
        }

        private void button2_Click(object sender, EventArgs e)
        {

        }

        private void button3_Click(object sender, EventArgs e)
        {
            saveFileDialog1.InitialDirectory = "D:\\Data_Image";
            if (saveFileDialog1.ShowDialog() == DialogResult.OK)
            {
                pictureBox1.Image.Save(saveFileDialog1.FileName);
            }
        }

        private async void comboBox1_SelectedIndexChanged_1(object sender, EventArgs e)
        {
            string esp32CamURL = "http://192.168.1.184/capture";
            string selectedResolution = comboBox1.SelectedItem.ToString();
            switch (selectedResolution)
            {
                case "320x240":
                    esp32CamURL = "http://192.168.1.184/capture?frame_size=320x240";
                    break;
                case "640x480":
                    esp32CamURL = "http://192.168.1.184/capture?frame_size=640x480";
                    break;
                case "1280x720":
                    esp32CamURL = "http://192.168.1.184/capture?frame_size=1280x720";
                    break;
            }

            // Gửi yêu cầu thay đổi kích thước khung hình
            await SetFrameSize(selectedResolution);

            // Thay đổi kích thước của pictureBox1
            UpdatePictureBoxSize(selectedResolution);
        }

        private void pictureBox1_Click(object sender, EventArgs e)
        {

        }

        private void Form1_Load_1(object sender, EventArgs e)
        {
            // Thêm các mục lựa chọn độ phân giải vào comboBox1
            comboBox1.Items.Add("320x240");
            comboBox1.Items.Add("640x480");
            comboBox1.Items.Add("1280x720");

            // Mặc định chọn độ phân giải là "640x480"
            comboBox1.SelectedIndex = 1; // Chọn mục thứ 1 trong danh sách (0-based index)
        }
        private void UpdatePictureBoxSize(string frameSize)
        {
            int width = 0;
            int height = 0;

            switch (frameSize)
            {
                case "320x240":
                    width = 320;
                    height = 240;
                    break;
                case "640x480":
                    width = 640;
                    height = 480;
                    break;
                case "1280x720":
                    width = 1280;
                    height = 720;
                    break;
            }

            pictureBox1.Width = width;
            pictureBox1.Height = height;
            pictureBox1.SizeMode = PictureBoxSizeMode.CenterImage;
        }

        private void button4_Click(object sender, EventArgs e)
        {

        }

        private void richTextBox1_TextChanged(object sender, EventArgs e)
        {

        }
    }
}

