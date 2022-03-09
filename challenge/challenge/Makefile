all:
	clang mte_wrappers.c diylist.c main.c -o chg -fpie -pie -fstack-protector -Wl,-z,relro,-z,now -march=armv8.5-a+memtag
