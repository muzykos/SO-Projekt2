OBJ = main main2

all: $(OBJ)
$(OBJ): %: %.c
	$(CC) $(CFLAGS) -pthread -g -lrt -o $@ $<


.PHONY: clean
clean:
	rm -f *.o main
