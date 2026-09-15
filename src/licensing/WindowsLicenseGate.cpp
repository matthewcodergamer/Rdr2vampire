#include "nightwalker/licensing/License.h"
#include "nightwalker/util/Logger.h"
#include <Windows.h>
#include <bcrypt.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#pragma comment(lib, "bcrypt.lib")

namespace nightwalker::licensing {
namespace {

struct AlgHandle final {
    BCRYPT_ALG_HANDLE value{nullptr};
    ~AlgHandle() { if (value) BCryptCloseAlgorithmProvider(value, 0); }
};
struct KeyHandle final {
    BCRYPT_KEY_HANDLE value{nullptr};
    ~KeyHandle() { if (value) BCryptDestroyKey(value); }
};
struct HashHandle final {
    BCRYPT_HASH_HANDLE value{nullptr};
    ~HashHandle() { if (value) BCryptDestroyHash(value); }
};

bool Sha256(std::span<const std::uint8_t> data, std::array<std::uint8_t, 32>& digest) {
    AlgHandle algorithm;
    if (BCryptOpenAlgorithmProvider(&algorithm.value, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) return false;
    DWORD objectLength = 0, bytes = 0;
    if (BCryptGetProperty(algorithm.value, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objectLength), sizeof(objectLength), &bytes, 0) != 0) return false;
    std::vector<std::uint8_t> object(objectLength);
    HashHandle hash;
    if (BCryptCreateHash(algorithm.value, &hash.value, object.data(), static_cast<ULONG>(object.size()), nullptr, 0, 0) != 0) return false;
    if (!data.empty() && BCryptHashData(hash.value, const_cast<PUCHAR>(data.data()), static_cast<ULONG>(data.size()), 0) != 0) return false;
    return BCryptFinishHash(hash.value, digest.data(), static_cast<ULONG>(digest.size()), 0) == 0;
}

std::string HexLower(std::span<const std::uint8_t> bytes) {
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (auto byte : bytes) out << std::setw(2) << static_cast<unsigned>(byte);
    return out.str();
}

bool ReadMachineGuid(std::wstring& guid) {
    HKEY key = nullptr;
    constexpr wchar_t kPath[] = L"SOFTWARE\\Microsoft\\Cryptography";
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, kPath, 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS) return false;
    std::array<wchar_t, 256> buffer{};
    DWORD type = 0;
    DWORD size = static_cast<DWORD>(buffer.size() * sizeof(wchar_t));
    const LONG status = RegQueryValueExW(key, L"MachineGuid", nullptr, &type, reinterpret_cast<LPBYTE>(buffer.data()), &size);
    RegCloseKey(key);
    if (status != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ)) return false;
    guid.assign(buffer.data());
    return !guid.empty();
}

std::string WideToUtf8(std::wstring_view text) {
    if (text.empty()) return {};
    const int length = static_cast<int>(text.size());
    const int needed = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), length, nullptr, 0, nullptr, nullptr);
    if (needed <= 0) return {};
    std::string utf8(static_cast<std::size_t>(needed), '\0');
    const int written = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), length, utf8.data(), needed, nullptr, nullptr);
    if (written != needed) return {};
    return utf8;
}

std::string DeviceHash() {
    std::wstring machineGuid;
    if (!ReadMachineGuid(machineGuid)) return {};
    const std::string utf8 = WideToUtf8(machineGuid);
    if (utf8.empty()) return {};
    const std::string source = "gamerstriperdev|nightwalker-rdr2|" + utf8;
    std::array<std::uint8_t, 32> digest{};
    if (!Sha256(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(source.data()), source.size()), digest)) return {};
    return HexLower(digest);
}

