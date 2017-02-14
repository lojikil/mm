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
    struct MLIST *next;
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

static int acall = 0, retcall = 0, relcall = 0, wcall = 0;

MList *init();
void *alloc(size_t, MList *);
void retain(void *, MList *);
void release(void *, MList *); 
void *weakalloc(size_t, MList *);
void clean(MList *);

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

UList *str2lst(char *, size_t, MList *);
UList *int2lst(int, MList *);
UList *cons(UList *, UList *, MList *);
UList *car(UList *, MList *);
UList *cdr(UList *, MList *);
void walk(UList *);
void print(UList *);

int
main() {
    char *t0 = nil, *t1 = nil;
    int tmp = 0;
    MList *region = nil;
    UList *userdata = nil, *tmpnode = nil;

    region = init();
    t0 = (char *) weakalloc(sizeof(char) * 128, region); // we turn around & retain in str2lst, so...
    t1 = (char *) weakalloc(sizeof(char) * 128, region); // we turn around & retain in str2lst, so...
    printf("Enter a string: ");
    scanf("%128s", &t0);
    tmpnode = str2lst(t0, sizeof(char) * 128, region); 
    userdata = cons(tmpnode, nil, region);
    while(tmp != -1) {
        printf("Enter an integer: ");
        scanf("%d", &tmp);
        if(tmp != -1) {
            tmpnode = int2lst(tmp, region);
            userdata = cons(tmpnode, userdata, region);
        }
    }
    printf("Enter a string: ");
    scanf("%128s", &t1);
    tmpnode = str2lst(t1, sizeof(char) * 128, region);
    userdata = cons(tmpnode, userdata, region);

    printf("allocated chars should work like normal strings:\n");
    printf("t0: %s\nt1: %s\n", t0, t1);

    printf("We should be able to walk that list too:\n");
    walk(userdata);
    tmpnode = userdata;
    while(tmpnode != nil) {
        tmpnode = car(userdata);
        userdata = cdr(userdata);
        release(tmpnode);
    }
    release(t0);
    release(t1);

    printf("some statistics: ");
    printf("allocation calls: %d\n", acall);
    printf("weak allocation calls: %d\n", wcall);
    printf("retain calls: %d\n", retcall);
    printf("release calls: %d\n", relcall);

    clean(region);
}
