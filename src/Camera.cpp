//
//  Camera.cpp
//
//  Created by Jean-Pierre Mouilleseaux on 08 Dec 2013.
//  Copyright 2013-2014 Chorded Constructions. All rights reserved.
//

#include "Camera.h"
#include "CameraBrowser.h"

namespace EDSDK {
    
#pragma mark CAMERA FILE
    
    CameraFileRef CameraFile::create(const EdsDirectoryItemRef& directoryItem) {
        return CameraFileRef(new CameraFile(directoryItem))->shared_from_this();
    }
    
    CameraFile::CameraFile(const EdsDirectoryItemRef& directoryItem) {
        if (!directoryItem) {
            throw std::runtime_error("");
        }
        
        EdsRetain(directoryItem);
        mDirectoryItem = directoryItem;
        
        EdsError error = EdsGetDirectoryItemInfo(mDirectoryItem, &mDirectoryItemInfo);
        if (error != EDS_ERR_OK) {
            std::cerr << "ERROR - failed to get directory item info" << std::endl;
            throw std::runtime_error("ERROR - failed to get directory item info");
        }
    }
    
    CameraFile::~CameraFile() {
        EdsRelease(mDirectoryItem);
        mDirectoryItem = NULL;
    }
    
#pragma mark - CAMERA
    
    CameraRef Camera::create(const EdsCameraRef& camera) {
        return CameraRef(new Camera(camera))->shared_from_this();
    }
    
    Camera::Camera(const EdsCameraRef& camera) {
        if (!camera) {
            throw std::runtime_error("");
        }
        
        EdsRetain(camera);
        mCamera = camera;
        
        EdsError error = EdsGetDeviceInfo(mCamera, &mDeviceInfo);
        if (error != EDS_ERR_OK) {
            std::cerr << "ERROR - failed to get device info" << std::endl;
            // TODO - NULL out mDeviceInfo
        }
        
        mHasOpenSession = false;
        mIsLiveView = false;
        
        // set event handlers
        error = EdsSetObjectEventHandler(mCamera, kEdsObjectEvent_All, Camera::handleObjectEvent, this);
        if (error != EDS_ERR_OK) {
            std::cerr << "ERROR - failed to set object event handler" << std::endl;
        }
        error = EdsSetPropertyEventHandler(mCamera, kEdsPropertyEvent_All, Camera::handlePropertyEvent, this);
        if (error != EDS_ERR_OK) {
            std::cerr << "ERROR - failed to set property event handler" << std::endl;
        }
        error = EdsSetCameraStateEventHandler(mCamera, kEdsStateEvent_All, Camera::handleStateEvent, this);
        if (error != EDS_ERR_OK) {
            std::cerr << "ERROR - failed to set object event handler" << std::endl;
        }
    }
    
    Camera::~Camera() {
        mRemovedHandler = NULL;
        mFileAddedHandler = NULL;
        
        if (mHasOpenSession) {
            requestCloseSession();
        }
        
        // NB - starting with EDSDK 2.10, this release will cause an EXC_BAD_ACCESS (code=EXC_I386_GPFLT) at the end of the runloop
        //    EdsRelease(mCamera);
        mCamera = NULL;
    }
    
#pragma mark -
    
    void Camera::connectRemovedHandler(const std::function<void(CameraRef)>& handler) {
        mRemovedHandler = handler;
    }
    
    void Camera::connectFileAddedHandler(const std::function<void(CameraRef, CameraFileRef)>& handler) {
        mFileAddedHandler = handler;
    }
    
    EdsError Camera::requestOpenSession(const Settings &settings) {
        if (mHasOpenSession) {
            return EDS_ERR_SESSION_ALREADY_OPEN;
        }
        
        EdsError error = EdsOpenSession(mCamera);
        if (error != EDS_ERR_OK) {
            std::cerr << "ERROR - failed to open camera session" << std::endl;
            return error;
        }
        mHasOpenSession = true;
        
        mShouldKeepAlive = settings.getShouldKeepAlive();
        EdsUInt32 saveTo = settings.getPictureSaveLocation();
        error = EdsSetPropertyData(mCamera, kEdsPropID_SaveTo, 0, sizeof(saveTo) , &saveTo);
        if (error != EDS_ERR_OK) {
            std::cerr << "ERROR - failed to set save destination host/device" << std::endl;
            return error;
        }
        
        if (saveTo == kEdsSaveTo_Host) {
            // ??? - requires UI lock?
            EdsCapacity capacity = {0x7FFFFFFF, 0x1000, 1};
            error = EdsSetCapacity(mCamera, capacity);
            if (error != EDS_ERR_OK) {
                std::cerr << "ERROR - failed to set capacity of host" << std::endl;
                return error;
            }
        }
        
        return EDS_ERR_OK;
    }
    
