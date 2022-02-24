#include "MyForm.h"
#include <pylon/PylonIncludes.h>
#include <pylon/BaslerUniversalInstantCamera.h>
#include <stdint.h>

using namespace Pylon;
using namespace Basler_UniversalCameraParams;
//using namespace std;

using namespace System;
using namespace System::Windows::Forms;


[STAThread]
void main(array<String^>^ args)
{
	Application::EnableVisualStyles();
	Application::SetCompatibleTextRenderingDefault(false);

	cameraapp::MyForm form;

    //camera parameter readout into controls

    PylonInitialize();
    try
    {
        PylonAutoInitTerm autoInitTerm;
        CTlFactory& TlFactory = CTlFactory::GetInstance();
        DeviceInfoList_t devices;
        TlFactory.EnumerateDevices(devices);
        if (!devices.empty()) {
            DeviceInfoList_t::const_iterator it;
            for (it = devices.begin(); it != devices.end(); ++it) {
                form.deviceSelect->Items->Add(gcnew String(it->GetFriendlyName()));
            }
        }

        CBaslerUniversalInstantCamera camera(CTlFactory::GetInstance().CreateFirstDevice());
        camera.Open();

        uint16_t width = 2048;
        bool success = true;
        
        success &= camera.Width.TrySetValue(width, IntegerValueCorrection_Nearest);
        success &= camera.Height.TrySetValue(width, IntegerValueCorrection_Nearest);
        success &= camera.OffsetX.TrySetValue((camera.Width.GetMax()-width)/2);
        success &= camera.TriggerSelector.TrySetValue(TriggerSelector_LineStart);
        success &= camera.TriggerMode.TrySetValue(TriggerMode_On);
        success &= camera.ExposureMode.TrySetValue(ExposureMode_TriggerWidth);
        success &= camera.AcquisitionStatusSelector.TrySetValue(AcquisitionStatusSelector_LineTriggerWait);
        success &= camera.LineSelector.TrySetValue(LineSelector_Out1);
        success &= camera.LineSource.TrySetValue(LineSource_LineTriggerWait);
        success &= camera.GevSCPSPacketSize.TrySetValue(camera.Width.GetMax());
        
        try {
            // The parameter MaxNumBuffer can be used to control the count of buffers
            // allocated for grabbing. The default value of this parameter is 10.
            camera.MaxNumBuffer = 5;
            // Start the grabbing of c_countOfImagesToGrab images.
            // The camera device is parameterized with a default configuration which
            // sets up free-running continuous acquisition.
            camera.StartGrabbing(10);
            // This smart pointer will receive the grab result data.
            CGrabResultPtr ptrGrabResult;
            // Camera.StopGrabbing() is called automatically by the RetrieveResult() method
            // when c_countOfImagesToGrab images have been retrieved.
            while (camera.IsGrabbing())
            {
                // Wait for an image and then retrieve it. A timeout of 5000 ms is used.
                camera.RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);

                // Image grabbed successfully?
                if (ptrGrabResult->GrabSucceeded())
                {
                    // Access the image data.
                    const uint8_t* pImageBuffer = (uint8_t*)ptrGrabResult->GetBuffer();
                    form.outLabel->Text = gcnew String(form.outLabel->Text + "Gray value of first pixel: " + (uint32_t)pImageBuffer[0] + "\n");
                }
                else
                {
                    std::cout << "Error: " << std::hex << ptrGrabResult->GetErrorCode() << std::dec << " " << ptrGrabResult->GetErrorDescription() << std::endl;
                    form.outLabel->Text = gcnew String(form.outLabel->Text + ptrGrabResult->GetErrorCode() + " error\n");
                }
            }
        }
        catch (const GenericException& e)
        {
            // Error handling.
            std::cerr << "An exception occurred." << std::endl
                << e.GetDescription() << std::endl;
        }

        // Close the camera.
        camera.Close();
    }
    catch (const GenericException& e)
    {
        /*
        cerr << "An exception occurred." << endl
            << e.GetDescription() << endl;
        exitCode = 1;*/
    }

	Application::Run(% form);
}