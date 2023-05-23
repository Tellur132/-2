file=achlys
$(file).scr: $(file).c
	gcc -std=c99 -Wall $(file).c -o $(file).scr -lscrnsave -lgdi32 -lopengl32
s:
	$(file).scr s
