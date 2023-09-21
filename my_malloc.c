/* we need this for uintptr_t */
#include <stdint.h>
/* we need this for memcpy/memset */
#include <string.h>
/* we need this to print out stuff*/
#include <stdio.h>
/* we need this for the metadata_t struct and my_malloc_err enum definitions */
#include "my_malloc.h"

/* Notes for the Programmer:
 * address_list is the freelist and *address list is a pointer 
 * to the first address in the free list
 * 
 * Done with helper functions, need to debug and check to make
 * sure they are right before moving on to main body functions
 * 
 * Once helper functions are checked and debugged, need to fix my_malloc
 *
 * Need to double check and potentially fix find_best_fit and split_block
 *
 * Have code for all, figure out all
 *
 * add/remove                                                                fg helper functions could be wrong
 */

/* Function Headers
 * Here is a place to put all of your function headers
 * Remember to declare them as static
 */
static metadata_t *find_right(metadata_t *freed_block);
static metadata_t *find_left(metadata_t *freed_block);
static void merge(metadata_t *left, metadata_t *right);
static metadata_t *split_block(metadata_t *block, size_t size);
static void add_to_addr_list(metadata_t *block);
static void remove_from_addr_list(metadata_t *block);
static metadata_t *find_best_fit(size_t size);

/* Our freelist structure - our freelist is represented as a singly linked list
 * the freelist is sorted by address;
 */
metadata_t *address_list;

/* Set on every invocation of my_malloc()/my_free()/my_realloc()/
 * my_calloc() to indicate success or the type of failure. See
 * the definition of the my_malloc_err enum in my_malloc.h for details.
 * Similar to errno(3).
 */
enum my_malloc_err my_malloc_errno;



// -------------------- PART 1: Helper functions --------------------

/* The following prototypes represent useful helper functions that you may want
 * to use when writing your malloc functions. You do not have to implement them
 * first, but we recommend reading over their documentation and prototypes;
 * having a good idea of the kinds of helpers you can use will make it easier to
 * plan your code.
 *
 * None of these functions will be graded individually. However, implementing
 * and using these will make programming easier. We may provide ungraded test
 * cases for some of these functions after the assignment is released.
 */


/* OPTIONAL HELPER FUNCTION: find_right
 * Given a pointer to a free block, this function searches the freelist for another block to the right of the provided block.
 * If there is a free block that is directly next to the provided block on its right side,
 * then return a pointer to the start of the right-side block.
 * Otherwise, return null.
 * This function may be useful when implementing my_free().
 */
static metadata_t *find_right(metadata_t *freed_block) {
    metadata_t *current = address_list;
    while (current != NULL) {
        if ((uintptr_t) (freed_block) + freed_block->size + TOTAL_METADATA_SIZE == (uintptr_t) (current)) {
            return current;
        } else {
            current = current->next;
        }
    }
    return NULL;
}

/* OPTIONAL HELPER FUNCTION: find_left
 * This function is the same as find_right, but for the other side of the newly freed block.
 * This function will be useful for my_free(), but it is also useful for my_malloc(), since whenever you sbrk a new block,
 * you need to merge it with the block at the back of the freelist if the blocks are next to each other in memory.
 */

static metadata_t *find_left(metadata_t *freed_block) {
    metadata_t *current = address_list;
    while (current != NULL) {
        if ((uintptr_t) (current) + current->size + TOTAL_METADATA_SIZE == (uintptr_t) (freed_block)) {
            return current;
        } else {
            current = current->next; 
        }
    }
    return NULL;
}

/* OPTIONAL HELPER FUNCTION: merge
 * This function should take two pointers to blocks and merge them together.
 * The most important step is to increase the total size of the left block to include the size of the right block.
 * You should also copy the right block's next pointer to the left block's next pointer. If both blocks are initially in the freelist, this will remove the right block from the list.
 * This function will be useful for both my_malloc() (when you have to merge sbrk'd blocks) and my_free().
 */
static void merge(metadata_t *left, metadata_t *right) {
    //perform merge
    left->size = left->size + right->size + TOTAL_METADATA_SIZE;
    //remove the correct node after merging
    left->next = right->next; 
}

/* OPTIONAL HELPER FUNCTION: split_block
 * This function should take a pointer to a large block and a requested size, split the block in two, and return a pointer to the new block (the right part of the split).
 * Remember that you must make the right side have the user-requested size when splitting. The left side of the split should have the remaining data.
 * We recommend doing the following steps:
 * 1. Compute the total amount of memory that the new block will take up (both metadata and user data).
 * 2. Using the new block's total size with the address and size of the old block, compute the address of the start of the new block.
 * 3. Shrink the size of the old/left block to account for the lost size. This block should stay in the freelist.
 * 4. Set the size of the new/right block and return it. This block should not go in the freelist.
 * This function will be useful for my_malloc(), particularly when the best-fit block is big enough to be split.
 */
