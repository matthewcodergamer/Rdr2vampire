#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace nightwalker::util { class Logger; }

namespace nightwalker::licensing {

enum class LicenseStatus : std::uint8_t {
    Valid,
    Missing,
    ParseError,
    UnsupportedSchema,
    WrongIssuer,
    WrongProduct,
    InvalidClaims,
    Expired,
    DeviceMismatch,
    BadSignatureEncoding,
    BadSignature,
    DeviceIdUnavailable,
};

struct LicenseClaims final {
    std::string licenseId;
    std::string customerId;
    std::string deviceHash;
    std::uint64_t issuedUnix{0};
    std::uint64_t expiresUnix{0};
};

struct LicenseResult final {
    LicenseStatus status{LicenseStatus::ParseError};
    LicenseClaims claims{};
    std::string message{};

    [[nodiscard]] bool IsValid() const noexcept { return status == LicenseStatus::Valid; }
};

using SignatureVerifier = std::function<bool(std::string_view canonicalPayload, std::span<const std::uint8_t> signature)>;

[[nodiscard]] LicenseResult ValidateLicenseText(
    std::string_view text,
    std::string_view localDeviceHash,
    std::uint64_t nowUnix,
    const SignatureVerifier& verifySignature);

[[nodiscard]] LicenseResult ValidateInstalledLicense(
    const std::filesystem::path& pluginDirectory,
    util::Logger& logger);

[[nodiscard]] std::string StatusName(LicenseStatus status);

} // namespace nightwalker::licensing
