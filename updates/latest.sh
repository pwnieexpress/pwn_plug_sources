echo "downloading latest..."
wget http://pub.pwnieexpress.com/pwnplug_patch_1.1.3.tar.gz

echo "checking patch hash..."
hash=2b341317357f178e5b0335e6425de5b8762c8d23
sha1sum pwnplug_patch_1.1.3.tar.gz > temp_hash
if [ `grep -o ${hash} temp_hash` ]; then
    echo "extracting latest patch"
    tar -zxvf pwnplug_patch_1.1.3.tar.gz
    echo "running latest patch"
    cd pwnplug_patch_1.1.3
    chmod +x pwnplug_patch_1.1.3.sh
    ./pwnplug_patch_1.1.3.sh
else
  echo "false"
fi

# Cleaning up... 
echo "cleaning up..."
rm temp_hash

echo "done"