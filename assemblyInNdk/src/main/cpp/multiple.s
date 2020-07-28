	.text
	.align	2
	.global	armFunction
	.type	armFunction, %function
armFunction:
	@ Multiply by 10, input value and return value in r0
	stmfd	sp!, {fp,ip,lr}
	mov	r3, r0, asl #3
	add	r0, r3, r0, asl #1
	ldmfd	sp!, {fp,ip,lr}
	bx	lr
	.size	armFunction, .-armFunction