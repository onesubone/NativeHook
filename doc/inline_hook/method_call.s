	.section	__TEXT,__text,regular,pure_instructions
	.macosx_version_min 10, 13
	.globl	_add
	.p2align	4, 0x90
_add:                                   ## @add
	.cfi_startproc
## BB#0:
	pushq	%rbp // 第一步：备份原来的帧基地址，将旧的帧基地址保存在栈中
Lcfi0:
	.cfi_def_cfa_offset 16
Lcfi1:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp  // 第一步：设置当前帧基地址为栈顶指针，即移动帧指针到栈顶，即开启新的一帧
Lcfi2:
	.cfi_def_cfa_register %rbp
	movl	%edi, -4(%rbp) // 第四步：*(%rbp-4)=*(%edi), 从寄存器中获取第一个入参
	movl	%esi, -8(%rbp) // 第四步：*(%rbp-8)=*(%esi), 从寄存器中获取第二个入参
	movl	-4(%rbp), %esi // 第四步：*(%esi)=*(%rbp-4), 从寄存器中获取第一个入参写入到esi中
	addl	-8(%rbp), %esi // 第四步：*(%esi)=*(%esi) + *(%rbp-8), 两个入参相加
	movl	%esi, -12(%rbp)// 第四步：*(%rbp-12)=*(%esi), 将a+b的值存储在返回空间
	movl	-12(%rbp), %eax // 第四步：*(%eax)=*(%rbp-12), 将a+b的值存储在eax寄存器中(此寄存器默认作为存储返回值的寄存器)
	popq	%rbp // 第七步：恢复调用者栈帧
	retq // 第八步：弹出返回地址，跳出当前过程，继续执行调用者的代码
	.cfi_endproc

	.globl	_main
	.p2align	4, 0x90
_main:             // 入口                     ## @main
	.cfi_startproc
## BB#0:
	pushq	%rbp // 第一步：备份原来的帧基地址，将旧的帧基地址保存在栈中
Lcfi3:
	.cfi_def_cfa_offset 16
Lcfi4:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp // 第一步：设置当前帧基地址为栈顶指针，即移动帧指针到栈顶，即开启新的一帧
Lcfi5:
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp // 第二步：分配临时变量的空间，栈顶指针偏移16字节,分配16个字节
	movl	$0, -4(%rbp) //第四步： *(%rbp-4)=0
	movl	$100, -8(%rbp) //第四步： *(%rbp-8)=100, 对应语句局部变量int a=100
	movl	$101, -12(%rbp) //第四步： *(%rbp-12)=101, 对应语句局部变量int b=101
	movl	-8(%rbp), %edi //第四步： *(%edi)=*(%rbp-8)=100, 方法add的第一个入参保存在寄存器edi中
	movl	-12(%rbp), %esi //第四步： *(%esi)=*(%rbp-12)=101, 方法add的第二个入参保存在寄存器esi中
	callq	_add // 第四步：调用add方法，goto add method
	movl	%eax, -16(%rbp) //第四步： *(%rbp-16)=*(%eax), eax为add方法的返回值，与前两步对应int c=add(a,b)
	movl	-16(%rbp), %eax //第四步： *(%eax)=*(%rbp-16)
	addq	$16, %rsp // 第四步：释放盛情的空间
	popq	%rbp // 第七步：恢复栈帧
	retq // 第八步：弹出返回地址，跳出当前过程，继续执行调用者的代码
	.cfi_endproc


.subsections_via_symbols
