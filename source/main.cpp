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

#include <unistd.h>

#include "fio.h"
#include "compress_fxe3.h"
#include "compress_fxe4.h"
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

	enum FXEPalgorithm { fxe0=0, fxe1, fxe2, fxe3, fxe4 };
	FXEPalgorithm algo = fxe3;
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
		int (*saveHeader)( int, FWriter*, char*, char*, char*, char*, char* );
		int (*fixHeader)( int, FWriter*, long );
		int (*saveDecompressor)( FWriter* );
		int (*fixDecompressor)( FWriter*, long, const fixInfo* );
		int (*postFix)( FWriter*, long );
	} compressionParams[] = {
		// FXE0
		{ 	NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			NULL,NULL, NULL, NULL, NULL },
		// FXE1
		{ 	NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			NULL,NULL, NULL, NULL, NULL },
		// FXE2
		{ 	NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			NULL,NULL, NULL, NULL, NULL },
		// FXE3
		{ 	buf, BLOCK_BUFFER, FXE3_GOOD_MATCH, FXE3_MAX_MATCH, 2, 32, 4096, 1, 6+8, FXE3_BLOCK_SIZE,
			saveFXEHeader,fixFXEHeader, saveFXE3Header, fixFXE3Header, NULL },
		// FXE4
		{ 	buf, BLOCK_BUFFER, FXE4_GOOD_MATCH, FXE4_MAX_MATCH, 2, 32, 4096, 1, 6+8, FXE4_BLOCK_SIZE,
			saveFXEHeader,fixFXEHeader, NULL, NULL, NULL }
	};

	// misc functions..
	//
	//

	void usage( char **argv ) {
		cerr << "Usage: " << argv[0] << " [-<options>] infile outfile" << endl;
		cerr << "Options:" << endl;
		//cerr << "  -t string Set game title (32 chars max, default=infile)" << endl;
		//cerr << "  -A n      Select algorithm (0=FXE0, 2=FXE2, 3=STC5, 4=STC6), default=3" << endl;
		cerr << "  -h        Show help" << endl;
		cerr << "  -d        Handle file as raw data" << endl;
		
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

	//

	cout << endl << "StoneCracker v5 - (c) 1994-2014 Jouni 'Mr.Spiv' Korhonen" << endl << endl;

  	// Check commandline & options..

	while ((ch = getopt(argc,argv,"t:A:hd")) != -1) {
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
	case fxe0:
	case fxe1:
    case fxe2:
		cerr << "Sorry.. No bonus!" << endl;
		exit(0);
		break;
	case fxe3:
		//cout << "compr = new CompressFXE2(NBLOCKS);" << endl;
		compr = new CompressFXE3(NBLOCKS);
    	ID = FXEP_TYPE3_ID;
		break;
	case fxe4:
		//cout << "compr = new CompressFXE2(NBLOCKS);" << endl;
		compr = new CompressFXE4(NBLOCKS);
    	ID = FXEP_TYPE4_ID;
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

	int gap = -1;
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
	long totalFileSize = 0;
	long comprFileSize = 0;
	long decomprOffset = 0;
	long n;

	rfile.seek(0,IO_SEEK_END);
	origiFileSize = rfile.tell();
	rfile.seek(0,IO_SEEK_SET);

	// Now we are ready to go.. first prepare the compressed
	// file header information..

	tb = buf;
	PUTL(tb,ID);
	wfile.write(buf,4);

	totalFileSize += 4;
	comprFileSize = 4;

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
		comprFileSize += n;
		totalFileSize += n;

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
		case FXEP_TYPE0_ID:
		case FXEP_TYPE2_ID:
		default:
			PUTW(tb,FXEP_EOF);
			break;
		case FXEP_TYPE3_ID:
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
		comprFileSize += 6;
		totalFileSize += 6;
    }
	if (n < 0) {
		cerr << "** Error writing file (2).." << endl;
		rfile.close();
		wfile.close();
		delete compr;
		return 0;
	}

	// Check alignment.. must be 4

#if 0

	if (comprFileSize & 0x03) {
		unsigned long pad = 0;
		long l = 4-(comprFileSize & 0x03);
		wfile.write(reinterpret_cast<unsigned char*>(&pad),l);
		comprFileSize += l;
		totalFileSize += l;
	}
#endif
	//
	// Fix headers..
	//

	fix.osize = origiFileSize;
	fix.csize = comprFileSize;
	fix.gap = gap;

	//
	// Calculate stuff..
	//

	double gain = 100.0 - static_cast<double>(totalFileSize)
                / static_cast<double>(origiFileSize) * 100.0;

	cout.precision(3);
	cout << "\rCrunched " << gain << "% - total " << totalFileSize << " bytes" << endl;

	//
	//
	//

	rfile.close();
	wfile.close();
	delete compr;

	//

  return 0;
}


