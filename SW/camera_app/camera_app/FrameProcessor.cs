using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Basler.Pylon;

namespace camera_app
{
    static class FrameProcessor
    {
        static public void ProcessGrabResult(IGrabResult grabResult, uint numOfPulses)
        {
            byte[] grab = grabResult.PixelData as byte[];
            List<byte[]> separatedImages = new List<byte[]>((int)numOfPulses);
            int imageWidth = grabResult.Width;
            int partialImageHeight = grabResult.Height / (int)numOfPulses;

            ImageWindow.DisplayImage(0, grabResult);

            for (int iIndex = 0; iIndex < numOfPulses; iIndex++)
            {
                byte[] partialImage = new byte[imageWidth * partialImageHeight];
                for (long line = 0; line < partialImageHeight; line++)
                {
                    Array.Copy(grab, imageWidth * (line * numOfPulses + iIndex), partialImage, line * imageWidth, imageWidth);
                }
                separatedImages.Add(partialImage);
                ImageWindow.DisplayImage<byte>(iIndex + 1, partialImage, grabResult.PixelTypeValue, imageWidth, partialImageHeight, grabResult.PaddingX, grabResult.Orientation);
            }

            ImageWindow.DisplayImage((int)numOfPulses + 1, HdrProcessor.ConstructHdr(separatedImages), grabResult.PixelTypeValue, imageWidth, partialImageHeight, grabResult.PaddingX, grabResult.Orientation);
        }

        static public void CloseWindows()
        {
            for (int i = 0; i < 11; i++) ImageWindow.Close(i);
        }
    }
}
