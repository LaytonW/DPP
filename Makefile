.PHONY: run clean

DPP: DPP.c
	gcc $^ -o $@ -Wall -pthread

run: DPP
	chmod +x DPP
	./DPP

clean:
	$(RM) DPP
