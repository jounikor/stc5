//
// Disclaimer.. some file io error checking is blatantly ignored ;)
//
//

#include <iostream>
#include <cstring>
#include <inttypes.h>

#include "headers.h"
#include "bitutils.h"
#include "fio.h"

//
//
//
//

static unsigned char s405_abs_hunk_header[S405_HUNK_HEADER_SIZE] = {
    0x00,0x00,0x03,0xf3,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x03,0xe9,0x00,0x00,0x00,0x00
};

static unsigned char s405_abs_hunk_trailer[S405_HUNK_TRAILER_SIZE] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xf2
};

unsigned char s405_abs_dec[S405_ABS_DEC_SIZE] = {    // 436
	0x41,0xfa,0x01,0xb6,0x48,0x79,0x00,0x06,
	0x00,0x00,0x43,0xf9,0x00,0x06,0x00,0x00,
	0x45,0xf9,0x00,0x07,0xf5,0x00,0x2c,0x49,
	0x28,0x48,0x30,0x1c,0xd8,0xc0,0x66,0xfa,
	0xdd,0xd4,0xb9,0xc9,0x63,0x10,0xb9,0xce,
	0x62,0x0c,0x26,0x4e,0x55,0x8c,0x17,0x24,
	0xb9,0xc8,0x62,0xfa,0x20,0x4b,0x54,0x48,
	0x4a,0x60,0x66,0x12,0x2c,0x78,0x00,0x04,
	0x0c,0x6e,0x00,0x25,0x00,0x14,0x65,0x04,
	0x48,0x6e,0xfd,0x84,0x4e,0x75,0x2e,0x18,
	0x48,0x47,0x7c,0x00,0x47,0xea,0x08,0x88,
	0x28,0x4a,0x4b,0xea,0x00,0x98,0x76,0x10,
	0x78,0x1d,0x7a,0x00,0x16,0xc5,0x20,0x07,
	0xe8,0xa8,0x1a,0xc0,0x72,0x03,0x61,0x78,
	0x52,0x45,0xb6,0x45,0x66,0xee,0x61,0x00,
	0x00,0xac,0xeb,0x4b,0x61,0x7c,0xe2,0x4b,
	0x61,0x78,0x61,0x76,0x47,0xea,0x00,0x98,
	0x61,0x46,0x94,0x43,0x6e,0x06,0x67,0xa8,
	0x12,0xc2,0x60,0xf0,0x53,0x42,0x6f,0x24,
	0x3a,0x02,0x47,0xea,0x05,0x10,0x61,0x30,
	0x38,0x02,0x66,0x04,0x38,0x0d,0x60,0x12,
	0x08,0x84,0x00,0x07,0x67,0x0c,0xe1,0x4c,
	0x47,0xea,0x07,0x88,0x61,0x1a,0x18,0x02,
	0x3a,0x44,0x38,0x44,0x30,0x05,0x26,0x49,
	0x97,0xcc,0x12,0xdb,0x51,0xc8,0xff,0xfc,
	0x33,0xc6,0x00,0xdf,0xf1,0x80,0x60,0xb4,
	0xbe,0x9b,0x62,0xfc,0x4c,0xab,0x00,0x03,
	0x00,0x38,0x24,0x07,0x42,0x42,0xe3,0xba,
	0x94,0x40,0xd4,0x42,0x34,0x33,0x20,0xfc,
	0xbc,0x41,0x6c,0x08,0xed,0xaf,0x3e,0x18,
	0x92,0x46,0x7c,0x10,0x9c,0x41,0xe3,0xaf,
	0x4e,0x75,0x30,0x03,0xd0,0x40,0x4b,0xf4,
	0x00,0x78,0x3a,0x03,0x26,0x4a,0x61,0xc8,
	0x47,0xea,0x08,0x88,0xd6,0xc2,0x30,0x02,
	0x14,0x13,0x60,0x04,0x17,0x63,0x00,0x01,
	0x51,0xc8,0xff,0xfa,0x16,0x82,0x1a,0xc2,
	0x53,0x45,0x66,0xe0,0x2a,0x4c,0x70,0x0f,
	0x42,0x9d,0x53,0x40,0x66,0xfa,0x32,0x03,
	0xd2,0x41,0xd2,0x43,0x47,0xf4,0x10,0x78,
	0x34,0x03,0x10,0x23,0x67,0x08,0xd0,0x00,
	0xd0,0x00,0x52,0x74,0x00,0xfe,0x53,0x42,
	0x66,0xf0,0x2a,0x4c,0x70,0x0f,0x7a,0x3c,
	0x28,0x1d,0x67,0x0a,0xd4,0x44,0x48,0xad,
	0x00,0x24,0xff,0xfc,0xda,0x44,0xd4,0x42,
	0x53,0x40,0x66,0xec,0x72,0x00,0x12,0x33,
	0x00,0x00,0x67,0x12,0xd2,0x01,0xd2,0x01,
	0x34,0x34,0x10,0xfe,0x52,0x74,0x10,0xfe,
	0xd4,0x42,0x39,0x80,0x20,0x00,0x52,0x40,
	0xb6,0x40,0x66,0xe2,0x2a,0x4c,0x72,0x01,
	0x78,0xff,0x7a,0x00,0x30,0x1c,0x34,0x1c,
	0x67,0x16,0xe3,0xac,0x88,0x40,0x53,0x44,
	0xe2,0xbc,0x2a,0xc4,0x94,0x45,0x90,0x42,
	0x48,0xad,0x00,0x03,0x00,0x38,0x54,0x45,
	0x52,0x41,0x0c,0x41,0x00,0x10,0x66,0xdc,
	0x28,0x4b,0x4e,0x75
};