static metadata_t *split_block(metadata_t *block, size_t size) {
    size_t new_size = size + TOTAL_METADATA_SIZE;
    size_t old_size = block->size + TOTAL_METADATA_SIZE;
    //new address calculations should always be initialized as a void pointer
    metadata_t *newp = (metadata_t *) ((uintptr_t) (block) + old_size - new_size);
    block->size -= new_size;
    newp->size = size;
    newp->next = NULL;
    return newp;
}

/* OPTIONAL HELPER FUNCTION: add_to_addr_list
 * This function should add a block to freelist.
 * Remember that the freelist must be sorted by address. You can compare the addresses of blocks by comparing the metadata_t pointers like numbers (do not dereference them).
 * Don't forget about the case where the freelist is empty. Remember what you learned from Homework 9.
 * This function will be useful for my_malloc() (mainly for adding in sbrk blocks) and my_free().
 */
static void add_to_addr_list(metadata_t *block) {
    //empty list
    if (address_list == NULL) {
        address_list = block; 
        block->next = NULL;
    } else {
        //beginning of list
        metadata_t *current = address_list;
        if (block < current) {
            block->next = current;
            address_list = block;
        //middle and end of list
        } else {
            while (current != NULL) {
                //end
                if (current->next == NULL) {
                    block->next = NULL;
                    current->next = block;
                    break;
                //middle of the list
                } else {
                    //check if between the right addresses
                    if (block > current && block < current->next) {
                        block->next = current->next;
                        current->next = block;
                        break;
                    }
                }
                current = current->next;
            }
        }
    }
}

/* OPTIONAL HELPER FUNCTION: remove_from_addr_list
 * This function should remove a block from the freelist.
 * Simply search through the freelist, looking for a node whose address matches the provided block's address.
 * This function will be useful for my_malloc(), particularly when the best-fit block is not big enough to be split.
 */
static void remove_from_addr_list(metadata_t *block) {
    //removing from front of list
    if (block == address_list) {
        address_list = block->next;
    } else {
        metadata_t *current = address_list;
        while (current->next != NULL) {
            if (block == current->next) {
                //remove the block from the free list
                metadata_t *toRemove = current->next;
                current->next = toRemove->next;
                break;
            } else {
                current = current->next;
            }
        }
    }
}

/* OPTIONAL HELPER FUNCTION: find_best_fit
 * This function should find and return a pointer to the best-fit block. See the PDF for the best-fit criteria.
 * Remember that if you find the perfectly sized block, you should return it immediately.
 * You should not return an imperfectly sized block until you have searched the entire list for a potential perfect block.
 */
static metadata_t *find_best_fit(size_t size) {
    if (address_list == NULL) {
        return NULL;
    }
    //search the entire list first for a perfectly sized block first
    metadata_t *current = address_list;
    int current_size = 10000000;
    metadata_t *best_fit = NULL;
    while (current != NULL) {
        //check for perfectly sized block
        if (current->size == size) {
            return current;
        } else {
            //finding smallest block that is still large enough
            if (current->size > size) {
                int min_allowed = (int) current->size - size;
                if (min_allowed > 0 && min_allowed < current_size) {
                    current_size = min_allowed;
                    best_fit = current;
                }
            }
        }
        current = current->next;
    }
    return best_fit;
}




// ------------------------- PART 2: Malloc functions -------------------------

/* Before starting each of these functions, you should:
 * 1. Understand what the function should do, what it should return, and what the freelist should look like after it finishes
 * 2. Develop a high-level plan for how to implement it; maybe sketch out pseudocode
 * 3. Check if the parameters have any special cases that need to be handled (when they're NULL, 0, etc.)
 * 4. Consider what edge cases the implementation needs to handle
 * 5. Think about any helper functions above that might be useful, and implement them if you haven't already
 */


/* MALLOC
 * See PDF for documentation
 */
