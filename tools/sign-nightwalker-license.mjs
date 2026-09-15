#!/usr/bin/env node
import { readFileSync, writeFileSync } from 'node:fs';
import { sign } from 'node:crypto';

function arg(name, fallback = '') {
  const index = process.argv.indexOf(`--${name}`);
  return index >= 0 && index + 1 < process.argv.length ? process.argv[index + 1] : fallback;
}

function requireToken(name, value) {
  if (!/^[A-Za-z0-9_.-]+$/.test(value)) throw new Error(`${name} must contain only letters, numbers, _, -, or .`);
}

const privateKeyPath = arg('private-key');
const licenseId = arg('license-id');
const customerId = arg('customer-id');
const deviceHash = arg('device-hash');
const expiresUnix = arg('expires-unix', '0');
const output = arg('output', 'Nightwalker.license');
const issuedUnix = arg('issued-unix', String(Math.floor(Date.now() / 1000)));

if (!privateKeyPath || !licenseId || !customerId || !deviceHash) {
  console.error('Usage: node tools/sign-nightwalker-license.mjs --private-key <pem> --license-id <id> --customer-id <id> --device-hash <64hex|ANY> [--expires-unix 0] [--output Nightwalker.license]');
  process.exit(2);
}

requireToken('license-id', licenseId);
requireToken('customer-id', customerId);
if (deviceHash !== 'ANY' && !/^[0-9a-fA-F]{64}$/.test(deviceHash)) throw new Error('device-hash must be ANY or exactly 64 hexadecimal characters');
if (!/^\d+$/.test(issuedUnix) || !/^\d+$/.test(expiresUnix)) throw new Error('issued-unix/expires-unix must be unsigned integers');

const canonical = [
  'schema=1',
  'issuer=gamerstriperdev',
  'product=nightwalker-rdr2',
  `license_id=${licenseId}`,
  `customer_id=${customerId}`,
  `issued_unix=${issuedUnix}`,
  `expires_unix=${expiresUnix}`,
  `device_hash=${deviceHash}`,
  '',
].join('\n');

const privateKey = readFileSync(privateKeyPath, 'utf8');
const signature = sign('sha256', Buffer.from(canonical, 'utf8'), {
  key: privateKey,
  dsaEncoding: 'ieee-p1363',
});
if (signature.length !== 64) throw new Error(`Expected a 64-byte P-256 signature, got ${signature.length}`);

const license = `${canonical}signature=${signature.toString('base64')}\n`;
writeFileSync(output, license, { encoding: 'utf8', mode: 0o600 });
console.log(`Wrote ${output}`);
console.log(`license_id=${licenseId}`);
console.log(`customer_id=${customerId}`);
console.log(`device_hash=${deviceHash}`);
