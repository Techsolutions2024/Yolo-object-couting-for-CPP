#ifndef CAMERAMANAGER_H
#define CAMERAMANAGER_H

#include <vector>
#include <memory>
#include <string>
#include <mutex>
#include "CameraSource.h"
#include <nlohmann/json.hpp>

class CameraManager {
public:
    CameraManager();
    ~CameraManager();

    // Camera management
    int addCamera(const std::string& name, CameraType type, const std::string& source);
    bool removeCamera(int id);
    bool updateCamera(int id, const std::string& name, const std::string& source);
    CameraSource* getCamera(int id);
    const std::vector<std::shared_ptr<CameraSource>>& getAllCameras() const { return cameras_; }

    // Camera operations
    bool openCamera(int id);
    void closeCamera(int id);
    void closeAllCameras();

    // Configuration persistence
    bool saveToFile(const std::string& filename) const;
    bool loadFromFile(const std::string& filename);

private:
    std::vector<std::shared_ptr<CameraSource>> cameras_;
    int nextId_;
    mutable std::mutex mutex_;

    int generateNextId();
};

#endif // CAMERAMANAGER_H