void *my_malloc(size_t size) {
    //1.
    if (size + TOTAL_METADATA_SIZE > SBRK_SIZE) {
        my_malloc_errno = SINGLE_REQUEST_TOO_LARGE;
        return NULL;
    }
    if (size == 0) {
        my_malloc_errno = NO_ERROR;
        return NULL;
    }
    //2. 
    metadata_t *best_fit = find_best_fit(size);
    //3.
    if (best_fit == NULL) {
        metadata_t *new_fit = (metadata_t *) (my_sbrk(SBRK_SIZE));
        if ((long) (new_fit) == -1) {
            my_malloc_errno = OUT_OF_MEMORY;
            return NULL;
        } else {
            new_fit->size = SBRK_SIZE - TOTAL_METADATA_SIZE;
            add_to_addr_list(new_fit);
            if (find_left(new_fit) != NULL) {
                merge(find_left(new_fit), new_fit);
            }
            best_fit = find_best_fit(size);
        }
    }
        //4.
        if (best_fit->size < MIN_BLOCK_SIZE + size) {
            remove_from_addr_list(best_fit);
            my_malloc_errno = NO_ERROR;
            return (void *) (best_fit + 1);
        } else {
            metadata_t *split = split_block(best_fit, size);
            my_malloc_errno = NO_ERROR;
            return (void *) (split + 1);
        }
    


    // Reminder of how to do malloc:
    // 1. Make sure the size is not too small or too big.
    // 2. Search for a best-fit block. See the PDF for information about what to check.
    // 3. If a block was not found:
    // 3.a. Call sbrk to get a new block.
    // 3.b. If sbrk fails (which means it returns -1), return NULL.
    // 3.c. If sbrk succeeds, add the new block to the freelist. If the new block is next to another block in the freelist, merge them.
    // 3.d. Go to step 2.
    // 4. If the block is too small to be split (see PDF for info regarding this), then remove the block from the freelist and return a pointer to the block's user section.
    // 5. If the block is big enough to split:
    // 5.a. Split the block into a left side and a right side. The right side should be the perfect size for the user's requested data.
    // 5.b. Keep the left side in the freelist.
    // 5.c. Return a pointer to the user section of the right side block.

    // A lot of these steps can be simplified by implementing helper functions. We highly recommend doing this!
    
}

/* REALLOC
 * See PDF for documentation
 */
void *my_realloc(void *ptr, size_t size) {
    //look at manpage for function memcpy
    void *block = NULL;
    if (ptr == NULL) {
        return my_malloc(size);
    }
    if (size == 0 && ptr != NULL) {
        my_free(ptr);
        return NULL;
    }
    //main body of realloc
    if ((block = my_malloc(size)) == NULL && size != 0) {
        return NULL;
    } else {
        //ptr is old block, block is new block
        my_malloc_errno = NO_ERROR;
        if (size < ((metadata_t *) ((uintptr_t) (ptr) - TOTAL_METADATA_SIZE))->size) {
            memcpy(block, ptr, size);
        } else {
            memcpy(block, ptr, ((metadata_t *) ((uintptr_t) (ptr) - TOTAL_METADATA_SIZE))->size);
        }
        my_free(ptr);
        return block;
    }


    // Reminder of how to do realloc:
    // 1. If ptr is NULL, then only call my_malloc(size). If the size is 0, then only call my_free(ptr).
    // 2. Call my_malloc to allocate the requested number of bytes. If this fails, immediately return NULL and do not free the old allocation.
    // 3. Copy the data from the old allocation to the new allocation. We recommend using memcpy to do this. Be careful not to read or write out-of-bounds!
    // 4. Free the old allocation and return the new allocation.
}

/* CALLOC
 * See PDF for documentation
 */
void *my_calloc(size_t nmemb, size_t size) {
    //look at man page for function memset
    void *mallocp;
    if ((mallocp = my_malloc(nmemb * size)) == NULL) {
        return NULL;
    } else {
        my_malloc_errno = NO_ERROR;
        memset(mallocp, 0, nmemb * size);
        return mallocp;
    }

    // Reminder for how to do calloc:
    // 1. Use my_malloc to allocate the appropriate amount of size.
    // 2. Clear all of the bytes that were allocated. We recommend using memset to do this.

}

/* FREE
 * See PDF for documentation
 */
void my_free(void *ptr) {
    if (ptr == NULL) {
        my_malloc_errno = NO_ERROR;
        return;
    }
    metadata_t *start_meta = (metadata_t *) ((uintptr_t) (ptr) - TOTAL_METADATA_SIZE);
    metadata_t *freed_block;
    add_to_addr_list(start_meta);
    if ((freed_block = find_right(start_meta)) != NULL) {
        merge(start_meta, freed_block);
    } 
    if ((freed_block = find_left(start_meta)) != NULL) {
        merge(freed_block, start_meta);
    }
    my_malloc_errno = NO_ERROR;
    // Reminder for how to do free:
    // 1. Since ptr points to the start of the user block, obtain a pointer to the metadata for the freed block.
    // 2. Look for blocks in the freelist that are positioned immediately before or after the freed block.
    // 2.a. If a block is found before or after the freed block, then merge the blocks.
    // 3. Once the freed block has been merged (if needed), add the freed block back to the freelist.
    // 4. Alternatively, you can do step 3 before step 2. Add the freed block back to the freelist,
    // then search through the freelist for consecutive blocks that need to be merged.

    // A lot of these steps can be simplified by implementing helper functions. We highly recommend doing this!

}


