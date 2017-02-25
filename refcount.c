#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define nil NULL
#define nul '\0'

#define debugln printf("here %d\n", __LINE__)

/* using a flat list of memory references
 * right now; would be neat to use something
 * like a HAMT or even just an AVL/BTree 
 * for this, but for a simple test, it's easy
 * to start here.
 */
typedef struct MLIST {
    void *object;
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
    char *t0 = nil, *t1 = nil, dummy;
    int tmp = 0;
    MList *region = nil;
    UList *userdata = nil, *tmpnode = nil;

    debugln;
    region = init();
    debugln;
    t0 = (char *) weakalloc(sizeof(char) * 128, region); // we turn around & retain in str2lst, so...
    debugln;
    t1 = (char *) weakalloc(sizeof(char) * 128, region); // we turn around & retain in str2lst, so...
    debugln;
    printf("Enter a string: ");
    fgets(t0, 128, stdin);
    printf("you entered: %s\n", t0);
    tmpnode = str2lst(t0, sizeof(char) * 128, region); 
    userdata = cons(tmpnode, nil, region);
    scanf("%c", &dummy); 
    while(tmp != -1) {
        printf("Enter an integer: ");
        scanf("%d", &tmp);
        if(tmp != -1) {
            tmpnode = int2lst(tmp, region);
            userdata = cons(tmpnode, userdata, region);
        }
    }
    printf("Enter a string: ");
    scanf("%128s", t1);
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

    printf("some statistics: ");
    printf("allocation calls: %d\n", acall);
    printf("weak allocation calls: %d\n", wcall);
    printf("retain calls: %d\n", retcall);
    printf("release calls: %d\n", relcall);

    clean(region);
}

MList
*init(){
    MList *tmp = (MList *)malloc(sizeof(MList));
    tmp->next = nil;
    tmp->count = -1;
    return tmp;
}

void *
rcalloc(size_t sze, MList *region) {
    MList *tmp = region;
    void *data = nil;

    while(tmp->next != nil) {
        tmp = tmp->next;
    }

    tmp->next = (MList *)malloc(sizeof(MList));
    tmp = tmp->next;
    tmp->next = nil;
    tmp->count = 1;

    data = malloc(sze);
    tmp->object = data;
    return data;
}

void
retain(void *object, MList *region) {
    MList *tmp = region;

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

    while(hare != nil) {
        if(hare->object == object) {
            hare->count -= 1;
            if(hare->count <= 0) {
                tortoise->next = hare->next;
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

    debugln;
    while(tmp->next != nil) {
        tmp = tmp->next;
    }
    debugln;
    tmp->next = (MList *)malloc(sizeof(MList));
    tmp = tmp->next;
    tmp->next = nil;
    tmp->count = 0;
    debugln;
    data = malloc(sze);
    tmp->object = data;
    debugln;
    return data;
}

void
clean(MList *region) {
    MList *tmp = region, *tmp0;

    while(tmp != nil) {
        tmp0 = tmp;
        tmp = tmp->next;
        free(tmp0->object);
        free(tmp);
    }
}

UList *
str2lst(char * str, size_t sze, MList *region) {
    UList *cell = (UList *)rcalloc(sizeof(UList), region);
    retain(str, region);
    cell->object.s = str;
    return cell;
}

UList *
int2lst(int i, MList *region) {
    UList *cell = (UList *)rcalloc(sizeof(UList), region);
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

    while(tmp != nil) {
        print(tmp->object.l);
        tmp = tmp->next;
    }
}

void
print(UList *hd){
    if(hd->type == LINT) {
        printf("%d ", hd->object.i);
    } else if(hd->type == LSTRING) {
        printf("%s ", hd->object.s);
    } else {
        walk(hd->object.l);
    }
}
