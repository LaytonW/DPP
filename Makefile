.PHONY: run clean

DPP: DPP.c
	gcc $^ -o $@ -Wall -pthread -ggdb

run: DPP
	chmod +x DPP
	./DPP

clean:
	$(RM) DPP
