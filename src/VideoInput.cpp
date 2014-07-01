/*
Copyright (c) 2012, Daniel Moreno and Gabriel Taubin
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Brown University nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL DANIEL MORENO AND GABRIEL TAUBIN BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "VideoInput.hpp"

#ifdef _MSC_VER
#   include <Dshow.h>
#endif

#ifdef Q_OS_MAC
#   include "VideoInput_QTkit.hpp"
#endif

#ifdef Q_OS_LINUX
#   include <unistd.h>
#   include <fcntl.h>
#   include <linux/videodev2.h>
#   include <sys/ioctl.h>
#   include <errno.h>
#   define V4L2_MAX_DEVICE_DRIVER_NAME 80
#   define V4L2_MAX_CAMERAS 8
#endif

#include <QApplication>
#include <QMetaType>
#include <QTime>

#include <stdio.h>
#include <iostream>

VideoInput::VideoInput(QObject  * parent): 
    QThread(parent),
    _camera_index(-1),
    _video_capture(NULL),
    _init(false),
    _stop(false)
{
    qRegisterMetaType<cv::Mat>("cv::Mat");
}

VideoInput::~VideoInput()
{
    stop_camera();
}

void VideoInput::run()
{
    _init = false;
    _stop = false;

    bool success = start_camera();

    _init = true;
    
    if (!success)
    {
        return;
    }

    int error = 0;
    int max_error = 10;
    int warmup = 10000;
    QTime timer;
    timer.start();
    while(_video_capture && !_stop && error<max_error)
    {
        IplImage * frame = cvQueryFrame(_video_capture);
        if (frame)
        {   //ok
            error = 0;
            emit new_image(cv::Mat(frame));
        }
        else
        {   //error
            if (timer.elapsed()>warmup) {error++;}
        }
    }

    //clean up
    stop_camera();
    QApplication::processEvents();
}

bool VideoInput::start_camera(void)
{
    if (_video_capture)
    {
        return false;
    }

    int index = _camera_index;
    if (index<0)
    {
        return false;
    }

#ifdef _MSC_VER
    int CLASS = CV_CAP_DSHOW;
#endif
#ifdef Q_OS_MAC
    int CLASS = CV_CAP_QT;
#endif
#ifdef Q_OS_LINUX
    int CLASS = CV_CAP_V4L2;
#endif

    _video_capture = cvCaptureFromCAM(CLASS + index);
    if(!_video_capture)
    {
        std::cerr << "camera open failed, index=" << index << std::endl;
        return false;
    }

    //set camera parameters (e.g. frame size)
    bool silent = true;
#ifdef _MSC_VER
    configure_dshow(index,silent);
#endif
#ifdef Q_OS_MAC
    configure_quicktime(index,silent);
#endif
#ifdef Q_OS_LINUX
    configure_v4l2(index,silent);
#endif

    return true;
}

void VideoInput::stop_camera(void)
{
    if (_video_capture)
    {
#ifndef Q_OS_MAC //HACK: do not close on mac because it hangs the application
        cvReleaseCapture(&_video_capture);
#endif
        _video_capture = NULL;
    }
}

void VideoInput::waitForStart(void)
{
    while (isRunning() && !_init)
    {
        QApplication::processEvents();
    }
}

QStringList VideoInput::list_devices(void)
{
    QStringList list;
    bool silent = true;
#ifdef _MSC_VER
    list = list_devices_dshow(silent);
#endif
#ifdef Q_OS_MAC
    list = list_devices_quicktime(silent);
#endif
#ifdef Q_OS_LINUX
    list = list_devices_v4l2(silent);
#endif

    return list;
}

/*
   listDevices_dshow() is based on videoInput library by Theodore Watson:
   http://muonics.net/school/spring05/videoInput/

   Below is the original copyright
*/

//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//THE SOFTWARE.

