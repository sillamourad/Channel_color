#!/system/bin/sh
# Installer for Live Hardware SNR & Signal Monitor Hook (Availink AVL6211/6261)
# Compatible with Icone Iron, Iron Pro, Iron Plus, and Wegoo

mount -o remount,rw /system 2>/dev/null

echo "[+] Installing libsnr_hook.so..."
curl -k -s -L -o /system/lib/libsnr_hook.so https://raw.githubusercontent.com/sillamourad/Channel_color/main/release/libsnr_hook.so
chmod 644 /system/lib/libsnr_hook.so

echo "[+] Setting up f_server hook..."
if [ -f "/system/bin/f_server" ] && [ ! -f "/system/bin/f_server_real" ]; then
    cp -f /system/bin/f_server /system/bin/f_server_real
    chmod 755 /system/bin/f_server_real
fi

if [ -f "/system/bin/f_server_real" ]; then
    cat << 'EOF' > /system/bin/f_server
#!/system/bin/sh
export LD_PRELOAD=/system/lib/libsnr_hook.so
exec /system/bin/f_server_real "$@"
EOF
    chmod 755 /system/bin/f_server
fi

sync
mount -o remount,ro /system 2>/dev/null

echo "[+] Restarting f_server..."
killall -9 f_server 2>/dev/null || pkill -9 -f f_server 2>/dev/null
sleep 2

if [ -f "/data/.snr_value.txt" ]; then
    echo "[OK] Live Hardware SNR & Signal Active!"
    cat /data/.snr_value.txt
else
    echo "[!] f_server restarted, please tune to any satellite channel."
fi
