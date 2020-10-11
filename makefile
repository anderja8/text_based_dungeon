rooms:
	gcc -o anderja8.buildrooms anderja8.buildrooms.c

adventure:
	gcc -o anderja8.adventure anderja8.adventure.c -lpthread

game:
	make rooms
	make adventure

clean:
	rm -f anderja8.buildrooms anderja8.adventure currentTime.txt
	rm -rf anderja8.rooms.*
