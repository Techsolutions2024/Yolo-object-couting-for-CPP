#ifndef CLASSFILTERMANAGER_H
#define CLASSFILTERMANAGER_H

#include <set>
#include <mutex>

/**
 * @brief Singleton manager for class-based filtering in object counting
 *
 * Manages which object classes should be counted in detection.
 * Default behavior: count all classes if none are selected.
 */
class ClassFilterManager {
public:
    // Singleton access
    static ClassFilterManager& getInstance();

    // Delete copy/move constructors and assignment operators
    ClassFilterManager(const ClassFilterManager&) = delete;
    ClassFilterManager& operator=(const ClassFilterManager&) = delete;
    ClassFilterManager(ClassFilterManager&&) = delete;
    ClassFilterManager& operator=(ClassFilterManager&&) = delete;

    /**
     * @brief Set the classes to count
     * @param classIds Set of class IDs to count (empty = count all)
     */
    void setSelectedClasses(const std::set<int>& classIds);

    /**
     * @brief Get the selected classes
     * @return Set of class IDs to count
     */
    std::set<int> getSelectedClasses() const;

    /**
     * @brief Check if a class should be counted
     * @param classId Class ID to check
     * @return true if class should be counted, false otherwise
     */
    bool shouldCountClass(int classId) const;

    /**
     * @brief Check if we're in "count all" mode
     * @return true if no specific classes selected (count all), false otherwise
     */
    bool isCountAllMode() const;

    /**
     * @brief Get number of selected classes
     * @return Number of selected classes (0 = count all)
     */
    int getSelectedClassCount() const;

    /**
     * @brief Clear all selections (revert to count all mode)
     */
    void clearSelection();

private:
    ClassFilterManager();
    ~ClassFilterManager() = default;

    mutable std::mutex mutex_;
    std::set<int> selectedClasses_;
};

#endif // CLASSFILTERMANAGER_H
