#include "nightwalker/licensing/License.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <string>
#include <unordered_map>

namespace nightwalker::licensing {
namespace {

bool IsSafeToken(std::string_view value, std::size_t minLen, std::size_t maxLen) {
    if (value.size() < minLen || value.size() > maxLen) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char c) {
        return std::isalnum(c) || c == '-' || c == '_' || c == '.';
    });
}

bool IsHex64(std::string_view value) {
    if (value.size() != 64) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char c) {
        return std::isxdigit(c) != 0;
    });
}

bool ParseU64(std::string_view value, std::uint64_t& out) {
    if (value.empty()) return false;
    const char* first = value.data();
    const char* last = first + value.size();
    auto result = std::from_chars(first, last, out, 10);
    return result.ec == std::errc{} && result.ptr == last;
}

int Base64Value(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

bool DecodeBase64(std::string_view input, std::vector<std::uint8_t>& out) {
    out.clear();
    if (input.empty() || (input.size() % 4) != 0) return false;
    out.reserve((input.size() / 4) * 3);

    for (std::size_t i = 0; i < input.size(); i += 4) {
        const bool pad2 = input[i + 2] == '=';
        const bool pad3 = input[i + 3] == '=';
        if (pad2 && !pad3) return false;
        if ((pad2 || pad3) && i + 4 != input.size()) return false;

        const int a = Base64Value(input[i]);
        const int b = Base64Value(input[i + 1]);
        const int c = pad2 ? 0 : Base64Value(input[i + 2]);
        const int d = pad3 ? 0 : Base64Value(input[i + 3]);
        if (a < 0 || b < 0 || c < 0 || d < 0) return false;

        const std::uint32_t packed = (static_cast<std::uint32_t>(a) << 18) |
                                     (static_cast<std::uint32_t>(b) << 12) |
                                     (static_cast<std::uint32_t>(c) << 6) |
                                     static_cast<std::uint32_t>(d);
        out.push_back(static_cast<std::uint8_t>((packed >> 16) & 0xFF));
        if (!pad2) out.push_back(static_cast<std::uint8_t>((packed >> 8) & 0xFF));
        if (!pad3) out.push_back(static_cast<std::uint8_t>(packed & 0xFF));
    }
    return true;
}

std::unordered_map<std::string, std::string> ParseFields(std::string_view text, bool& ok) {
    std::unordered_map<std::string, std::string> fields;
    ok = true;
    std::size_t pos = 0;
    while (pos < text.size()) {
        std::size_t end = text.find('\n', pos);
        if (end == std::string_view::npos) end = text.size();
        auto line = text.substr(pos, end - pos);
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        pos = end == text.size() ? text.size() : end + 1;
        if (line.empty()) continue;
        const auto eq = line.find('=');
        if (eq == std::string_view::npos || eq == 0 || eq + 1 > line.size()) { ok = false; return {}; }
        std::string key(line.substr(0, eq));
        std::string value(line.substr(eq + 1));
        if (fields.contains(key)) { ok = false; return {}; }
        fields.emplace(std::move(key), std::move(value));
    }
    return fields;
}

LicenseResult Fail(LicenseStatus status, std::string message) {
    LicenseResult result;
    result.status = status;
    result.message = std::move(message);
    return result;
}

} // namespace

std::string StatusName(LicenseStatus status) {
    switch (status) {
        case LicenseStatus::Valid: return "valid";
        case LicenseStatus::Missing: return "missing";
        case LicenseStatus::ParseError: return "parse_error";
        case LicenseStatus::UnsupportedSchema: return "unsupported_schema";
        case LicenseStatus::WrongIssuer: return "wrong_issuer";
        case LicenseStatus::WrongProduct: return "wrong_product";
        case LicenseStatus::InvalidClaims: return "invalid_claims";
        case LicenseStatus::Expired: return "expired";
        case LicenseStatus::DeviceMismatch: return "device_mismatch";
        case LicenseStatus::BadSignatureEncoding: return "bad_signature_encoding";
        case LicenseStatus::BadSignature: return "bad_signature";
        case LicenseStatus::DeviceIdUnavailable: return "device_id_unavailable";
    }
    return "unknown";
}

