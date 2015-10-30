	# $FreeBSD$
.file	"x86-gf2m.s"
.text
.type	_mul_1x1_mmx,@function
.align	16
_mul_1x1_mmx:
	subl	$36,%esp
	movl	%eax,%ecx
	leal	(%eax,%eax,1),%edx
	andl	$1073741823,%ecx
	leal	(%edx,%edx,1),%ebp
	movl	$0,(%esp)
	andl	$2147483647,%edx
	movd	%eax,%mm2
	movd	%ebx,%mm3
	movl	%ecx,4(%esp)
	xorl	%edx,%ecx
	pxor	%mm5,%mm5
	pxor	%mm4,%mm4
	movl	%edx,8(%esp)
	xorl	%ebp,%edx
	movl	%ecx,12(%esp)
	pcmpgtd	%mm2,%mm5
	paddd	%mm2,%mm2
	xorl	%edx,%ecx
	movl	%ebp,16(%esp)
	xorl	%edx,%ebp
	pand	%mm3,%mm5
	pcmpgtd	%mm2,%mm4
	movl	%ecx,20(%esp)
	xorl	%ecx,%ebp
	psllq	$31,%mm5
	pand	%mm3,%mm4
	movl	%edx,24(%esp)
	movl	$7,%esi
	movl	%ebp,28(%esp)
	movl	%esi,%ebp
	andl	%ebx,%esi
	shrl	$3,%ebx
	movl	%ebp,%edi
	psllq	$30,%mm4
	andl	%ebx,%edi
	shrl	$3,%ebx
	movd	(%esp,%esi,4),%mm0
	movl	%ebp,%esi
	andl	%ebx,%esi
	shrl	$3,%ebx
	movd	(%esp,%edi,4),%mm2
	movl	%ebp,%edi
	psllq	$3,%mm2
	andl	%ebx,%edi
	shrl	$3,%ebx
	pxor	%mm2,%mm0
	movd	(%esp,%esi,4),%mm1
	movl	%ebp,%esi
	psllq	$6,%mm1
	andl	%ebx,%esi
	shrl	$3,%ebx
	pxor	%mm1,%mm0
	movd	(%esp,%edi,4),%mm2
	movl	%ebp,%edi
	psllq	$9,%mm2
	andl	%ebx,%edi
	shrl	$3,%ebx
	pxor	%mm2,%mm0
	movd	(%esp,%esi,4),%mm1
	movl	%ebp,%esi
	psllq	$12,%mm1
	andl	%ebx,%esi
	shrl	$3,%ebx
	pxor	%mm1,%mm0
	movd	(%esp,%edi,4),%mm2
	movl	%ebp,%edi
	psllq	$15,%mm2
	andl	%ebx,%edi
	shrl	$3,%ebx
	pxor	%mm2,%mm0
	movd	(%esp,%esi,4),%mm1
	movl	%ebp,%esi
	psllq	$18,%mm1
	andl	%ebx,%esi
	shrl	$3,%ebx
	pxor	%mm1,%mm0
	movd	(%esp,%edi,4),%mm2
	movl	%ebp,%edi
	psllq	$21,%mm2
	andl	%ebx,%edi
	shrl	$3,%ebx
	pxor	%mm2,%mm0
	movd	(%esp,%esi,4),%mm1
	movl	%ebp,%esi
	psllq	$24,%mm1
	andl	%ebx,%esi
	shrl	$3,%ebx
	pxor	%mm1,%mm0
	movd	(%esp,%edi,4),%mm2
	pxor	%mm4,%mm0
	psllq	$27,%mm2
	pxor	%mm2,%mm0
	movd	(%esp,%esi,4),%mm1
	pxor	%mm5,%mm0
	psllq	$30,%mm1
	addl	$36,%esp
	pxor	%mm1,%mm0
	ret
