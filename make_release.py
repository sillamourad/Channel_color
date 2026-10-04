#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
make_release.py - ChannelColor Release & Key Management Tool
"""

import os
import sys
import argparse
import datetime
import hashlib
import shutil
import nacl.signing

KEYS_DIR = r"C:\Users\tx\keys"
DEFAULT_PRIV_KEY = os.path.join(KEYS_DIR, "update_private.key")
DEFAULT_PUB_KEY = os.path.join(KEYS_DIR, "update_public.key")
RELEASE_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "release")

# manifest.txt / min_version are ALWAYS integers:  major*10000 + minor*100
#   "1.0" -> 10000 , "1.1" -> 10100 , "2.0" -> 20000
# legacy 4-digit codes (5001, 7003) are accepted as-is.
SEMVER_FACTOR_MAJOR = 10000
SEMVER_FACTOR_MINOR = 100


def parse_version_arg(value: str) -> int:
    """argparse type: accept a human version '1.0' or a raw code (10000 / 7003)."""
    s = str(value).strip()
    if "." in s:
        parts = s.split(".")
        if len(parts) != 2:
            raise argparse.ArgumentTypeError(
                f"invalid version '{value}' (expected major.minor, e.g. 1.0)")
        try:
            major = int(parts[0])
            minor = int(parts[1])
        except ValueError:
            raise argparse.ArgumentTypeError(f"invalid version '{value}' (non numeric)")
        if major < 0 or minor < 0 or minor > 99:
            raise argparse.ArgumentTypeError(
                f"invalid version '{value}' (major >= 0, minor 0..99)")
        return major * SEMVER_FACTOR_MAJOR + minor * SEMVER_FACTOR_MINOR
    try:
        n = int(s)
    except ValueError:
        raise argparse.ArgumentTypeError(
            f"invalid version '{value}' (expected '1.0' or a number like 10000)")
    if n < 1:
        raise argparse.ArgumentTypeError(f"invalid version '{value}' (must be >= 1)")
    return n


def version_display(num: int) -> str:
    """Best effort human form of a version number (raw code if not semver shaped)."""
    if num >= SEMVER_FACTOR_MAJOR and num % SEMVER_FACTOR_MINOR == 0:
        return f"{num // SEMVER_FACTOR_MAJOR}.{(num % SEMVER_FACTOR_MAJOR) // SEMVER_FACTOR_MINOR}"
    return str(num)


def format_c_array(pub_bytes: bytes) -> str:
    lines = []
    lines.append("static const uint8_t UPDATE_PUBKEY[32] = {")
    for i in range(0, 32, 8):
        chunk = pub_bytes[i:i+8]
        hex_vals = ", ".join(f"0x{b:02x}" for b in chunk)
        if i + 8 < 32:
            lines.append(f"    {hex_vals},")
        else:
            lines.append(f"    {hex_vals}")
    lines.append("};")
    return "\n".join(lines)


def load_private_key(key_path: str) -> nacl.signing.SigningKey:
    if not os.path.exists(key_path):
        raise FileNotFoundError(f"Private key file not found: {key_path}")
    with open(key_path, "rb") as f:
        data = f.read().strip()
    if len(data) == 32:
        return nacl.signing.SigningKey(data)
    elif len(data) == 64:
        # Hex encoded 32 bytes or 64-byte key
        try:
            raw = bytes.fromhex(data.decode("ascii"))
            if len(raw) == 32:
                return nacl.signing.SigningKey(raw)
        except Exception:
            pass
        return nacl.signing.SigningKey(data[:32])
    elif len(data) == 128:
        # Hex encoded 64 bytes
        raw = bytes.fromhex(data.decode("ascii"))
        return nacl.signing.SigningKey(raw[:32])
    else:
        raise ValueError(f"Invalid private key size: {len(data)} bytes")


def cmd_genkey(args):
    os.makedirs(KEYS_DIR, exist_ok=True)
    priv_path = os.path.abspath(args.key_out or DEFAULT_PRIV_KEY)
    pub_path = os.path.abspath(args.pub_out or DEFAULT_PUB_KEY)

    if os.path.exists(priv_path) and not args.force:
        print(f"ERROR: Private key already exists at: {priv_path}", file=sys.stderr)
        print("Use --force to overwrite.", file=sys.stderr)
        sys.exit(1)

    signing_key = nacl.signing.SigningKey.generate()
    verify_key = signing_key.verify_key

    # Save private key (32 bytes seed)
    with open(priv_path, "wb") as f:
        f.write(signing_key.encode())

    # Save public key (hex)
    pub_hex = verify_key.encode().hex()
    with open(pub_path, "w", encoding="ascii") as f:
        f.write(pub_hex + "\n")

    c_array = format_c_array(verify_key.encode())

    print("=" * 60)
    print("Ed25519 Keypair Generated Successfully")
    print("=" * 60)
    print(f"Private key path: {priv_path}")
    print(f"Public key path:  {pub_path}")
    print(f"Public key (hex): {pub_hex}")
    print("\nC array for channelcolor_v5.cpp:")
    print(c_array)
    print("=" * 60)


def create_gitattributes():
    proj_dir = os.path.dirname(os.path.abspath(__file__))
    gitattr_path = os.path.join(proj_dir, ".gitattributes")
    content = (
        "manifest.txt -text\n"
        "manifest.sig -text\n"
        "ChannelColor -text\n"
        "OverlayHud.jar -text\n"
    )
    with open(gitattr_path, "w", encoding="utf-8", newline="\n") as f:
        f.write(content)
    print(f"Created/updated .gitattributes at: {gitattr_path}")


def cmd_release(args):
    # Verify files exist in current directory or specified paths
    files_to_pack = [args.binary]
    if args.jar:
        files_to_pack.append(args.jar)

    for fp in files_to_pack:
        if not os.path.isfile(fp):
            print(f"ERROR: File not found: {fp}", file=sys.stderr)
            sys.exit(1)

    priv_path = os.path.abspath(args.key or DEFAULT_PRIV_KEY)
    if not os.path.exists(priv_path):
        print(f"ERROR: Private key not found at {priv_path}", file=sys.stderr)
        print("Run 'python make_release.py genkey' first.", file=sys.stderr)
        sys.exit(1)

    signing_key = load_private_key(priv_path)

    os.makedirs(RELEASE_DIR, exist_ok=True)

    # Calculate sizes and SHA-512
    file_records = []
    for fp in files_to_pack:
        fname = os.path.basename(fp)
        fsize = os.path.getsize(fp)
        hasher = hashlib.sha512()
        with open(fp, "rb") as f:
            while chunk := f.read(65536):
                hasher.update(chunk)
        fhash = hasher.hexdigest().lower()
        file_records.append((fname, fsize, fhash, fp))

    # Format manifest.txt
    rel_date = datetime.date.today().strftime("%Y-%m-%d")
    manifest_lines = [
        f"version={args.version}",
        f"min_version={args.min}",
        f"released={rel_date}",
        f"notes={args.notes.strip()}",
    ]
    for fname, fsize, fhash, _ in file_records:
        manifest_lines.append(f"file={fname}|{fsize}|{fhash}")

    manifest_content = "\n".join(manifest_lines) + "\n"
    manifest_bytes = manifest_content.encode("utf-8")

    manifest_path = os.path.join(RELEASE_DIR, "manifest.txt")
    with open(manifest_path, "wb") as f:
        f.write(manifest_bytes)

    # Sign exact bytes
    signed = signing_key.sign(manifest_bytes)
    sig_hex = signed.signature.hex()
    sig_path = os.path.join(RELEASE_DIR, "manifest.sig")
    with open(sig_path, "wb") as f:
        f.write(sig_hex.encode("ascii"))

    # Copy files
    for fname, _, _, src_path in file_records:
        dst_path = os.path.join(RELEASE_DIR, fname)
        if os.path.abspath(src_path) != os.path.abspath(dst_path):
            shutil.copy2(src_path, dst_path)

    # Update .gitattributes
    create_gitattributes()

    print("=" * 60)
    print(f"Release v{version_display(args.version)} ({args.version}) Generated Successfully in:")
    print(f"  {RELEASE_DIR}")
    print("=" * 60)
    print("--- manifest.txt ---")
    print(manifest_content.strip())
    print("--- manifest.sig ---")
    print(sig_hex.strip())
    print("=" * 60)
    for fname, fsize, fhash, _ in file_records:
        print(f"Packaged: {fname} ({fsize} bytes) SHA-512: {fhash[:16]}...")
    print("=" * 60)


def main():
    parser = argparse.ArgumentParser(description="ChannelColor Release Tool")
    subparsers = parser.add_subparsers(dest="subcommand", required=True)

    # genkey subcommand
    p_genkey = subparsers.add_parser("genkey", help="Generate Ed25519 keypair")
    p_genkey.add_argument("--key-out", default=DEFAULT_PRIV_KEY, help="Path for private key output")
    p_genkey.add_argument("--pub-out", default=DEFAULT_PUB_KEY, help="Path for public key output")
    p_genkey.add_argument("--force", action="store_true", help="Overwrite existing private key")
    p_genkey.set_defaults(func=cmd_genkey)

    # release subcommand
    p_rel = subparsers.add_parser("release", help="Build release manifest and sign")
    p_rel.add_argument("--version", type=parse_version_arg, required=True,
                       help='Release version as "1.0" (written to manifest as 10000) or a raw code (e.g. 7003)')
    p_rel.add_argument("--min", type=parse_version_arg, required=True,
                       help='Minimum supported version as "1.0" (written as 10000) or a raw code (e.g. 5001)')
    p_rel.add_argument("--notes", type=str, required=True, help="Release notes (single line)")
    p_rel.add_argument("--key", default=DEFAULT_PRIV_KEY, help="Path to private key")
    p_rel.add_argument("binary", help="Path to ChannelColor binary")
    p_rel.add_argument("jar", nargs="?", default=None, help="Optional path to OverlayHud.jar")
    p_rel.set_defaults(func=cmd_release)

    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
