#ifndef _headers_h_included
#define _headers_h_included

#include "fio.h"

// S405 header..

#define S405_HUNK_HEADER_SIZE   32
#define S405_ABS_DEC_SIZE       450
#define S405_HUNK_TRAILER_SIZE  8

// Common stuff

#define FXE_HEADER_SIZE 1128
#define FXE_TOTALFILESIZE_POS 4
#define FXE_TITLE_POS 12
#define FXE_AUTHOR_POS 44
#define FXE_ICON_POS 92
#define FXE_FILESIZE_POS 1116

// config structure for header post fixing

struct fixInfo {
	long osize;		// original file size
	long csize;		// compressed file size
    long tsize;     // total size including headers and such
	long flash;		// flash effect address
    long load;      // load address
    long jump;      // jump address
    long work;      // work area address

    long headerPos;     // start of the header.. should be 0
    long decompPos;     // start of decompressor
    long trailerPos;    // stat of trailer

	fixInfo() {
        load = 0x40000;
        jump = 0x40000;
        work = 0x80000-0xb00;
		flash = 0xdff180;
        headerPos = 0;
        decompPos = 0;
        trailerPos = 0;
        osize = csize = tsize = 0;
    }
	~fixInfo() {}

};

//

namespace Headers {
    int saveS405ABSHunkHeader( FWriter*, char*, fixInfo*, ... );
	int fixS405ABSHunkHeader( FWriter*, fixInfo* );
    int saveS405ABSHeader( FWriter*, fixInfo* );
	int fixS405ABSHeader( FWriter*, const fixInfo* );
	int saveS405ABSHunkTrailer( FWriter*, fixInfo* );




	//

};

#endif
