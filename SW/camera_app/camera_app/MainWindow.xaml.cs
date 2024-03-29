﻿using System;
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
        private static bool loaded = false;
        private static AcquisitionHandler acqHandler = new AcquisitionHandler();
        
        private void applicationExitEventHandler(object sender, EventArgs e)
        {
            if (acqHandler.Camera != null) acqHandler.Camera.Close();
        }
        private void cameraDisconnectedEventHandler(object sender, EventArgs e)
        {
            outputText("Camera disconnected");
            StopAcquisition_Click(new object(), new RoutedEventArgs());
        }
        private void imageGrabbedEventHandler(object sender, EventArgs e)
        {
            try
            {
                this.Dispatcher.Invoke(() =>
                {
                    ImagesLabel.Content = acqHandler.GrabbedImagesOkCount;
                    ErrorsLabel.Content = acqHandler.GrabbedImagesFailCount;
                });
            }
            catch { }
        }
        private void refreshDevices()
        {
            DeviceSelect.Items.Clear();
            List<ICameraInfo> cameras = CameraFinder.Enumerate();
            foreach (ICameraInfo camera in cameras)
            {
                string name = camera.GetValueOrDefault("FriendlyName", "property not found");
                int addedItemIndex = DeviceSelect.Items.Add(name);
                if (acqHandler.Camera == null) continue;
                if (camera.GetValueOrDefault("FriendlyName", "") == acqHandler.Camera.CameraInfo.GetValueOrDefault("FriendlyName", "")) DeviceSelect.SelectedIndex = addedItemIndex;
            }
        }
        private void enableCameraControls(bool enabled)
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

        private void enableControllerControls(bool enabled)
        {
            TriggerSource.IsEnabled = enabled;
            TriggerPeriodBox.IsEnabled = enabled;
            TriggerPolarity.IsEnabled = enabled;
            NumberOfPulsesBox.IsEnabled = enabled;
            PulseConfigSelect.IsEnabled = enabled;
            PulseOutput.IsEnabled = enabled;
            PulsePeriodBox.IsEnabled = enabled;
            ConstructHdrCheck.IsEnabled = enabled;
            ConstructRgbCheck.IsEnabled = enabled;
            DisplayRawCheck.IsEnabled = enabled;
            DisplayPartialCheck.IsEnabled = enabled;
            if (enabled) updateTriggerControls();
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

        private void outputText(string text)
        {
            OutBox.AppendText(text);
            OutBox.AppendText("\u2028");
            OutBox.ScrollToEnd();
        }

        private void updateHeightSliderMaximum()
        {
            HeightSlider.Maximum = acqHandler.MaxHeight / Convert.ToUInt32(NumberOfPulsesBox.Text);
        }

        private void updateTriggerControls()
        {
            TriggerPeriodBox.IsEnabled = TriggerSource.SelectedIndex == 2;
            TriggerPolarity.IsEnabled = TriggerSource.SelectedIndex == 3;
        }

        public MainWindow()
        {
            InitializeComponent();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            loaded = true;
            enableCameraControls(false);
            enableControllerControls(false);
            refreshDevices();
            acqHandler.CameraDisconnectedEvent += new EventHandler(cameraDisconnectedEventHandler);
            acqHandler.ImageGrabbedEvent += new EventHandler(imageGrabbedEventHandler);
            App.ApplicationExitEvent += new EventHandler(applicationExitEventHandler);
        }

        private void DeviceSelectButton_Click(object sender, RoutedEventArgs e)
        {
            enableCameraControls(false);
            if (DeviceSelect.SelectedItem == null)
            {
                outputText("Camera initialization FAIL");
                return;
            }
            if (!acqHandler.SetModel(DeviceSelect.SelectedItem.ToString()))
            {
                outputText("Camera initialization FAIL");
                return;
            }
            WidthSlider.Maximum = acqHandler.OrigWidth;
            WidthSlider.Value = acqHandler.OrigWidth;
            WidthSlider_ValueChanged(new object(), new RoutedPropertyChangedEventArgs<double>(0, 1));
            updateHeightSliderMaximum();
            HeightSlider.Value = acqHandler.OrigHeight;
            XOffsetSlider.Value = acqHandler.OrigXOffset;
            XOffsetSlider_ValueChanged(new object(), new RoutedPropertyChangedEventArgs<double>(0, 1));
            enableCameraControls(true);
            StopAcquisition.IsEnabled = false;
            outputText("Camera initialization OK");
        }

        private void StartAcquisition_Click(object sender, RoutedEventArgs e)
        {
            if (!acqHandler.UploadConfig(Convert.ToInt64(WidthSlider.Value), Convert.ToInt64(HeightSlider.Value) * Convert.ToUInt32(NumberOfPulsesBox.Text), Convert.ToInt64(XOffsetSlider.Value)))
            {
                outputText("Camera configuration FAIL");
                return;
            }
            outputText("Camera configuration OK");
            if (!ControllerConfig.UploadConfig((uint)TriggerSource.SelectedIndex, Convert.ToUInt16(TriggerPeriodBox.Text), (uint)TriggerPolarity.SelectedIndex, Convert.ToUInt16(NumberOfPulsesBox.Text)))
            {
                outputText("Controller configuration FAIL");
                return;
            }
            outputText("Controller configuration OK");
            if (!acqHandler.StartGrabbing())
            {
                outputText("Acquisition initialization FAIL");
                return;
            }
            outputText("Acquisition initialization OK");
            ControllerConfig.EnableTriggering(true);
            ControllerConfig.EnableTriggering(true);
            enableCameraControls(false);
            StopAcquisition.IsEnabled = true;
            enableControllerControls(false);
            DeviceSelectButton.IsEnabled = false;
            SearchControllerButton.IsEnabled = false;
            ImagesLabel.Content = 0;
            ErrorsLabel.Content = 0;
        }

        private void StopAcquisition_Click(object sender, RoutedEventArgs e)
        {
            acqHandler.StopGrabbing();
            enableCameraControls(true);
            StopAcquisition.IsEnabled = false;
            enableControllerControls(true);
            DeviceSelectButton.IsEnabled = true;
            SearchControllerButton.IsEnabled = true;
        }

        private void Refresh_Click(object sender, RoutedEventArgs e)
        {
            refreshDevices();
        }

        private void WidthSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (!loaded) return;
            WidthBox.Text = WidthSlider.Value.ToString();
            long maxOffset = acqHandler.MaxWidth - Convert.ToInt64(WidthSlider.Value);
            XOffsetSlider.Maximum = maxOffset;
            XOffsetBox.Text = XOffsetSlider.Value.ToString();
            if (CenterXCheck.IsChecked ?? false) XOffsetSlider.Value = maxOffset / 2;
            acqHandler.SetWidth(Convert.ToInt64(WidthSlider.Value));
            updateHeightSliderMaximum();
        }

        private void HeightSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            HeightBox.Text = HeightSlider.Value.ToString();
        }

        private void XOffsetSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            XOffsetBox.Text = XOffsetSlider.Value.ToString();
            long maxWidth = acqHandler.MaxWidth - Convert.ToInt64(XOffsetSlider.Value);
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
            HeightSlider.Value = evalTextInRange(HeightBox.Text, 1, acqHandler.MaxHeight);
        }

        private void XOffsetBox_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter) XOffsetBox_LostFocus(new object(), new RoutedEventArgs());
        }

        private void XOffsetBox_LostFocus(object sender, RoutedEventArgs e)
        {
            XOffsetSlider.Value = evalTextInRange(XOffsetBox.Text, 0, Convert.ToInt64(XOffsetSlider.Maximum));
        }

        private void CenterXCheck_Click(object sender, RoutedEventArgs e)
        {
            if (CenterXCheck.IsChecked ?? false)
            {
                XOffsetSlider.IsEnabled = false;
                XOffsetBox.IsEnabled = false;
                WidthSlider_ValueChanged(new object(), new RoutedPropertyChangedEventArgs<double>(0, 1));
            }
            else
            {
                XOffsetSlider.IsEnabled = true;
                XOffsetBox.IsEnabled = true;
            }
        }

        private void SearchControllerButton_Click(object sender, RoutedEventArgs e)
        {
            enableControllerControls(false);
            ControllerConnectedCheck.IsChecked = false;
            if (!ControllerConfig.Search())
            {
                outputText("No controller found");
                return;
            }
            TriggerSource.SelectedIndex = Convert.ToInt32(evalTextInRange(ControllerConfig.GetResponse("GTRS"), 0, ControllerConfig.MaxTriggerSource));
            TriggerPolarity.SelectedIndex = Convert.ToInt32(evalTextInRange(ControllerConfig.GetResponse("GHTP"), 0, ControllerConfig.MaxTriggerPolarity));
            PulseConfigSelect.SelectedIndex = -1;
            PulseConfigSelect.SelectedIndex = 0;
            ControllerConnectedCheck.IsChecked = true;
            TriggerPeriodBox.Text = evalTextInRange(ControllerConfig.GetResponse("GTTP"), ControllerConfig.MinTimedTriggerPeriod, ControllerConfig.MaxTimedTriggerPeriod).ToString();
            NumberOfPulsesBox.Text = ControllerConfig.NumOfPulses.ToString();
            updateHeightSliderMaximum();
            NumberOfPulsesBox_LostFocus(new object(), new RoutedEventArgs());
            enableControllerControls(true);
            outputText("Controller found");
        }

        private void TriggerPeriodBox_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter) TriggerPeriodBox_LostFocus(new object(), new RoutedEventArgs());
        }

        private void TriggerPeriodBox_LostFocus(object sender, RoutedEventArgs e)
        {
            TriggerPeriodBox.Text = evalTextInRange(TriggerPeriodBox.Text, ControllerConfig.MinTimedTriggerPeriod, ControllerConfig.MaxTimedTriggerPeriod).ToString();
        }

        private void NumberOfPulsesBox_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter) NumberOfPulsesBox_LostFocus(new object(), new RoutedEventArgs());
        }

        private void NumberOfPulsesBox_LostFocus(object sender, RoutedEventArgs e)
        {
            uint numOfPulses = (uint)evalTextInRange(NumberOfPulsesBox.Text, 1, ControllerConfig.MaxNumPulses);
            NumberOfPulsesBox.Text = numOfPulses.ToString();
            PulseConfigSelect.Items.Clear();
            for (uint i = 1; i <= numOfPulses; i++) PulseConfigSelect.Items.Add(i);
            PulseConfigSelect.SelectedIndex = 0;
            updateHeightSliderMaximum();
        }

        private void PulsePeriodBox_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter) PulsePeriodBox_LostFocus(new object(), new RoutedEventArgs());
        }

        private void PulsePeriodBox_LostFocus(object sender, RoutedEventArgs e)
        {
            PulsePeriodBox.Text = evalTextInRange(PulsePeriodBox.Text, AcquisitionHandler.MinExposure, AcquisitionHandler.MaxExposure).ToString();
            ControllerConfig.PulsePeriod[PulseConfigSelect.SelectedIndex] = uint.Parse(PulsePeriodBox.Text);
        }

        private void PulseOutput_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ControllerConfig.PulseOutput[PulseConfigSelect.SelectedIndex] = (uint)PulseOutput.SelectedIndex;
        }

        private void PulseConfigSelect_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (!loaded) return;
            try
            {
                PulseOutput.SelectedIndex = (int)ControllerConfig.PulseOutput[PulseConfigSelect.SelectedIndex];
                PulsePeriodBox.Text = ControllerConfig.PulsePeriod[PulseConfigSelect.SelectedIndex].ToString();
            }
            catch { }
        }

        private void TriggerSource_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (!loaded) return;
            updateTriggerControls();
        }

        private void ConstructHdrCheck_Click(object sender, RoutedEventArgs e)
        {
            if (ConstructHdrCheck.IsChecked ?? false) FrameProcessor.ConstructHdr = true;
            else FrameProcessor.ConstructHdr = false;
        }

        private void ConstructRgbCheck_Click(object sender, RoutedEventArgs e)
        {
            if (ConstructRgbCheck.IsChecked ?? false) FrameProcessor.ConstructRgb = true;
            else FrameProcessor.ConstructRgb = false;
        }

        private void DisplayRawCheck_Click(object sender, RoutedEventArgs e)
        {
            if (DisplayRawCheck.IsChecked ?? false) FrameProcessor.DisplayRaw = true;
            else FrameProcessor.DisplayRaw = false;
        }

        private void DisplayPartialCheck_Click(object sender, RoutedEventArgs e)
        {
            if (DisplayPartialCheck.IsChecked ?? false) FrameProcessor.DisplayPartial = true;
            else FrameProcessor.DisplayPartial = false;
        }
    }
}
