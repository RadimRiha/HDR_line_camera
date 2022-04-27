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
        static public bool DisplayRaw = false;
        static public bool DisplayPartial = false;
        static public bool ConstructHdr = false;
        static public bool ConstructRgb = false;

        static private int currentImageWindow = 0;

        static private int imageWidth = 0;
        static private int partialImageHeight = 0;
        static private int paddingX = 0;
        static private ImageOrientation orientation = 0;

        static private void displayImage(Image img, PixelType pxType)
        {
            try
            {
                ImageWindow.DisplayImage<byte>(currentImageWindow, img.Data, pxType, imageWidth, partialImageHeight, paddingX, orientation);
                currentImageWindow++;
            }
            catch { }
        }

        static public void ProcessGrabResult(IGrabResult grabResult, uint numOfPulses)
        {
            //init variables
            currentImageWindow = 0;
            byte[] grab = grabResult.PixelData as byte[];
            imageWidth = grabResult.Width;
            partialImageHeight = grabResult.Height / (int)numOfPulses;
            paddingX = grabResult.PaddingX;
            orientation = grabResult.Orientation;

            //display original
            if (DisplayRaw)
            {
                ImageWindow.DisplayImage(currentImageWindow, grabResult);
                currentImageWindow++;
            }

            if (numOfPulses <= 1) return;

            //separate and display images
            List<Image> separatedImages = new List<Image>((int)numOfPulses);
            for (int iIndex = 0; iIndex < numOfPulses; iIndex++)
            {
                Image partialImage = new Image(new byte[imageWidth * partialImageHeight], ControllerConfig.PulsePeriod[iIndex]);
                for (long line = 0; line < partialImageHeight; line++)
                {
                    Array.Copy(grab, imageWidth * (line * numOfPulses + iIndex), partialImage.Data, line * imageWidth, imageWidth);
                }
                separatedImages.Add(partialImage);
                if (DisplayPartial) displayImage(partialImage, grabResult.PixelTypeValue);
            }

            //split separated images into channels based on pulse output
            List<List<Image>> channels = new List<List<Image>>();
            for (int channel = 0; channel < 5; channel++)
            {
                List<Image> currentChannel = new List<Image>();
                //find all images that belong to current channel number
                for (int iIndex = 0; iIndex < numOfPulses; iIndex++)
                {
                    if (ControllerConfig.PulseOutput[iIndex] == channel) currentChannel.Add(separatedImages[iIndex]);
                }
                channels.Add(currentChannel);
            }

            //process HDR on each channel separately
            List<Image> hdrChannels = new List<Image>();
            if (ConstructHdr)
            {
                foreach (List<Image> channel in channels)
                {
                    if (channel.Count > 1) hdrChannels.Add(HdrProcessor.ToneMap(HdrProcessor.ConstructHdr(channel), 8));
                    else if (channel.Count == 1) hdrChannels.Add(channel[0]);
                    else hdrChannels.Add(new Image(new byte[0], 0));
                }
            }

            //display appropriate output
            if (ConstructHdr)
            {
                for (int ch = 0; ch < channels.Count; ch++)
                {
                    if (channels[ch].Count > 1)  //display HDR images that were constructed from more than 1 image
                    {
                        displayImage(hdrChannels[ch], PixelType.Mono8);
                    }
                }
                if (ConstructRgb)   //colored HDR image
                {
                    displayImage(makeRgb(hdrChannels[1], hdrChannels[2], hdrChannels[3]), PixelType.RGB8planar);
                }
            }
            else if (ConstructRgb)
            {
                try { displayImage(makeRgb(channels[1][0], channels[2][0], channels[3][0]), PixelType.RGB8planar); }
                catch { return; }
            }
        }

        static public void CloseWindows()
        {
            for (int i = 0; i < currentImageWindow; i++) ImageWindow.Close(i);
        }

        static private Image makeRgb(Image R, Image G, Image B)
        {
            Image result = new Image(new byte[new[] { R.Data.Length, G.Data.Length, B.Data.Length }.Max() * 3], 0);
            Array.Copy(R.Data, 0, result.Data, 0, R.Data.Length);
            Array.Copy(G.Data, 0, result.Data, result.Data.Length / 3, G.Data.Length);
            Array.Copy(B.Data, 0, result.Data, result.Data.Length / 3 * 2, B.Data.Length);
            return result;
        }
    }
}
