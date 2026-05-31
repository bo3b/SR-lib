using System;
using SimulatedReality;

namespace example_cpp
{
    class Program
    {
        static void Main(string[] args)
        {
            SR.acceptHandPoseCallback acceptHandPose = (SR_handPose handPose) =>
            {
                Console.WriteLine(handPose.joints[0]);
            };

            var context = SR.newSRContext();
            var handTracker = SR.createHandTracker(context);
            var handPoseListener = SR.createHandPoseListener(handTracker, acceptHandPose);
            SR.initializeSRContext(context);

            Console.ReadKey();

            SR.deleteHandPoseListener(handPoseListener);
            SR.deleteSRContext(context);
        }
    }
}
