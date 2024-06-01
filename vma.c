#include "vma.h"
#include <stdlib.h>
#include <stdio.h>

#define READ_MASK 1
#define WRITE_MASK 2
#define EXEC_MASK 4

uint64_t min(uint64_t a, uint64_t b)
{
	return a > b ? b : a;
}

int debug;

list_t *create_list(void)
{
	list_t *list = (list_t *)malloc(sizeof(list_t));
	list->head = NULL;
	list->size = 0;
	return list;
}

list_elem *create_list_elem(void *elem)
{
	list_elem *e = malloc(sizeof(list_elem));
	e->elem = elem;
	e->prev = NULL;
	e->next = NULL;
	return e;
}

void add_to_list(list_t *list, void *elem)
{
	list_elem *e = create_list_elem(elem);
	e->next = list->head;
	if (list->head)
		list->head->prev = e;
	list->head = e;
	list->size = list->size + 1;
}

void remove_from_list(list_t *list, list_elem *e)
{
	if (list->size == 0)
		return;
	if (!e->prev) {		//head
		if (list->size > 1)
			e->next->prev = NULL;
		list->head = e->next;
	} else if (!e->next) {	//tail
		e->prev->next = NULL;
	} else {							// xi <-> e <-> xi+2
		e->prev->next = e->next;
		e->next->prev = e->prev;
	}
	free(e);
	list->size = list->size - 1;
}

miniblock_t *create_minibloc(const uint64_t address, const uint64_t size)
{
	miniblock_t *miniblock = (miniblock_t *)malloc(sizeof(miniblock_t));
	miniblock->start_address = address;
	miniblock->size = size;
	miniblock->perm = READ_MASK | WRITE_MASK;
	miniblock->rw_buffer = calloc(size, 1);
	return miniblock;
}

void free_miniblock(miniblock_t *miniblock)
{
	free(miniblock->rw_buffer);
	free(miniblock);
}

block_t *create_block(const uint64_t address)
{
	block_t *block = (block_t *)malloc(sizeof(block_t));
	block->start_address = address;
	block->size = 0;
	block->miniblock_list = create_list();
	return block;
}

void rearrange_list(list_t *list, int list_type)
{
	list_elem *tmp = list->head;
	while (tmp->next) {
		uint64_t tmp_start_addr;
		uint64_t next_start_addr;
		if (list_type == 1) {
			block_t *block = tmp->elem;
			tmp_start_addr = block->start_address;
			block = tmp->next->elem;
			next_start_addr = block->start_address;
		} else {
			miniblock_t *mini = tmp->elem;
			tmp_start_addr = mini->start_address;
			mini = tmp->next->elem;
			next_start_addr = mini->start_address;
		}
		if (next_start_addr < tmp_start_addr) {
			void *aux = tmp->elem;
			tmp->elem = tmp->next->elem;
			tmp->next->elem = aux;
		}
		tmp = tmp->next;
	}
}

void add_miniblock_to_block(block_t *block, miniblock_t *miniblock)
{
	block->size = block->size + miniblock->size;
	add_to_list(block->miniblock_list, miniblock);
	rearrange_list(block->miniblock_list, 2);
}

void free_block_t(block_t *block)
{
	if (!block)
		return;
	list_t *list = block->miniblock_list;
	while (list->size) {
		free_miniblock(list->head->elem);
		remove_from_list(list, list->head);
	}
	free(list);
	free(block);
}

arena_t *alloc_arena(const uint64_t size)
{
	arena_t *arena = (arena_t *)malloc(sizeof(arena_t));
	arena->arena_size = size;
	arena->alloc_list = create_list();
	return arena;
}

void dealloc_arena(arena_t *arena)
{
	while (arena->alloc_list->size) {
		free_block_t(arena->alloc_list->head->elem);
		remove_from_list(arena->alloc_list, arena->alloc_list->head);
	}
	free(arena->alloc_list);
	free(arena);
}

list_elem *get_block_contain_addr(arena_t *arena, const uint64_t address)
{
	list_t *list = arena->alloc_list;
	if (list->size == 0)
		return NULL;
	list_elem *x = list->head;
	while (x) {
		block_t *block = x->elem;
		uint64_t start_addr = block->start_address;
		uint64_t end_addr = start_addr + block->size;
		if (start_addr <= address && address < end_addr)
			return x;
		x = x->next;
	}
	return NULL;
}

void shrink_blocks(list_t *list, block_t *block, list_elem *next_bl)
{
	block_t *next_block = next_bl->elem;
	list_elem *current_miniblock = next_block->miniblock_list->head;
	while (current_miniblock) {
		list_elem *next = current_miniblock->next;
		add_miniblock_to_block(block, current_miniblock->elem);
		remove_from_list(next_block->miniblock_list, current_miniblock);
		current_miniblock = next;
	}
	free_block_t(next_block);
	remove_from_list(list, next_bl);
}

