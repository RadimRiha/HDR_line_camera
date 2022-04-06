using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace camera_app
{
    class Image
    {
        public byte[] Data = new byte[0];
        public uint ExpTime = 0;
        public Image(byte[] data, uint expTime)
        {
            Data = data;
            ExpTime = expTime;
        }
    }
}
