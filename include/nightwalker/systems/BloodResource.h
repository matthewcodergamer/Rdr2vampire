#pragma once

namespace nightwalker::systems {

class BloodResource final {
public:
    explicit BloodResource(double initial = 50.0) noexcept;

    void Reset(double value) noexcept;
    void Add(double amount) noexcept;
    bool TryConsume(double amount) noexcept;

    [[nodiscard]] double Value() const noexcept { return value_; }
    [[nodiscard]] double Normalized() const noexcept { return value_ / 100.0; }

private:
    static double Clamp(double value) noexcept;
    double value_{50.0};
};

} // namespace nightwalker::systems
