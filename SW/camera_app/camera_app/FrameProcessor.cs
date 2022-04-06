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
        static public bool ConstructHdr = false;
        static public bool ConstructRgb = false;

        static private int currentImageWindow = 0;

        static public void ProcessGrabResult(IGrabResult grabResult, uint numOfPulses)
        {
            currentImageWindow = 0;

            //display original
            ImageWindow.DisplayImage(currentImageWindow, grabResult);
            currentImageWindow++;

            if (numOfPulses <= 1) return;

            //separate and display images
            byte[] grab = grabResult.PixelData as byte[];
            List<byte[]> separatedImages = new List<byte[]>((int)numOfPulses);
            int imageWidth = grabResult.Width;
            int partialImageHeight = grabResult.Height / (int)numOfPulses;

            for (int iIndex = 0; iIndex < numOfPulses; iIndex++)
            {
                byte[] partialImage = new byte[imageWidth * partialImageHeight];
                for (long line = 0; line < partialImageHeight; line++)
                {
                    Array.Copy(grab, imageWidth * (line * numOfPulses + iIndex), partialImage, line * imageWidth, imageWidth);
                }
                separatedImages.Add(partialImage);
                ImageWindow.DisplayImage<byte>(currentImageWindow, partialImage, grabResult.PixelTypeValue, imageWidth, partialImageHeight, grabResult.PaddingX, grabResult.Orientation);
                currentImageWindow++;
            }

            //split separated images into channels based on pulse output
            List<List<byte[]>> channels = new List<List<byte[]>>();
            for (int channel = 0; channel < 5; channel++)
            {
                List<byte[]> currentChannel = new List<byte[]>();
                //find all images that belong to current channel number
                for (int iIndex = 0; iIndex < numOfPulses; iIndex++)
                {
                    if (ControllerConfig.PulseOutput[iIndex] == channel) currentChannel.Add(separatedImages[iIndex]);
                }
                channels.Add(currentChannel);
            }

            //process HDR on each channel separately
            List<byte[]> hdrChannels = new List<byte[]>();
            if (ConstructHdr)
            {
                foreach (List<byte[]> channel in channels)
                {
                    if (channel.Count > 1) hdrChannels.Add(HdrProcessor.ToneMap(HdrProcessor.ConstructHdr(channel), 8));
                    else if (channel.Count == 1) hdrChannels.Add(channel[0]);
                    else hdrChannels.Add(new byte[0]);
                }
            }

            //display appropriate output
            if (ConstructHdr)
            {
                if (ConstructRgb)   //colored HDR image
                {
                    try
                    {
                        ImageWindow.DisplayImage(currentImageWindow, makeRgb(hdrChannels[1], hdrChannels[2], hdrChannels[3]), PixelType.RGB8planar, imageWidth, partialImageHeight, grabResult.PaddingX, grabResult.Orientation);
                        currentImageWindow++;
                    }
                    catch { }
                }
                else    //HDR channels separately, no color mixing
                {
                    for (int ch = 0; ch < channels.Count; ch++)
                    {
                        if (channels[ch].Count > 1)  //display HDR images that were constructed from more than 1 image
                        {
                            ImageWindow.DisplayImage(currentImageWindow, hdrChannels[ch], grabResult.PixelTypeValue, imageWidth, partialImageHeight, grabResult.PaddingX, grabResult.Orientation);
                            currentImageWindow++;
                        }
                    }
                }
            }
            else if (ConstructRgb)  //mix RGB channels separately, no HDR processing
            {

            }
        }

        static public void CloseWindows()
        {
            for (int i = 0; i < currentImageWindow; i++) ImageWindow.Close(i);
        }

        static private byte[] makeRgb(byte[] R, byte[] G, byte[] B)
        {
            byte[] result = new byte[new[] { R.Length, G.Length, B.Length }.Max() * 3];
            Array.Copy(R, 0, result, 0, R.Length);
            Array.Copy(G, 0, result, result.Length / 3, G.Length);
            Array.Copy(B, 0, result, result.Length / 3 * 2, B.Length);
            return result;
        }
    }
}
