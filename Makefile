CC=gcc
CFLAGS=-Wall -Wextra -std=c99
EXNAME=main
LANGUAGE=c
build: $(EXNAME).$(LANGUAGE)
	@$(CC) -o vma $(EXNAME).$(LANGUAGE) $(CFLAGS)

run_vma:
	@./$(EXNAME)

.PHONY: clean

clean:
	@rm vma
