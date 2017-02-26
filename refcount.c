#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define nil NULL
#define nul '\0'

static const uint8_t debugging = 0;

#define debugln if(debugging) printf("here %d\n", __LINE__) 

/* using a flat list of memory references
 * right now; would be neat to use something
 * like a HAMT or even just an AVL/BTree 
 * for this, but for a simple test, it's easy
 * to start here.
 */
typedef struct MLIST {
    void *object;
    size_t size;
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
void printstats(void *, MList *);
void *rcalloc(size_t, MList *);
void retain(void *, MList *);
void release(void *, MList *); 
void *weakalloc(size_t, MList *);
void clean(MList *);

/* actual testing stuffs.
 */

typedef enum LTYPE {
    LINT, LSTRING, LNIL, LLIST
} ListType;

/* I know this is a strange
 * structure to use for a cons-cell,
 * but as this is a test I don't really
 * care all that much.
 */
typedef struct ULIST {
    ListType type;
    union {
        int i;
        char *s;
        struct ULIST *l;
    } object;
    size_t olength;
    size_t length;
    struct ULIST *next;
} UList;

UList *str2lst(char *, size_t, MList *);
UList *int2lst(int, MList *);
UList *cons(UList *, UList *, MList *);
UList *car(UList *);
UList *cdr(UList *);
void walk(UList *);
void print(UList *);

int
main() {
    char *t0 = nil, *t1 = nil, *ctmp = nil, dummy;
    int tmp = 0;
    size_t len = 0;
    MList *region = nil;
    UList *userdata = nil, *tmpnode = nil;

    debugln;
    region = init();
    debugln;
    t0 = (char *) weakalloc(sizeof(char) * 128, region); // we turn around & retain in str2lst, so...
    debugln;
    t1 = (char *) weakalloc(sizeof(char) * 128, region); // we turn around & retain in str2lst, so...
    debugln;
    if(debugging) {
        printf("t0 stats: \n");
        printstats(t0, region);
        printf("t1 stats: \n");
        printstats(t1, region);
    }
    printf("Enter a string: ");
    fgets(t0, 128, stdin);
    len = strnlen(t0, 128);
    t0[len - 1] = nul;
    printf("you entered: %s\n", t0);
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
    scanf("%c", &dummy);
    if(debugging) {
        printf("t1 == nil? %s\n", t1 == nil ? "yes" : "no");
    }
    printf("Enter a string: ");
    fgets(t1, 128, stdin);
    len = strnlen(t1, 128);
    t1[len - 1] = nul;
    if(debugging) {
        if(ctmp == nil) {
            printf("fgets(3) returned nil!\n");
        } else if(feof(stdin)) {
            printf("error on stdin?");
        } else {
            printf("should have entered a string... but didn't?\n");
        }
        printf("strlens: %lu and %lu\n", strnlen(t0, 128), strnlen(t1, 128));
    }
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
        release(tmpnode, region);
    }
    release(t0, region);
    release(t1, region);

    printf("\nsome statistics: \n");
    printf("allocation calls: %d\n", acall);
    printf("weak allocation calls: %d\n", wcall);
    printf("retain calls: %d\n", retcall);
    printf("release calls: %d\n", relcall);
    clean(region);

    return 0;
}

MList
*init(){
    MList *tmp = (MList *)malloc(sizeof(MList));
    tmp->next = nil;
    tmp->count = -1;
    tmp->size = 0;
    return tmp;
}

void
printstats(void *object, MList *region) {
    uint8_t flag = 0;
    MList *tmp = region;
    printf("printing stats for %p\n", object);
    while(tmp != nil) {
        if(tmp-> object == object) {
            flag = 1;
            printf("size: %lu\n", tmp->size);
        }
        tmp = tmp->next;
    }

    if(!flag) {
        printf("object not found\n");
    }
}

void *
rcalloc(size_t sze, MList *region) {
    MList *tmp = region;
    void *data = nil;

    acall += 1;

    while(tmp->next != nil) {
        tmp = tmp->next;
    }

    tmp->next = (MList *)malloc(sizeof(MList));
    tmp = tmp->next;
    tmp->next = nil;
    tmp->count = 1;
    tmp->size = sze;

    data = malloc(sze);
    if(debugging) {
        printf("allocated %p and %p\n", tmp, data);
    }
    tmp->object = data;
    return data;
}

void
retain(void *object, MList *region) {
    MList *tmp = region;

    retcall += 1;

    while(tmp != nil) {
        if(tmp->object == object) {
            tmp->count += 1;
            break;
        }
        tmp = tmp->next;
    }
}

void
release(void *object, MList * region) {
    MList *tortoise = region, *hare = region;

    relcall += 1;

    while(hare != nil) {
        if(hare->object == object) {
            hare->count -= 1;
            if(hare->count <= 0) {
                tortoise->next = hare->next;
                if(debugging) {
                    printf("freeing two hares: %p and %p\n", hare, hare->object);
                }
                free(hare->object);
                free(hare);
                break;
            }
        }
        tortoise = hare;
        hare = hare->next;
    }

}

void *
weakalloc(size_t sze, MList *region) {
    MList *tmp = region;
    void *data = nil;

    wcall += 1;

    debugln;
    while(tmp->next != nil) {
        tmp = tmp->next;
    }
    debugln;
    tmp->next = (MList *)malloc(sizeof(MList));
    tmp = tmp->next;
    tmp->next = nil;
    tmp->count = 0;
    tmp->size = sze;
    debugln;
    data = malloc(sze);
    tmp->object = data;
    debugln;
    if(debugging) {
        printf("weakly allocated %p and %p\n", tmp, data);
    }
    return data;
}

void
clean(MList *region) {
    MList *tmp = region, *tmp0;

    while(tmp != nil) {
        tmp0 = tmp;
        tmp = tmp->next;
        if(debugging) {
            printf("cleaning two objects: %p and %p\n", tmp0, tmp0->object);
        }
        if(tmp0->object != nil && tmp0->size > 0) {
            free(tmp0->object);
        }
        free(tmp0);
    }
}

UList *
str2lst(char * str, size_t sze, MList *region) {
    UList *cell = (UList *)rcalloc(sizeof(UList), region);
    retain(str, region);
    cell->type = LSTRING;
    cell->object.s = str;
    return cell;
}

UList *
int2lst(int i, MList *region) {
    UList *cell = (UList *)rcalloc(sizeof(UList), region);
    cell->type = LINT;
    cell->object.i = i;
    return cell;
}

UList *
cons(UList *hd, UList *tl, MList *region) {
    UList *cell = rcalloc(sizeof(UList), region);
    cell->type = LLIST;
    cell->object.l = hd;
    cell->next = tl;
    return cell;
}

UList *
car(UList *hd) {
    if(hd != nil && hd->type == LLIST) {
        return hd->object.l;
    }
    return nil;
}

UList *
cdr(UList *hd) {
    if(hd != nil && hd->type == LLIST) {
        return hd->next;
    }
    return nil;
}

void
walk(UList *hd) {
    UList *tmp = hd;

    if(tmp->type != LLIST) {
        return;
    }

    printf("(");
    while(tmp != nil) {
        print(tmp->object.l);
        if(tmp->next != nil) {
            printf(" ");
        }
        tmp = tmp->next;
    }

    printf(")");
}

void
print(UList *hd){
    if(hd->type == LINT) {
        printf("%d", hd->object.i);
    } else if(hd->type == LSTRING) {
        printf("\"%s\"", hd->object.s);
    } else {
        walk(hd->object.l);
    }
}
