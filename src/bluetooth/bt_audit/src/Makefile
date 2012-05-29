all: psm_scan rfcomm_scan

psm_scan: psm_scan.c
	gcc psm_scan.c -o psm_scan -lbluetooth

rfcomm_scan: rfcomm_scan.c
	gcc rfcomm_scan.c -o rfcomm_scan -lbluetooth

clean:
	rm -f psm_scan
	rm -f rfcomm_scan