LicenseResult ValidateLicenseText(
    std::string_view text,
    std::string_view localDeviceHash,
    std::uint64_t nowUnix,
    const SignatureVerifier& verifySignature) {

    bool parsed = false;
    auto fields = ParseFields(text, parsed);
    if (!parsed) return Fail(LicenseStatus::ParseError, "License file is malformed.");

    static constexpr std::array<std::string_view, 8> required = {
        "schema", "issuer", "product", "license_id", "customer_id", "issued_unix", "expires_unix", "device_hash"
    };
    for (auto key : required) {
        if (!fields.contains(std::string(key))) return Fail(LicenseStatus::ParseError, "License file is missing required fields.");
    }
    if (!fields.contains("signature")) return Fail(LicenseStatus::ParseError, "License file is missing its signature.");
    if (fields.size() != required.size() + 1) return Fail(LicenseStatus::ParseError, "License file contains unsupported fields.");

    if (fields["schema"] != "1") return Fail(LicenseStatus::UnsupportedSchema, "Unsupported Nightwalker license schema.");
    if (fields["issuer"] != "gamerstriperdev") return Fail(LicenseStatus::WrongIssuer, "License was not issued by gamerstriperdev.");
    if (fields["product"] != "nightwalker-rdr2") return Fail(LicenseStatus::WrongProduct, "License is for a different product.");
    if (!IsSafeToken(fields["license_id"], 8, 96) || !IsSafeToken(fields["customer_id"], 3, 96)) {
        return Fail(LicenseStatus::InvalidClaims, "License identity fields are invalid.");
    }

    std::uint64_t issued = 0;
    std::uint64_t expires = 0;
    if (!ParseU64(fields["issued_unix"], issued) || !ParseU64(fields["expires_unix"], expires)) {
        return Fail(LicenseStatus::InvalidClaims, "License timestamps are invalid.");
    }
    if (issued == 0 || issued > nowUnix + 86400ULL) return Fail(LicenseStatus::InvalidClaims, "License issue time is invalid.");
    if (expires != 0 && nowUnix > expires) return Fail(LicenseStatus::Expired, "Nightwalker license has expired.");

    const auto& device = fields["device_hash"];
    if (device != "ANY") {
        if (!IsHex64(device)) return Fail(LicenseStatus::InvalidClaims, "License device binding is invalid.");
        if (localDeviceHash.size() != 64 || !std::equal(device.begin(), device.end(), localDeviceHash.begin(), localDeviceHash.end(), [](char a, char b) {
                return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
            })) {
            return Fail(LicenseStatus::DeviceMismatch, "This Nightwalker license belongs to another PC.");
        }
    }

    std::vector<std::uint8_t> signature;
    if (!DecodeBase64(fields["signature"], signature) || signature.size() != 64) {
        return Fail(LicenseStatus::BadSignatureEncoding, "License signature encoding is invalid.");
    }

    std::string canonical;
    canonical.reserve(320);
    canonical += "schema=1\n";
    canonical += "issuer=gamerstriperdev\n";
    canonical += "product=nightwalker-rdr2\n";
    canonical += "license_id=" + fields["license_id"] + "\n";
    canonical += "customer_id=" + fields["customer_id"] + "\n";
    canonical += "issued_unix=" + fields["issued_unix"] + "\n";
    canonical += "expires_unix=" + fields["expires_unix"] + "\n";
    canonical += "device_hash=" + fields["device_hash"] + "\n";

    if (!verifySignature || !verifySignature(canonical, signature)) {
        return Fail(LicenseStatus::BadSignature, "Nightwalker license signature is not valid.");
    }

    LicenseResult result;
    result.status = LicenseStatus::Valid;
    result.claims.licenseId = fields["license_id"];
    result.claims.customerId = fields["customer_id"];
    result.claims.deviceHash = fields["device_hash"];
    result.claims.issuedUnix = issued;
    result.claims.expiresUnix = expires;
    result.message = "Nightwalker license accepted.";
    return result;
}

} // namespace nightwalker::licensing
