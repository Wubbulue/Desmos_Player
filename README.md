# Desmos_Player
This is a project to play a youtube video in the desmos graphing calculator. It is inspired by junferno https://github.com/kevinjycui/DesmosBezierRenderer. However, his implementation runs only linux, and uses python. My implementation uses C++, and runs on windows. This repo also serves as an example for building the potrace library for windows, since there is only official support for building potrace on windows. 
# Build Instructions:
[First install open cv cpp and add it to your path.](https://docs.opencv.org/3.4/da/df6/tutorial_py_table_of_contents_setup.html) Make sure that Cmake can find it with the find package directory. After this, build your environment of choice with cmake and build the DesmosTracer project. 
# Usage Instructions:
Open main.cpp and set a path for an input video file in the VIDEO_FILE define. Set the video output file under the VIDEO_FILE_OUTPUT define. Then, run the DesmosTracer project. Then open a browser window, and navigate to [localhost:800](localhost:8000). Finally, open the console and type start(). This will render your video in desmos.
