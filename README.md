Calibrator
==============

![Calibrator logo](https://github.com/hezhao/Calibrator/raw/master/resources/icon-about.png)

##### Calibrate your projector-camera system on a single click.

* Generates both projector and camera intrinsic and extrinsic matrices
* Spacial scan supports 720p and 1080p projectors
* Spacial scan supports Canon DSLR
* Live view with Canon DSLR
* Saves spacial scan image
* Supports Windows and OS X

### Usage

##### Parameters

**Corners**: checkerboard pattern size (e.g. [this pattern](http://docs.opencv.org/_downloads/pattern.png) has corners of 9x6)

**Size**: physical dimension of checkerboard grid or circle spacing, usually in millimeter

**Threshold**: projector view binary image threshold

**b**:  percentage of light emitted by a turned-off projector pixel

**m**: 


### Dependencies

- [Qt](http://qt-project.org/) (4.8.6)
- [OpenCV](http://opencv.org/) (2.4.11)
- [EDSDK](http://www.usa.canon.com/cusa/consumer/standard_display/sdk_homepage) (2.15)


### Build

#### OS X
1. Install Qt and OpenCV from homebrew.

	```$ brew install qt opencv```
	
2. Move the `EDSDK` folder to `lib/EDSDK`, replace `lib/EDSDK/Framekwork/EDSDK.framework` to be the 64 bit version.

4. Build with `make` or [Qt Creator](https://qt-project.org/downloads#qt-creator) IDE.

	```
	$ cd calibrator/build
	$ qmake
	$ make
	```

#### Windows

1. Install [Qt libraries 4.8.6 for Visual Studio 2010](http://download.qt-project.org/official_releases/qt/4.8/4.8.6/qt-opensource-windows-x86-vs2010-4.8.6.exe) and latest OpenCV binary. Remember to add `C:\Qt\4.8.6\bin` and `C:\opencv\build\x86\vc10\bin` to environment variable PATH.

2. Build with `nmake` or [Qt Creator](https://qt-project.org/downloads#qt-creator) IDE.

	```
	$ cd calibrator/build
	$ qmake
	$ nmake release
	```

### License

[BSD 3-Clause License](LICENSE)

### Credits

This software is modified from the [scan3d-capture](http://mesh.brown.edu/calibration/software.html) originally written by Daniel Moreno and Gabriel Taubin from Brown University, based on calibration method described in the following paper and   patent.

Daniel Moreno and Gabriel Taubin. [Simple, Accurate, and Robust Projector-Camera Calibration](http://dx.doi.org/10.1109/3DIMPVT.2012.77). *3D Imaging, Modeling, Processing, 2012 Second International Conference on Visualization and Transmission (3DIMPVT).* IEEE, 2012.

Gabriel Taubin and Daniel Moreno. Method for generating an array of 3-d points. [U.S. Patent 20140078260](http://www.google.com/patents/US20140078260), March 20, 2014.
