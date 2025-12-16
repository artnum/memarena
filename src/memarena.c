#include "include/memarena.h"
#include <assert.h>
#include <bits/time.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#define ALIGNED_SIZE(x)                                                        \
  ((((x) + (MEMARENA_ALIGNMENT - 1)) / MEMARENA_ALIGNMENT) * MEMARENA_ALIGNMENT)
#define REGION_FREE_SPACE(r)                                                   \
  ((r)->capacity - (r)->used - ALIGNED_SIZE(sizeof(size_t)))
#define GET_SIZE_T_PTR_FROM_PTR(ptr)                                           \
  (size_t *)((uint8_t *)(ptr) - ALIGNED_SIZE(sizeof(size_t)));

/* an arena with 1 allocation will use that amount of data, at least :
 * -> the embedd arena structure
 * -> the embedd region structure
 * -> the size_t where the 1 allocation will store its size
 */
#define MIN_OVERHEAD_R0                                                        \
  (sizeof(mem_arena_t) + sizeof(mem_arena_region_t) + sizeof(size_t))
#define MIN_OVERHEAD_RX (sizeof(mem_arena_region_t) + sizeof(size_t))

static mem_arena_region_t *_new_region(size_t size, int pagesize) {
  size = ((size + pagesize - 1) / pagesize) * pagesize;
  mem_arena_region_t *region = mmap(NULL, size, PROT_READ | PROT_WRITE,
                                    MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (region != MAP_FAILED) {
    size_t head_size = ALIGNED_SIZE(sizeof(*region));
    region->alloc_cnt = 0;
    region->data = (unsigned char *)region + head_size;
    region->capacity = size - head_size;
    region->used = 0;
    region->next = NULL;
  } else {
    return NULL;
  }
  return region;
}

mem_arena_t *mem_arena_new(size_t size) {
  size_t pagesize = getpagesize();
  if (size == 0) {
    size = pagesize;
  }
  mem_arena_t *arena = NULL;
  mem_arena_region_t *region = _new_region(size + MIN_OVERHEAD_R0, pagesize);
  if (region) {
    size_t head_size = ALIGNED_SIZE(sizeof(*arena));
    arena = (mem_arena_t *)region->data;
    memset(arena, 0, sizeof(*arena));

    arena->head = region;
    arena->tail = region;
    arena->pagesize = pagesize;
    arena->default_size = size + MIN_OVERHEAD_RX;

    region->data = (unsigned char *)region->data + head_size;
    region->capacity -= head_size;
  }
  return arena;
}

mem_arena_t *mem_arena_new_embed(size_t size, size_t embed_size, void **ptr) {
  assert(ptr != NULL);
  mem_arena_t *arena = mem_arena_new(size + embed_size);
  if (arena) {
    mem_arena_region_t *region = arena->head;
    embed_size = ALIGNED_SIZE(embed_size);
    *ptr = region->data;
    region->data += embed_size;
    region->capacity -= embed_size;
    arena->embed = embed_size;
  }
  return arena;
}

void mem_arena_dump(mem_arena_t *arena) {
  if (arena) {
    size_t total_size = 0;
    size_t used_size = 0;
    for (mem_arena_region_t *r = arena->head; r;
         r = (mem_arena_region_t *)r->next) {
      fprintf(stderr, "capacity %ld\n", r->capacity);
      used_size += r->used;
      total_size += r->capacity;
    }

    fprintf(stderr,
            "ARENA %p\n\t- Page size\t\t%6ld\n\t- Default size\t\t%6ld\n\t- "
            "Total size\t\t%6ld\n\t- Used "
            "size\t\t%6ld (%.2f %%)\n",
            arena, arena->pagesize, arena->default_size, total_size, used_size,
            (double)used_size * 100 / total_size);
    int i = 0;
    for (mem_arena_region_t *r = arena->head; r;
         r = (mem_arena_region_t *)r->next) {

      fprintf(stderr,
              "\t* REGION %d\n\t\t- Capacity\t%6ld\n\t\t- Used\t\t%6ld\n", i++,
              r->capacity, r->used);
    }
  }
}

void mem_arena_reset(mem_arena_t *arena) {
  if (!arena) {
    return;
  }

  if (!arena->head) {
    return;
  }

  arena->tail = arena->head;
  for (mem_arena_region_t *r = arena->head; r;
       r = (mem_arena_region_t *)r->next) {
    r->used = 0;
    r->alloc_cnt = 0;
  }
}

void mem_arena_destroy(mem_arena_t *arena) {
  if (arena == NULL) {
    return;
  }
  size_t arena_head_size = ALIGNED_SIZE(sizeof(*arena)) + arena->embed;
  size_t head_size = ALIGNED_SIZE(sizeof(mem_arena_region_t));
  for (mem_arena_region_t *r = arena->head; r != NULL;) {
    mem_arena_region_t *n = (mem_arena_region_t *)r->next;
    munmap(r->data - (head_size + arena_head_size),
           r->capacity + head_size + arena_head_size);
    /* we embed arena struct within first region, so next region have no arena
     * head size */
    arena_head_size = 0;
    r = n;
  }
}

/* TODO : take care of INT_MAX ... */
void *mem_alloc(mem_arena_t *arena, size_t size) {
  if (!arena || size < 1) {
    return NULL;
  }
  mem_arena_region_t *region = NULL;
  mem_arena_region_t *previous = NULL;

  region = arena->tail;
  while (region != NULL && REGION_FREE_SPACE(region) < size) {
    previous = region;
    region = (mem_arena_region_t *)region->next;
  }

  if (region == NULL) {
    region =
        _new_region((arena->default_size < size ? size : arena->default_size) +
                        MIN_OVERHEAD_RX,
                    arena->pagesize);
  }

  uint8_t *ptr = NULL;
  if (region) {
    *(size_t *)(region->data + region->used) = size;
    ptr = region->data + region->used + ALIGNED_SIZE(sizeof(size_t));
    region->used += size + ALIGNED_SIZE(sizeof(size_t));
    region->last_alloc = ptr;
    region->alloc_cnt++;
    arena->tail = region;
    if (previous) {
      previous->next = region;
    }
  }
  return (void *)ptr;
}

void *mem_realloc(mem_arena_t *arena, void *ptr, size_t new_size) {
  if (arena == NULL || new_size < 1) {
    return NULL;
  }
  if (ptr == NULL) {
    return mem_alloc(arena, new_size);
  }

  size_t *old_size = GET_SIZE_T_PTR_FROM_PTR(ptr);
  if (*old_size > new_size) {
    *old_size = new_size;
    return ptr;
  }

  for (mem_arena_region_t *r = arena->head; r;
       r = (mem_arena_region_t *)r->next) {
    if (r->last_alloc == ptr && REGION_FREE_SPACE(r) >= new_size - *old_size) {
      r->used += new_size - *old_size;
      *old_size = new_size;
      return ptr;
    }
  }

  void *new_ptr = mem_alloc(arena, new_size);
  if (new_ptr) {
    memcpy(new_ptr, ptr, *old_size);
  }
  return new_ptr;
}

/* TODO : think if I should have a pointer in the region to point to the
 * real tail, so I skip going through some region to move it to the end. If
 * so, do this modification ... but I wonder about adding more pointer.
 */
static void _move_empty_region_to_end(mem_arena_t *arena,
                                      mem_arena_region_t *region,
                                      mem_arena_region_t *prev) {
  /* alread tail so we done */
  if (arena->tail == region) {
    return;
  }

  /* it's the only region so it gets reused next alloc */
  if (region == arena->head && region->next == NULL) {
    if (arena->tail != region && arena->tail != NULL) {
      arena->tail->next = region;
    }
    arena->tail = region;
    return;
  }

  /* disconnect region */
  if (prev) {
    prev->next = region->next;
  }
  region->next = NULL;

  /* put region after tail so it can get picked for allocation */
  mem_arena_region_t *tail = arena->tail;
  while (tail->next != NULL) {
    tail = tail->next;
  }
  tail->next = region;

  if (region == arena->head) {
    arena->head = region->next;
  }
}

void mem_free(mem_arena_t *arena, void *ptr) {
  if (arena == NULL || ptr == NULL) {
    return;
  }

  mem_arena_region_t *region = arena->head, *prev = NULL;

  /* find which region pointer belong */
  while (region != NULL && !(ptr >= (void *)region->data &&
                             ptr < (void *)(region->data + region->used))) {
    prev = region;
    region = (mem_arena_region_t *)region->next;
  }
  /* ptr doesn't belong to us */
  if (region == NULL) {
    return;
  }

  if (region->last_alloc == ptr) {
    size_t *old_size = GET_SIZE_T_PTR_FROM_PTR(ptr);
    /* use count the head size, so we should remove it */
    region->used -= (*old_size + ALIGNED_SIZE(sizeof(size_t)));
    region->alloc_cnt--;
    region->last_alloc = NULL;
    if (region->alloc_cnt <= 0) {
      _move_empty_region_to_end(arena, region, prev);
    }
    return;
  }

  region->alloc_cnt--;
  if (region->alloc_cnt <= 0) {
    region->alloc_cnt = 0;
    region->used = 0;
    _move_empty_region_to_end(arena, region, prev);
  }
}

/* *** String function *** */

char *mem_strndup(mem_arena_t *arena, const char *string, size_t length) {
  if (arena == NULL || string == NULL || length == 0) {
    return NULL;
  }

  size_t slen = strlen(string);
  if (slen < length) {
    length = slen;
  }
  char *new_str = mem_alloc(arena, length + 1);
  if (new_str) {
    memcpy(new_str, string, length);
    new_str[length] = '\0';
  }

  return new_str;
}

char *mem_strdup(mem_arena_t *arena, const char *string) {
  if (arena == NULL || string == NULL) {
    return NULL;
  }
  size_t slen = strlen(string);
  char *new_str = mem_alloc(arena, slen + 1);
  if (new_str) {
    memcpy(new_str, string, slen);
    new_str[slen] = '\0';
  }
  return new_str;
}

/* *** Utility function *** */

void *mem_memdup(mem_arena_t *arena, const void *ptr, size_t length) {
  if (arena == NULL || ptr == NULL || length <= 0) {
    return 0;
  }
  void *new_ptr = mem_alloc(arena, length);
  if (new_ptr) {
    memcpy(new_ptr, ptr, length);
  }
  return new_ptr;
}

size_t mem_memsize(mem_arena_t *arena, const void *ptr) {
  if (ptr == NULL || arena == NULL) {
    return 0;
  }
  return *GET_SIZE_T_PTR_FROM_PTR(ptr);
}