.size	_mul_1x1_mmx,.-_mul_1x1_mmx
.type	_mul_1x1_ialu,@function
.align	16
_mul_1x1_ialu:
	subl	$36,%esp
	movl	%eax,%ecx
	leal	(%eax,%eax,1),%edx
	leal	(,%eax,4),%ebp
	andl	$1073741823,%ecx
	leal	(%eax,%eax,1),%edi
	sarl	$31,%eax
	movl	$0,(%esp)
	andl	$2147483647,%edx
	movl	%ecx,4(%esp)
	xorl	%edx,%ecx
	movl	%edx,8(%esp)
	xorl	%ebp,%edx
	movl	%ecx,12(%esp)
	xorl	%edx,%ecx
	movl	%ebp,16(%esp)
	xorl	%edx,%ebp
	movl	%ecx,20(%esp)
	xorl	%ecx,%ebp
	sarl	$31,%edi
	andl	%ebx,%eax
	movl	%edx,24(%esp)
	andl	%ebx,%edi
	movl	%ebp,28(%esp)
	movl	%eax,%edx
	shll	$31,%eax
	movl	%edi,%ecx
	shrl	$1,%edx
	movl	$7,%esi
	shll	$30,%edi
	andl	%ebx,%esi
	shrl	$2,%ecx
	xorl	%edi,%eax
	shrl	$3,%ebx
	movl	$7,%edi
	andl	%ebx,%edi
	shrl	$3,%ebx
	xorl	%ecx,%edx
	xorl	(%esp,%esi,4),%eax
	movl	$7,%esi
	andl	%ebx,%esi
	shrl	$3,%ebx
	movl	(%esp,%edi,4),%ebp
	movl	$7,%edi
	movl	%ebp,%ecx
	shll	$3,%ebp
	andl	%ebx,%edi
	shrl	$29,%ecx
	xorl	%ebp,%eax
	shrl	$3,%ebx
	xorl	%ecx,%edx
	movl	(%esp,%esi,4),%ecx
	movl	$7,%esi
	movl	%ecx,%ebp
	shll	$6,%ecx
	andl	%ebx,%esi
	shrl	$26,%ebp
	xorl	%ecx,%eax
	shrl	$3,%ebx
	xorl	%ebp,%edx
	movl	(%esp,%edi,4),%ebp
	movl	$7,%edi
	movl	%ebp,%ecx
	shll	$9,%ebp
	andl	%ebx,%edi
	shrl	$23,%ecx
	xorl	%ebp,%eax
	shrl	$3,%ebx
	xorl	%ecx,%edx
	movl	(%esp,%esi,4),%ecx
	movl	$7,%esi
	movl	%ecx,%ebp
	shll	$12,%ecx
	andl	%ebx,%esi
	shrl	$20,%ebp
	xorl	%ecx,%eax
	shrl	$3,%ebx
	xorl	%ebp,%edx
	movl	(%esp,%edi,4),%ebp
	movl	$7,%edi
	movl	%ebp,%ecx
	shll	$15,%ebp
	andl	%ebx,%edi
	shrl	$17,%ecx
	xorl	%ebp,%eax
	shrl	$3,%ebx
	xorl	%ecx,%edx
	movl	(%esp,%esi,4),%ecx
	movl	$7,%esi
	movl	%ecx,%ebp
	shll	$18,%ecx
	andl	%ebx,%esi
	shrl	$14,%ebp
	xorl	%ecx,%eax
	shrl	$3,%ebx
	xorl	%ebp,%edx
	movl	(%esp,%edi,4),%ebp
	movl	$7,%edi
	movl	%ebp,%ecx
	shll	$21,%ebp
	andl	%ebx,%edi
	shrl	$11,%ecx
	xorl	%ebp,%eax
	shrl	$3,%ebx
	xorl	%ecx,%edx
	movl	(%esp,%esi,4),%ecx
	movl	$7,%esi
	movl	%ecx,%ebp
	shll	$24,%ecx
	andl	%ebx,%esi
	shrl	$8,%ebp
	xorl	%ecx,%eax
	shrl	$3,%ebx
	xorl	%ebp,%edx
	movl	(%esp,%edi,4),%ebp
	movl	%ebp,%ecx
	shll	$27,%ebp
	movl	(%esp,%esi,4),%edi
	shrl	$5,%ecx
	movl	%edi,%esi
	xorl	%ebp,%eax
	shll	$30,%edi
	xorl	%ecx,%edx
	shrl	$2,%esi
	xorl	%edi,%eax
	xorl	%esi,%edx
	addl	$36,%esp
	ret