void shrink_list(list_t *list)
{
	if (list->size < 2)
		return;
	list_elem *e = list->head;
	while (e->next) {
		block_t *block = e->elem;
		uint64_t last_addr_e = block->start_address + block->size;
		block = e->next->elem;
		uint64_t first_addr_next = block->start_address;
		if (last_addr_e == first_addr_next) {
			shrink_blocks(list, e->elem, e->next);
			shrink_list(list);
			return;
		}
		e = e->next;
	}
}

void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size)
{
	if (address >= arena->arena_size) {
		printf("The allocated address is outside the size of arena\n");
		return;
	}
	if (address + size > arena->arena_size) {
		printf("The end address is past the size of the arena\n");
		return;
	}
	int is_memory_free = 1;
	// !!! a bad performance code->modify later (if tests don't pass)
	for (uint64_t i = address; i < address + size; i++)
		if (get_block_contain_addr(arena, i)) {
			is_memory_free = 0;
			break;
		}
	if (is_memory_free) {
		miniblock_t *miniblock = create_minibloc(address, size);
		block_t *block = create_block(address);
		add_miniblock_to_block(block, miniblock);
		add_to_list(arena->alloc_list, block);
		rearrange_list(arena->alloc_list, 1);
		shrink_list(arena->alloc_list);
	} else {
		printf("This zone was already allocated.\n");
		return;
	}
}

void split_block_if_necessary(list_t *block_list, block_t *block)
{
	if (block->miniblock_list->size < 2)
		return;
	list_elem *list = block->miniblock_list->head;
	uint64_t size = 0;
	while (list->next) {
		miniblock_t *miniblock = list->elem;
		size += miniblock->size;
		miniblock_t *next_mini = list->next->elem;
		if (miniblock->start_address + miniblock->size !=
				next_mini->start_address) {
			block_t *new_block = create_block(next_mini->start_address);
			list_elem *tmp = list->next;
			while (tmp) {
				add_miniblock_to_block(new_block, tmp->elem);
				list_elem *last = tmp;
				tmp = tmp->next;
				remove_from_list(block->miniblock_list, last);
			}
			add_to_list(block_list, new_block);
			block->size = size;
			rearrange_list(block_list, 1);
			return;
		}
		list = list->next;
	}
}

list_elem *get_miniblock_contain_addr(list_t *list, const uint64_t address)
{
	if (list->size == 0)
		return NULL;
	list_elem *x = list->head;
	while (x) {
		miniblock_t *mini = x->elem;
		uint64_t start_addr = mini->start_address;
		uint64_t end_addr = start_addr + mini->size;
		if (start_addr <= address && address < end_addr)
			return x;
		x = x->next;
	}
	return NULL;
}

void free_block(arena_t *arena, const uint64_t address)
{
	list_elem *block_e = get_block_contain_addr(arena, address);
	if (!block_e) {
		printf("Invalid address for free.\n");
		return;
	}
	block_t *block = block_e->elem;
	list_elem *miniblock =
		get_miniblock_contain_addr(block->miniblock_list, address);
	if (!miniblock) {
		printf("Invalid address for free.\n");
		return;
	}
	miniblock_t *mini = miniblock->elem;
	if (mini->start_address != address) {
		printf("Invalid address for free.\n");
		return;
	}
	free_miniblock(miniblock->elem);
	remove_from_list(block->miniblock_list, miniblock);
	if (block->miniblock_list->size == 0) {
		free_block_t(block);
		remove_from_list(arena->alloc_list, block_e);
	} else {
		split_block_if_necessary(arena->alloc_list, block);
	}
}

void read_from_block(block_t *block, uint64_t address, uint64_t size)
{
	list_elem *miniblock =
		get_miniblock_contain_addr(block->miniblock_list, address);
	char buffer[size + 1];
	uint64_t j = 0;
	while (size) {
		miniblock_t *mini = miniblock->elem;
		char *source = mini->rw_buffer;
		uint64_t last = min(mini->size - address + mini->start_address, size);
		uint64_t i = 0;
		for (; i < last; i++)
			buffer[j++] = source[address - mini->start_address + i];
		size -= i;
		address += i;
		miniblock = miniblock->next;
	}
	buffer[j] = 0;
	printf("%s\n", buffer);
}

void read(arena_t *arena, uint64_t address, uint64_t size)
{
	list_elem *block_e = get_block_contain_addr(arena, address);
	if (!block_e) {
		printf("Invalid address for read.\n");
		return;
	}
	block_t *block = block_e->elem;
	for (uint64_t i = address; i < address + size &&
		 i < (block->start_address + block->size); i++) {
		miniblock_t *mini =
			get_miniblock_contain_addr(block->miniblock_list, i)->elem;
		if ((mini->perm & READ_MASK) == 0) {
			printf("Invalid permissions for read.\n");
			return;
		}
	}
	if (size > block->size - (address - block->start_address)) {
		printf("Warning: size was bigger than the block size. ");
		printf("Reading %ld characters.\n",
			   block->size - (address - block->start_address));
		size = block->size - (address - block->start_address);
	}
	read_from_block(block, address, size);
}

