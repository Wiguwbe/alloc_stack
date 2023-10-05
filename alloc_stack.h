
#ifndef _ALLOC_STACK_H
#define _ALLOC_STACK_H

// for internal use
struct ptr_btn;
struct alloc_stack
{
  struct alloc_stack *parent;

  struct ptr_btn *root;
};

/*
  utility macro
  calls as_init and registers
  a as_fini at end of scope.
  using __attribute__ __cleanup__ of gcc
*/
#define AS_INIT() \
  struct alloc_stack __alloc_stack __attribute__ ((__cleanup__(_as_fini))); \
  as_init(&__alloc_stack);

// mostly internal
int _as_fini(struct alloc_stack *_unused);

// create stack, passing a alloc_stack struct pointer
void as_init(struct alloc_stack *);

// free alloc stack and all pointers inside
// NOTE not needed if using `as_init` macro
int as_fini(void);

// malloc like functions
void *as_malloc(size_t size);
void as_free(void *ptr); // free memory (manually)
void *as_calloc(size_t nmemb, size_t size);
/*
  WARNING: due to implementation details, realloc* functions
  may not function the intended way ON FAILURES
*/
void *as_realloc(void* ptr, size_t size);
void *as_reallocarray(void *ptr, size_t nmemb, size_t size);


// move memory up (transfer ownership)
int as_up(void *ptr);

// take ownership of pointer
int as_take(void *ptr);

#endif
