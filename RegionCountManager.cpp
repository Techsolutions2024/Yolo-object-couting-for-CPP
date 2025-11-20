#include "RegionCountManager.h"
#include <fstream>
#include <iostream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

RegionCountManager::RegionCountManager() {
    // Constructor
}

RegionCountManager& RegionCountManager::getInstance() {
    static RegionCountManager instance;
    return instance;
}

bool RegionCountManager::recordObjectEntry(const std::string& regionName,
                                          size_t trackId,
                                          const std::string& cameraName) {
    bool needsAutoSave = false;
    bool isNewId = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto& regionData = regionData_[regionName];

        // Check if this ID is already in the set
        auto result = regionData.uniqueIds.insert(trackId);

        // If insertion successful (new unique ID)
        if (result.second) {
            regionData.count = static_cast<int>(regionData.uniqueIds.size());
            isNewId = true;
            needsAutoSave = autoSaveEnabled_;
        }
    } // Lock released here

    // Perform auto-save outside the lock to avoid deadlock
    if (needsAutoSave) {
        performAutoSave();
    }

    return isNewId;
}

int RegionCountManager::getRegionCount(const std::string& regionName) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = regionData_.find(regionName);
    if (it != regionData_.end()) {
        return it->second.count;
    }
    return 0;
}

std::set<size_t> RegionCountManager::getRegionIds(const std::string& regionName) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = regionData_.find(regionName);
    if (it != regionData_.end()) {
        return it->second.uniqueIds;
    }
    return std::set<size_t>();
}

std::map<std::string, std::pair<int, std::set<size_t>>> RegionCountManager::getAllRegionData() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::map<std::string, std::pair<int, std::set<size_t>>> result;
    for (const auto& pair : regionData_) {
        result[pair.first] = std::make_pair(pair.second.count, pair.second.uniqueIds);
    }
    return result;
}

void RegionCountManager::clearAll() {
    bool needsAutoSave = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        regionData_.clear();
        needsAutoSave = autoSaveEnabled_;
    } // Lock released here

    if (needsAutoSave) {
        performAutoSave();
    }
}

void RegionCountManager::clearRegion(const std::string& regionName) {
    bool needsAutoSave = false;
    bool found = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = regionData_.find(regionName);
        if (it != regionData_.end()) {
            regionData_.erase(it);
            found = true;
            needsAutoSave = autoSaveEnabled_;
        }
    } // Lock released here

    if (found && needsAutoSave) {
        performAutoSave();
    }
}

bool RegionCountManager::saveToJson(const std::string& filePath) const {
    std::lock_guard<std::mutex> lock(mutex_);

    try {
        QJsonObject root;

        for (const auto& pair : regionData_) {
            const std::string& regionName = pair.first;
            const RegionData& data = pair.second;

            QJsonObject regionObj;
            regionObj["count"] = data.count;

            QJsonArray idsArray;
            for (size_t id : data.uniqueIds) {
                idsArray.append(static_cast<qint64>(id));
            }
            regionObj["ids"] = idsArray;

            root[QString::fromStdString(regionName)] = regionObj;
        }

        QJsonDocument doc(root);

        QFile file(QString::fromStdString(filePath));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            std::cerr << "Failed to open file for writing: " << filePath << std::endl;
            return false;
        }

        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Exception while saving to JSON: " << e.what() << std::endl;
        return false;
    }
}

bool RegionCountManager::loadFromJson(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(mutex_);

    try {
        QFile file(QString::fromStdString(filePath));
        if (!file.exists()) {
            std::cout << "JSON file does not exist: " << filePath << std::endl;
            return false;
        }

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            std::cerr << "Failed to open file for reading: " << filePath << std::endl;
            return false;
        }

        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isObject()) {
            std::cerr << "Invalid JSON format" << std::endl;
            return false;
        }

        QJsonObject root = doc.object();
        regionData_.clear();

        for (auto it = root.begin(); it != root.end(); ++it) {
            std::string regionName = it.key().toStdString();
            QJsonObject regionObj = it.value().toObject();

            RegionData data;
            data.count = regionObj["count"].toInt();

            QJsonArray idsArray = regionObj["ids"].toArray();
            for (const auto& idValue : idsArray) {
                data.uniqueIds.insert(static_cast<size_t>(idValue.toInteger()));
            }

            regionData_[regionName] = data;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Exception while loading from JSON: " << e.what() << std::endl;
        return false;
    }
}

void RegionCountManager::setAutoSave(bool enabled, const std::string& filePath) {
    std::lock_guard<std::mutex> lock(mutex_);
    autoSaveEnabled_ = enabled;
    autoSaveFilePath_ = filePath;
}

void RegionCountManager::performAutoSave() {
    // This method is called without holding the lock
    // saveToJson will acquire the lock internally
    saveToJson(autoSaveFilePath_);
}