bool VerifyEcdsaP256(std::string_view payload, std::span<const std::uint8_t> signature) {
    if (signature.size() != 64) return false;

    static constexpr std::array<std::uint8_t, 32> kPublicX = {
        0xEF,0x61,0x4D,0xE8,0x5F,0x46,0x1E,0x82,0x5A,0x52,0x2F,0x8C,0xC7,0x83,0x02,0x9D,
        0x13,0x86,0x8A,0x70,0xB1,0x72,0x08,0x9E,0xE6,0x46,0x8C,0xF1,0x65,0xC9,0x02,0x41
    };
    static constexpr std::array<std::uint8_t, 32> kPublicY = {
        0xB9,0xF9,0x1D,0x80,0xC6,0x03,0x03,0xD2,0x79,0x0B,0x45,0xBF,0xBB,0x96,0x4D,0x4E,
        0xB1,0xC8,0xA5,0x4B,0x73,0x8E,0x5B,0xF4,0xE7,0x90,0xCB,0xD2,0x89,0x75,0x64,0x02
    };

    AlgHandle algorithm;
    if (BCryptOpenAlgorithmProvider(&algorithm.value, BCRYPT_ECDSA_P256_ALGORITHM, nullptr, 0) != 0) return false;

    std::vector<std::uint8_t> blob(sizeof(BCRYPT_ECCKEY_BLOB) + kPublicX.size() + kPublicY.size());
    auto* header = reinterpret_cast<BCRYPT_ECCKEY_BLOB*>(blob.data());
    header->dwMagic = BCRYPT_ECDSA_PUBLIC_P256_MAGIC;
    header->cbKey = static_cast<ULONG>(kPublicX.size());
    std::copy(kPublicX.begin(), kPublicX.end(), blob.begin() + sizeof(BCRYPT_ECCKEY_BLOB));
    std::copy(kPublicY.begin(), kPublicY.end(), blob.begin() + sizeof(BCRYPT_ECCKEY_BLOB) + kPublicX.size());

    KeyHandle key;
    if (BCryptImportKeyPair(algorithm.value, nullptr, BCRYPT_ECCPUBLIC_BLOB, &key.value, blob.data(), static_cast<ULONG>(blob.size()), 0) != 0) return false;

    std::array<std::uint8_t, 32> digest{};
    if (!Sha256(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size()), digest)) return false;

    return BCryptVerifySignature(
        key.value,
        nullptr,
        digest.data(),
        static_cast<ULONG>(digest.size()),
        const_cast<PUCHAR>(signature.data()),
        static_cast<ULONG>(signature.size()),
        0) == 0;
}

void WriteDeviceIdFile(const std::filesystem::path& pluginDirectory, std::string_view deviceHash) {
    if (deviceHash.empty()) return;
    std::ofstream out(pluginDirectory / L"Nightwalker.device.txt", std::ios::binary | std::ios::trunc);
    if (!out) return;
    out << "gamerstriperdev Nightwalker device id\n";
    out << "product=nightwalker-rdr2\n";
    out << "device_hash=" << deviceHash << "\n";
    out << "Use this device id when activating your Nightwalker purchase.\n";
}

std::string ReadAllText(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return {};
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

} // namespace

LicenseResult ValidateInstalledLicense(const std::filesystem::path& pluginDirectory, util::Logger& logger) {
#ifdef NIGHTWALKER_DEV_LICENSE_BYPASS
    logger.Write(util::LogLevel::Warning, "Commercial license verification bypassed by developer build flag.");
    LicenseResult result;
    result.status = LicenseStatus::Valid;
    result.message = "Developer license bypass active.";
    return result;
#else
    const std::string deviceHash = DeviceHash();
    if (deviceHash.empty()) {
        return {LicenseStatus::DeviceIdUnavailable, {}, "Could not derive this PC's Nightwalker device id."};
    }
    WriteDeviceIdFile(pluginDirectory, deviceHash);

    const auto licensePath = pluginDirectory / L"Nightwalker.license";
    if (!std::filesystem::is_regular_file(licensePath)) {
        return {LicenseStatus::Missing, {}, "Nightwalker.license is missing. Activate your gamerstriperdev purchase for this PC."};
    }

    const std::string text = ReadAllText(licensePath);
    if (text.empty()) return {LicenseStatus::ParseError, {}, "Nightwalker.license is empty or unreadable."};

    const auto now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    auto result = ValidateLicenseText(text, deviceHash, static_cast<std::uint64_t>(now), VerifyEcdsaP256);
    if (result.IsValid()) {
        logger.Write(util::LogLevel::Info, "gamerstriperdev commercial license accepted: " + result.claims.licenseId);
    }
    return result;
#endif
}

} // namespace nightwalker::licensing
