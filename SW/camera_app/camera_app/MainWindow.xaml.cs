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
        private static bool loaded = false;
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
            loaded = true;
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
            if (!loaded) return;
            WidthBox.Text = WidthSlider.Value.ToString();
            long maxOffset = CameraConfig.MaxWidth - Convert.ToInt64(WidthSlider.Value);
            XOffsetSlider.Maximum = maxOffset;
            XOffsetBox.Text = XOffsetSlider.Value.ToString();
        }

        private void HeightSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            HeightBox.Text = HeightSlider.Value.ToString();
        }

        private void XOffsetSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            XOffsetBox.Text = XOffsetSlider.Value.ToString();
            long maxWidth = CameraConfig.MaxWidth - Convert.ToInt64(XOffsetSlider.Value);
            WidthSlider.Maximum = maxWidth;
            WidthBox.Text = WidthSlider.Value.ToString();
        }

        private void WidthBox_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                long val;
                try { val = long.Parse(WidthBox.Text); }
                catch { val = 1; }
                if (val > WidthSlider.Maximum) val = Convert.ToInt64(WidthSlider.Maximum);
                if (val < 1) val = 1;
                WidthSlider.Value = val;
            }
        }

        private void HeightBox_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                long val;
                try { val = long.Parse(HeightBox.Text); }
                catch { val = 1; }
                if (val > CameraConfig.MaxHeight) val = CameraConfig.MaxHeight;
                if (val < 1) val = 1;
                HeightSlider.Value = val;
            }
        }

        private void XOffsetBox_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                long val;
                try { val = long.Parse(XOffsetBox.Text); }
                catch { val = 0; }
                if (val > XOffsetSlider.Maximum) val = Convert.ToInt64(XOffsetSlider.Maximum);
                if (val < 0) val = 0;
                XOffsetSlider.Value = val;
            }
        }
    }
}
