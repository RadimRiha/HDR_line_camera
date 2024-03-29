﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Basler.Pylon;

namespace camera_app
{
    class AcquisitionHandler
    {
        public Camera Camera;

        static public readonly long MinExposure = 2;
        static public readonly long MaxExposure = 10000;

        static readonly string[] supportedCameras = {
        "Basler raL6144-16gm"
        };

        public long MaxWidth = 0;
        public long MaxHeight = 0;
        public long OrigWidth = 1;
        public long OrigHeight = 1;
        public long OrigXOffset = 0;

        public long GrabbedImagesOkCount = 0;
        public long GrabbedImagesFailCount = 0;

        public event EventHandler CameraDisconnectedEvent;
        public event EventHandler ImageGrabbedEvent;

        private void onCameraDisconnected(Object sender, EventArgs e)
        {
            if (CameraDisconnectedEvent != null) CameraDisconnectedEvent(new object(), e);
        }

        private void onImageGrabbed(Object sender, ImageGrabbedEventArgs e)
        {
            // The grab result is automatically disposed when the event call back returns.
            // The grab result can be cloned using IGrabResult.Clone if you want to keep a copy of it (not shown in this sample).
            IGrabResult grabResult = e.GrabResult;
            if (grabResult.GrabSucceeded)
            {
                GrabbedImagesOkCount++;
                FrameProcessor.ProcessGrabResult(grabResult, ControllerConfig.NumOfPulses);
            }
            else
            {
                GrabbedImagesFailCount++;
            }
            if (ImageGrabbedEvent != null) ImageGrabbedEvent(new object(), e);
        }

        public bool SetWidth(long width)
        {
            try
            {
                Camera.Open();
                Camera.Parameters[PLCamera.Width].TrySetValue(width, IntegerValueCorrection.Nearest);
            }
            catch
            {
                Camera.Close();
                return false;
            }
            MaxHeight = Camera.Parameters[PLCamera.Height].GetMaximum();
            Camera.Close();
            return true;
        }

        public bool SetModel(string camera)
        {
            try
            {
                if (!Array.Exists(supportedCameras, element => camera.StartsWith(element))) return false;
                string serial = camera.Substring(camera.IndexOf('(') + 1);
                serial = serial.Substring(0, serial.Length - 1);
                Camera = new Camera(serial);
                Camera.ConnectionLost += onCameraDisconnected;
                Camera.StreamGrabber.ImageGrabbed += onImageGrabbed;
            }
            catch { return false; }
            return Init();
        }

        private bool Init()
        {
            if (Camera == null) return false;
            try
            {
                Camera.Open();
                OrigWidth = Camera.Parameters[PLCamera.Width].GetValue();
                OrigHeight = Camera.Parameters[PLCamera.Height].GetValue();
                OrigXOffset = Camera.Parameters[PLCamera.OffsetX].GetValue();
                // config necessary to get max vals
                Camera.Parameters[PLCamera.Height].TrySetValue(256, IntegerValueCorrection.Nearest);
                Camera.Parameters[PLCamera.OffsetX].TrySetValue(0, IntegerValueCorrection.Nearest);
                MaxWidth = Camera.Parameters[PLCamera.Width].GetMaximum();
                MaxHeight = Camera.Parameters[PLCamera.Height].GetMaximum();
                // restore original settings
                Camera.Parameters[PLCamera.Height].TrySetValue(OrigHeight, IntegerValueCorrection.Nearest);
                Camera.Parameters[PLCamera.OffsetX].TrySetValue(OrigXOffset, IntegerValueCorrection.Nearest);
                // increase packet size
                Camera.Parameters[PLCamera.GevSCPSPacketSize].TrySetValue(MaxWidth);
            }
            catch
            {
                Camera.Close();
                return false;
            }
            Camera.Close();
            return true;
        }

        public bool UploadConfig(long frameWidth, long frameHeight, long XOffset)
        {
            if (Camera == null) return false;
            try
            {
                Camera.Open();
                Camera.Parameters[PLCamera.Width].TrySetValue(frameWidth, IntegerValueCorrection.Nearest);
                Camera.Parameters[PLCamera.Height].TrySetValue(frameHeight, IntegerValueCorrection.Nearest);
                Camera.Parameters[PLCamera.OffsetX].TrySetValue(XOffset, IntegerValueCorrection.Nearest);
                //config for hardware pulse exposure mode
                Camera.Parameters[PLCamera.TriggerSelector].TrySetValue(PLCamera.TriggerSelector.LineStart);
                Camera.Parameters[PLCamera.TriggerMode].TrySetValue(PLCamera.TriggerMode.On);
                Camera.Parameters[PLCamera.ExposureMode].TrySetValue(PLCamera.ExposureMode.TriggerWidth);
                Camera.Parameters[PLCamera.LineSelector].TrySetValue(PLCamera.LineSelector.Out1);
                Camera.Parameters[PLCamera.LineSource].TrySetValue(PLCamera.LineSource.LineTriggerWait);
            }
            catch
            {
                Camera.Close();
                return false;
            }
            Camera.Close();
            return true;
        }

        public bool StartGrabbing()
        {
            if (Camera == null) return false;
            try { Camera.Open(); }
            catch { return false; }
            Camera.StreamGrabber.Start(GrabStrategy.LatestImages, GrabLoop.ProvidedByStreamGrabber);
            GrabbedImagesOkCount = 0;
            GrabbedImagesFailCount = 0;
            FrameProcessor.CloseWindows();
            return true;
        }

        public bool StopGrabbing()
        {
            if (Camera == null) return false;
            Camera.StreamGrabber.Stop();
            Camera.Close();
            return true;
        }
    }
}
