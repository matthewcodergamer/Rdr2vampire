#include "nightwalker/licensing/License.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

using namespace nightwalker::licensing;

namespace {

std::string BaseLicense(std::string_view device = "ANY", std::string_view expires = "0", std::string_view signature = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA==") {
    std::string text;
    text += "schema=1\n";
    text += "issuer=gamerstriperdev\n";
    text += "product=nightwalker-rdr2\n";
    text += "license_id=NW-TEST-0001\n";
    text += "customer_id=CUSTOMER-001\n";
    text += "issued_unix=1789490000\n";
    text += "expires_unix=" + std::string(expires) + "\n";
    text += "device_hash=" + std::string(device) + "\n";
    text += "signature=" + std::string(signature) + "\n";
    return text;
}

} // namespace

int main() {
    const std::uint64_t now = 1789491666ULL;
    std::string capturedPayload;
    auto accept = [&](std::string_view payload, std::span<const std::uint8_t> signature) {
        capturedPayload.assign(payload);
        return signature.size() == 64;
    };

    auto valid = ValidateLicenseText(BaseLicense(), std::string(64, 'a'), now, accept);
    assert(valid.IsValid());
    assert(valid.claims.licenseId == "NW-TEST-0001");
    assert(capturedPayload.find("signature=") == std::string::npos);
    assert(capturedPayload.ends_with("device_hash=ANY\n"));

    const std::string device(64, 'a');
    auto bound = ValidateLicenseText(BaseLicense(device), device, now, accept);
    assert(bound.IsValid());

    auto wrongDevice = ValidateLicenseText(BaseLicense(device), std::string(64, 'b'), now, accept);
    assert(wrongDevice.status == LicenseStatus::DeviceMismatch);

    auto expired = ValidateLicenseText(BaseLicense("ANY", "1789490001"), device, now, accept);
    assert(expired.status == LicenseStatus::Expired);

    auto wrongIssuerText = BaseLicense();
    const auto issuer = wrongIssuerText.find("issuer=gamerstriperdev");
    wrongIssuerText.replace(issuer, std::string("issuer=gamerstriperdev").size(), "issuer=somebodyelse");
    auto wrongIssuer = ValidateLicenseText(wrongIssuerText, device, now, accept);
    assert(wrongIssuer.status == LicenseStatus::WrongIssuer);

    auto reject = [](std::string_view, std::span<const std::uint8_t>) { return false; };
    auto badSignature = ValidateLicenseText(BaseLicense(), device, now, reject);
    assert(badSignature.status == LicenseStatus::BadSignature);

    auto duplicate = BaseLicense();
    duplicate += "product=nightwalker-rdr2\n";
    auto duplicateResult = ValidateLicenseText(duplicate, device, now, accept);
    assert(duplicateResult.status == LicenseStatus::ParseError);

    std::cout << "Nightwalker commercial license tests passed\n";
    return 0;
}
