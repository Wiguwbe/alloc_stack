
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "alloc_stack.h"

/*
  using a binary tree for fast searching
*/

struct ptr_btn
{
  // the value
  void *data;

  struct ptr_btn *left, *right;
};
typedef struct ptr_btn btn;	// binary tree node
typedef struct ptr_btn node;

typedef struct alloc_stack as_t;

// one stack per thread
__thread struct alloc_stack *_stack = NULL;

/*
  internal functions
*/

/* binary tree taken from github */

static node* new_node(void* data) {
    node *n = malloc(sizeof (node));
    if(!n)
      return NULL;
    n->data = data;
    n->left = n->right = NULL;
    return n;
}

static void init_root(node **root) {
    *root = NULL;
}

static int insert_node(node **n, void* data) {
    if (*n == NULL) {
        node *nn = new_node(data);
        if(!nn)
          return 1;
        *n = nn;
        return 0;
    } else {
        if ((*n)->data > data) {
            return insert_node(&((*n)->left), data);
        } else {
            return insert_node(&((*n)->right), data);
        }
    }
}

// 1 on error/not found
static int remove_node(node **n, void* data) {
    node *temp, *iter_node, *parent_iter_node;

    if (*n == NULL) {
        return 1;
    } else {
        if ((*n)->data > data) {
            return remove_node(&((*n)->left), data);
        } else if ((*n)->data < data) {
            return remove_node(&((*n)->right), data);
        } else {
            if ((*n)->left == NULL && (*n)->right == NULL) {
                free(*n);
                *n = NULL;
            } else if ((*n)->left != NULL && (*n)->right == NULL) {
                temp = *n;
                *n = (*n)->left;

                free(temp);
                temp = NULL;
            } else if ((*n)->left == NULL && (*n)->right != NULL) {
                temp = *n;
                *n = (*n)->right;

                free(temp);
                temp = NULL;
            } else {
                temp = *n;

                parent_iter_node = *n;
                iter_node = (*n)->right;

                while (iter_node->left != NULL) {
                    parent_iter_node = iter_node;
                    iter_node = iter_node->left;
                }

                iter_node->left = (*n)->left;

                if (*n != parent_iter_node) {
                    iter_node->right = (*n)->right;
                    parent_iter_node->left = NULL;
                } else {
                    iter_node->right = NULL;
                }

                *n = iter_node;

                free(temp);
                temp = NULL;
            }

            return 0;
        }
    }
}

// 0 on not found
static int node_search(node *n, void *data)
{
  if(!n)
    return 0;
  if(data < n->data)
    return node_search(n->left, data);
  if(data > n->data)
    return node_search(n->right, data);
  return 1;
}

static void node_free(node *n)
{
  if(!n) return;
  node_free(n->left);
  node_free(n->right);
  free(n->data);
  free(n);
}

// for debug
static void node_print(node *n)
{
  // LNR
  if(!n)
    return;
  node_print(n->left);
  printf("%p ", n->data);
  node_print(n->right);
}
// for debug
void _print_stack(as_t* as)
{
  if(!as)
    return;
  _print_stack(as->parent);
  puts("----");
  node_print(as->root);
  putchar(10);
}

void print_stack()
{
  _print_stack(_stack);
  puts("----");
}

/*
  public interface
*/

void as_init(as_t *ns)
{
  ns->parent = _stack;
  ns->root = NULL;
  _stack = ns;
}

int _as_fini(as_t *_unused)
{
  return as_fini();
}

int as_fini()
{
  as_t* ns = _stack;
  if(!ns)
    return 1;
  node_free(ns->root);
  _stack = ns->parent;
  return 0;
}

int as_take(void *ptr)
{
  as_t *ns = _stack;
  if(!ns)
    return 1;
  if(insert_node(&ns->root, ptr))
    return 1;
  return 0;
}

void *as_malloc(size_t size)
{
  as_t *ns = _stack;
  if(!ns)
    return NULL;
  void *ptr = malloc(size);
  if(!ptr)
    return NULL;

  if(insert_node(&ns->root, ptr))
  {
    free(ptr);
    return NULL;
  }

  return ptr;
}

void *as_calloc(size_t nmemb, size_t size)
{
  void *ptr = as_malloc(nmemb * size);
  if(!ptr)
    return NULL;
  memset(ptr, 0, nmemb * size);
  return ptr;
}

void *as_realloc(void *ptr, size_t size)
{
  as_t *ns = _stack;
  if(!ns)
    return NULL;

  void *old_ptr = ptr;
  if(remove_node(&ns->root, old_ptr))
    // we don't have it
    return NULL;

  ptr = realloc(ptr, size);
  if(!ptr)
  {
    // re-insert old
    if(insert_node(&ns->root, old_ptr))
    {
      free(old_ptr);
    }
    return NULL;
  }

  if(insert_node(&ns->root, ptr))
  {
    // if we fail, old ptr will be NULL
    free(ptr);
    return NULL;
  }
  return ptr;
}

void *as_reallocarray(void *ptr, size_t nmemb, size_t size)
{
  as_t *ns = _stack;
  if(!ns)
    return NULL;

  void *old_ptr = ptr;
  if(remove_node(&ns->root, old_ptr))
    // we don't have it
    return NULL;

  ptr = reallocarray(ptr, nmemb, size);
  if(!ptr)
  {
    // re-insert old
    if(insert_node(&ns->root, old_ptr))
    {
      free(old_ptr);
    }
    return NULL;
  }

  if(insert_node(&ns->root, ptr))
  {
    // if we fail, old ptr will be NULL
    free(ptr);
    return NULL;
  }
  return ptr;
}

void as_free(void *ptr)
{
  as_t *ns = _stack;
  if(!ns)
    return;

  if(remove_node(&ns->root, ptr))
    // not ours
    return;

  free(ptr);
}

int as_up(void *ptr)
{
  as_t *ns = _stack;
  if(!ns)
    return 1;

  if(remove_node(&ns->root, ptr))
    // not ours
    return 2;

  if(!ns->parent)
    // it's not our problem anymore
    return 0;

  return insert_node(&(ns->parent->root), ptr);
}
  
