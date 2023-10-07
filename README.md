# C Stack Allocator

_A `malloc` wrapper with some kind of pointer ownership_

## Motivation

When porting managed memory code (in my case from Golang) to
unmanaged memory code (C or even C++ and variants), I see that
a lot of the times there are lots of pointer/object creation,
that, due to the properties of such languages, are cleaned up
_sometime_.

In general, every pointer created in a function, that doesn't
_"go up"_ (to be taken/used by the caller), will get discarded.

That is a property of the _stack_: unless you pass your variables
up (either through memory or register, depending on ABI and
variable type), will be "deleted" (more like _overriden_ sometime
in the future).

This _"stack allocator"_ will create a concept of stack, but
using the heap memory.

## How it works

At the beggining of the function/scope, you can create a "stack frame",
usually through the recommended macros `AS_INIT()` or `AS_INITV(COUNT)`
(or manually with `as_init(struct alloc_stack *)` or the vector counterpart)
and the use the `as_*alloc` functions to allocate memory.

The frame will keep track of those pointers.

At the end of the scope (automatically with the use of `AS_INIT()`)
or manually with `as_fini()`), the frame will be freed, freeing all
pointer it is currently tracking.

To move data to the caller (up), you call the `as_up(void *ptr)`
function.
This function will have pass the pointer reference to the upper frame.

If getting a pointer from a function that does not track the pointers
(e.g. from a third party function), the pointer can be tracked
by calling the `as_take(ptr)` function (see details "Taking ownership").

### Some implementation details

**One stack per thread**

The library keeps track of the stack (and current frame) through
a thread local "global" variable: `__thread struct alloc_stack *_stack;`.

Effectively creating one stack per thread. (Should make it thread safe-enough).

**AS_INIT/V macro**

The `AS_INIT` macro will create a `struct alloc_stack`, registering it
for cleanup using the `__attribute__ ((__cleanup__(...)))` and
calls the initializer (`as_init(...)`).

This makes it call the frame finalizer at the end of scope (including
when exceptions are caught).

**Reference storage**

The stack/frame structure uses a binary search tree to store
the pointers it is tracking. This choice was done for quick searches
when pushing pointers up.

Using an array/vector is also possible instead of the initial BST.
The reference macro `AS_INITV(COUNT)` creates an array with `COUNT`
elements (storage for pointers) and passes it to the `as_initv(..)`
function.

Using the vector initialization functions allows for all extra
storage (to store the pointers) be allocated in the stack, reducing
the calls to heap memory allocation, hence reducing margin for errors

**Moving up, outside the stack**

If a push up is done (`as_up`) and there isn't a parent frame,
the pointer will not be tracked anymore.

**Taking ownership**

As of time of writing, if the pointer is already being tracked by
an upper frame, it **will not** be removed from such frame,
in effect making it duplicate (double free).

## Example

(see also `example.c`)

```c
#include <stdio.h>
#include <string.h>

#include "alloc_stack.h"

char *xor42(const char *str)
{
  AS_INIT();

  int slen = strlen(str);

  // pointer is created on this frame
  char *xored = as_malloc(slen + 1);
  if(!xored)
    return NULL;
  xored[slen] = '\0';

  for(int i = 0; i < slen; i++)
    xored[i] = str[i] ^ 42;

  // move pointer up the stack
  as_up(xored);
  // parent now has reference
  return xored;

  // end of scope, no pointer to clean
}

char *concat_xor42(char *a, char *b)
{
  AS_INITV(4);

  int len_a = strlen(a);
  int len_b = strlen(b);
  int len_c = len_a + len_b;

  // pointer is created in this frame
  char *concat = as_malloc(len_a + len_b + 1);
  if(!concat)
    return NULL;

  memcpy(concat, a, len_a);
  memcpy(concat + len_a, b, len_b);
  concat[len_c] = '\0';

  char *xored = xor42(concat);
  // pointer is inhereted from `xor42`

  // and moved up
  as_up(xored);

  return xored;

  // end of scope, will free the `concat` string
}

int main(int argc, char**argv)
{
  AS_INIT();
  if(argc != 3)
    return 1;  //printf(...)

  // we take a pointer without reference ( :o ) because we can
  printf("xored42: %s\n", concat_xor42(argv[1], argv[2]));

  return 0;

  // end of scope, the "dangling" pointer will be freed
}
```

## Build

Simply run `gcc`/`clang` or copy those files to your project
