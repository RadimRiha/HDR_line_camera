using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using Basler.Pylon;

namespace camera_app
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public static Label OutLabelInstance;
        private void refreshDevices()
        {
            DeviceSelect.Items.Clear();
            List<ICameraInfo> cameras = CameraFinder.Enumerate();
            foreach (ICameraInfo camera in cameras)
            {
                DeviceSelect.Items.Add(camera.GetValueOrDefault("FriendlyName", "property not found"));
            }
        }

        public MainWindow()
        {
            InitializeComponent();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            OutLabelInstance = OutLabel;
            refreshDevices();
        }

        private void DeviceSelectButton_Click(object sender, RoutedEventArgs e)
        {
            if (DeviceSelect.SelectedItem != null) CameraConfig.SetModel(DeviceSelect.SelectedItem.ToString());
            WidthSlider.Maximum = CameraConfig.MaxWidth;
            HeightSlider.Maximum = CameraConfig.MaxHeight;
            XOffsetSlider.Maximum = 0;
            WidthSlider.Value = CameraConfig.MaxWidth;
            HeightSlider.Value = CameraConfig.DEFAULT_HEIGHT;
            XOffsetSlider.Value = 0;
            WidthBox.Text = WidthSlider.Value.ToString();
            HeightBox.Text = HeightSlider.Value.ToString();
            XOffsetBox.Text = XOffsetSlider.Value.ToString();
        }

        private void StartAcquisition_Click(object sender, RoutedEventArgs e)
        {
            AcquisitionHandler.Start();
        }

        private void StopAcquisition_Click(object sender, RoutedEventArgs e)
        {

        }

        private void Refresh_Click(object sender, RoutedEventArgs e)
        {
            refreshDevices();
        }

        private void WidthSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {

        }

        private void HeightSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            HeightBox.Text = HeightSlider.Value.ToString();
        }

        private void XOffsetSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {

        }

        private void WidthBox_PreviewKeyDown(object sender, KeyEventArgs e)
        {

        }

        private void HeightBox_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                long val;
                try
                {
                    val = long.Parse(HeightBox.Text);
                }
                catch
                {
                    val = 1;
                }
                if (val > CameraConfig.MaxHeight) val = CameraConfig.MaxHeight;
                if (val < 1) val = 1;
                HeightSlider.Value = val;
                HeightBox.Text = val.ToString();
            }
        }
    }
}
