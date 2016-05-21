/////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
//
//
//
//
//
/////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <cstring>
#include "port.h"
#include <inttypes.h>

#include <unistd.h>

#include "fio.h"
#include "compress_fxe3.h"
#include "headers.h"


using namespace Headers;
//
//
//

#define NBLOCKS 16
#define BLOCK_BUFFER 32768+16384
#define FXEP_BLOCK_SIZE FXE0_BLOCK_SIZE

// We'll take the larger buffer..
// And also reserve same extra space to avoid
// writing to disk too often..

// static stuff..

namespace {
	// Some local functions...

	void usage( char ** );

	// static constants..

	enum FXEPalgorithm { s405_abs=0 };
	FXEPalgorithm algo = s405_abs;
	static bool RAW = false;

	// Our write buffers..

	unsigned char buf[BLOCK_BUFFER];

	//
	// Define compression parameters..
	//

	struct _compressionParams {
		unsigned char* buffer;
		long bufferSize;
		int goodMatch;
		int maxMatch;
		int minMatchThres;
		int minOffsetThres;
		int maxChain;
		int numLazy;
		int numDistBits;
		int windowSize;
		int (*saveHeader)( FWriter*, char*, fixInfo*, ... );
		int (*fixHeader)( FWriter*, fixInfo*  );
		int (*saveDecompressor)( FWriter*, fixInfo*  );
		int (*fixDecompressor)( FWriter*, const fixInfo* );
		int (*postFix)( FWriter*, fixInfo* );
	} compressionParams[] = {
		// S405_abs
		{ 	buf, BLOCK_BUFFER, FXE3_GOOD_MATCH, FXE3_MAX_MATCH, 2, 32, 4096, 1, 6+8, FXE3_BLOCK_SIZE,
			saveS405ABSHunkHeader,fixS405ABSHunkHeader, saveS405ABSHeader, fixS405ABSHeader,
            saveS405ABSHunkTrailer
        }
	};

	// misc functions..
	//
	//

	void usage( char **argv ) {
		cerr << "Usage: " << argv[0] << " [-<options>] infile outfile" << endl;
		cerr << "Options:" << endl;
		cerr << "  -A n      Select algorithm (0=S405 absolute/data), default=0" << endl;
		cerr << "  -h        Show help" << endl;
		cerr << "  -d        Handle file as raw data" << endl;
		cerr << "  -s addr   Load address in hex for the decompressed data" << endl;
        cerr << "  -j addr   Execute start address in hex for the compressed data" << endl;
        cerr << "  -w addr   Location in hes for the work area (0xB00 bytes)" << endl;
        cerr << "  -e n      Select decrompression effect (none=0, color0=1), default=1" << endl;
        exit(1);
	}
  
  	//
  	//
  	//
  
  	#define FIXHDRSIZE 28
  
};

//
//
//


