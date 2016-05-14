#include <stdio.h>
#include <stdint.h>



#define MAXDEP 15
#define MAXSYM 16

struct node {
    union {
        uint32_t code;
        struct {
            uint16_t count;
            uint16_t prefix;
        } p;
        struct {
            uint16_t index;
            uint16_t prefix;
        } i;
        struct {
            uint16_t index;
            uint16_t length;
        } l;
    } c;
};

//

static struct node codes[2*MAXDEP] = {0};
static uint8_t  symbols[MAXSYM] = {1,2,3,5,6,7,7,7,7,7,7,7,7,7,0,0};
static uint16_t outsyms[MAXSYM] = {0};

void dumpCodes( char *str ) {
    int n;
    printf("** %s\n",str);

    for (n=0; n<MAXDEP+1; n++) {
        printf("%2d: %04x %04x\n",n,codes[n].c.p.count,codes[n].c.p.prefix);
    }
    fflush(stdout);
}

void dumpSymbols( void ) {
    int n;

    printf("** Symbols and output symbols\n");
    for (n=0; n < MAXSYM; n++) {
        printf("%2d: %2d - %2d\n",n,symbols[n],outsyms[n]);
    }
}

void dumpCde( void ) {
    int n;

    printf("** cde array..\n");
    for (n=0; n<(MAXDEP+1)*2; n++) {
        printf("%2d: %08x \n",n,codes[n].c.code);
    }
}


void symbols2codes( void ) {
    int n,i;
    uint16_t prefix;
    uint16_t index;
    uint16_t tmp;
    uint32_t cde;

    // count how many symbols/leaves..
    for (n=0; n<MAXSYM; n++) {
        if ((i = symbols[n]) > 0) {
            ++codes[i].c.p.count;
        }
    }

    //

    dumpCodes("Codes after calculating counts..");

    index = 0;
    prefix = 0;

    for (n=1; n<MAXDEP+1; n++) {
        if ((tmp = codes[n].c.p.count) > 0) {
            //codes[n].c.i.prefix = prefix;
            codes[n].c.i.prefix = prefix+tmp;
            codes[n].c.i.index = index;
            prefix += tmp;
            index += tmp;
        }
        prefix <<= 1;
    }

    //  

    dumpCodes("Codes after calculating bases..");

    //

    for (n=0; n<MAXSYM; n++) {
        if ((index = symbols[n]) > 0) {
            outsyms[codes[index].c.i.index++] = n;
            //++codes[index].c.i.index;
            //++codes[index].c.i.prefix;
        }
    }

    dumpCodes("Codes after calculating prefixes..");
    dumpSymbols();

    // Calculate codes..

    for (i=0,n=1; n<MAXDEP+1; n++) {
        if ((index = codes[n].c.i.index) > 0) {
            prefix = codes[n].c.i.prefix;
            cde = (0xffffffff >> n) | ((prefix-1) << (32-n));
            codes[i++].c.code = cde;
            codes[i+MAXDEP].c.l.length = n;
            codes[i+MAXDEP].c.l.index = (prefix-index);
        }
    }

    //

    dumpCde();


}




int main( int argc, char **argv ) {
    symbols2codes();
    return 0;
}
