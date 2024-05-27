# Restimulator
Control restim software with external sources

Currently you can control restim by using pose tracking via a webcam (or video via a virtual webcam)

## Notes
Use --help on the command line to see all available arguments
If you have a GPU that supports DirectX 12, you can significantly increase performance by using directml with --directml on the command line
There are currently 2 pose tracking models that can be used.  The small one runs faster but may not be as accurate.  You can choose which to use on the command line with --posemodel
If you use a webcam, make sure to have good lighting with most of your body in view in the center of the camera
You can use a video as input by using a virtual webcam software such as the one included in OBS.  Note that any scene changes in the video will cause the pose tracking to jump, so videos with fixed cameras and no scene changes are ideal.

## Compiling
A compiler that supports C++17 is required.  OpenCV and nana gui libraries are required.
