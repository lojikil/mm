#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* using a flat list of memory references
 * right now; would be neat to use something
 * like a HAMT or even just an AVL/BTree 
 * for this, but for a simple test, it's easy
 * to start here.
 */
typedef struct MLIST {
    void *item;
    uint32_t count;
} MList;

/* technically, I could just
 * create a global, but this
 * way I can have "regions".
 * besides, this is just a test,
 * mostly to see if I can get
 * rid of Boehm in compiling
 * carML/c
 */

void *alloc(size_t, MList *);
void retain(void *, MList *);
void release(void *, MList *); 
void *weakalloc(size_t, MList *);

/* actual testing stuffs.
 */

typedef enum NTYPE {
    LINT, LSTRING, LNIL, LLIST
} NodeType;

typedef struct ULIST {
    ListType type;
    union {
        int i;
        char *s;
    } object;
    size_t olength;
    size_t length;
    struct ULIST *next;
} UList;

UList *cons(UList *, UList *);
UList *car(UList *);
UList *cdr(UList *);
void walk(UList *);

int
main() {

}
