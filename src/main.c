#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#define IN_USE_FLAG  0b00000001
#define IS_LAST_FLAG 0b00000010

struct block_header {
    // Byte size of this block. Must include both header and data.
    size_t size;

    // Flags about this memory. Currently mostly used for in_use (bit 0), but
    // we have space for more.
    u_int8_t flags;

    // TODO: Store function pointer.

    // Data comes after this header.
};

struct memory_size_block {
    struct block_header *head;
};

/**
 * At this point we are assuming that the `start` block header is unused. We do
 * not double check that. IF the next one is also unused and the same size we
 * can just bump this up to the next biggest size.
 */
bool maybe_combine(struct block_header *start) {
    /**
     * TODO: How do we know that we are at the end? Currently this will run off
     *       the end and die.
     */
    struct block_header *next =
        (struct block_header *)(
            ((char *)start) +
                (start->size)
        );
    
    /**
     * We have decided to consume the block after us. We do not change the data
     * in our block or the second block for efficiency reasons. This is a big
     * part of why you would never want to use this for sensitive info.
     */
    if (next->flags & IN_USE_FLAG) {
        start->size = start->size * 2;
        return true;
    }

    return false;
}

struct block_header *get_next_block(struct block_header *block) {
    if (block->flags & IS_LAST_FLAG) return NULL;
    return (struct block_header *)(((char *)block) + (block->size));
}

void display(struct block_header *block) {
    int count = 0;
    while (true) {
        printf(
            "Block: %p\n\tBytes: %lu\n\tIN USE: %s\n\tIS_LAST: %s\n",
            block,
            block->size,
            (block->flags & IN_USE_FLAG) ? "true" : "false",
            (block->flags & IS_LAST_FLAG) ? "true" : "false"
        );
        block = get_next_block(block);
        if (block == NULL) break;
        count++;
        // if (count > 10) break;
    }
}

size_t next_power_of_two(size_t n) {
    size_t p = 1;

    if (n && !(n & (n - 1))) {
        return n;
    }
    
    while (p < n) {
        p <<= 1;
    }
    
    return p; 
}

struct block_header *allocate(struct block_header *head, size_t size) {
    // We also store the header inside the structure itself.
    // TODO: Does this guarantee alignment at all?
    size += sizeof(struct block_header);

    size_t target_size = next_power_of_two(size);
    printf("Allocating with target size: %lu\n", target_size);
    
    // Find a block that is the perfect size.
    struct block_header *block = head;
    struct block_header *best_to_split = NULL;
    while (true) {
        if (block == NULL) break;

        if ((block->flags & IN_USE_FLAG) != 0) {
            block = get_next_block(block);
            continue;
        }

        if (block->size != target_size) {
            if (block->size < target_size) {
                block = get_next_block(block);
                continue;
            }

            if (best_to_split == NULL || best_to_split->size > block->size) {
                best_to_split = block;
            }

            // TODO: Mark this for maybe splitting.
            block = get_next_block(block);
            continue;
        }

        block->flags = block->flags | IN_USE_FLAG;
        return block;
    }

    if (best_to_split != NULL) {
        block = best_to_split;
        while (true) {
            size_t new_size = block->size / 2;
            block->size = new_size;
            bool was_last = (block->flags & IS_LAST_FLAG) != 0;
            if (was_last)
                block->flags = block->flags ^ IS_LAST_FLAG;

            struct block_header *second_head = get_next_block(block);
            if (second_head == NULL) {
                display(head);
                return NULL;
            }
            second_head->flags = block->flags | (was_last ? IS_LAST_FLAG : 0);
            second_head->size = new_size;

            if (block->size == target_size) {
                block->flags = block->flags | IN_USE_FLAG;
                return block;
            } else if (block->size < target_size) {
                printf("\tERROR\n");
                display(head);
                return NULL;
            }
        }
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    // TODO: Calculate or take an argument for the max size.
    // TODO: While I think I only want to support power-of-two sizes, I should
    //       probably allow multiples of. That is, instead of only allowing a
    //       base of a single 1024 block, allow a base of 3 x 1024 blocks.
    size_t max_size = 1024 * 1024;
    void *backing = malloc(max_size);

    struct block_header *head = (struct block_header *)backing;
    head->flags = IS_LAST_FLAG;
    head->size = max_size;

    // display(head);

    allocate(head, 13);
    allocate(head, 13);
    allocate(head, 13);

    display(head);

    free(backing);
    return 0;
}