	CPU	386
	BITS	16
;
struc	DiskAddrPacket
	.size:		resb	1
			resb	1 ;unused, should be zero
	.numsecs:	resw	1
	.buf:		resd	1
	.offset:	resq	1
endstruc
;
	section	.text
	entry	equ	0x7c00
	reloc	equ	0x6000
;
	org	reloc
;
start:	xor	ax,ax
	cli
	mov	ss,ax
	mov	sp,reloc
	sti
;
	mov	ds,ax
	mov	es,ax
	cld
	mov	si,entry
	mov	di,reloc
	mov	cx,0x100	; 256 words
	rep movsw
	jmp	0:setcs
setcs:	push	dx
	lea	si,[dap_ok]
	call	pstr
	lea	si,[no_dap]
	call	pstr
	pop	dx
;
;	cli
halt:	hlt
	jmp	halt
;
	jmp	0:entry
;
;	Print a string out to VGA
;	input: ds:si --> string
pstr:	pusha
	mov	ah,0x0e
	mov	bx,0x0007
.pchar:	lodsb			; al = [ds:si] ++
	test	al,al
	jz	.end
	int	0x10
	jmp	.pchar
.end:	popa
	ret
;
dap_ok:	db	'D', 'A', 'P', ' ', 'O', 'K', '\n', 0
no_dap:	db	'N', 'O', ' ', 'D', 'A', 'P', 0
;
pad:	times 446-(pad-start) db 0
;
dap	equ	reloc+512