//////////////////////////////////////////////////////////
//Written by Theodore Watson - theo.watson@gmail.com    //
//Do whatever you want with this code but if you find   //
//a bug or make an improvement I would love to know!    //
//                                                      //
//Warning This code is experimental                     //
//use at your own risk :)                               //
//////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
/*                     Shoutouts

Thanks to:

           Dillip Kumar Kara for crossbar code.
           Zachary Lieberman for getting me into this stuff
           and for being so generous with time and code.
           The guys at Potion Design for helping me with VC++
           Josh Fisher for being a serious C++ nerd :)
           Golan Levin for helping me debug the strangest
           and slowest bug in the world!

           And all the people using this library who send in
           bugs, suggestions and improvements who keep me working on
           the next version - yeah thanks a lot ;)

*/
/////////////////////////////////////////////////////////

QStringList VideoInput::list_devices_dshow(bool silent)
{
    if (!silent) { printf("\nVIDEOINPUT SPY MODE!\n\n"); }

    QStringList list;

#ifdef _MSC_VER
    ICreateDevEnum *pDevEnum = NULL;
    IEnumMoniker *pEnum = NULL;
    int deviceCounter = 0;
    
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL,
        CLSCTX_INPROC_SERVER, IID_ICreateDevEnum,
        reinterpret_cast<void**>(&pDevEnum));

    if (SUCCEEDED(hr))
    {
        // Create an enumerator for the video capture category.
        hr = pDevEnum->CreateClassEnumerator(
            CLSID_VideoInputDeviceCategory,
            &pEnum, 0);

        if(hr == S_OK){

            if (!silent) { printf("SETUP: Looking For Capture Devices\n"); }
            IMoniker *pMoniker = NULL;

            while (pEnum->Next(1, &pMoniker, NULL) == S_OK){

                IPropertyBag *pPropBag;
                hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag,
                    (void**)(&pPropBag));

                if (FAILED(hr)){
                    pMoniker->Release();
                    continue;  // Skip this one, maybe the next one will work.
                }


                 // Find the description or friendly name.
                VARIANT varName;
                VariantInit(&varName);
                hr = pPropBag->Read(L"Description", &varName, 0);

                if (FAILED(hr)) hr = pPropBag->Read(L"FriendlyName", &varName, 0);

                if (SUCCEEDED(hr)){

                    hr = pPropBag->Read(L"FriendlyName", &varName, 0);

                    char deviceName[255] = {0};

                    int count = 0;
                    int maxLen = sizeof(deviceName)/sizeof(deviceName[0]) - 2;
                    while( varName.bstrVal[count] != 0x00 && count < maxLen) {
                        deviceName[count] = (char)varName.bstrVal[count];
                        count++;
                    }
                    deviceName[count] = 0;
                    list.append(deviceName);

                    if (!silent) { printf("SETUP: %i) %s \n",deviceCounter, deviceName); }
                }

                pPropBag->Release();
                pPropBag = NULL;

                pMoniker->Release();
                pMoniker = NULL;

                deviceCounter++;
            }

            pDevEnum->Release();
            pDevEnum = NULL;

            pEnum->Release();
            pEnum = NULL;
        }

         if (!silent) { printf("SETUP: %i Device(s) found\n\n", deviceCounter); }
    }
#endif //_MSC_VER

    return list;
}

void VideoInput::configure_dshow(int index, bool silent)
{
    // HACK: if removed, OpenCV falls back to 640x480 by default
    //ask for the maximum resolution 
    // (it works with the current implementation of OpencV)
    cvSetCaptureProperty(_video_capture, CV_CAP_PROP_FRAME_WIDTH, 64000);
    cvSetCaptureProperty(_video_capture, CV_CAP_PROP_FRAME_HEIGHT, 48000);
}

QStringList VideoInput::list_devices_quicktime(bool silent)
{
    if (!silent) { std::cerr << "\n[list_devices_quicktime]\n\n"; }

    QStringList list;

#ifdef Q_OS_MAC
    list = list_devices_qtkit(silent);
#endif //Q_OS_MAC

    return list;
}

void VideoInput::configure_quicktime(int index, bool silent)
{
}

