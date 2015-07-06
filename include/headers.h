#ifndef _headers_h_included
#define _headers_h_included

#include "fio.h"

// S405 header..

#define FXE3_CRT0_SIZE 			840
#define FXE3_FIX_DATASTART_POS	12
#define FXE3_FIX_DATAEND_POS	16
#define FXE3_FIX_BSSSTART_POS	20
#define FXE3_FIX_BSSEND_POS		24

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
	long flash;		// flash effect address
    long load;      // load address
    long jump;      // jump address
    long work;      // work area address

	fixInfo() {
        load = 0x40000;
        jump = 0x40000;
        work = 0x80000-0xa20;
		flash = 0xdff180;
	}
	~fixInfo() {}

};

//

namespace Headers {
	int saveFXEHeader( int, FWriter*, char*, char*, char*, char*, char* );
	int fixFXEHeader( int, FWriter*, long );
	int saveS405ABSHeader( FWriter* );
	int fixS405ABSHeader( FWriter*, long, const fixInfo* );

	//

	extern"C" unsigned char s405_abs_dec[];
};



#endif
