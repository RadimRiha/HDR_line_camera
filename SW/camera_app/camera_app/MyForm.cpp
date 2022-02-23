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

        camera.Width.TrySetValue(width, IntegerValueCorrection_Nearest);
        camera.Height.TrySetValue(width, IntegerValueCorrection_Nearest);
        camera.OffsetX.TrySetValue((camera.Width.GetMax()-width)/2);

        camera.TriggerSelector.TrySetValue(TriggerSelector_LineStart);
        camera.TriggerMode.TrySetValue(TriggerMode_On);

        // Close the camera.
        camera.Close();
    }
    catch (const GenericException& e)
    {
        /*
        // Error handling.
        cerr << "An exception occurred." << endl
            << e.GetDescription() << endl;
        exitCode = 1;*/
    }

	Application::Run(% form);
}