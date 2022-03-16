using System;
using System.Collections.Generic;
using System.Configuration;
using System.Data;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;

namespace camera_app
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : Application
    {
        static public event EventHandler ApplicationExitEvent;
        private void Application_Exit(object sender, ExitEventArgs e)
        {
            if (ApplicationExitEvent != null) ApplicationExitEvent(new object(), e);
        }
    }
}
