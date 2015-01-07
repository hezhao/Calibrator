//
//  Camera.h
//
//  Created by Jean-Pierre Mouilleseaux on 08 Dec 2013.
//  Copyright 2013-2014 Chorded Constructions. All rights reserved.
//

#pragma once

#ifdef __APPLE__
    #define __MACOS__
#elif _WIN32_
    #error Target platform unsupported by Cinder-EDSDK
#elif __linux__
    #error Target platform unsupported by EDSDK
#endif

#include <iostream>
#include <boost/filesystem.hpp>
#include "EDSDK.h"

namespace EDSDK {

namespace fs = boost::filesystem;

typedef std::shared_ptr<class Camera> CameraRef;
typedef std::shared_ptr<class CameraFile> CameraFileRef;

class CameraFile : public std::enable_shared_from_this<CameraFile> {
public:
    static CameraFileRef create(const EdsDirectoryItemRef& directoryItem);
	~CameraFile();

    inline std::string getName() const {
        return std::string(mDirectoryItemInfo.szFileName);
    }
    inline uint32_t getSize() const {
        return mDirectoryItemInfo.size;
    }

private:
    CameraFile(const EdsDirectoryItemRef& directoryItem);

    EdsDirectoryItemRef mDirectoryItem;
    EdsDirectoryItemInfo mDirectoryItemInfo;

    friend class Camera;
};

class Camera : public std::enable_shared_from_this<Camera> {
public:
    struct Settings {
        Settings() : mShouldKeepAlive(true), mPictureSaveLocation(kEdsSaveTo_Host) {}

        Settings& setShouldKeepAlive(bool flag) {
            mShouldKeepAlive = flag; return *this;
        }
        inline bool getShouldKeepAlive() const {
            return mShouldKeepAlive;
        }
        Settings& setPictureSaveLocation(EdsUInt32 location)  {
            mPictureSaveLocation = location; return *this;
        }
        inline EdsUInt32 getPictureSaveLocation() const {
            return mPictureSaveLocation;
        }

    private:
        bool mShouldKeepAlive;
        EdsUInt32 mPictureSaveLocation;
    };

    static CameraRef create(const EdsCameraRef& camera);
    ~Camera();

    template<typename T, typename Y>
    inline void connectRemovedHandler(T handler, Y* obj) { connectRemovedHandler(std::bind(handler, obj, std::placeholders::_1)); }
    void connectRemovedHandler(const std::function<void(CameraRef)>& handler);
    template<typename T, typename Y>
    inline void connectFileAddedHandler(T handler, Y* obj) { connectFileAddedHandler(std::bind(handler, obj, std::placeholders::_1, std::placeholders::_2)); }
    void connectFileAddedHandler(const std::function<void(CameraRef, CameraFileRef)>& handler);

    inline std::string getName() const {
        return std::string(mDeviceInfo.szDeviceDescription);
    }
    inline std::string getPortName() const {
        return std::string(mDeviceInfo.szPortName);
    }

    inline bool hasOpenSession() const {
        return mHasOpenSession;
    }
    EdsError requestOpenSession(const Settings& settings = Settings());
    EdsError requestCloseSession();

    EdsError requestTakePicture();
    void requestDownloadFile(const CameraFileRef& file, const fs::path& destinationFolderPath, const std::function<void(EdsError error, fs::path outputFilePath)>& callback);
//    void requestReadFile(const CameraFileRef& file, const std::function<void(EdsError error, ci::Surface8u surface)>& callback);

private:
    Camera(const EdsCameraRef& camera);

    static EdsError EDSCALLBACK handleObjectEvent(EdsUInt32 inEvent, EdsBaseRef inRef, EdsVoid* inContext);
    static EdsError EDSCALLBACK handlePropertyEvent(EdsUInt32 inEvent, EdsUInt32 inPropertyID, EdsUInt32 inParam, EdsVoid* inContext);
    static EdsError EDSCALLBACK handleStateEvent(EdsUInt32 inEvent, EdsUInt32 inParam, EdsVoid* inContext);

    std::function<void (CameraRef)> mRemovedHandler;
    std::function<void (CameraRef, CameraFileRef)> mFileAddedHandler;
    EdsCameraRef mCamera;
    EdsDeviceInfo mDeviceInfo;
    bool mHasOpenSession;
    bool mShouldKeepAlive;
};

}
