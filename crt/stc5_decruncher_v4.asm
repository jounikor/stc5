;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 
; Version: 4
;
; EXPERIMENTAL STC5 algorithm with original length at the end of file.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; This file is released into the public domain for commercial
; or non-commercial usage with no restrictions placed upon it.
;
; This version adds a heapsort into tree building phase..
;
;
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

		rsreset	; DO NOT REARRANGE
LTMTABLE	rs.b	(16*8+512*2)
HGHTABLE	rs.b	(16*8+256*2)
LOWTABLE	rs.b	(16*8+256*2)
PRETABLE	rs.b	(16*8+16*2)
TABLE_SIZE	rs.b	0
MTF		equ	LOWTABLE+64

USE_HUNKS	equ	1



;		section	decruncher,code
;
;
;
;


j:

		lea	e+6(pc),a0		; crunched data+6
		lea	$40000,a1		; destination
		lea	$80000-TABLE_SIZE,a2	; work area - $a20 bytes
		bsr.s	decrunch
		rts



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Parameters:
;  A0 - src i.e. a ptr to the compressed data. Must be 16bits aligned.
;  A1 - dst i.e. a ptr to the destination memory.
;  A2 = ptr yo work area (TABLE_SIZE bytes)
;
; Returns:
;  none
;
; Notes:
;  The routine does not check if source and destination memory areas
;  overlap. In such case a crash is very probable.
;  Runtime memory usage is 2592 ($a20) bytes, freely reloctable using A2.
;
; Prototype:
;  void decrunch3( void *src, void *dst, void *tmp );
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;





decrunch:	;
		; A0 = ptr to crunched data (ID already skipped) + 2
		; A1 = ptr to destination mem
		; A2 = ptr to work area

blockLoop:	tst.w	-(a0)		; Needed because the bit stream
					; extracting function is always
					; one word ahead.
		bne.b	blockCont
blockExit:	move.l	$4.w,a6
		cmp.w	#$25,$14(a6)
		bcs.b	.nokick2
		pea	-$27c(a6)	; CacheClearU()
.nokick2:	rts
		;
		; Initialize the bitshifter
blockCont:	move.l	(a0)+,d7
		moveq	#0,d6
		swap	d7
		;
		; A0 = src 
		; A1 = dst
		; a2 = tmp
		; D6 = _bc
		; D7 = _bb
		;
decodeTrees:	lea	PRETABLE+128(a2),a4	; was A4 = PRETABLE+16*8 - 2
		moveq	#16,d3

		; Init MTF table M

		moveq	#15,d0
		lea	MTF(a2),a3

.mtf:		move.b	d0,0(a3,d0.w)	; (a2+MTF+D0)
		dbf	d0,.mtf

		;move.w	d6,(a4)+	; pretable end marker

		; Read in depths and store them into the PRETABLE
		; The format is: depth<<11 | leaf#

		move.l	a4,a3
		moveq	#29,d4
		moveq	#0,d5
.getb3:
		move.l	d7,d0
		lsr.l	d4,d0
		ror.w	#4+1,d0
		or.w	d5,d0
		move.w	d0,(a3)+
		moveq	#3,d1
		bsr.b	getB
		addq.w	#1,d5
		cmp.w	d3,d5
		blo.b	.getb3

		; Build pretree..
		; D3 = num symbols = 16
		; A4 = ptr to decoding structure

		bsr.w	buildDecodingTree	; PRETREE
donePreTable:

		lsl.w	#5,d3
		; D3 = 512
		move.l	a2,a4
		bsr.b	buildDecodingTree3

		lsr.w	#1,d3
		; D3 = 256
		; A4 = A2+HGHTABLE
		bsr.b	buildDecodingTree3
		;

		; D3 = 256
		; A4 = A2+LOWTABLE
		bsr.b	buildDecodingTree3
		;
		; D3 = 256
		; D5 = oldLength (PMR)      no init required
		; A4 = oldOffset (PMR)      no init required
		; A5 = oldOffsetLong (PMR)  no init required
		;
mainLoop:	move.l	a2,a6			; LTMTABLE
		bsr.b	getSyml
		sub.w	d3,d2
