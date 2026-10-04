#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
build_universal_ngz.py
======================
Builds the standalone plug-and-play .ngz package for ICONE Iron, Iron Pro, Plus & WeGoo.
Automatically sets up:
  - ColorPro v1.4 daemon
  - Hardware SNR dB & Signal Monitor (embedded auto-install for f_server & libsnr_hook.so)
  - Color switching (Gold, Green, Red, Cyan, Normal)
  - Orca channel sync
  - Overlay HUD
"""

import os
import tarfile
import hashlib
import io

ROOT = os.path.dirname(os.path.abspath(__file__))
OUT_NGZ = os.path.join(ROOT, "ColorPro_v1.4_Universal.ngz")

# Descr content
DESCR = """[NEW_API_V1]
NAME=ColorPro
TYPE=0
DESC=ColorPro v1.4 - Live Hardware SNR dB & Colors Manager (Press MENU or Long-Press RED)
APIVERSION=1.0
AUTHOR=VIP
"""

# Version
VERSION = "1.4\n"

# Autorun script
AUTORUN = """/data/plugin/ColorPro
"""

def add_entry(tar, arcname, data, mode, is_dir=False):
    ti = tarfile.TarInfo(name=arcname)
    ti.mode = mode
    ti.uid = 0
    ti.gid = 0
    ti.uname = "root"
    ti.gname = "root"
    ti.mtime = 1791150000
    if is_dir:
        ti.type = tarfile.DIRTYPE
        tar.addfile(ti)
    else:
        ti.size = len(data)
        ti.type = tarfile.REGTYPE
        tar.addfile(ti, io.BytesIO(data))

def main():
    print(f"[*] Building {OUT_NGZ}...")
    
    with open(os.path.join(ROOT, "release", "ColorPro"), "rb") as f:
        bin_data = f.read()
    with open(os.path.join(ROOT, "OverlayHud.jar"), "rb") as f:
        hud_data = f.read()
    with open(os.path.join(ROOT, "sqlite3"), "rb") as f:
        sql_data = f.read()
    with open(os.path.join(ROOT, "orca_open_keys.bin"), "rb") as f:
        keys_data = f.read()
    with open(os.path.join(ROOT, "libsnr_hook.so"), "rb") as f:
        hook_data = f.read()
    with open(os.path.join(ROOT, "ColorPro.png"), "rb") as f:
        png_data = f.read()
        
    with tarfile.open(OUT_NGZ, "w:gz") as tar:
        # Directories
        add_entry(tar, "data", b"", 0o755, is_dir=True)
        add_entry(tar, "data/plugin", b"", 0o755, is_dir=True)
        add_entry(tar, "data/plugin/ColorPro_data", b"", 0o755, is_dir=True)
        add_entry(tar, "data/plugin/ChannelColor_data", b"", 0o755, is_dir=True)
        
        # ColorPro plugin files
        add_entry(tar, "data/plugin/ColorPro", bin_data, 0o755)
        add_entry(tar, "data/plugin/ColorPro.descr", DESCR.encode("utf-8"), 0o644)
        add_entry(tar, "data/plugin/ColorPro.png", png_data, 0o644)
        add_entry(tar, "data/plugin/ColorPro.version", VERSION.encode("utf-8"), 0o644)
        
        # Data directory files
        add_entry(tar, "data/plugin/ColorPro_data/OverlayHud.jar", hud_data, 0o644)
        add_entry(tar, "data/plugin/ColorPro_data/sqlite3", sql_data, 0o755)
        add_entry(tar, "data/plugin/ColorPro_data/orca_open_keys.bin", keys_data, 0o644)
        add_entry(tar, "data/plugin/ColorPro_data/libsnr_hook.so", hook_data, 0o644)
        add_entry(tar, "data/plugin/ColorPro_data/variant_counts.txt", b"gold=0\ngreen=0\nred=0\ncyan=0\n", 0o644)
        
        # Backward-compatible ChannelColor entries
        add_entry(tar, "data/plugin/ChannelColor", bin_data, 0o755)
        add_entry(tar, "data/plugin/ChannelColor.descr", DESCR.encode("utf-8"), 0o644)
        add_entry(tar, "data/plugin/ChannelColor.png", png_data, 0o644)
        add_entry(tar, "data/plugin/ChannelColor.version", VERSION.encode("utf-8"), 0o644)
        add_entry(tar, "data/plugin/ChannelColor_data/OverlayHud.jar", hud_data, 0o644)
        add_entry(tar, "data/plugin/ChannelColor_data/sqlite3", sql_data, 0o755)
        add_entry(tar, "data/plugin/ChannelColor_data/orca_open_keys.bin", keys_data, 0o644)
        add_entry(tar, "data/plugin/ChannelColor_data/libsnr_hook.so", hook_data, 0o644)
        add_entry(tar, "data/plugin/ChannelColor_data/variant_counts.txt", b"gold=0\ngreen=0\nred=0\ncyan=0\n", 0o644)

    # Compute MD5
    with open(OUT_NGZ, "rb") as f:
        ngz_bytes = f.read()
    md5 = hashlib.md5(ngz_bytes).hexdigest()
    
    with open(OUT_NGZ + ".md5", "w") as f:
        f.write(f"{md5} *{os.path.basename(OUT_NGZ)}\n")
        
    print(f"[OK] Successfully built: {OUT_NGZ}")
    print(f"    Size: {len(ngz_bytes)} bytes ({len(ngz_bytes)/1024/1024:.2f} MB)")
    print(f"    MD5:  {md5}")

if __name__ == "__main__":
    main()
