all: l2cappacket l2cap_overflow

l2cappacket:
	gcc -lbluetooth l2cap-packet.c -o l2cap-packet

l2cap_overflow:
	gcc -lbluetooth l2cap_headersize_overflow.c -o l2cap_headersize_overflow

install:
	mv l2cap_headersize_overflow /usr/sbin
	mv l2cap-packet /usr/sbin 

clean:
	rm *~ l2cap-packet l2cap_headersize_overflow
