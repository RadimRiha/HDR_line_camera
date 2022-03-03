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
                string name = camera.GetValueOrDefault("FriendlyName", "property not found");
                int addedItemIndex = DeviceSelect.Items.Add(name);
                if (name.Contains(CameraConfig.CameraSerialNumber) && CameraConfig.CameraSerialNumber != "")
                {
                    DeviceSelect.SelectedIndex = addedItemIndex;
                }
            }
        }
        private void enableControls(bool enabled)
        {
            WidthSlider.IsEnabled = enabled;
            WidthBox.IsEnabled = enabled;
            HeightSlider.IsEnabled = enabled;
            HeightBox.IsEnabled = enabled;
            XOffsetSlider.IsEnabled = enabled;
            XOffsetBox.IsEnabled = enabled;
            CenterXCheck.IsEnabled = enabled;
            StartAcquisition.IsEnabled = enabled;
            StopAcquisition.IsEnabled = enabled;
        }

        private long evalTextInRange(string text, long min, long max)
        {
            long val;
            try { val = long.Parse(text); }
            catch { val = min; }
            if (val > max) val = max;
            if (val < min) val = min;
            return val;
        }

        public MainWindow()
        {
            InitializeComponent();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            OutLabelInstance = OutLabel;
            loaded = true;
            enableControls(false);
            refreshDevices();
        }

        private void DeviceSelectButton_Click(object sender, RoutedEventArgs e)
        {
            enableControls(false);
            if (DeviceSelect.SelectedItem != null)
            {
                if (CameraConfig.SetModel(DeviceSelect.SelectedItem.ToString()))
                {
                    WidthSlider.Maximum = CameraConfig.MaxWidth;
                    HeightSlider.Maximum = CameraConfig.MaxHeight;
                    XOffsetSlider.Maximum = 0;
                    WidthSlider.Value = CameraConfig.MaxWidth;
                    HeightSlider.Value = CameraConfig.DEFAULT_HEIGHT;
                    XOffsetSlider.Value = 0;
                    WidthBox.Text = WidthSlider.Value.ToString();
                    HeightBox.Text = HeightSlider.Value.ToString();
                    XOffsetBox.Text = XOffsetSlider.Value.ToString();
                    enableControls(true);
                }
            }
        }

        private void StartAcquisition_Click(object sender, RoutedEventArgs e)
        {
            CameraConfig.UpdateParams(Convert.ToInt64(WidthSlider.Value), Convert.ToInt64(HeightSlider.Value), Convert.ToInt64(XOffsetSlider.Value));
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
            if (CenterXCheck.IsChecked ?? false) XOffsetSlider.Value = maxOffset / 2;
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
            if (e.Key == Key.Enter) WidthBox_LostFocus(new object(), new RoutedEventArgs());
        }

        private void WidthBox_LostFocus(object sender, RoutedEventArgs e)
        {
            WidthSlider.Value = evalTextInRange(WidthBox.Text, 1, Convert.ToInt64(WidthSlider.Maximum));
        }

        private void HeightBox_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter) HeightBox_LostFocus(new object(), new RoutedEventArgs());
        }

        private void HeightBox_LostFocus(object sender, RoutedEventArgs e)
        {
            HeightSlider.Value = evalTextInRange(HeightBox.Text, 1, CameraConfig.MaxHeight);
        }

        private void XOffsetBox_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter) XOffsetBox_LostFocus(new object(), new RoutedEventArgs());
        }

        private void XOffsetBox_LostFocus(object sender, RoutedEventArgs e)
        {
            XOffsetSlider.Value = evalTextInRange(HeightBox.Text, 0, Convert.ToInt64(XOffsetSlider.Maximum));
        }

        private void CenterXCheck_Click(object sender, RoutedEventArgs e)
        {
            if(CenterXCheck.IsChecked ?? false)
            {
                XOffsetSlider.IsEnabled = false;
                XOffsetBox.IsEnabled = false;
                WidthSlider_ValueChanged(new object(), new RoutedPropertyChangedEventArgs<double>(0,1));
            }
            else
            {
                XOffsetSlider.IsEnabled = true;
                XOffsetBox.IsEnabled = true;
            }
        }

        private void SearchControllerButton_Click(object sender, RoutedEventArgs e)
        {

        }

        private void TriggerPeriodBox_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter) TriggerPeriodBox_LostFocus(new object(), new RoutedEventArgs());
        }

        private void TriggerPeriodBox_LostFocus(object sender, RoutedEventArgs e)
        {
            TriggerPeriodBox.Text = evalTextInRange(TriggerPeriodBox.Text, 4, 0xffff).ToString();
        }

        private void NumberOfPulsesBox_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter) NumberOfPulsesBox_LostFocus(new object(), new RoutedEventArgs());
        }

        private void NumberOfPulsesBox_LostFocus(object sender, RoutedEventArgs e)
        {
            long numOfPulses = evalTextInRange(NumberOfPulsesBox.Text, 1, 10);
            NumberOfPulsesBox.Text = numOfPulses.ToString();
            PulseConfigSelect.Items.Clear();
            for (uint i = 1; i <= numOfPulses; i++) PulseConfigSelect.Items.Add(i);
            PulseConfigSelect.SelectedIndex = 0;
        }

        private void PulsePriodBox_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter) PulsePriodBox_LostFocus(new object(), new RoutedEventArgs());
        }
        
        private void PulsePriodBox_LostFocus(object sender, RoutedEventArgs e)
        {
            PulsePriodBox.Text = evalTextInRange(PulsePriodBox.Text, 2, 10000).ToString();
        }
    }
}