decodeLiteral:	;
		; Symbol > 256 => PMR or match

		bgt.b	matchFound

		;
		; Symbol = 256 => End of Block

		beq.b	blockLoop

		;
		; Symbol < 256 => Literal

		move.b	d2,(a1)+
		bra.b	mainLoop
		;
matchFound:	
		subq.w	#1,d2
		ble.b	copyLoop		; Symbol = 257 => PMR 

		; D2 = match_length-1
		move.w	d2,d5			; D5 = PMR oldLength
decodeOffset:
		lea	HGHTABLE(a2),a6
		bsr.b	getSyml

		move.w	d2,d4
		bne.b	.notoldoffsetlong
		move.w	a5,d4
		bra.b	oldOffsetLong
.notoldoffsetlong:
		bclr	#7,d4
		beq.b	oldOffsetLong
		;
		lsl.w	#8,d4

		;
		lea	LOWTABLE(a2),a6
		bsr.b	getSyml
		move.b	d2,d4
		;
		move.w	d4,a5			; A5 = PMR oldOffsetLong
oldOffsetLong:	move.w	d4,a4			; A4 = PMR oldOffset
copyLoop:	move.w	d5,d0			; D5 = PMR lonLength

		;
		; D3 = matchLength-1
		; D4 = offset

		move.l	a1,a6
		sub.l	a4,a6
		;
.copy:		move.b	(a6)+,(a1)+		
		dbf	d0,.copy
	move.w	d6,$dff180
		bra.b	mainLoop

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Description:
;  This function decodes the next symbol from the given huffman tree.
;
; Parameters:
;  a6 = ptr to (huffman) tables..
;
; Returns:
;  D1 = cnt (num bits needed for symbol..)
;  D2 = symbol
;
; Trashes:
;  D0,D1,D2,A3,a6
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

getSyml:	move.l	a6,a3
.lut:		cmp.l	(a6)+,d7
		bhi.b	.lut
		;
		movem.w	16*4-4(a6),d0/d1	; Clears bits 31-16 of D1
		; D0 = base index
		; D1 = number of bits to extract

		move.l	d7,d2
		clr.w	d2
		rol.l	d1,d2
		sub.w	d0,d2
		add.w	d2,d2
		move.w	0(a3,d2.w),d2		;lea	(16*8,a6),a3

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 
; Description:
;  This function extracts n bits from the compressed data stream.
;
; Parameters:
;  D1.w = num_bits_to_extract
;
; Returns:
;  Nothing.
;
; Trashes:
;  flags,D1
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

getB:		sub.w	d1,d6
		bge.b	.getb
		add.w	d1,d6
		lsl.l	d6,d7
		move.w	(a0)+,d7
		sub.w	d6,d1
		moveq	#16,d6
		sub.w	d1,d6
.getb:		lsl.l	d1,d7
return:
		rts


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Description:
;  Build huffman tree & decoding structures. This function also reads symbols
;  from the compressed stream and then does inverse MTF (move to front) to
;  symbols. This function is only used to decode/build the pretree.
;
; Parameters:
;  D3 = num symbold
;  A4 = dest
;  a2 = ptr to tmp
;
; Returns:
;  D3 = num of symbols
;  A4 = ptr to dest table
;
; Trashes:
;  D0,D1,D2,D5,A4,A5,a2
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


buildDecodingTree3:
		lea	128(a4),a4
		move.l	a4,a5
		moveq	#0,d5
.loop:		lea	PRETABLE(a2),a6		; PRETABLE
		bsr.b	getSyml			; trashes a6/A3

		; inverse MTF

		lea	MTF(a2),a6
		add.w	d2,a6
		move.w	d2,d0
		move.b	(a6),d2
		bra.b	.mtf1

		;
