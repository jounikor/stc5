;APS0000000000000000000004EA00000000000000000000000000000000000000000000000000000000
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
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


LUTSIZE     equ 15*8
HALFLUTSIZE equ LUTSIZE/2
PRESYMS     equ 16
LTMSYMS     equ 512
HGHSYMS     equ 256
LOWSYMS     equ 256

		rsreset	; DO NOT REARRANGE
PRETABLE	rs.b	(LUTSIZE+PRESYMS*2)
LTMTABLE	rs.b	(LUTSIZE+LTMSYMS*2)
HGHTABLE	rs.b	(LUTSIZE+HGHSYMS*2)
LOWTABLE	rs.b	(LUTSIZE+LOWSYMS*2)
OVLTABLE	rs.b    256
TABLE_SIZE	rs.b	0
MTF		equ	LOWTABLE+HGHSYMS




USE_HUNKS	equ	1



;		section	decruncher,code
;
;
;
;


j:
		lea	e+4(pc),a0
jumpAddr:	pea	$40000
loadAddr:	lea	$40000,a1
workAddr:	lea	$80000-TABLE_SIZE,a2
		;
		move.l	a1,a6
		move.l	a0,a4
.findEnd:	move.w	(a4)+,d0
		add.w	d0,a4
		bne.b	.findEnd
		add.l	(a4),a6
		;
		; A4 = end of crunched data
		; A6 = end of decrunched area
		;
		cmp.l	a1,a4
		bls.b	decrunch
		cmp.l	a6,a4
		bhi.b	decrunch
		; move..
		move.l	a6,a3
		subq.l	#2,a4
.move:		move.w	-(a4),-(a3)
		cmp.l	a0,a4
		bhi.b	.move
		move.l	a3,a0
		;


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
;  Runtime memory usage is 2816 ($b00) bytes, freely reloctable using A2.
;
; Prototype:
;  void decrunch3( void *src, void *dst, void *tmp );
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


decrunch:	addq.w	#2,a0
		;
		; A0 = ptr to crunched data (ID already skipped) + 2
		; A1 = ptr to destination mem
		; A2 = ptr to work area

blockLoop:	subq.l	#2,a0		; Needed because the bit stream
					; extracting function is always
					; one word ahead.
		cmp.l	a1,a6
		bhi.b	blockCont
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
decodeTrees:
		lea     MTF(a2),a3		; MTF table
		move.l  a2,a4			; A4 = PRETABLE
		lea     LUTSIZE+PRESYMS*2(a2),a5	; leaf depths

		; Init MTF table M at the same time reading 16 leaf depths.
		;

		moveq	#PRESYMS,d3
		moveq	#29,d4
		moveq	#0,d5           ; max leaf depth is 15
.getb3:
		move.b  d5,(a3)+                ; init MTF
		move.l	d7,d0
		lsr.l	d4,d0
		move.b  d0,(a5)+
		moveq	#3,d1
		bsr.b	getB
		addq.w	#1,d5
		cmp.w	d5,d3
		bne.b	.getb3

		; Build pretree..
		; D3 = num symbols = 16
		; A4 = ptr to PRETABLE

		bsr.w	buildDecodingTables	; PRETREE
donePreTable:

		;; Build LTMTABLE
		lsl.w	#5,d3
		; D3 = 512
		lea     LTMTABLE(a2),a4
		bsr.b	buildDecodingTree

		lsr.w	#1,d3
		; D3 = 256
		; A4 = A2+HGHTABLE
		bsr.b	buildDecodingTree
		;

		; D3 = 256
		; A4 = A2+LOWTABLE
		bsr.b	buildDecodingTree
		;
		; D3 = 256
		; D5 = oldLength (PMR)      no init required
		; A4 = oldOffset (PMR)      no init required
		; A5 = oldOffsetLong (PMR)  no init required
		;
mainLoop:	
		lea	LTMTABLE(a2),a3			; LTMTABLE
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
		lea	HGHTABLE(a2),a3
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
		lea	LOWTABLE(a2),a3
		bsr.b	getSyml
		move.b	d2,d4
		;
		move.w	d4,a5			; A5 = PMR oldOffsetLong
oldOffsetLong:	move.w	d4,a4			; A4 = PMR oldOffset
copyLoop:	move.w	d5,d0			; D5 = PMR lonLength

		;
		; D3 = matchLength-1
		; D4 = offset

		move.l	a1,a3
		sub.l	a4,a3
		;
.copy:		move.b	(a3)+,(a1)+		
		dbf	d0,.copy
