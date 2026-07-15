#pragma once

#include <functional>
#include <string>

// This struct will hold all the "global" services for a given automation run.
struct AutomationContext {
    // For now, it only has our one callback.
    std::function<void(const std::string&, int)> stateChange;
};