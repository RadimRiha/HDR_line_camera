using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Basler.Pylon;

namespace camera_app
{
    static class AcquisitionHandler
    {
        public static void Start()
        {
            try
            {
                using (Camera camera = new Camera(CameraConfig.CameraSerialNumber))
                {/*
                    camera.Open();
                    camera.Parameters[PLCameraInstance.MaxNumBuffer].SetValue(5);
                    camera.StreamGrabber.Start();

                    // Grab a number of images.
                    for (int i = 0; i < 10; ++i)
                    {
                        // Wait for an image and then retrieve it. A timeout of 5000 ms is used.
                        IGrabResult grabResult = camera.StreamGrabber.RetrieveResult(5000, TimeoutHandling.ThrowException);
                        using (grabResult)
                        {
                            // Image grabbed successfully?
                            if (grabResult.GrabSucceeded)
                            {
                                byte[] buffer = grabResult.PixelData as byte[];
                                //MainWindow.OutLabelInstance.Content = MainWindow.OutLabelInstance.Content + "Gray value of first pixel: " + buffer[0] + "\n";
                            }
                            else
                            {
                                //MainWindow.OutLabelInstance.Content = MainWindow.OutLabelInstance.Content + "error\n";
                            }
                        }
                    }
                    camera.StreamGrabber.Stop();
                    camera.Close();*/
                }
            }
            catch
            {
                
            }
        }
    }
}
