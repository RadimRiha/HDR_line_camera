using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO.Ports;

namespace camera_app
{
    static class ControllerConfig
    {
        static public readonly uint MaxNumPulses = 10;
        static public readonly long MaxPulseOutput = 8;
        static public readonly long MaxTriggerSource = 4;
        static public readonly long MinTimedTriggerPeriod = 4;
        static public readonly long MaxTimedTriggerPeriod = 0xffff;
        static public readonly long MaxTriggerPolarity = 3;
        

        static private SerialPort serialPort = new SerialPort();
        static public int[] PulseOutput = new int[MaxNumPulses];
        static public long[] PulsePeriod = new long[MaxNumPulses];

        static public bool Search()
        {
            if (serialPort.IsOpen) serialPort.Close();
            foreach (string portName in SerialPort.GetPortNames())
            {
                serialPort = new SerialPort(portName, 9600);
                serialPort.ReadTimeout = 500;
                serialPort.WriteTimeout = 500;
                try
                {
                    serialPort.Open();
                    serialPort.WriteLine("GID");
                    if (serialPort.ReadLine().StartsWith("CONTROLLER"))
                    {
                        loadPulseConfig();
                        return true;
                    }
                }
                catch { }
            }
            return false;
        }

        static private void loadPulseConfig()
        {

        }

        static public string GetResponse(string msg) {
            if (serialPort.IsOpen)
            {
                try
                {
                    serialPort.WriteLine(msg);
                    return serialPort.ReadLine();
                }
                catch { }
            }
            return "";
        }

        static public bool Config()
        {
            return false;
        }
    }
}