.size	_mul_1x1_ialu,.-_mul_1x1_ialu
.globl	bn_GF2m_mul_2x2
.type	bn_GF2m_mul_2x2,@function
.align	16
bn_GF2m_mul_2x2:
.L_bn_GF2m_mul_2x2_begin:
	leal	OPENSSL_ia32cap_P,%edx
	movl	(%edx),%eax
	movl	4(%edx),%edx
	testl	$8388608,%eax
	jz	.L000ialu
	testl	$16777216,%eax
	jz	.L001mmx
	testl	$2,%edx
	jz	.L001mmx
	movups	8(%esp),%xmm0
	shufps	$177,%xmm0,%xmm0
.byte	102,15,58,68,192,1
	movl	4(%esp),%eax
	movups	%xmm0,(%eax)
	ret
.align	16
.L001mmx:
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	movl	24(%esp),%eax
	movl	32(%esp),%ebx
	call	_mul_1x1_mmx
	movq	%mm0,%mm7
	movl	28(%esp),%eax
	movl	36(%esp),%ebx
	call	_mul_1x1_mmx
	movq	%mm0,%mm6
	movl	24(%esp),%eax
	movl	32(%esp),%ebx
	xorl	28(%esp),%eax
	xorl	36(%esp),%ebx
	call	_mul_1x1_mmx
	pxor	%mm7,%mm0
	movl	20(%esp),%eax
	pxor	%mm6,%mm0
	movq	%mm0,%mm2
	psllq	$32,%mm0
	popl	%edi
	psrlq	$32,%mm2
	popl	%esi
	pxor	%mm6,%mm0
	popl	%ebx
	pxor	%mm7,%mm2
	movq	%mm0,(%eax)
	popl	%ebp
	movq	%mm2,8(%eax)
	emms
	ret
.align	16
.L000ialu:
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	subl	$20,%esp
	movl	44(%esp),%eax
	movl	52(%esp),%ebx
	call	_mul_1x1_ialu
	movl	%eax,8(%esp)
	movl	%edx,12(%esp)
	movl	48(%esp),%eax
	movl	56(%esp),%ebx
	call	_mul_1x1_ialu
	movl	%eax,(%esp)
	movl	%edx,4(%esp)
	movl	44(%esp),%eax
	movl	52(%esp),%ebx
	xorl	48(%esp),%eax
	xorl	56(%esp),%ebx
	call	_mul_1x1_ialu
	movl	40(%esp),%ebp
	movl	(%esp),%ebx
	movl	4(%esp),%ecx
	movl	8(%esp),%edi
	movl	12(%esp),%esi
	xorl	%edx,%eax
	xorl	%ecx,%edx
	xorl	%ebx,%eax
	movl	%ebx,(%ebp)
	xorl	%edi,%edx
	movl	%esi,12(%ebp)
	xorl	%esi,%eax
	addl	$20,%esp
	xorl	%esi,%edx
	popl	%edi
	xorl	%edx,%eax
	popl	%esi
	movl	%edx,8(%ebp)
	popl	%ebx
	movl	%eax,4(%ebp)
	popl	%ebp
	ret
.size	bn_GF2m_mul_2x2,.-.L_bn_GF2m_mul_2x2_begin
.byte	71,70,40,50,94,109,41,32,77,117,108,116,105,112,108,105
.byte	99,97,116,105,111,110,32,102,111,114,32,120,56,54,44,32
.byte	67,82,89,80,84,79,71,65,77,83,32,98,121,32,60,97
.byte	112,112,114,111,64,111,112,101,110,115,115,108,46,111,114,103
.byte	62,0
.comm	OPENSSL_ia32cap_P,16,4
