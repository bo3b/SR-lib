#include "sr/types.h"

#include "sr/core_c.h"
#include "sr/handtrackers_c.h"
 
#include <conio.h>
#include <stdio.h>
 
void listenerFunction(SR_handPose handpose) {
    printf("%f\n", handpose.palm.x);
}

int main() {
    SRContext context = newSRContext();
    SR_handTracker handTracker = createHandTracker(context);
    SR_handPoseListener handPoseListener = createHandPoseListener(handTracker, &listenerFunction);
    initializeSRContext(context);
    getch();

    deleteHandPoseListener(handPoseListener);
    deleteSRContext(context);
}