effect:		move.w	d6,$dff180
		bra.b	mainLoop

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Description:
;  This function decodes the next symbol from the given huffman tree.
;
; Parameters:
;  a3 = ptr to (huffman) tables..
;
; Returns:
;  D1 = cnt (num bits needed for symbol..)
;  D2 = symbol
;
; Trashes:
;  D0,D1,D2,a6
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

getSyml:
.lut:		cmp.l	(a3)+,d7
		bhi.b	.lut
		;
		movem.w	HALFLUTSIZE-4(a3),d0/d1	; Clears bits 31-16 of D1
		; D0 = base index
		; D1 = number of bits to extract

		move.l	d7,d2
		clr.w	d2
		rol.l	d1,d2
		sub.w	d0,d2
		add.w	d2,d2
		move.w  -4(a3,d2.w),d2

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
;  A4 = ptr to the tree
;
; Returns:
;  D3 = num of symbols
;  A4 = ptr to dest table
;
; Trashes:
;  D0,D1,D2,D4,D5,A3,A4,A5
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


buildDecodingTree:
		move.w	d3,d0
		add.w	d0,d0
		lea	LUTSIZE(a4,d0.w),a5
		moveq	#0,d5
.loop:	
		move.l  a2,a3		; PRETABLE
		bsr.b	getSyml		; trashes a3

		; inverse MTF

		lea	MTF(a2),a3
		add.w	d2,a3
		move.w	d2,d0
		move.b	(a3),d2
		bra.b	.mtf1
		;
.mtf:		move.b	-(a3),1(a3)
.mtf1:		dbf	d0,.mtf
		move.b	d2,(a3)
		move.b	d2,(a5)+
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
;  A4 = ptr to the tree table
;
; Returns:
;  D3 = num of symbols
;  A4 = ptr to next table
;
; Trashes:
;  
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

		;must	preserve a2/D3/D6/D7

buildDecodingTables:
		move.l  a4,a5
		moveq   #HALFLUTSIZE/4-1,d0
		; clear LUTTABLE
.clrLoop:
		clr.l   (a5)+
		dbf     d0,.clrLoop
		;
		move.w  d3,d1
		add.w   d1,d1
		add.w	d3,d1
		lea     LUTSIZE(a4,d1.w),a3
		; Read depths and count occurrences of each depth
		; A3 = leaf depths
		; A4 = tree/lut
		add.w   d3,d0
		moveq	#0,d1
.countLoop:
		move.b	-(a3),d1
		beq.b	.zeroDepth
		add.b	d1,d1
		add.b	d1,d1
		addq.w	#1,2-4(a4,d1.w)       ; Count or Index
.zeroDepth:
		dbf	d0,.countLoop
        	; count prefix and base
 		move.l  a4,a5
		moveq	#15-1,d0
		moveq   #0,d2			; prefix
		moveq	#HALFLUTSIZE,d5		; inxed or count
.indexLoop:
		move.l  (a5)+,d4
		beq.b   .zeroCount
		add.w   d4,d2
		movem.w d2/d5,-4(a5)
		add.w   d4,d5
.zeroCount:
		add.w   d2,d2
		dbf	d0,.indexLoop
		;
		; Sort symbols
		moveq	#0,d0
.sortLoop:
		move.b  0(a3,d0.w),d1
		beq.b   .zeroIndex
		add.b   d1,d1
		add.b   d1,d1
		move.w  2-4(a4,d1.w),d2
		addq.w  #1,2-4(a4,d1.w)       ; Count or Index
		add.w   d2,d2
		move.w  d0,0(a4,d2.w)
.zeroIndex:
		addq.w	#1,d0
		cmp.w	d0,d3
		bne.b	.sortLoop
		;
		; Calculate LUT tables
		move.l	a4,a5
		moveq   #1,d1
		moveq	#-1,d4
		moveq   #0,d5
.lutLoop:	;
		move.w	(a4)+,d0	; prefix
		move.w	(a4)+,d2	; index
		beq.b	.zeroLut
		lsl.l	d1,d4
		or.w	d0,d4
		subq.w	#1,d4
		ror.l	d1,d4
		move.l	d4,(a5)+	; code
		;
		sub.w	d5,d2
		sub.w	d2,d0
		movem.w	d0/d1,HALFLUTSIZE-4(a5)
		addq.w	#2,d5
.zeroLut:
		addq.w	#1,d1
		cmp.w	#16,d1		; this counter is wrong
		bne.b	.lutLoop	; should be max 15 times
		move.l	a3,a4
		rts
e:
;		incbin	"data:tmp/pouet.stc5"
o:



