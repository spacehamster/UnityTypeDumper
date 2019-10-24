using System;
using System.IO;
using System.Runtime.InteropServices;

namespace Loader
{
    public static class Loader
    {
        [DllImport("kernel32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        public static extern IntPtr LoadLibrary(string libname);
        [DllImport("kernel32.dll", CharSet = CharSet.Auto)]
        public static extern bool FreeLibrary(IntPtr hModule);
        [DllImport("UnityStructGen.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "Dump")]
        public static extern void Dump(string unityVersion, bool isDebug);
        public static void InitLog()
        {
            File.WriteAllText("LoaderLog.txt", "");
        }
        public static void Log(string message)
        {
            File.AppendAllText("LoaderLog.txt", message + "\n");
        }
        public static void Main(string[] args)
        {
            InitLog();
            Log("Hello world!");
            AppDomain.CurrentDomain.AssemblyLoad += OnLoad;
        }
        static void OnLoad(object sender, AssemblyLoadEventArgs args)
        {
            try
            {
                Log(string.Format("AssemblyLoaded {0}", args.LoadedAssembly.FullName));
                if (args.LoadedAssembly.FullName.StartsWith("UnityEngine"))
                {
                    AppDomain.CurrentDomain.AssemblyLoad -= OnLoad;
                    var application = args.LoadedAssembly.GetType("UnityEngine.Application");
                    var debug = args.LoadedAssembly.GetType("UnityEngine.Debug");
                    string version = (string)application.GetProperty("unityVersion").GetValue(null, new object[] { });
                    bool isDebug = (bool)debug.GetProperty("isDebugBuild").GetValue(null, new object[] { });
                    Log(string.Format("Found version {0}, {1}", version, isDebug ? "Debug" : "Release"));
                    Log("Calling UnityStructGen");
                    Dump(version, isDebug);
                }
            }
            catch (Exception ex)
            {
                Log(ex.ToString());
            }
        }
    }
}
