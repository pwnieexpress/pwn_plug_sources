/*
   RedFang - The Bluetooth Device Hunter
   Copyright (C) 2003 @stake inc

   Written 2003 by Ollie Whitehouse <ollie@atstake.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 as
   published by the Free Software Foundation;

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.
   IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) AND AUTHOR(S) BE LIABLE FOR ANY CLAIM,
   OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER
   RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
   NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
   USE OR PERFORMANCE OF THIS SOFTWARE.

   ALL LIABILITY, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PATENTS, COPYRIGHTS,
   TRADEMARKS OR OTHER RIGHTS, RELATING TO USE OF THIS SOFTWARE IS DISCLAIMED.
*/

struct {
   char *name;
   char *addr;
   char *desc;
} manf[] = {
   {"3com",             "000BAC", "3Com Europe Ltd."},
   {"Ericsson",         "0001EC", "Ericsson Group (pre Sony-Ericsson)"},
   {"SE",               "008037", "Ericsson Group (Sony-Ericsson)"},
   {"SE2",              "000AD9", "Sony Ericsson Mobile Communications Ab"},
   {"murata",           "006057", "Murata Manufacturing Co., Ltd. (Nokia)"},
   {"Nokia",            "0002EE", "Nokia Danmark A/s"},
   {"tdk",              "008098", "TDK Corporation"},
   {"dlink",            "0080C8", "D-link Systems, Inc. (CSR Chipset)"},
   {"digianswer",       "0050CD", "Digianswer A/s"},
   {"Tecom",            "0003C9", "Tecom Co., Ltd."},
   {"apple",		"000393", "Apple Computer, Inc."},
   {"siwave",		"00033A", "Silicon Wave, Inc."},
   {"csr",		"00025B", "Cambridge Silicon Radio"},
   {"widcomm",		"000361", "Widcomm, Inc."},
   {"redm",		"000A1E", "Red-M (Communications) Limited"},
   {"billion",		"001060", "Billionton Systems, Inc."},
   {"Nokia2",		"00E003", "Nokia Wireless Business Communications"},
   {"alpsipaq",		"0002C7", "Alps Electric Co., Ltd. (Ipaq 38xx)"},
   {"intelbt",		"00D0B7", "Intel Corporation (Bluetooth)"},
   {"3com2",		"000476", "3 Com Corporation (Bluetooth)"},
   {"cmt",		"00308E", "Cross Match Technologies, Inc. (Axis)"},
   {"windigo",		"00081B", "Windigo Systems"},
   {"taiyo",		"00037A", "Taiyo Yuden Co., Ltd."},
   {"abocom",		"00E098", "AboCom Systems, Inc. (Palladio USB CSR Chipset)"},
   {"anicom",		"004005", "Ani Communications Inc."},
   {"palm",		"0007E0", "Palm Inc."},
   {0, 0}
};
