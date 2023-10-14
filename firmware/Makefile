build: FORCE
	idf.py -DCMAKE_EXPORT_COMPILE_COMMANDS=1 build

menuconfig:
	idf.py menuconfig

clean:
	idf.py clean
	touch build.file

fullclean:
	idf.py fullclean
	idf.py python-clean

flash:
	idf.py -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -p /dev/ttyUSB0 flash

monitor:
	idf.py -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -p /dev/ttyUSB0 monitor

fm:
	idf.py -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -p /dev/ttyUSB0 flash monitor

docs:
	idf.py docs

size:
	idf.py size

filesizes:
	idf.py -DOUTPUT_JSON=1 size-files

version:
	git log --pretty=format:"%h" -1 > version.txt

build.file:

FORCE:
