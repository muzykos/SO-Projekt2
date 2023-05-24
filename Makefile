OBJ = main

all: $(OBJ)
$(OBJ): %: %.c
	$(CC) $(CFLAGS) -pthread -o $@ $<


.PHONY: clean
clean:
	rm -f *.o main
