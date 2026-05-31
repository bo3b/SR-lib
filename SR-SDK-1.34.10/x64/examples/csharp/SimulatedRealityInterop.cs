using System;
using System.Runtime.InteropServices;

namespace SimulatedReality
{
    [StructLayout(LayoutKind.Sequential)]
    public struct SR_handPose
    {
        public ulong frameId;
        public ulong time;
        public ulong handId;
        public ulong side;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3*21)]
        public double[] joints;
    }

    public static class SR
    {
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void acceptHandPoseCallback(SR_handPose handPose);

#if WIN64
        [DllImport("SimulatedRealityCore.dll")]
        public extern static IntPtr newSRContext();
        [DllImport("SimulatedRealityCore.dll")]
        public extern static void initializeSRContext(IntPtr context);
        [DllImport("SimulatedRealityCore.dll")]
        public extern static void deleteSRContext(IntPtr context);
        [DllImport("SimulatedRealityHandTrackers.dll")]
        public extern static IntPtr createHandTracker(IntPtr context);
        [DllImport("SimulatedRealityHandTrackers.dll")]
        public extern static IntPtr createHandPoseListener(IntPtr handTracker, acceptHandPoseCallback callback);
        [DllImport("SimulatedRealityHandTrackers.dll")]
        public extern static void deleteHandPoseListener(IntPtr handPoseListener);
#else
        [DllImport("SimulatedRealityCore32.dll")]
        public extern static IntPtr newSRContext();
        [DllImport("SimulatedRealityCore32.dll")]
        public extern static void initializeSRContext(IntPtr context);
        [DllImport("SimulatedRealityCore32.dll")]
        public extern static void deleteSRContext(IntPtr context);
        [DllImport("SimulatedRealityHandTrackers32.dll")]
        public extern static IntPtr createHandTracker(IntPtr context);
        [DllImport("SimulatedRealityHandTrackers32.dll")]
        public extern static IntPtr createHandPoseListener(IntPtr handTracker, acceptHandPoseCallback callback);
        [DllImport("SimulatedRealityHandTrackers32.dll")]
        public extern static void deleteHandPoseListener(IntPtr handPoseListener);
#endif
    }
}
