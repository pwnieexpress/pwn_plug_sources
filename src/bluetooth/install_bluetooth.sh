#!/bin/bash

BT_ROOT=/var/pwnplug/bluetooth
BIN_DIR=/usr/bin/

#For testing
INPUT=0

echo "Install BlueZ"
read INPUT


# BlueZ
cd $BT_ROOT/bluez/
apt-get install libfuse-dev libdbus-1-3 libdbus-1-dev obexftp libobexftp* libdbus-glib-1-2 libdbus-glib-1-dev obexfs
./configure --host=arm-linux --enable-tools --enable-test --enable-usb --enable-hidd --enable-dund --enable-pand --enable-cups --prefix=/ 
make
make install
#mv /usr/etc/dbus-1/system.d/bluetooth.conf /etc/dbus-1/system.d/
cd test
# Link files not installed by default
ln -sf $BT_ROOT/bluez/test/agent $BIN_DIR/agent
ln -sf $BT_ROOT/bluez/test/list-devices $BIN_DIR/list-devices
ln -sf $BT_ROOT/bluez/test/bdaddr $BIN_DIR/bdaddr
ln -sf $BT_ROOT/bluez/test/attest $BIN_DIR/attest
ln -sf $BT_ROOT/bluez/test/hsplay $BIN_DIR/hsplay
ln -sf $BT_ROOT/bluez/test/l2test $BIN_DIR/l2test
ln -sf $BT_ROOT/bluez/test/hstest $BIN_DIR/hstest
ln -sf $BT_ROOT/bluez/test/simple-agent $BIN_DIR/simple-agent
ln -sf $BT_ROOT/bluez/test/monitor-bluetooth $BIN_DIR/monitor-bluetooth
# Start service
service dbus restart

#echo "Install Obexd"
#read INPUT
# Obexd
#cd $BT_ROOT/obexd
#./configure  --host=arm-linux --prefix=/usr
#make
#make install
#make clean

echo "Install HCIDump"
read INPUT

# HCIDump
cd $BT_ROOT/hcidump/
./configure  --host=arm-linux --prefix=/usr
make
make install
make clean

echo "Install Bluediving"
read INPUT

#BlueDiving
cd $BT_ROOT/bluediving/tools
gem install xml-simple
ln -sf $BT_ROOT/bluez/test/hsplay $BIN_DIR/play
#ln -sf $BT_ROOT/bluediving/bluedivingNG.pl $BIN_DIR/bluedivingNG.pl
./make_tools.sh

echo "Install BlueBugger"
read INPUT

# BlueBugger
cd $BT_ROOT/bluebugger/source
make
mv $BT_ROOT/bluebugger/source/bluebugger $BIN_DIR

echo "Install BlueLog"
read INPUT

# BlueLog
cd $BT_ROOT/bluelog
make
make install

echo "Install Bluesnarfer"
read INPUT

# BlueSnarfer
cd $BT_ROOT/bluesnarfer
make
mv $BT_ROOT/bluesnarfer/bluesnarfer $BIN_DIR

echo "Install BSS"
read INPUT

# Bluetooth Stack Smasher
cd $BT_ROOT/bss/source
make
mv $BT_ROOT/bss/source/bss $BT_ROOT/bss/

echo "Install BT_Audit"
read INPUT

# BT Audit
cd $BT_ROOT/bt_audit/src
make
mv $BT_ROOT/bt_audit/src/psm_scan $BIN_DIR
mv $BT_ROOT/bt_audit/src/rfcomm_scan $BIN_DIR

echo "Install Carwhisperer"
read INPUT

# CarWhisperer
cd $BT_ROOT/carwhisperer
make
ln -sf $BT_ROOT/carwhisperer/carwhisperer $BIN_DIR/carwhisperer

echo "Install HidAttack"
read INPUT

# HID Attack
cd $BT_ROOT/hidattack
make

echo "Install L2CAP Packet"
read INPUT

# L2CAP Packet
cd $BT_ROOT/l2cap-packet
make
make install

echo "Install RedFang"
read INPUT

# Redfang
cd $BT_ROOT/redfang/source
make
mv $BT_ROOT/redfang/source/fang $BIN_DIR
ln -sf $BIN_DIR/fang $BIN_DIR/redfang

echo "Install SpoofTooph"
read INPUT

# SpoofTooph
cd $BT_ROOT/spooftooph/source
make
make install
#mv $BT_ROOT/spooftooph/source/spooftooph $BT_ROOT/spooftooph/

echo "Install USSP-Push"
read INPUT

# USSP-Push
cd $BT_ROOT/ussp-push/source
./configure --host=arm-linux
make
make install
#cp $BT_ROOT/ussp-push/source/src/ussp-push $BT_ROOT/ussp-push/

echo "Install sobexsrv"
read INPUT

# sobexsrv
cd $BT_ROOT/sobexsrv/
make
make install

echo "Install PwnTooth"
read INPUT

# PwnTooth
cd $BT_ROOT/pwntooth/
make
make install
make clean

echo -e "\nDone installing Bluetooth on PwnPlug. \nHappy Hacking :)\n"
