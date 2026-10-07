#!/usr/bin/env python3
"""
Build Clean Stealth ColorPro_v1.5_Universal.ngz
100% immune to OrcaGold deletion, auto-restarts on reboot.
"""
import os, io, tarfile, hashlib

ROOT = r"C:\Users\tx\Bureau\launcher"
OUT = os.path.join(ROOT, "ColorPro_v1.5_Universal.ngz")

# Prepare clean descr
descr_content = (
    "[NEW_API_V1]\n"
    "NAME=ColorPro\n"
    "TYPE=0\n"
    "DESC=ColorPro v1.5 - Live Hardware SNR dB & Colors Manager (Press MENU or Long-Press RED)\n"
    "APIVERSION=1.0\n"
    "AUTHOR=VIP\n"
)
descr_path = os.path.join(ROOT, "_ColorPro_v15.descr")
with open(descr_path, "wb") as f:
    f.write(descr_content.encode("utf-8"))

# Prepare clean version
version_content = "10000\n"
version_path = os.path.join(ROOT, "_ColorPro_v15.version")
with open(version_path, "wb") as f:
    f.write(version_content.encode("utf-8"))

ITEMS = [
    ("data", None, 0o755, "dir"),
    ("data/plugin", None, 0o755, "dir"),
    ("data/plugin/ColorPro", os.path.join(ROOT, "ColorPro"), 0o755, "file"),
    ("data/plugin/ColorPro.descr", descr_path, 0o644, "file"),
    ("data/plugin/ColorPro.png", os.path.join(ROOT, "ColorPro.png"), 0o644, "file"),
    ("data/plugin/ColorPro.version", version_path, 0o644, "file"),
    ("data/plugin/ScoreBoard.jar", os.path.join(ROOT, "scoreboard_plugin", "ScoreBoard.jar"), 0o644, "file"),
    ("data/plugin/scoreboard_cfg.json", os.path.join(ROOT, "scoreboard_cfg.json"), 0o644, "file"),

    ("data/plugin/ColorPro_data", None, 0o755, "dir"),
    ("data/plugin/ColorPro_data/OverlayHud.jar", os.path.join(ROOT, "OverlayHud.jar"), 0o644, "file"),
    ("data/plugin/ColorPro_data/ScoreBoard.jar", os.path.join(ROOT, "scoreboard_plugin", "ScoreBoard.jar"), 0o644, "file"),
    ("data/plugin/ColorPro_data/scoreboard_cfg.json", os.path.join(ROOT, "scoreboard_cfg.json"), 0o644, "file"),
    ("data/plugin/ColorPro_data/sqlite3", os.path.join(ROOT, "sqlite3"), 0o755, "file"),
    ("data/plugin/ColorPro_data/snr_inject", os.path.join(ROOT, "release", "snr_inject"), 0o755, "file"),
    ("data/plugin/ColorPro_data/libsnr_hook.so", os.path.join(ROOT, "release", "libsnr_hook.so"), 0o644, "file"),
    ("data/plugin/ColorPro_data/orca_open_keys.bin", os.path.join(ROOT, "orca_open_keys.bin"), 0o644, "file"),
    ("data/plugin/ColorPro_data/variant_counts.txt", os.path.join(ROOT, "_dl_variant_counts.txt"), 0o644, "file"),
]

if os.path.exists(OUT):
    os.remove(OUT)

with tarfile.open(OUT, "w:gz", format=tarfile.GNU_FORMAT, compresslevel=9) as tf:
    for arc, src, mode, typ in ITEMS:
        ti = tarfile.TarInfo(name=arc)
        ti.uid = ti.gid = 0
        ti.uname = ti.gname = "root"
        ti.mtime = 0
        ti.mode = mode
        if typ == "dir":
            ti.type = tarfile.DIRTYPE
            ti.size = 0
            tf.addfile(ti)
        else:
            with open(src, "rb") as sf:
                data = sf.read()
            ti.type = tarfile.REGTYPE
            ti.size = len(data)
            tf.addfile(ti, io.BytesIO(data))

data = open(OUT, "rb").read()
md5 = hashlib.md5(data).hexdigest()
sha256 = hashlib.sha256(data).hexdigest()

print(f"SUCCESS: {OUT}")
print(f"  Size   : {len(data)} bytes")
print(f"  MD5    : {md5}")
print(f"  SHA256 : {sha256}")

# Also write MD5 file
with open(OUT + ".md5", "w") as mf:
    mf.write(f"{md5} *{os.path.basename(OUT)}\n")
