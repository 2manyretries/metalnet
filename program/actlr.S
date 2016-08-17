
	.text
	.global set_actlr
	.global	get_actlr

set_actlr:
	mcr p15, 0,r0, c1, c0, 1
	bx lr

get_actlr:
	mrc p15, 0,r0, c1, c0, 1
	bx lr