QStringList VideoInput::list_devices_v4l2(bool silent)
{
    if (!silent) { std::cerr << "\n[list_devices_v4l2]\n\n"; }

    QStringList list;

#ifdef Q_OS_LINUX

    /* Simple test program: Find number of Video Sources available.
       Start from 0 and go to MAX_CAMERAS while checking for the device with that name.
       If it fails on the first attempt of /dev/video0, then check if /dev/video is valid.
       Returns the global numCameras with the correct value (we hope) */

    int CameraNumber = 0;
    char deviceName[V4L2_MAX_DEVICE_DRIVER_NAME];

    while(CameraNumber < V4L2_MAX_CAMERAS) 
    {
        /* Print the CameraNumber at the end of the string with a width of one character */
        sprintf(deviceName, "/dev/video%1d", CameraNumber);

        /* Test using an open to see if this new device name really does exists. */
        int deviceHandle = open(deviceName, O_RDONLY);
        if (deviceHandle != -1)
        {
            /* This device does indeed exist - add it to the total so far */
            list.append(deviceName);
        }

        close(deviceHandle);

        /* Set up to test the next /dev/video source in line */
        CameraNumber++;
    } /* End while */

    if (!silent) { fprintf(stderr, "v4l numCameras %d\n", list.length()); }

#endif //Q_OS_LINUX

    return list;
}

void VideoInput::configure_v4l2(int index, bool silent)
{
#ifdef Q_OS_LINUX

    cv::Size requestSize(0,0);
    int CameraNumber = index;

    /* Print the CameraNumber at the end of the string with a width of one character */
    char deviceName[V4L2_MAX_DEVICE_DRIVER_NAME];
    sprintf(deviceName, "/dev/video%1d", CameraNumber);

    /* Test using an open to see if this new device name really does exists. */
    int deviceHandle = open(deviceName, O_RDONLY);
    if (deviceHandle != -1)
    {
        //find the maximum resolution
        struct v4l2_fmtdesc fmtdesc;
        fmtdesc.index = 0;
        fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        while (ioctl(deviceHandle, VIDIOC_ENUM_FMT, &fmtdesc)==0 && fmtdesc.index!=EINVAL)
        {
            if (!silent) { fprintf(stderr, "v4l cam %d, pixel_format %d: %s\n", CameraNumber, fmtdesc.index, fmtdesc.description); }
            
            struct v4l2_frmsizeenum frmsizeenum;
            frmsizeenum.index = 0;
            frmsizeenum.pixel_format = fmtdesc.pixelformat;

            while (ioctl(deviceHandle, VIDIOC_ENUM_FRAMESIZES, &frmsizeenum)==0 && frmsizeenum.index!=EINVAL)
            {
                cv::Size size;
                if (frmsizeenum.type==V4L2_FRMSIZE_TYPE_DISCRETE)
                {
                    size = cv::Size(frmsizeenum.discrete.width, frmsizeenum.discrete.height);
                }
                else
                {
                    size = cv::Size(frmsizeenum.stepwise.max_width, frmsizeenum.stepwise.min_height);
                }

                if (requestSize.width<size.width)
                {
                    requestSize = size;
                }
                
                if (!silent) { fprintf(stderr, "v4l cam %d, supported size w=%d h=%d\n", CameraNumber, size.width, size.height); }

                ++frmsizeenum.index;
            }
            ++fmtdesc.index;
        }
    }
    close(deviceHandle);

    if (_video_capture && requestSize.width>0)
    {
        if (!silent) { fprintf(stderr, " *** v4l cam %d, selected size w=%d h=%d\n", CameraNumber, requestSize.width, requestSize.height); }
        
        cvSetCaptureProperty(_video_capture, CV_CAP_PROP_FRAME_WIDTH, requestSize.width);
        cvSetCaptureProperty(_video_capture, CV_CAP_PROP_FRAME_HEIGHT, requestSize.height);
    }

#endif //Q_OS_LINUX
}
