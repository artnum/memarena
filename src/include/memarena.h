#ifndef MEMARENA_H__
#define MEMARENA_H__

#include <iso646.h>
#ifndef MEMARENA_ALIGNMENT
#define MEMARENA_ALIGNMENT sizeof(max_align_t)
#endif /* MEMARENA_ALIGNMENT */

#include <stddef.h>

/* bump arena using mmap for block */
typedef struct {
  unsigned char *data;
  unsigned char *last_alloc;
  int alloc_cnt;
  size_t capacity;
  size_t used;
  void *next;
} mem_arena_region_t;

typedef struct {
  size_t pagesize;
  size_t default_size;
  size_t embed;
  mem_arena_region_t *head;
  mem_arena_region_t *tail;
} mem_arena_t;

/* *** Arena init and destroy *** */
/**
 * Create a new arena.
 *
 * Arena is composed of region and use getpagesize() for the region size.
 * Some memory is used for structure so not the full page is available. If an
 * allocation is bigger than default region size, it will allocate enough
 * memory for that
 *
 * \param[in] size  Memory region will be allocated of that size. If 0 it
 *                  uses getpagesize().
 *
 * \return An arena object or NULL in case of failure
 */
mem_arena_t *mem_arena_new(size_t size);

/**
 * Create a new arean with embedd data.
 *
 * The first region embed some data for arena management. This data is not
 * freed by mem_arena_reset. So this allow to add your own custom data
 * structure within that embedd space. Useful for building stuff like dynamic
 * array where you put the structure within the embedd space and, to empty
 * the whole array, you just mem_arena_reset without being worried your
 * structure is freed.
 *
 * \param[in]  size        Memory region will be allocated of that size. If
 *                         0 it uses getpagesize().
 * \param[in]  embed_size  Size to be embedd
 * \param[out] ptr         Pointer to the allocated embedd region of size
 *                         embed_size.
 * \return An arena object or NULL in case of failure
 */
mem_arena_t *mem_arena_new_embed(size_t size, size_t embed_size, void **ptr);

/**
 * Reset an arena.
 *
 * The whole arena is now free again for new allocation
 *
 * \param[in] arena  The arena to reset.
 */
void mem_arena_reset(mem_arena_t *arena);

/**
 * Destroy an arena.
 *
 * The whole arena is now invalid and the memory is released to the operating
 * system.
 *
 * \param[in] arena  The arena to destroy.
 */
void mem_arena_destroy(mem_arena_t *arena);
/**
 * Dump stats
 * Dump some stats on stderr.
 */
void mem_arena_dump(mem_arena_t *arena);

/* *** Allocation, free, ... *** */

/** Malloc but with arena */
void *mem_alloc(mem_arena_t *arena, size_t size);
/** Realloc but with arena */
void *mem_realloc(mem_arena_t *arena, void *ptr, size_t new_size);
/**
 * Free
 * Try to reclaim a region if possible. Useful when working with dynamic
 * array as mmap'd region can be reclaimed in some case. Allocation keep
 * count and if it happen that a region has seen as many mem_free as
 * mem_alloc, the region is considered empty and will be recycled later on.
 * Also, as realloc, if free is done on a last allocation of a region, it
 * gives back the space to the region.
 */
void mem_free(mem_arena_t *arena, void *ptr);

/* *** String function *** */

/** Strndup but with arena */
char *mem_strndup(mem_arena_t *arena, const char *string, size_t length);
/** Strdup but with arena */
char *mem_strdup(mem_arena_t *arena, const char *string);

/* *** Utility function *** */

/** Duplicate memory */
void *mem_memdup(mem_arena_t *arena, const void *ptr, size_t length);

/** Get the size of the allocated memory
 *
 * \param arena The arena where ptr belong
 * \param ptr   The pointer to memory we want size of
 *
 * \return The size of the allocated memory
 */
size_t mem_memsize(mem_arena_t *arena, const void *ptr);
#endif /* MEMARENA_H__ */
