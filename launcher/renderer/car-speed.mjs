// Same conversion as TachometerSpeedValue in draw_tachometer.c. Two decimal
// places preserve every signed 16-bit threshold when converted back to raw.
export const displayedShiftSpeed=raw=>Number((raw*160/1168).toFixed(2));
export const storedShiftSpeed=value=>Math.round(value*1168/160);
