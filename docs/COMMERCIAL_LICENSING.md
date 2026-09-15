# gamerstriperdev commercial licensing

Nightwalker commercial builds use a signed local license file named `Nightwalker.license` placed beside `Nightwalker.asi`.

## Trust model

- The customer build contains only the gamerstriperdev **public** ECDSA P-256 verification key.
- The private signing key must never be committed, bundled with the mod, uploaded to GitHub, or embedded in the website client.
- The seller/website signs licenses server-side.
- The ASI verifies the signature locally before any gameplay systems initialize.
- A missing, expired, malformed, wrong-product, wrong-device, or forged license prevents the paid mod from starting.
- Debug builds define `NIGHTWALKER_DEV_LICENSE_BYPASS=1`. Release builds do not.

This is intended to stop casual copying/key generation. No client-side DRM can make a native PC mod impossible to patch, so do not add invasive drivers, anti-debug malware, or personal-data collection.

## License format

Schema 1 is UTF-8 text:

```text
schema=1
issuer=gamerstriperdev
product=nightwalker-rdr2
license_id=NW-XXXXXXXX
customer_id=CUSTOMER-XXXXXXXX
issued_unix=1789491666
expires_unix=0
device_hash=<64-hex device id or ANY>
signature=<base64 raw P-256 r||s signature>
```

`expires_unix=0` means perpetual. Production customer licenses should normally use the 64-hex device id rather than `ANY`.

The signature covers the canonical first eight lines, including the trailing newline after `device_hash`. It uses SHA-256 + ECDSA P-256 and IEEE-P1363/raw `r||s` signature encoding.

## Customer activation flow

1. Customer purchases Nightwalker from gamerstriperdev.
2. Customer installs the normal Nightwalker package and launches once without a license.
3. The ASI writes `Nightwalker.device.txt` beside itself. This contains a one-way product-scoped SHA-256 device id derived locally from Windows MachineGuid.
4. Customer enters that device id on the gamerstriperdev activation page.
5. The server checks purchase entitlement and activation allowance.
6. The server signs a device-bound `Nightwalker.license` using the private key.
7. Customer downloads the license beside `Nightwalker.asi` and launches the game.

The ASI never needs the private key and does not need to contact the server during gameplay.

## Activation limits

Activation counts are a **server policy**, not something the offline ASI can enforce by itself. A practical default is 2 or 3 active PCs per purchase. The server can revoke/reissue activations by deciding whether to sign another device-bound license.

## Manual license issuing before the website exists

A developer-only Node.js signer is provided:

```text
node tools/sign-nightwalker-license.mjs \
  --private-key /secure/path/nightwalker_private.pem \
  --license-id NW-00000001 \
  --customer-id ORDER-00000001 \
  --device-hash <64hex> \
  --output Nightwalker.license
```

Node's built-in crypto module is used; no runtime dependency is added to the mod.

## Packaging

Do not put a universal `Nightwalker.license` in the public/customer base ZIP. The store delivery layer should either:

- provide the standard Nightwalker ZIP plus the customer's signed `Nightwalker.license`, or
- construct a customer-specific ZIP that adds that license file.

`Nightwalker.license` is intentionally not stored in source control.

## Key rotation

The current public key is compiled into the verifier. Rotating the signing key requires shipping a new ASI containing the replacement public key (or later extending the verifier to support multiple key ids during a transition period).
