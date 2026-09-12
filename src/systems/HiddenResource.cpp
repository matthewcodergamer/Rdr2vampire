#include "nightwalker/systems/HiddenResource.h"
#include <algorithm>

namespace nightwalker::systems {

HiddenResource::HiddenResource(double initial) noexcept : value_(ClampValue(initial)) {}
void HiddenResource::Reset(double value) noexcept { value_ = ClampValue(value); }
void HiddenResource::Gain(double amount) noexcept { if (amount > 0.0) value_ = ClampValue(value_ + amount); }
double HiddenResource::ClampValue(double value) noexcept { return std::clamp(value, 0.0, 100.0); }

} // namespace nightwalker::systems