.mtf:		move.b	-(a6),1(a6)
.mtf1:		dbf	d0,.mtf
		move.b	d2,(a6)
		;
		ror.w	#4+1,d2
		or.w	d5,d2
		move.w	d2,(a5)+
		addq.w	#1,d5
		cmp.w	d5,d3
		bne.b	.loop


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Description:
;  Build huffman tree & decoding structures. This function assumes that all
;  symbols are already loaded into memory.
;
; Parameters:
;  D3 = num symbols
;  A4 = dest
;
; Returns:
;  D3 = num of symbols
;  A4 = ptr to next table
;
; Trashes:
;  r0,r1,r5,r6,r7
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

		;must	preserve a3/D3/D6/D7


buildDecodingTree:
		movem.l	d3/d6/d7,-(sp)
		moveq	#1,d4		; current heap max
		move.w	#$07ff,d6
		move.l	a4,a6		; A6 = heap and start of symbols
		;
buildHeap:	move.w	(a4)+,d0
		;
		; Format of D0 is:  "dddddnnn|nnnnnnnn" where
		;  'd' is the depth in the huffman tree and
		;  'n' is the leaf number.
		;
		cmp.w	d6,d0
		bls.b	.skipZero	; skip symbols with zero depth
		move.w	d4,d5
		add.w	d5,d5
		;
.buildLoop:
		move.w	d5,d2
		move.w	d0,-2(a6,d5.w)
		lsr.w	#2,d5
		beq.b	.root
		add.w	d5,d5
		move.w	-2(a6,d5.w),d1
		cmp.w	d1,d0
		bls.b	.root
		move.w	d1,-2(a6,d2.w)
		bra.b	.buildLoop
		;
.root:		addq.w	#1,d4		; increase size of heap
.skipZero:	subq.w	#1,d3
		bne.b	buildHeap
		subq.w	#2,d4
		bmi.s	allZero
		move.w	d4,d3		; D3 = heapsize-1
		;
		add.w	d4,d4
		lea	2(a6,d4.w),a3
		;
		; D3 = heapsize-1
		; D4 = (heapsize*2)-2
		; A3 = end of heap+2
		; A4 = end of symbols
		;
		clr.w	(a3)		; Termination code for
					; calculateCanonicalCodes
heapSort:	move.w	-(a3),d1
		move.w	(a6),(a3)
		moveq	#2,d5
.siftDown:	move.w	d5,d2
		; left leaf
		add.w	d2,d2
		cmp.w	d4,d2
		bhi.b	.done
		beq.b	.leftMax
		move.w	0(a6,d2.w),d0
		cmp.w	-2(a6,d2.w),d0
		bls.b	.leftMax
		addq.w	#2,d2
.leftMax:	cmp.w	-2(a6,d2.w),d1
		bhs.b	.done
		; swap leaf and parent
.move:		move.w	-2(a6,d2.w),-2(a6,d5.w)
		move.w	d2,d5
		bra.b	.siftDown
		;
.done:		move.w	d1,-2(a6,d5.w)
		subq.w	#2,d4
		bgt.b	heapSort
		;
calculateCanonicalCodes:
		lea	-128(a6),a5
		moveq	#64,d2		; artificial shift.. see getSyml()
		moveq	#0,d4
		
		; a6 = _alp+_start 
		; A4 = dest = _tlb
		; D3 = _alpsize-1
		; D4 = 0
		; d2 = 0
		;

		moveq	#11,d7
	

.loop:		move.w	(a6),d0
		move.l	(a6),d1		
		lsr.w	d7,d0
		lsr.w	d7,d1
		sub.w	d0,d1
		beq.b	.skip
		
		moveq	#-1,d5
		lsl.w	d0,d5
		or.w	d4,d5
		ror.l	d0,d5
		move.l	d5,(a5)+		; cde
		move.w	d4,d5
		sub.w	d2,d5			; ind = (unsigned short)(c-n)
		swap	d5
		move.w	d0,d5

		move.l	d5,16*4-4(a5)		; ind << 16 | bts

.skip:		addq.w	#1,d4
		lsl.l	d1,d4			; D1 can be zero..
		addq.w	#1,d2

		and.w	d6,(a6)+		; and.w	#$07ff,(a6)+
		dbf	d3,.loop

allZero:	movem.l	(sp)+,d3/d6/d7
		rts
e:
		incbin	"data:tmp/pouet.stc5"
		;incbin	"data:tmp/flastro40000.stc"
o:



