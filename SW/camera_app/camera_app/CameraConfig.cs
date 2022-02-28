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
		static readonly string[] supportedCameras = {
		"Basler raL6144-16gm"
		};

		static string cameraSerialNumber;

		public static bool setCameraModel(string camera)
		{
			if (!Array.Exists(supportedCameras, element => camera.StartsWith(element))) return false;
			cameraSerialNumber = camera.Substring(camera.IndexOf('(') + 1);
			cameraSerialNumber = cameraSerialNumber.Substring(0,cameraSerialNumber.Length-1);
			updateParams();
			return true;
		}

		private static bool updateParams()
		{
			using (Camera camera = new Camera(cameraSerialNumber))
			{
				bool success = true;
				camera.Open();
				success &= camera.Parameters[PLCamera.Width].TrySetValue(1000, IntegerValueCorrection.Nearest);
				success &= camera.Parameters[PLCamera.Height].TrySetValue(1000, IntegerValueCorrection.Nearest);
				success &= camera.Parameters[PLCamera.OffsetX].TrySetValue(1000, IntegerValueCorrection.Nearest);
				success &= camera.Parameters[PLCamera.TriggerSelector].TrySetValue(PLCamera.TriggerSelector.LineStart);
				success &= camera.Parameters[PLCamera.TriggerMode].TrySetValue(PLCamera.TriggerMode.On);
				success &= camera.Parameters[PLCamera.ExposureMode].TrySetValue(PLCamera.ExposureMode.TriggerWidth);
				success &= camera.Parameters[PLCamera.LineSelector].TrySetValue(PLCamera.LineSelector.Out1);
				success &= camera.Parameters[PLCamera.LineSource].TrySetValue(PLCamera.LineSource.LineTriggerWait);
				success &= camera.Parameters[PLCamera.GevSCPSPacketSize].TrySetValue(camera.Parameters[PLCamera.Width].GetMaximum());
				camera.Close();
				return success;
			}
		}
	}
}
