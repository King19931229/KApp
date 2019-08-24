default	rel
%define XMMWORD
%define YMMWORD
%define ZMMWORD
EXTERN	OPENSSL_cpuid_setup

section	.CRT$XCU rdata align=8
		DQ	OPENSSL_cpuid_setup


common	OPENSSL_ia32cap_P 16

section	.text code align=64


global	OPENSSL_atomic_add

ALIGN	16
OPENSSL_atomic_add:
	mov	eax,DWORD[rcx]
$L$spin:	lea	r8,[rax*1+rdx]
DB	0xf0
	cmpxchg	DWORD[rcx],r8d
	jne	NEAR $L$spin
	mov	eax,r8d
DB	0x48,0x98
	DB	0F3h,0C3h		;repret


global	OPENSSL_rdtsc

ALIGN	16
OPENSSL_rdtsc:
	rdtsc
	shl	rdx,32
	or	rax,rdx
	DB	0F3h,0C3h		;repret


global	OPENSSL_ia32_cpuid

ALIGN	16
OPENSSL_ia32_cpuid:
	mov	QWORD[8+rsp],rdi	;WIN64 prologue
	mov	QWORD[16+rsp],rsi
	mov	rax,rsp
$L$SEH_begin_OPENSSL_ia32_cpuid:
	mov	rdi,rcx


	mov	r8,rbx

	xor	eax,eax
	mov	DWORD[8+rdi],eax
	cpuid
	mov	r11d,eax

	xor	eax,eax
	cmp	ebx,0x756e6547
	setne	al
	mov	r9d,eax
	cmp	edx,0x49656e69
	setne	al
	or	r9d,eax
	cmp	ecx,0x6c65746e
	setne	al
	or	r9d,eax
	jz	NEAR $L$intel

	cmp	ebx,0x68747541
	setne	al
	mov	r10d,eax
	cmp	edx,0x69746E65
	setne	al
	or	r10d,eax
	cmp	ecx,0x444D4163
	setne	al
	or	r10d,eax
	jnz	NEAR $L$intel


	mov	eax,0x80000000
	cpuid
	cmp	eax,0x80000001
	jb	NEAR $L$intel
	mov	r10d,eax
	mov	eax,0x80000001
	cpuid
	or	r9d,ecx
	and	r9d,0x00000801

	cmp	r10d,0x80000008
	jb	NEAR $L$intel

	mov	eax,0x80000008
	cpuid
	movzx	r10,cl
	inc	r10

	mov	eax,1
	cpuid
	bt	edx,28
	jnc	NEAR $L$generic
	shr	ebx,16
	cmp	bl,r10b
	ja	NEAR $L$generic
	and	edx,0xefffffff
	jmp	NEAR $L$generic

$L$intel:
	cmp	r11d,4
	mov	r10d,-1
	jb	NEAR $L$nocacheinfo

	mov	eax,4
	mov	ecx,0
	cpuid
	mov	r10d,eax
	shr	r10d,14
	and	r10d,0xfff

$L$nocacheinfo:
	mov	eax,1
	cpuid
	and	edx,0xbfefffff
	cmp	r9d,0
	jne	NEAR $L$notintel
	or	edx,0x40000000
	and	ah,15
	cmp	ah,15
	jne	NEAR $L$notP4
	or	edx,0x00100000
$L$notP4:
	cmp	ah,6
	jne	NEAR $L$notintel
	and	eax,0x0fff0ff0
	cmp	eax,0x00050670
	je	NEAR $L$knights
	cmp	eax,0x00080650
	jne	NEAR $L$notintel
$L$knights:
	and	ecx,0xfbffffff

$L$notintel:
	bt	edx,28
	jnc	NEAR $L$generic
	and	edx,0xefffffff
	cmp	r10d,0
	je	NEAR $L$generic

	or	edx,0x10000000
	shr	ebx,16
	cmp	bl,1
	ja	NEAR $L$generic
	and	edx,0xefffffff
$L$generic:
	and	r9d,0x00000800
	and	ecx,0xfffff7ff
	or	r9d,ecx

	mov	r10d,edx

	cmp	r11d,7
	jb	NEAR $L$no_extended_info
	mov	eax,7
	xor	ecx,ecx
	cpuid
	bt	r9d,26
	jc	NEAR $L$notknights
	and	ebx,0xfff7ffff
$L$notknights:
	mov	DWORD[8+rdi],ebx
$L$no_extended_info:

	bt	r9d,27
	jnc	NEAR $L$clear_avx
	xor	ecx,ecx
DB	0x0f,0x01,0xd0
	and	eax,6
	cmp	eax,6
	je	NEAR $L$done
$L$clear_avx:
	mov	eax,0xefffe7ff
	and	r9d,eax
	and	DWORD[8+rdi],0xffffffdf
$L$done:
	shl	r9,32
	mov	eax,r10d
	mov	rbx,r8
	or	rax,r9
	mov	rdi,QWORD[8+rsp]	;WIN64 epilogue
	mov	rsi,QWORD[16+rsp]
	DB	0F3h,0C3h		;repret
$L$SEH_end_OPENSSL_ia32_cpuid:

global	OPENSSL_cleanse

ALIGN	16
OPENSSL_cleanse:
	xor	rax,rax
	cmp	rdx,15
	jae	NEAR $L$ot
	cmp	rdx,0
	je	NEAR $L$ret
$L$ittle:
	mov	BYTE[rcx],al
	sub	rdx,1
	lea	rcx,[1+rcx]
	jnz	NEAR $L$ittle
$L$ret:
	DB	0F3h,0C3h		;repret
ALIGN	16
$L$ot:
	test	rcx,7
	jz	NEAR $L$aligned
	mov	BYTE[rcx],al
	lea	rdx,[((-1))+rdx]
	lea	rcx,[1+rcx]
	jmp	NEAR $L$ot
$L$aligned:
	mov	QWORD[rcx],rax
	lea	rdx,[((-8))+rdx]
	test	rdx,-8
	lea	rcx,[8+rcx]
	jnz	NEAR $L$aligned
	cmp	rdx,0
	jne	NEAR $L$ittle
	DB	0F3h,0C3h		;repret

global	OPENSSL_wipe_cpu

ALIGN	16
OPENSSL_wipe_cpu:
	pxor	xmm0,xmm0
	pxor	xmm1,xmm1
	pxor	xmm2,xmm2
	pxor	xmm3,xmm3
	pxor	xmm4,xmm4
	pxor	xmm5,xmm5
	xor	rcx,rcx
	xor	rdx,rdx
	xor	r8,r8
	xor	r9,r9
	xor	r10,r10
	xor	r11,r11
	lea	rax,[8+rsp]
	DB	0F3h,0C3h		;repret

global	OPENSSL_ia32_rdrand

ALIGN	16
OPENSSL_ia32_rdrand:
	mov	ecx,8
$L$oop_rdrand:
DB	72,15,199,240
	jc	NEAR $L$break_rdrand
	loop	$L$oop_rdrand
$L$break_rdrand:
	cmp	rax,0
	cmove	rax,rcx
	DB	0F3h,0C3h		;repret


global	OPENSSL_ia32_rdseed

ALIGN	16
OPENSSL_ia32_rdseed:
	mov	ecx,8
$L$oop_rdseed:
DB	72,15,199,248
	jc	NEAR $L$break_rdseed
	loop	$L$oop_rdseed
$L$break_rdseed:
	cmp	rax,0
	cmove	rax,rcx
	DB	0F3h,0C3h		;repret

