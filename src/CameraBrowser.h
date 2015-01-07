//
//  CameraBrowser.h
//
//  Created by Jean-Pierre Mouilleseaux on 08 Dec 2013.
//  Copyright 2013-2014 Chorded Constructions. All rights reserved.
//

#pragma once

#include <vector>
#include "Camera.h"

namespace EDSDK {

typedef std::shared_ptr<class CameraBrowser> CameraBrowserRef;

class CameraBrowser : public std::enable_shared_from_this<CameraBrowser>, private boost::noncopyable {
public:
    static CameraBrowserRef instance();
    ~CameraBrowser();

    template<typename T, typename Y>
    inline void connectAddedHandler(T handler, Y* obj) { connectAddedHandler(std::bind(handler, obj, std::placeholders::_1)); }
    void connectAddedHandler(const std::function<void(CameraRef)>& handler);
    template<typename T, typename Y>
    inline void connectRemovedHandler(T handler, Y* obj) { connectRemovedHandler(std::bind(handler, obj, std::placeholders::_1)); }
    void connectRemovedHandler(const std::function<void(CameraRef)>& handler);
    template<typename T, typename Y>
    inline void connectEnumeratedHandler(T handler, Y* obj) { connectEnumeratedHandler(std::bind(handler, obj)); }
    void connectEnumeratedHandler(const std::function<void(void)>& handler);

//    bool isBrowsing() const;
    void start();
//    void stop();

    inline std::vector<CameraRef>& getCameras() {
        return mCameras;
    }

private:
    CameraBrowser();
    void enumerateCameraList();
    void removeCamera(const CameraRef& camera);
    CameraRef cameraForPortName(const std::string& name) const;

    static EdsError EDSCALLBACK handleCameraAdded(EdsVoid* inContext);

    static CameraBrowserRef sInstance;
    std::function<void (CameraRef)> mAddedHandler;
    std::function<void (CameraRef)> mRemovedHandler;
    std::function<void (void)> mEnumeratedHandler;
    std::vector<CameraRef> mCameras;
    bool mIsBrowsing;

    friend class Camera;
};

}
