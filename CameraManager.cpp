#include "CameraManager.h"
#include <fstream>
#include <iostream>
#include <algorithm>

using json = nlohmann::json;

CameraManager::CameraManager() : nextId_(1) {
}

CameraManager::~CameraManager() {
    closeAllCameras();
}

int CameraManager::generateNextId() {
    return nextId_++;
}

int CameraManager::addCamera(const std::string& name, CameraType type, const std::string& source) {
    std::lock_guard<std::mutex> lock(mutex_);

    int id = generateNextId();
    auto camera = std::make_shared<CameraSource>(id, name, type, source);
    cameras_.push_back(camera);

    std::cout << "Camera added: ID=" << id << ", Name='" << name << "'" << std::endl;
    return id;
}

bool CameraManager::removeCamera(int id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = std::find_if(cameras_.begin(), cameras_.end(),
                          [id](const std::shared_ptr<CameraSource>& cam) {
                              return cam->getId() == id;
                          });

    if (it != cameras_.end()) {
        (*it)->close();
        cameras_.erase(it);
        std::cout << "Camera removed: ID=" << id << std::endl;
        return true;
    }

    std::cerr << "Camera not found: ID=" << id << std::endl;
    return false;
}

bool CameraManager::updateCamera(int id, const std::string& name, const std::string& source) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto camera = getCamera(id);
    if (camera) {
        camera->setName(name);
        camera->setSource(source);
        std::cout << "Camera updated: ID=" << id << std::endl;
        return true;
    }

    std::cerr << "Camera not found for update: ID=" << id << std::endl;
    return false;
}

CameraSource* CameraManager::getCamera(int id) {
    auto it = std::find_if(cameras_.begin(), cameras_.end(),
                          [id](const std::shared_ptr<CameraSource>& cam) {
                              return cam->getId() == id;
                          });

    return (it != cameras_.end()) ? it->get() : nullptr;
}

bool CameraManager::openCamera(int id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto camera = getCamera(id);
    if (camera) {
        return camera->open();
    }

    std::cerr << "Camera not found: ID=" << id << std::endl;
    return false;
}

void CameraManager::closeCamera(int id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto camera = getCamera(id);
    if (camera) {
        camera->close();
    }
}

void CameraManager::closeAllCameras() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& camera : cameras_) {
        camera->close();
    }
    std::cout << "All cameras closed." << std::endl;
}

bool CameraManager::saveToFile(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(mutex_);

    try {
        json j;
        j["next_id"] = nextId_;
        j["cameras"] = json::array();

        for (const auto& camera : cameras_) {
            j["cameras"].push_back(camera->toJson());
        }

        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for writing: " << filename << std::endl;
            return false;
        }

        file << j.dump(4);
        file.close();

        std::cout << "Configuration saved to: " << filename << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving configuration: " << e.what() << std::endl;
        return false;
    }
}

bool CameraManager::loadFromFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);

    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for reading: " << filename << std::endl;
            return false;
        }

        json j;
        file >> j;
        file.close();

        // Clear existing cameras
        cameras_.clear();

        // Load next ID
        if (j.contains("next_id")) {
            nextId_ = j["next_id"].get<int>();
        }

        // Load cameras
        if (j.contains("cameras")) {
            for (const auto& camJson : j["cameras"]) {
                auto camera = std::make_shared<CameraSource>(CameraSource::fromJson(camJson));
                cameras_.push_back(camera);
            }
        }

        std::cout << "Configuration loaded from: " << filename << std::endl;
        std::cout << "Loaded " << cameras_.size() << " camera(s)." << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading configuration: " << e.what() << std::endl;
        return false;
    }
}