void write_in_block(block_t *block, uint64_t address, uint64_t
					 size, int8_t *data)
{
	list_elem *miniblock =
		get_miniblock_contain_addr(block->miniblock_list, address);
	while (size) {
		miniblock_t *mini = miniblock->elem;
		char *dest = mini->rw_buffer;
		uint64_t last = min(mini->size - address + mini->start_address, size);
		uint64_t i = 0;
		for (; i < last; i++)
			dest[address - mini->start_address + i] = data[i];
		size -= i;
		data += i;
		address += i;
		miniblock = miniblock->next;
	}
}

void write(arena_t *arena, const uint64_t address, const uint64_t
					size, int8_t *data)
{
	uint64_t new_size = size;
	list_elem *block_e = get_block_contain_addr(arena, address);
	if (!block_e) {
		printf("Invalid address for write.\n");
		return;
	}
	block_t *block = block_e->elem;
	for (uint64_t i = address; i < address + size &&
		 i < (block->start_address + block->size); i++) {
		miniblock_t *mini =
			get_miniblock_contain_addr(block->miniblock_list, i)->elem;
		if ((mini->perm & WRITE_MASK) == 0) {
			printf("Invalid permissions for write.\n");
			return;
		}
	}

	if (size > block->size - (address - block->start_address)) {
		printf("Warning: size was bigger than the block size. ");
		printf("Writing %ld characters.\n",
			   block->size - (address - block->start_address));
		new_size = block->size - (address - block->start_address);
	}
	write_in_block(block, address, new_size, data);
}

uint64_t get_occupied_memory(const arena_t *arena)
{
	uint64_t total_mem = 0;
	list_elem *e = arena->alloc_list->head;
	while (e) {
		block_t *block = e->elem;
		total_mem += block->size;
		e = e->next;
	}
	return total_mem;
}

uint64_t count_miniblocks(const arena_t *arena)
{
	uint64_t total_miniblocks = 0;
	list_elem *e = arena->alloc_list->head;
	while (e) {
		block_t *block = e->elem;
		total_miniblocks += block->miniblock_list->size;
		e = e->next;
	}
	return total_miniblocks;
}

void print_miniblock(miniblock_t *mini, int i)
{
	char perm[4];
	perm[0] = (READ_MASK & mini->perm) ? 'R' : '-';
	perm[1] = (WRITE_MASK & mini->perm) ? 'W' : '-';
	perm[2] = (EXEC_MASK & mini->perm) ? 'X' : '-';
	perm[3] = 0;
	printf("Miniblock %d:\t\t0x%lX\t\t-\t\t0x%lX\t\t| %s\n",
		   i, mini->start_address, mini->start_address + mini->size, perm);
}

void print_block(block_t *block, int i)
{
	printf("\nBlock %d begin\n", i);
	printf("Zone: 0x%lX - 0x%lX\n", block->start_address,
		   block->start_address + block->size);
	int i_miniblock = 1;
	list_elem *head = block->miniblock_list->head;
	while (head) {
		print_miniblock(head->elem, i_miniblock);
		i_miniblock++;
		head = head->next;
	}
	printf("Block %d end\n", i);
}

void print_blocks(const arena_t *arena)
{
	if (arena->alloc_list->size == 0)
		return;
	int i_block = 1;
	list_elem *head = arena->alloc_list->head;
	while (head) {
		print_block(head->elem, i_block);
		i_block++;
		head = head->next;
	}
}

void pmap(const arena_t *arena)
{
	printf("Total memory: 0x%lX bytes\n", arena->arena_size);
	printf("Free memory: 0x%lX bytes\n",
		   arena->arena_size - get_occupied_memory(arena));
	printf("Number of allocated blocks: %ld\n", arena->alloc_list->size);
	printf("Number of allocated miniblocks: %ld\n", count_miniblocks(arena));
	print_blocks(arena);
}

void mprotect(arena_t *arena, uint64_t address, int8_t *permission)
{
	list_elem *block_e = get_block_contain_addr(arena, address);
	if (!block_e) {
		printf("Invalid address for mprotect.\n");
		return;
	}
	block_t *block = block_e->elem;
	list_elem *miniblock =
		get_miniblock_contain_addr(block->miniblock_list, address);
	miniblock_t *mini = miniblock->elem;
	if (mini->start_address != address) {
		printf("Invalid address for mprotect.\n");
		return;
	}
	mini->perm = *permission;
}