/*  v5c
static unsigned char s405_abs_dec[S405_ABS_DEC_SIZE] = {
	0x41,0xfa,0x01,0xc4,0x48,0x79,0x00,0x04,
	0x00,0x00,0x43,0xf9,0x00,0x04,0x00,0x00,
	0x45,0xf9,0x00,0x07,0xf5,0x00,0x2c,0x49,
	0x28,0x48,0x30,0x1c,0xd8,0xc0,0x66,0xfa,
	0xdd,0xd4,0xb9,0xc9,0x63,0x10,0xb9,0xce,
	0x62,0x0c,0x26,0x4e,0x55,0x8c,0x17,0x24,
	0xb9,0xc8,0x62,0xfa,0x20,0x4b,0x54,0x48,
	0x55,0x88,0xbd,0xc9,0x62,0x12,0x2c,0x78,
	0x00,0x04,0x0c,0x6e,0x00,0x25,0x00,0x14,
	0x65,0x04,0x48,0x6e,0xfd,0x84,0x4e,0x75,
	0x2e,0x18,0x7c,0x00,0x48,0x47,0x47,0xea,
	0x08,0x88,0x28,0x4a,0x4b,0xea,0x00,0x98,
	0x76,0x10,0x78,0x1d,0x7a,0x00,0x16,0xc5,
	0x20,0x07,0xe8,0xa8,0x1a,0xc0,0x72,0x03,
	0x61,0x7c,0x52,0x45,0xb6,0x45,0x66,0xee,
	0x61,0x00,0x00,0xb4,0xeb,0x4b,0x49,0xea,
	0x00,0x98,0x61,0x7e,0xe2,0x4b,0x61,0x7a,
	0x61,0x78,0x47,0xea,0x00,0x98,0x61,0x46,
	0x94,0x43,0x6e,0x06,0x67,0xa2,0x12,0xc2,
	0x60,0xf0,0x53,0x42,0x6f,0x24,0x3a,0x02,
	0x47,0xea,0x05,0x10,0x61,0x30,0x38,0x02,
	0x66,0x04,0x38,0x0d,0x60,0x12,0x08,0x84,
	0x00,0x07,0x67,0x0c,0xe1,0x4c,0x47,0xea,
	0x07,0x88,0x61,0x1a,0x18,0x02,0x3a,0x44,
	0x38,0x44,0x30,0x05,0x26,0x49,0x97,0xcc,
	0x12,0xdb,0x51,0xc8,0xff,0xfc,0x33,0xc6,
	0x00,0xdf,0xf1,0x80,0x60,0xb4,0xbe,0x9b,
	0x62,0xfc,0x4c,0xab,0x00,0x03,0x00,0x38,
	0x24,0x07,0x42,0x42,0xe3,0xba,0x94,0x40,
	0xd4,0x42,0x34,0x33,0x20,0xfc,0x9c,0x41,
	0x6c,0x0c,0xdc,0x41,0xed,0xaf,0x3e,0x18,
	0x92,0x46,0x7c,0x10,0x9c,0x41,0xe3,0xaf,
	0x4e,0x75,0x30,0x03,0xd0,0x40,0x4b,0xf4,
	0x00,0x78,0x7a,0x00,0x26,0x4a,0x61,0xc6,
	0x47,0xea,0x08,0x88,0xd6,0xc2,0x30,0x02,
	0x14,0x13,0x60,0x04,0x17,0x63,0x00,0x01,
	0x51,0xc8,0xff,0xfa,0x16,0x82,0x1a,0xc2,
	0x52,0x45,0xb6,0x45,0x66,0xde,0x2a,0x4c,
	0x70,0x0e,0x42,0x9d,0x51,0xc8,0xff,0xfc,
	0x32,0x03,0xd2,0x41,0xd2,0x43,0x47,0xf4,
	0x10,0x78,0xd0,0x43,0x72,0x00,0x12,0x23,
	0x67,0x08,0xd2,0x01,0xd2,0x01,0x52,0x74,
	0x10,0xfe,0x51,0xc8,0xff,0xf2,0x2a,0x4c,
	0x70,0x0e,0x74,0x00,0x7a,0x3c,0x28,0x1d,
	0x67,0x0a,0xd4,0x44,0x48,0xad,0x00,0x24,
	0xff,0xfc,0xda,0x44,0xd4,0x42,0x51,0xc8,
	0xff,0xee,0x70,0x00,0x12,0x33,0x00,0x00,
	0x67,0x12,0xd2,0x01,0xd2,0x01,0x34,0x34,
	0x10,0xfe,0x52,0x74,0x10,0xfe,0xd4,0x42,
	0x39,0x80,0x20,0x00,0x52,0x40,0xb6,0x40,
	0x66,0xe2,0x2a,0x4c,0x72,0x01,0x78,0xff,
	0x7a,0x00,0x30,0x1c,0x34,0x1c,0x67,0x16,
	0xe3,0xac,0x88,0x40,0x53,0x44,0xe2,0xbc,
	0x2a,0xc4,0x94,0x45,0x90,0x42,0x48,0xad,
	0x00,0x03,0x00,0x38,0x54,0x45,0x52,0x41,
	0x0c,0x41,0x00,0x10,0x66,0xdc,0x28,0x4b,
	0x4e,0x75
};
*/

