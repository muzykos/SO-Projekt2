OBJ = main

all: $(OBJ)
$(OBJ): %: %.c
	$(CC) $(CFLAGS) -pthread -lrt -o $@ $<


.PHONY: clean
clean:
	rm -f *.o main
