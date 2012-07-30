all:
	mkdir -p bin
	make -C media
clean:
	make -C media clean
	rm -rf bin
install:
	make -C media install
