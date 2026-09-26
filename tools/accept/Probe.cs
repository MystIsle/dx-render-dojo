using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO.MemoryMappedFiles;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

public static class Probe
{
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    static extern IntPtr CreateEventW(IntPtr attr, bool manual, bool initial, string name);
    [DllImport("kernel32.dll")] static extern bool SetEvent(IntPtr h);
    [DllImport("kernel32.dll")] static extern uint WaitForSingleObject(IntPtr h, uint ms);
    [DllImport("kernel32.dll")] static extern uint GetACP();
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern IntPtr FindWindowW(string cls, string title);
    [DllImport("user32.dll")] public static extern IntPtr SendMessageW(IntPtr h, uint msg, IntPtr w, IntPtr l);
    [DllImport("user32.dll")] public static extern bool PostMessageW(IntPtr h, uint msg, IntPtr w, IntPtr l);
    [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr after, int x, int y, int cx, int cy, uint flags);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool IsWindowEnabled(IntPtr h);
    [DllImport("user32.dll")] public static extern IntPtr GetDlgItem(IntPtr h, int id);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern int GetWindowTextW(IntPtr h, StringBuilder s, int n);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern int GetClassNameW(IntPtr h, StringBuilder s, int n);
    delegate bool EnumProc(IntPtr h, IntPtr l);
    [DllImport("user32.dll")] static extern bool EnumWindows(EnumProc f, IntPtr l);
    [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; public override string ToString() { return string.Format("{0}x{1} @({2},{3})", Right - Left, Bottom - Top, Left, Top); } }

    static readonly List<string> Lines = new List<string>();
    static volatile bool Running;
    static int TargetPid;

    public static void StartListener()
    {
        IntPtr ready = CreateEventW(IntPtr.Zero, false, false, "DBWIN_BUFFER_READY");
        IntPtr data = CreateEventW(IntPtr.Zero, false, false, "DBWIN_DATA_READY");
        var map = MemoryMappedFile.CreateOrOpen("DBWIN_BUFFER", 4096);
        var view = map.CreateViewAccessor();
        var enc = Encoding.GetEncoding((int)GetACP());
        Running = true;
        var t = new Thread(() =>
        {
            var buf = new byte[4092];
            while (Running)
            {
                SetEvent(ready);
                if (WaitForSingleObject(data, 200) != 0) continue;
                int pid = view.ReadInt32(0);
                view.ReadArray(4, buf, 0, buf.Length);
                int n = Array.IndexOf(buf, (byte)0); if (n < 0) n = buf.Length;
                if (pid == TargetPid) lock (Lines) Lines.Add(enc.GetString(buf, 0, n).TrimEnd('\n', '\r'));
            }
        });
        t.IsBackground = true; t.Start();
    }

    public static void SetTarget(int pid) { TargetPid = pid; }
    public static void StopListener() { Running = false; }
    public static string[] TakeLines() { lock (Lines) { var a = Lines.ToArray(); Lines.Clear(); return a; } }

    public static IntPtr FindProcessWindow(int pid, string cls)
    {
        IntPtr found = IntPtr.Zero;
        EnumWindows((h, l) =>
        {
            uint p; GetWindowThreadProcessId(h, out p);
            if (p != pid) return true;
            var sb = new StringBuilder(256); GetClassNameW(h, sb, 256);
            if (sb.ToString() == cls) { found = h; return false; }
            return true;
        }, IntPtr.Zero);
        return found;
    }

    public static string WindowText(IntPtr h) { var sb = new StringBuilder(2048); GetWindowTextW(h, sb, 2048); return sb.ToString(); }
}
