#ifndef RAGE_CHASE_CAMERA_H
#define RAGE_CHASE_CAMERA_H

/* Return the host-only yaw offset, in the game's 12-bit angle units, used to
 * point the chase camera into a turn. */
int ChaseCameraYawOffset(int steeringAngle);

#endif
