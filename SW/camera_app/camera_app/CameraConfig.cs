using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Basler.Pylon;

namespace camera_app
{
	static class CameraConfig
    {
		static public readonly long MinExposure = 2;
		static public readonly long MaxExposure = 10000;

		static readonly string[] supportedCameras = {
		"Basler raL6144-16gm"
		};
		
		public static string CameraSerialNumber = "";
		public static long MaxWidth = 0;
		public static long MaxHeight = 0;
		static public long OrigWidth = 1;
		static public long OrigHeight = 1;
		static public long OrigXOffset = 0;

		public static bool SetModel(string camera)
		{
			if (!Array.Exists(supportedCameras, element => camera.StartsWith(element))) return false;
			CameraSerialNumber = camera.Substring(camera.IndexOf('(') + 1);
			CameraSerialNumber = CameraSerialNumber.Substring(0, CameraSerialNumber.Length-1);
			return Init();
		}

		private static bool Init()
        {
			using (Camera camera = new Camera(CameraSerialNumber))
			{
				try { camera.Open(); }
				catch { return false; }
				OrigWidth = camera.Parameters[PLCamera.Width].GetValue();
				OrigHeight = camera.Parameters[PLCamera.Height].GetValue();
				OrigXOffset = camera.Parameters[PLCamera.OffsetX].GetValue();
				// config necessary to get max vals
				camera.Parameters[PLCamera.Height].TrySetValue(256, IntegerValueCorrection.Nearest);
				camera.Parameters[PLCamera.OffsetX].TrySetValue(0, IntegerValueCorrection.Nearest);
				MaxWidth = camera.Parameters[PLCamera.Width].GetMaximum();
				MaxHeight = camera.Parameters[PLCamera.Height].GetMaximum();
				// restore original settings
				camera.Parameters[PLCamera.Height].TrySetValue(OrigHeight, IntegerValueCorrection.Nearest);
				camera.Parameters[PLCamera.OffsetX].TrySetValue(OrigXOffset, IntegerValueCorrection.Nearest);
				// increase packet size
				camera.Parameters[PLCamera.GevSCPSPacketSize].TrySetValue(MaxWidth);
				camera.Close();
				return true;
			}
		}

		public static bool UploadConfig(long frameWidth, long frameHeight, long XOffset)
		{
			using (Camera camera = new Camera(CameraSerialNumber))
			{
				bool success = true;
				try { camera.Open(); }
				catch { return false; }
				success &= camera.Parameters[PLCamera.Width].TrySetValue(frameWidth, IntegerValueCorrection.Nearest);
				success &= camera.Parameters[PLCamera.Height].TrySetValue(frameHeight, IntegerValueCorrection.Nearest);
				success &= camera.Parameters[PLCamera.OffsetX].TrySetValue(XOffset, IntegerValueCorrection.Nearest);
				//config for hardware pulse exposure mode
				success &= camera.Parameters[PLCamera.TriggerSelector].TrySetValue(PLCamera.TriggerSelector.LineStart);
				success &= camera.Parameters[PLCamera.TriggerMode].TrySetValue(PLCamera.TriggerMode.On);
				success &= camera.Parameters[PLCamera.ExposureMode].TrySetValue(PLCamera.ExposureMode.TriggerWidth);
				success &= camera.Parameters[PLCamera.LineSelector].TrySetValue(PLCamera.LineSelector.Out1);
				success &= camera.Parameters[PLCamera.LineSource].TrySetValue(PLCamera.LineSource.LineTriggerWait);
				camera.Close();
				return success;
			}
		}
	}
}
