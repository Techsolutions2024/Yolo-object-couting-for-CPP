#include "ClassFilterManager.h"
#include <iostream>

ClassFilterManager::ClassFilterManager() {
    // Start in "count all" mode (empty set)
}

ClassFilterManager& ClassFilterManager::getInstance() {
    static ClassFilterManager instance;
    return instance;
}

void ClassFilterManager::setSelectedClasses(const std::set<int>& classIds) {
    std::lock_guard<std::mutex> lock(mutex_);
    selectedClasses_ = classIds;

    if (selectedClasses_.empty()) {
        std::cout << "ClassFilter: Count ALL classes mode enabled" << std::endl;
    } else {
        std::cout << "ClassFilter: Counting " << selectedClasses_.size()
                  << " selected class(es)" << std::endl;
    }
}

std::set<int> ClassFilterManager::getSelectedClasses() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return selectedClasses_;
}

bool ClassFilterManager::shouldCountClass(int classId) const {
    std::lock_guard<std::mutex> lock(mutex_);

    // If no classes selected, count all
    if (selectedClasses_.empty()) {
        return true;
    }

    // Check if this class is in the selected set
    return selectedClasses_.find(classId) != selectedClasses_.end();
}

bool ClassFilterManager::isCountAllMode() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return selectedClasses_.empty();
}

int ClassFilterManager::getSelectedClassCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(selectedClasses_.size());
}

void ClassFilterManager::clearSelection() {
    std::lock_guard<std::mutex> lock(mutex_);
    selectedClasses_.clear();
    std::cout << "ClassFilter: Selection cleared - counting all classes" << std::endl;
}
