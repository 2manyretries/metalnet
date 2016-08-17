
	.text
	.global set_sctlr
	.global	get_sctlr

set_sctlr:
	mcr p15, 0,r0, c1, c0, 0
	bx lr

get_sctlr:
	mrc p15, 0,r0, c1, c0, 0
	bx lr

