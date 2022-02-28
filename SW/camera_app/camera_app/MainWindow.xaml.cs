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
        public MainWindow()
        {
            InitializeComponent();
        }

        private void DeviceSelectButton_Click(object sender, RoutedEventArgs e)
        {
            if(DeviceSelect.SelectedItem != null) CameraConfig.setCameraModel(DeviceSelect.SelectedItem.ToString());
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            DeviceSelect.Items.Clear();
            List<ICameraInfo> cameras = CameraFinder.Enumerate();
            foreach (ICameraInfo camera in cameras)
            {
                DeviceSelect.Items.Add(camera.GetValueOrDefault("FriendlyName","property not found"));
            }
        }

        private void StartAcquisition_Click(object sender, RoutedEventArgs e)
        {

        }

        private void StopAcquisition_Click(object sender, RoutedEventArgs e)
        {

        }
    }
}
