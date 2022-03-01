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
		public const long DEFAULT_HEIGHT = 256;

		static readonly string[] supportedCameras = {
		"Basler raL6144-16gm"
		};
		
		public static string CameraSerialNumber = "";
		public static long MaxWidth = 0;
		public static long MaxHeight = 0;

		public static bool SetModel(string camera)
		{
			if (!Array.Exists(supportedCameras, element => camera.StartsWith(element))) return false;
			CameraSerialNumber = camera.Substring(camera.IndexOf('(') + 1);
			CameraSerialNumber = CameraSerialNumber.Substring(0, CameraSerialNumber.Length-1);
			Init();
			return true;
		}

		private static void Init()
        {
			using (Camera camera = new Camera(CameraSerialNumber))
			{
				camera.Open();
				camera.Parameters[PLCamera.OffsetX].TrySetValue(0, IntegerValueCorrection.Nearest);
				camera.Parameters[PLCamera.Height].TrySetValue(DEFAULT_HEIGHT, IntegerValueCorrection.Nearest);
				MaxWidth = camera.Parameters[PLCamera.Width].GetMaximum();
				MaxHeight = camera.Parameters[PLCamera.Height].GetMaximum();
				camera.Parameters[PLCamera.Width].TrySetValue(MaxWidth, IntegerValueCorrection.Nearest);
				camera.Close();
			}
		}

		public static bool UpdateParams()
		{
			using (Camera camera = new Camera(CameraSerialNumber))
			{
				bool success = true;
				camera.Open();
				success &= camera.Parameters[PLCamera.GevSCPSPacketSize].TrySetValue(camera.Parameters[PLCamera.Width].GetMaximum());
				success &= camera.Parameters[PLCamera.Width].TrySetValue(1000, IntegerValueCorrection.Nearest);
				success &= camera.Parameters[PLCamera.Height].TrySetValue(1000, IntegerValueCorrection.Nearest);
				success &= camera.Parameters[PLCamera.OffsetX].TrySetValue(1000, IntegerValueCorrection.Nearest);
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
