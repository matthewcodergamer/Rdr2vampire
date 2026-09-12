#pragma once

namespace nightwalker::systems {

class HiddenResource final {
public:
    explicit HiddenResource(double initial = 50.0) noexcept;
    void Reset(double value) noexcept;
    void Gain(double amount) noexcept;
    [[nodiscard]] double Value() const noexcept { return value_; }
    [[nodiscard]] double Normalized() const noexcept { return value_ / 100.0; }
private:
    static double ClampValue(double value) noexcept;
    double value_{50.0};
};

} // namespace nightwalker::systems
