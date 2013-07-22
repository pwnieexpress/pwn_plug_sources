echo "[+] Ensuring clean environment..."
rm pwnplug_patch_1.1.3.tar.gz*
rm -rf pwnplug_patch_1.1.3

echo "[+] Downloading latest patch..."
wget http://pub.pwnieexpress.com/pwnplug_patch_1.1.3.tar.gz

echo "[+] Checking patch hash..."
hash=f4c52c8242f7e24f2ede21da25e9050c13c5c0cd
sha1sum pwnplug_patch_1.1.3.tar.gz > temp_hash
if [ `grep -o ${hash} temp_hash` ]; then
    echo "[+] Extracting latest patch"
    tar -zxf pwnplug_patch_1.1.3.tar.gz
    echo "[+] Running latest patch"
    cd pwnplug_patch_1.1.3
    chmod +x ./pwnplug_patch_1.1.3.sh
    ./pwnplug_patch_1.1.3.sh
    cd ..
else
  echo "[-] Unable to verify hash of latest patch"
fi

# Cleaning up... 
echo "[+] Cleaning up..."
rm -rf pwnplug_patch_1.1.3
rm pwnplug_patch_1.1.3.tar.gz
rm ./temp_hash

echo "[+] Done"
