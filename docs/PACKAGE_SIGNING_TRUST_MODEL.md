# Fusion Package Signing and Publisher Trust — Design

## Goal

Add publisher authenticity on top of package format 2 integrity verification.
SHA-256 hashes detect payload changes relative to a manifest, but do not prove
who created that manifest. A detached digital signature will bind the manifest
and its file hashes to a trusted publisher key.

This document is a design contract. It does not claim that signatures are
implemented or that packages are currently authenticated.

## Proposed signed package format

Signed packages use `packageFormatVersion: 3` and retain the format 2
`files` array. They add these required manifest fields:

- `publisherId`: stable Fusion publisher identifier.
- `publisherKeyId`: identifier for the specific signing key.

The package contains a detached `mod.json.sig` file. It is a JSON object:

```json
{
  "algorithm": "Ed25519",
  "keyId": "publisher-key-2026-01",
  "signature": "base64-encoded 64-byte Ed25519 signature"
}
```

The signer signs the exact UTF-8 bytes of `mod.json`, without parsing and
re-serializing the JSON. This avoids ambiguous JSON canonicalization rules.
The signature file itself is not included in the `files` hash list. All
payload files remain covered by the manifest's SHA-256 entries.

The `keyId` in the detached signature must equal `publisherKeyId` in the
signed manifest. The declared algorithm must match the key registry entry;
algorithm downgrade or fallback is not permitted.

## Trust and verification policy

- A trusted publisher key is identified by publisher ID, key ID, algorithm,
  public key bytes, status, and validity interval.
- The trust registry is delivered through an authenticated Fusion update
  channel and verified independently of the mod package.
- The loader must never trust a public key merely because the package contains
  it.
- Online catalog installs require a valid signature from a currently trusted
  publisher key.
- Local developer/sideload packages may be allowed only through an explicit
  user choice and must be clearly marked untrusted; this path must not silently
  downgrade an online install.
- Unknown, revoked, expired, mismatched, malformed, or invalid signatures are
  rejected for online installs.
- Revocation must take precedence over cached package metadata.
- Key rotation publishes a new key and an overlap period, then retires the old
  key. Compromised keys can be revoked immediately.
- Publisher identity and account ownership are separate from possession of a
  signing key; recovery and key replacement require an audited publisher
  ownership process.

## Cryptographic implementation requirements

- Use Ed25519 with a maintained cryptographic implementation; do not write
  custom cryptography.
- Verify signatures before staging or loading package code.
- Bind publisher ID, publisher key ID, package metadata, and every payload
  hash by signing the complete manifest bytes.
- Compare signatures in the crypto library's verification routine.
- Keep private signing keys out of the game, installer, loader, repository,
  and client application. Signing belongs in a controlled publisher/release
  workflow.
- Do not embed a universal private key in Fusion.
- Define a key-registry format, authenticated update mechanism, rollback
  protection, and offline behavior before enabling mandatory signature
  enforcement.

## Compatibility and rollout

- Format 1 remains legacy and unsigned.
- Format 2 remains hash-verified but unauthenticated; it is suitable only for
  explicitly allowed local development or migration scenarios.
- Format 3 is the signed format intended for online catalog distribution.
- Online catalog policy must reject format 1 and format 2 packages once signed
  releases are enabled.
- Do not silently treat an unsigned package as trusted because its hashes pass.

## Required tests before release

- Valid signature from a trusted key.
- Signature mismatch after changing any manifest byte.
- Payload mismatch against a signed file hash.
- Unknown, revoked, expired, and wrong-publisher keys.
- Signature metadata key ID mismatch.
- Malformed Base64 and wrong signature length.
- Algorithm mismatch and unsupported algorithm.
- Key rotation overlap and immediate revocation.
- Online install rejects unsigned legacy formats.
- Local developer install clearly identifies an untrusted package.

## Explicit non-goals

This design does not yet implement signing, key distribution, publisher
registration, moderation, or trust UI. Those are separate implementation
milestones and must be reviewed before enabling online signed installs.
