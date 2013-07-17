echo "[+] downloading latest..."
wget http://pub.pwnieexpress.com/pwnplug_patch_1.1.3.tar.gz

echo "[+] checking patch hash..."
hash=40cde500c51793865956a60e390fee262e9e5705
sha1sum pwnplug_patch_1.1.3.tar.gz > temp_hash
if [ `grep -o ${hash} temp_hash` ]; then
    echo "[+] extracting latest patch"
    tar -zxvf pwnplug_patch_1.1.3.tar.gz
    echo "[+] running latest patch"
    cd pwnplug_patch_1.1.3
    chmod +x ./pwnplug_patch_1.1.3.sh
    ./pwnplug_patch_1.1.3.sh
    cd ..
else
  echo "[-] unable to verify hash of latest patch"
fi

# Cleaning up... 
echo "[+] cleaning up..."
rm ./temp_hash

echo "[+] done"