namespace Headers {

    int saveS405ABSHunkHeader( FWriter* w, char* str, fixInfo* fix, ... ) {
        int n;
        fix->headerPos = w->tell();
        n = w->write(s405_abs_hunk_header,S405_HUNK_HEADER_SIZE);

        if (n == S405_HUNK_HEADER_SIZE) {
            fix->tsize = S405_HUNK_HEADER_SIZE;
        }
        return n;
    }

    int fixS405ABSHunkHeader( FWriter* w, fixInfo* fix ) {
        // fix hunk header
        uint8_t* b;
        uint8_t bb[4];
        long t;

        b = bb;
        t = fix->csize >> 2;
        PUTL(b,t);
        w->seek(fix->headerPos+20,IO_SEEK_SET);
        w->write(bb,4);
        w->seek(fix->headerPos+28,IO_SEEK_SET);
        w->write(bb,4);
        w->seek(0,IO_SEEK_END);
        return 0;
    }

	int saveS405ABSHeader( FWriter* w, fixInfo* fix ) {
        int m;

        fix->decompPos = w->tell();
        m = w->write(s405_abs_dec,S405_ABS_DEC_SIZE);
        fix->tsize += m;
        fix->csize += m;
        return m;
	}
	
	int fixS405ABSHeader( FWriter* w, const fixInfo* fix ) {
        unsigned char b[4];
        unsigned char *p;

        // fix decruncher jump address
        
        p = b;
        PUTL(p,fix->jump);
        w->seek(fix->decompPos+6,IO_SEEK_SET);
        w->write(b,4);
        
        // fix decruncher load address

        p = b;
        PUTL(p,fix->load);
        w->seek(fix->decompPos+12,IO_SEEK_SET);
        w->write(b,4);

        
        // fix decruncher work address
        
        p = b;
        PUTL(p,fix->work);
        w->seek(fix->decompPos+18,IO_SEEK_SET);
        w->write(b,4);
        
        // fix decruncher effect address
        p = b;
        PUTL(p,fix->flash);
        w->seek(fix->decompPos+202,IO_SEEK_SET);
        return w->write(b,4);

	} 

    int saveS405ABSHunkTrailer( FWriter* w, fixInfo* fix ) {
        int n,m;
        
        w->seek(0,IO_SEEK_END);

        if ((m = fix->tsize & 3) == 0) {
            m = 4;
        }
        
        n = w->write(s405_abs_hunk_trailer+m,S405_HUNK_TRAILER_SIZE-m);
        fix->trailerPos = w->tell();
        fix->tsize += n;
        fix->csize += 4-m;
        return n;
    }

	//
	//
	//
};
