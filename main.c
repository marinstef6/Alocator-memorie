#include "vma.h"
#include "vma.c"
#include <string.h>

int main(void)
{
	arena_t *arena = NULL;
	char command[50];
	do {
		scanf("%s", command);
		if (strcmp(command, "ALLOC_ARENA") == 0) {
			uint64_t size;
			scanf("%ld", &size);
			arena = alloc_arena(size);
		} else if (strcmp(command, "ALLOC_BLOCK") == 0) {
			uint64_t addr, size;
			scanf("%ld %ld", &addr, &size);
			alloc_block(arena, addr, size);
		} else if (strcmp(command, "FREE_BLOCK") == 0) {
			uint64_t addr;
			scanf("%ld", &addr);
			free_block(arena, addr);
		} else if (strcmp(command, "PMAP") == 0) {
			pmap(arena);
		} else if (strcmp(command, "READ") == 0) {
			uint64_t addr, size;
			scanf("%ld %ld", &addr, &size);
			read(arena, addr, size);
		} else if (strcmp(command, "WRITE") == 0) {
			char args[100];
			fgets(args, 100, stdin);
			char *p = strtok(args, " ");
			uint64_t addr = atoi(p), size = atoi(p = strtok(NULL, " "));
			while (p[0] != '\0')
				p++;
			p++;
			char buffer[size + 1];
			strcpy(buffer, p);
			while (strlen(buffer) < size) {
				fgets(args, 100, stdin);
				strcat(buffer, args);
			}
			write(arena, addr, size, (int8_t *)buffer);
		} else if (strcmp(command, "MPROTECT") == 0) {
			uint64_t addr;
			scanf("%ld ", &addr);
			char buffer[50];
			fgets(buffer, 50, stdin);
			int8_t permissions = 0;
			if (strstr(buffer, "PROT_READ"))
				permissions |= READ_MASK;
			if (strstr(buffer, "PROT_WRITE"))
				permissions |= WRITE_MASK;
			if (strstr(buffer, "PROT_EXEC"))
				permissions |= EXEC_MASK;
			mprotect(arena, addr, &permissions);
		} else if (strcmp(command, "DEALLOC_ARENA") != 0) {
			printf("Invalid command. Please try again.\n");
		} else {
			dealloc_arena(arena);
			break;
		}
	} while (1);
	return 0;
}