    EdsError Camera::requestCloseSession() {
        if (!mHasOpenSession) {
            return EDS_ERR_SESSION_NOT_OPEN;
        }
        
        EdsError error = EdsCloseSession(mCamera);
        if (error != EDS_ERR_OK) {
            std::cerr << "ERROR - failed to close camera session" << std::endl;
            return error;
        }
        
        mHasOpenSession = false;
        return EDS_ERR_OK;
    }
    
    EdsError Camera::requestTakePicture() {
        if (!mHasOpenSession) {
            return EDS_ERR_SESSION_NOT_OPEN;
        }
        
        EdsError error = EdsSendCommand(mCamera, kEdsCameraCommand_TakePicture, 0);
        if (error != EDS_ERR_OK) {
            std::cerr << "ERROR - failed to take picture" << std::endl;
        }
        return error;
    }
    
    void Camera::requestDownloadFile(const CameraFileRef& file, const QDir& destinationFolderPath, const std::function<void(EdsError error, QString outputFilePath)>& callback) {
        // check if destination exists and create if not
        if ( !destinationFolderPath.exists() ) {
            bool status = destinationFolderPath.mkpath(".");
            if (!status) {
                std::cerr << "ERROR - failed to create destination folder path '" << destinationFolderPath.dirName().toStdString() << "'" << std::endl;
                return callback(EDS_ERR_INTERNAL_ERROR, NULL);
            }
        }
        
        QString filePath = destinationFolderPath.filePath(QString::fromStdString(file->getName()));
        
        EdsStreamRef stream = NULL;
        EdsError error = EdsCreateFileStream(filePath.toStdString().c_str(), kEdsFileCreateDisposition_CreateAlways, kEdsAccess_ReadWrite, &stream);
        if (error != EDS_ERR_OK) {
            std::cerr << "ERROR - failed to create file stream" << std::endl;
            goto download_cleanup;
        }
        
        error = EdsDownload(file->mDirectoryItem, file->getSize(), stream);
        if (error != EDS_ERR_OK) {
            std::cerr << "ERROR - failed to download" << std::endl;
            goto download_cleanup;
        }
        
        error = EdsDownloadComplete(file->mDirectoryItem);
        if (error != EDS_ERR_OK) {
            std::cerr << "ERROR - failed to mark download as complete" << std::endl;
            goto download_cleanup;
        }
        
    download_cleanup:
        if (stream) {
            EdsRelease(stream);
        }
        
        callback(error, filePath);
    }
    
    void Camera::requestReadFile(const CameraFileRef& file, const std::function<void(EdsError error, QImage img)>& callback) {
       QImage img;
    
       EdsStreamRef stream = NULL;
       EdsError error = EdsCreateMemoryStream(0, &stream);
       if (error != EDS_ERR_OK) {
           std::cerr << "ERROR - failed to create memory stream" << std::endl;
           goto read_cleanup;
       }
    
       error = EdsDownload(file->mDirectoryItem, file->getSize(), stream);
       if (error != EDS_ERR_OK) {
           std::cerr << "ERROR - failed to download" << std::endl;
           goto read_cleanup;
       }
    
       error = EdsDownloadComplete(file->mDirectoryItem);
       if (error != EDS_ERR_OK) {
           std::cerr << "ERROR - failed to mark download as complete" << std::endl;
           goto read_cleanup;
       }
    
       unsigned char* data;
       error = EdsGetPointer(stream, (EdsVoid**)&data);
       if (error != EDS_ERR_OK) {
           std::cerr << "ERROR - failed to get pointer from stream" << std::endl;
           goto read_cleanup;
       }
    
       EdsUInt32 length;
       error = EdsGetLength(stream, &length);
       if (error != EDS_ERR_OK) {
           std::cerr << "ERROR - failed to get stream length" << std::endl;
           goto read_cleanup;
       }
    
       img = QImage::fromData(data, length, "JPG");
    
    read_cleanup:
       if (stream) {
           EdsRelease(stream);
       }
    
       callback(error, img);
    }
    
    
    void Camera::startLiveView()
    {
        std::cout << "start live view" << std::endl;
        EdsError err = EDS_ERR_OK;
        
        // Get the output device for the live view image
        EdsUInt32 device;
        err = EdsGetPropertyData(mCamera, kEdsPropID_Evf_OutputDevice, 0, sizeof(device), &device );
        
        // PC live view starts by setting the PC as the output device for the live view image.
        if(err == EDS_ERR_OK)
        {
            device |= kEdsEvfOutputDevice_PC;
            err = EdsSetPropertyData(mCamera, kEdsPropID_Evf_OutputDevice, 0 , sizeof(device), &device);
            mIsLiveView = true;
        }
        
        // A property change event notification is issued from the camera if property settings are made successfully.
        // Start downloading of the live view image once the property change notification arrives.
    }
    
