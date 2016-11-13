.PHONY: run clean

DPP: DPP.c
	gcc $^ -o $@ -Wall -pthread -ggdb

run: DPP
	chmod +x DPP
	./DPP 5 423785293 30

clean:
	$(RM) DPP
