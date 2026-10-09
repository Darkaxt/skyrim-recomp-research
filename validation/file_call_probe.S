.text
.globl file_call_probe
.def file_call_probe; .scl 2; .type 32; .endef
.seh_proc file_call_probe
file_call_probe:
    pushq %rbx
    .seh_pushreg %rbx
    pushq %rbp
    .seh_pushreg %rbp
    pushq %rdi
    .seh_pushreg %rdi
    pushq %rsi
    .seh_pushreg %rsi
    pushq %r12
    .seh_pushreg %r12
    pushq %r13
    .seh_pushreg %r13
    pushq %r14
    .seh_pushreg %r14
    pushq %r15
    .seh_pushreg %r15
    subq $0x118, %rsp
    .seh_stackalloc 0x118
    .set slot, 0x50
    .irp reg,6,7,8,9,10,11,12,13,14,15
    movaps %xmm\reg, slot(%rsp)
    .seh_savexmm %xmm\reg, slot
    .set slot, slot+16
    .endr
    .seh_endprologue
    movq %rdx, 0x38(%rsp)
    movq %rcx, 0x40(%rsp)
    movq %rsp, 0xf0(%rsp)
    movabsq $0x13579bdf2468ace0, %r10
    movq %r10, 0x100(%rsp)
    .irp reg,rbx,rbp,rdi,rsi,r12,r13,r14,r15
    movq %r10, %\reg
    .endr
    .irp reg,6,7,8,9,10,11,12,13,14,15
    movaps file_probe_seed(%rip), %xmm\reg
    .endr
    movq %rdx, %r10
    movq 32(%r10), %rax
    movq %rax, 0x20(%rsp)
    movq 40(%r10), %rax
    movq %rax, 0x28(%rsp)
    movq 0(%r10), %rcx
    movq 8(%r10), %rdx
    movq 16(%r10), %r8
    movq 24(%r10), %r9
    call *0x40(%rsp)
    movq 0x38(%rsp), %r10
    movq %rax, 48(%r10)
    stmxcsr 56(%r10)
    xorq %rax, %rax
    movabsq $0x13579bdf2468ace0, %r10
    .set bit, 0
    .irp reg,rbx,rbp,rdi,rsi,r12,r13,r14,r15
    cmpq %r10, %\reg
    je 1f
    btsq $bit, %rax
1:
    .set bit, bit+1
    .endr
    .irp reg,6,7,8,9,10,11,12,13,14,15
    movaps %xmm\reg, %xmm0
    pcmpeqb file_probe_seed(%rip), %xmm0
    pmovmskb %xmm0, %r11d
    cmpl $65535, %r11d
    je 1f
    btsq $bit, %rax
1:
    .set bit, bit+1
    .endr
    cmpq %rsp, 0xf0(%rsp)
    je 1f
    btsq $18, %rax
1:
    cmpq %r10, 0x100(%rsp)
    je 1f
    btsq $19, %rax
1:
    .set slot, 0x50
    .irp reg,6,7,8,9,10,11,12,13,14,15
    movaps slot(%rsp), %xmm\reg
    .set slot, slot+16
    .endr
    addq $0x118, %rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rsi
    popq %rdi
    popq %rbp
    popq %rbx
    ret
.seh_endproc
.section .rdata,"dr"
.balign 16
file_probe_seed:
    .quad 0x13579bdf2468ace0, 0xfedcba9876543210

.text
.globl file_bad_abi
.def file_bad_abi; .scl 2; .type 32; .endef
file_bad_abi:
    xorq %rbx, %rbx
    xorq %rax, %rax
    ret