int main( int argc, char** argv ) {
	Compress* compr;
	unsigned long ID;
	bool dummy = true;
	unsigned char* tb;
	LZParams lzp;
	FReader rfile;
	FWriter wfile;

	int ch;
	char *title = NULL;

	fixInfo fix;
	char* infile;


    // constants..

	cout << endl << "StoneCracker v5 (5b) - (c) 1994-2016 Jouni 'Mr.Spiv' Korhonen" << endl << endl;

  	// Check commandline & options..

	while ((ch = getopt(argc,argv,"t:A:hds:j:w:e:")) != -1) {
		switch (ch) {
		case 't':	// title
			title = optarg;
			break;
		case 'A':
			algo = static_cast<FXEPalgorithm>(atoi(optarg));
			break;
		case 'd':	//
			RAW = true;
			break;
        case 's':
            fix.load = strtol(optarg,NULL,16);
            break;
        case 'j':
            fix.jump = strtol(optarg,NULL,16);
            break;
        case 'w':
            fix.work = strtol(optarg,NULL,16);
            break;
		case 'e':
            if (atoi(optarg) == 0) {
                fix.flash = 0xdff1fe;
            }
            break;
        case '?':
		case 'h':
		default:
			usage(argv);
		}
	}

	if (optind >= argc) {
		usage(argv);
		return 0;
	}

	// default name..
	
	if (title == NULL) { title = argv[optind]; }


	// Get the compressor..

	switch (algo) {
	case s405_abs:
		//cout << "compr = new CompressFXE2(NBLOCKS);" << endl;
		compr = new CompressFXE3(NBLOCKS);
    	ID = S405_TYPE_ID;
		break;
	default:
		cerr << "** Error: unknown compression algorithm.." << endl;
		return 0;
		break;
	}

	//
	// Setup rest of LZ parameters..
	//

	CallBack cback(compressionParams[algo].windowSize);
	

	lzp.buf 			= compressionParams[algo].buffer;
	lzp.bufSize 		= compressionParams[algo].bufferSize;
	lzp.goodMatch 		= compressionParams[algo].goodMatch;
	lzp.maxMatch 		= compressionParams[algo].maxMatch;
	lzp.minMatchThres 	= compressionParams[algo].minMatchThres;
	lzp.minOffsetThres 	= compressionParams[algo].minOffsetThres;
	lzp.maxChain 		= compressionParams[algo].maxChain;
	lzp.numLazy 		= compressionParams[algo].numLazy;
	lzp.numDistBits 	= compressionParams[algo].numDistBits;
	lzp.windowSize 		= compressionParams[algo].windowSize;
	lzp.r 				= &rfile;
	lzp.c 				= &cback;

	//
	// Do the size fix for incorrect files..
	// Also check for code & data section gaps
	//
	//

	infile = argv[optind];

	//
	//
	//

	compr->init(&lzp);

	// Open files..

	rfile.open(infile,IO_READ);
	wfile.open(argv[optind+1],IO_CREATE);

	if (!rfile.isOpen()) {
		delete compr;
		cerr << "** Error: failed to open file '" << infile << "'.." << endl;
		return 0;
	}
	if (!wfile.isOpen()) {
		delete compr;
		rfile.close();
		cerr << "** Error: failed to open file '" << argv[optind+1] << "'.." << endl;
		return 0;
	}

	//
	//
	//
	// Save headers..
	//

	long origiFileSize = 0;
	long n;

	rfile.seek(0,IO_SEEK_END);
	origiFileSize = rfile.tell();
	rfile.seek(0,IO_SEEK_SET);

	// Now we are ready to go.. first prepare the compressed
	// file header information..

    if (!RAW) {
        int n, m;
        n = compressionParams[algo].saveHeader(&wfile, NULL, &fix);
        m = compressionParams[algo].saveDecompressor(&wfile, &fix );
        
        if (n < 0 || m < 0) {
		    cerr << "** Error writing file (2).." << endl;
		    rfile.close();
		    wfile.close();
		    delete compr;
		    return 0;
	    }
    }

	tb = buf;
	PUTL(tb,ID);
	wfile.write(buf,4);

	fix.tsize += 4;
	fix.csize += 4;

	cout << "Crunching (0/" << origiFileSize << ")";
	cout.flush();

	//
	//
	//

	ch = 10;

	while ((n = compr->compressBlock(&lzp,dummy)) > 0) {
		unsigned short bl;
    	tb = buf;
 		GETW(tb,bl);

		// Fix block size NOT to include FXEP_BLOCK_HEADER

		bl -= FXEP_BLOCK_HEADER_SIZE;
		tb = buf;
		PUTW(tb,bl);

		if (wfile.write(buf,n) != n) {
			n = -1;
			break;
		}
		fix.csize += n;
		fix.tsize += n;

		if (--ch == 0) {
			cout << "\rCrunching (" << compr->getOriginalSize() << "/" << origiFileSize << ")";
			cout.flush();
			ch = 10;
		}
	}

    // Original file must be even bytes length.. if not add an rtificial
    // byte.. otherwise the decompressor has issues..

    if (origiFileSize & 1) {
        cout << "\rWarning: input file has no even bytes length.. rounding up..";
        cout.flush();
        ++origiFileSize;
    }

	//
	// Finish the output file.. add EOF
	//

	if (n == 0) {
		tb = buf;
		
		// check for a proper EOF mark
		
		switch (compr->getTypeID()) {
		default:
			PUTW(tb,FXEP_EOF);
			break;
		case S405_TYPE_ID:
			PUTW(tb,FXEP_EOF_NEW);
			break;
		}
	
        PUTL(tb,origiFileSize);

		if (wfile.write(buf,6) != 6) {
			cerr << "** Error writing file (1).." << endl;
			rfile.close();
			wfile.close();
			delete compr;
			return 0;
		}
		fix.csize += 6;
		fix.tsize += 6;
    }

    //cerr << "compr: " << fix.csize << endl;

    //
	// Fix headers..
	//

	fix.osize = origiFileSize;

    if (!RAW) {
        compressionParams[algo].postFix(&wfile,&fix);
        compressionParams[algo].fixHeader(&wfile,&fix);
        compressionParams[algo].fixDecompressor(&wfile,&fix);
    }

	//
	// Calculate stuff..
	//

	double gain = 100.0 - static_cast<double>(fix.tsize)
                / static_cast<double>(fix.osize) * 100.0;

	cout.precision(3);
	cout << "\rCrunched " << gain << "% - total " << fix.tsize << " bytes" << endl;

	//
	//
	//

	rfile.close();
	wfile.close();
	delete compr;

	//

  return 0;
}


