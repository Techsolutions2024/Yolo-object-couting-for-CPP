#ifndef REGIONCOUNTMANAGER_H
#define REGIONCOUNTMANAGER_H

#include <string>
#include <map>
#include <set>
#include <mutex>
#include <memory>

/**
 * @brief Manages unique object counting per region across all cameras
 *
 * This singleton class tracks unique object IDs that enter each region,
 * ensuring each ID is counted only once per region. It provides thread-safe
 * operations and JSON persistence.
 */
class RegionCountManager {
public:
    // Singleton access
    static RegionCountManager& getInstance();

    // Delete copy/move constructors and assignment operators
    RegionCountManager(const RegionCountManager&) = delete;
    RegionCountManager& operator=(const RegionCountManager&) = delete;
    RegionCountManager(RegionCountManager&&) = delete;
    RegionCountManager& operator=(RegionCountManager&&) = delete;

    /**
     * @brief Record that an object with given ID has entered a region
     * @param regionName Name of the region
     * @param trackId Unique tracking ID of the object
     * @param cameraName Name of the camera (for context)
     * @return true if this is a new unique ID for the region, false if already counted
     */
    bool recordObjectEntry(const std::string& regionName,
                          size_t trackId,
                          const std::string& cameraName);

    /**
     * @brief Get the count of unique objects for a specific region
     * @param regionName Name of the region
     * @return Number of unique objects that have entered the region
     */
    int getRegionCount(const std::string& regionName) const;

    /**
     * @brief Get the set of unique IDs for a specific region
     * @param regionName Name of the region
     * @return Set of track IDs
     */
    std::set<size_t> getRegionIds(const std::string& regionName) const;

    /**
     * @brief Get all region data (for export/display)
     * @return Map of region name to (count, set of IDs)
     */
    std::map<std::string, std::pair<int, std::set<size_t>>> getAllRegionData() const;

    /**
     * @brief Clear all counting data for all regions
     */
    void clearAll();

    /**
     * @brief Clear counting data for a specific region
     * @param regionName Name of the region to clear
     */
    void clearRegion(const std::string& regionName);

    /**
     * @brief Save current counts to JSON file
     * @param filePath Path to JSON file (default: "region_count.json")
     * @return true if save successful, false otherwise
     */
    bool saveToJson(const std::string& filePath = "region_count.json") const;

    /**
     * @brief Load counts from JSON file
     * @param filePath Path to JSON file
     * @return true if load successful, false otherwise
     */
    bool loadFromJson(const std::string& filePath = "region_count.json");

    /**
     * @brief Enable/disable auto-save on changes
     * @param enabled If true, automatically save to JSON on every change
     * @param filePath Path for auto-save file
     */
    void setAutoSave(bool enabled, const std::string& filePath = "region_count.json");

private:
    RegionCountManager();
    ~RegionCountManager() = default;

    // Perform auto-save if enabled
    void performAutoSave();

    // Structure to hold region data
    struct RegionData {
        std::set<size_t> uniqueIds;  // Set of unique track IDs
        int count = 0;                // Count of unique objects
    };

    mutable std::mutex mutex_;
    std::map<std::string, RegionData> regionData_;

    // Auto-save configuration
    bool autoSaveEnabled_ = false;
    std::string autoSaveFilePath_ = "region_count.json";
};

#endif // REGIONCOUNTMANAGER_H
