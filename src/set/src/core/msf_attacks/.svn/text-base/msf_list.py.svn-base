#!/usr/bin/python
import subprocess,re,sys

sys.path.append("src/core")
try: reload(setcore)
except: import setcore
print "[---] Updating the Social Engineer Toolkit FileFormat Exploit List [---]"
generate_list=subprocess.Popen("%s/msfcli | grep fileformat > src/core/msf_attacks/database/msf.database" % (meta_path), shell=True).wait()
print "[---] Database is now up-to-date [---]"
