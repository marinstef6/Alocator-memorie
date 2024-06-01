#pragma once
#include <inttypes.h>
#include <stddef.h>

/* TODO : add your implementation for doubly-linked list */
typedef struct list_elem {
	void *elem;
	struct list_elem *prev;
	struct list_elem *next;
} list_elem;

typedef struct {
	list_elem *head;
	size_t size;
} list_t;

typedef struct {
	uint64_t start_address;
	size_t size;
	list_t *miniblock_list;
} block_t;

typedef struct {
	uint64_t start_address;
	size_t size;
	uint8_t perm;
	void *rw_buffer;
} miniblock_t;

typedef struct {
	uint64_t arena_size;
	list_t *alloc_list;
} arena_t;

arena_t *alloc_arena(const uint64_t size);
void dealloc_arena(arena_t *arena);

void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size);
void free_block(arena_t *arena, const uint64_t address);

void read(arena_t *arena, uint64_t address, uint64_t size);
void write(arena_t *arn, const uint64_t adr, const uint64_t size, int8_t *dta);
void pmap(const arena_t *arena);
void mprotect(arena_t *arena, uint64_t address, int8_t *permission);

void print_blocks(const arena_t *arena);
void print_block(block_t *block, int i);
void print_miniblock(miniblock_t *mini, int i);
uint64_t count_miniblocks(const arena_t *arena);
uint64_t get_occupied_memory(const arena_t *arena);
list_elem *get_miniblock_contain_addr(list_t *list, const uint64_t address);
void split_block_if_necessary(list_t *block_list, block_t *block);
void shrink_list(list_t *list);
void shrink_blocks(list_t *list, block_t *block, list_elem *next_bl);
list_elem *get_block_contain_addr(arena_t *arena, const uint64_t address);
void free_block_t(block_t *block);
void add_miniblock_to_block(block_t *block, miniblock_t *miniblock);
void rearrange_list(list_t *list, int list_type);
block_t *create_block(const uint64_t address);
void free_miniblock(miniblock_t *miniblock);
miniblock_t *create_minibloc(const uint64_t address, const uint64_t size);
void remove_from_list(list_t *list, list_elem *e);
void add_to_list(list_t *list, void *elem);
list_elem *create_list_elem(void *elem);
list_t *create_list(void);