    void Camera::endLiveView()
    {
        std::cout << "end live view" << std::endl;
        EdsError err = EDS_ERR_OK;
        
        // Get the output device for the live view image
        EdsUInt32 device;
        err = EdsGetPropertyData(mCamera, kEdsPropID_Evf_OutputDevice, 0, sizeof(device), &device );
        
        // PC live view ends if the PC is disconnected from the live view image output device.
        if(err == EDS_ERR_OK)
        {
            device &= ~kEdsEvfOutputDevice_PC;
            err = EdsSetPropertyData(mCamera, kEdsPropID_Evf_OutputDevice, 0 , sizeof(device), &device);
        }
        
        mIsLiveView = false;
    }
    
    void Camera::toggleLiveView()
    {
        if (mIsLiveView) {
            endLiveView();
        }
        else {
            startLiveView();
        }
    }
    
    EdsError Camera::requestDownloadEvfData( QImage& img )
    {
        if( !mIsLiveView ){
            std::cerr << "No live view" << std::endl;
            startLiveView();
            return EDS_ERR_OK;
        }
        
        EdsError err = EDS_ERR_OK;
        EdsStreamRef stream = NULL;
        EdsEvfImageRef evfImage = NULL;
        
        // Create memory stream.
        err = EdsCreateMemoryStream( 0, &stream);
        
        // Create EvfImageRef.
        if(err == EDS_ERR_OK) {
            err = EdsCreateEvfImageRef(stream, &evfImage);
        }
        
        // Download live view image data.
        if(err == EDS_ERR_OK){
            err = EdsDownloadEvfImage(mCamera, evfImage);
        }
        
        // Display image
        EdsUInt32 length;
        unsigned char* image_data;
        EdsGetLength( stream, &length );
        if( length <= 0 ) return EDS_ERR_OK;
        
        EdsGetPointer( stream, (EdsVoid**)&image_data );
        
        // reserve memory
        img = QImage::fromData(image_data, length, "JPG");
        
        //    Buffer buffer( image_data, length );
        //    surface = Surface( loadImage( DataSourceBuffer::create(buffer), ImageSource::Options(), "jpg" ) );
        
        // Release stream
        if(stream != NULL) {
            EdsRelease(stream);
            stream = NULL;
        }
        // Release evfImage
        if(evfImage != NULL) {
            EdsRelease(evfImage);
            evfImage = NULL;
        }
        
        return EDS_ERR_OK;
    }
    
    
#pragma mark - CALLBACKS
    
    EdsError EDSCALLBACK Camera::handleObjectEvent(EdsUInt32 inEvent, EdsBaseRef inRef, EdsVoid* inContext) {
        Camera* c = (Camera*)inContext;
        CameraRef camera = CameraBrowser::instance()->cameraForPortName(c->getPortName());
        switch (inEvent) {
            case kEdsObjectEvent_DirItemRequestTransfer: {
                EdsDirectoryItemRef directoryItem = (EdsDirectoryItemRef)inRef;
                CameraFileRef file = NULL;
                try {
                    file = CameraFile::create(directoryItem);
                } catch (...) {
                    EdsRelease(directoryItem);
                    break;
                }
                EdsRelease(directoryItem);
                directoryItem = NULL;
                if (camera->mFileAddedHandler) {
                    camera->mFileAddedHandler(camera, file);
                }
                break;
            }
            default:
                if (inRef) {
                    EdsRelease(inRef);
                    inRef = NULL;
                }
                break;
        }
        return EDS_ERR_OK;
    }
    
    EdsError EDSCALLBACK Camera::handlePropertyEvent(EdsUInt32 inEvent, EdsUInt32 inPropertyID, EdsUInt32 inParam, EdsVoid* inContext) {
        return EDS_ERR_OK;
    }
    
    EdsError EDSCALLBACK Camera::handleStateEvent(EdsUInt32 inEvent, EdsUInt32 inParam, EdsVoid* inContext) {
        Camera* c = (Camera*)inContext;
        CameraRef camera = c->shared_from_this();
        switch (inEvent) {
            case kEdsStateEvent_WillSoonShutDown:
                if (camera->mHasOpenSession && camera->mShouldKeepAlive) {
                    EdsError error = EdsSendCommand(camera->mCamera, kEdsCameraCommand_ExtendShutDownTimer, 0);
                    if (error != EDS_ERR_OK) {
                        std::cerr << "ERROR - failed to extend shut down timer" << std::endl;
                    }
                }
                break;
            case kEdsStateEvent_Shutdown:
                camera->requestCloseSession();
                // send handler and browser removal notices
                if (camera->mRemovedHandler) {
                    camera->mRemovedHandler(camera);
                }
                CameraBrowser::instance()->removeCamera(camera);
                break;
            default:
                break;
        }
        return EDS_ERR_OK;
    }
    
}
