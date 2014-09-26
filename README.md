Calibrator
==============

Calibrate your projector-camera system on a single click.

# Usage

usage description wiki

# Build

The software requires Qt and OpenVV libraries. It should run on any system that satisfies those requirements. It has been developed and tested using Qt 4.8.4 and OpencV 2.4.3 in Microsoft Windows 7. It has also been tried in Mac OS X and while it builds and runs fine no extensive testing has been done on this platform.

Feel free to modify the paths on top of 'build/scan3d-capture.pro'.


## Windows

1) Install Qt (from binaries)

Download http://releases.qt-project.org/qt4/source/qt-win-opensource-4.8.4-vs2010.exe

Install to C:\Qt\4.8.4
Add C:\Qt\4.8.4\bin to PATH

2) Install OpenCV 2.4.3 (from binaries)

Download http://sourceforge.net/projects/opencvlibrary/files/opencv-win/2.4.3/OpenCV-2.4.3.exe
Extract/Install to C:\opencv\opencv-2.4.3
Add C:\opencv\opencv-2.4.3\opencv\build\x86\vc10\bin to PATH

3) Build scanning software

cd scan3d-capture/build
qmake
nmake release

## Mac
1. Install Qt and OpenCV from homebrew

```
$ brew install qt opencv
```

2. Edit `scan3d-capture.pro` file, use `QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6` for Lion and Mountain Lion, use `QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.9` for Mavericks and Yosemite.

```
$ nano scan3d-capture/build/scan3d-capture.pro
```

3. Build scanning software

```
$ cd scan3d-capture/build
$ qmake
$ make
```

## Linux Debian

1) Install Qt and libraries (from packages)

sudo apt-get install libqt4-dev libv4l-dev

2) Install OpenCV 2.4.3 (from source)

mkdir ~/opencv && cd ~/opencv
wget -c -O OpenCV-2.4.3.tar.bz2 http://sourceforge.net/projects/opencvlibrary/files/opencv-unix/2.4.3/OpenCV-2.4.3.tar.bz2
tar xjf OpenCV-2.4.3.tar.bz2
mkdir build && cd build
cmake ../OpenCV-2.4.3
make && sudo make install

3) Build scanning software

cd scan3d-capture/build
qmake
make


# Features

* Generates both projector and camera intrinsic and extrinsic matrices
* Spacial scan works with either 720p or 1080p projectors
* Supports Windows, Mac OS X, and Linux

# License

BSD 3-Clause License

# Credits

This software is modified from the [3D scanning software](http://mesh.brown.edu/calibration/software.html) originally written by Daniel Moreno and Gabriel Taubin from Brown University, based on calibration method from the following paper.

Moreno, Daniel, and Gabriel Taubin. [Simple, accurate, and robust projector-camera calibration](http://dx.doi.org/10.1109/3DIMPVT.2012.77) *3D Imaging, Modeling, Processing, 2012 Second International Conference on Visualization and Transmission (3DIMPVT).* IEEE, 2012.
