using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO.Ports;
using System.Threading;

namespace camera_app
{
    static class ControllerConfig
    {
        static public readonly uint MaxNumPulses = 10;
        static public readonly uint MaxPulseOutput = 4;
        static public readonly uint MaxTriggerSource = 4;
        static public readonly uint MinTimedTriggerPeriod = 4;
        static public readonly uint MaxTimedTriggerPeriod = 0xffff;
        static public readonly uint MaxTriggerPolarity = 3;

        static private SerialPort serialPort = new SerialPort();
        static public uint[] PulseOutput = new uint[MaxNumPulses];
        static public uint[] PulsePeriod = new uint[MaxNumPulses];
        static public uint NumOfPulses = 0;

        static public bool Search()
        {
            if (serialPort.IsOpen) serialPort.Close();
            foreach (string portName in SerialPort.GetPortNames())
            {
                serialPort = new SerialPort(portName, 9600);
                serialPort.ReadTimeout = 1000;
                serialPort.WriteTimeout = 1000;
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
            uint i = 0;
            foreach (string puo in GetResponse("GPUO").Split(','))
            {
                try
                {
                    PulseOutput[i] = uint.Parse(puo);
                    if (PulseOutput[i] > MaxPulseOutput) PulseOutput[i] -= MaxPulseOutput;  //change L+T configurations into just L configurations
                }
                catch { break; }
                i++;
            }
            NumOfPulses = i;
            for (; i < MaxNumPulses; i++) { PulseOutput[i] = 0; }
            i = 0;
            foreach (string pup in GetResponse("GPUP").Split(','))
            {
                try { PulsePeriod[i] = uint.Parse(pup); }
                catch { break; }
                i++;
            }
            for (; i < MaxNumPulses; i++) { PulsePeriod[i] = 1000; }
        }

        static public string GetResponse(string msg)
        {
            if (serialPort.IsOpen)
            {
                try
                {
                    Thread.Sleep(200);
                    serialPort.WriteLine(msg);
                    return serialPort.ReadLine();
                }
                catch { }
            }
            return "";
        }

        static private bool trySet(string msg)
        {
            return GetResponse(msg).StartsWith("OK");
        }

        static public bool UploadConfig(uint triggerSource, uint trigerPeriod, uint triggerPolarity, uint numberOfPulses)
        {
            bool success = true;
            success &= trySet("SPUO" + String.Join(",", new ArraySegment<uint>(PulseOutput, 0, (int)numberOfPulses).ToArray()));
            success &= trySet("SPUP" + String.Join(",", new ArraySegment<uint>(PulsePeriod, 0, (int)numberOfPulses).ToArray()));
            success &= trySet("STRS" + triggerSource.ToString());
            success &= trySet("STTP" + trigerPeriod.ToString());
            success &= trySet("SHTP" + triggerPolarity.ToString());
            NumOfPulses = numberOfPulses;
            return success;
        }
    }
}
