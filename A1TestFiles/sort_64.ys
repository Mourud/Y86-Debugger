.pos 0x100
sort:            irmovq $aSize, %rax      # rax = &aSize
                 mrmovq 0x0(%rax), %rax   # rax = aSize = i
                 irmovq $a, %rcx          # rcx = &a
                 mrmovq 0x0(%rcx), %rcx   # rcx = a
outer:           irmovq $0x1, %rdi        # rdi = 1
                 subq   %rdi, %rax        # i -= 1
                 jl     done              # goto done if i < 0
                 mrmovq 0x0(%rcx), %rbx   # rbx = *a
                 rrmovq %rax, %rdx        # j = i
                 rrmovq %rcx, %rsi        # b = a
                 irmovq $0x8, %rdi        # rdi = 8
                 addq   %rdi, %rsi        # b++
inner:           irmovq $0x1, %rdi        # rdi = 1
                 subq   %rdi, %rdx        # j--
                 jl     endInner          # goto endInner if j<0
                 mrmovq 0x0(%rsi), %rdi   # rdi = *b
                 rrmovq %rdi, %rbp        # t = *b
                 subq   %rbx, %rbp        # t = *b - *a
                 jge    skipSwap          # goto skipSwap if *b >= *a
                 rmmovq %rbx, 0x0(%rsi)   # *b = *a
                 rmmovq %rdi, 0x0(%rcx)   # *a = *b
                 rrmovq %rdi, %rbx        # rbx = *b
skipSwap:        irmovq $0x8, %rdi        # rdi = 4
                 addq   %rdi, %rsi        # b++
                 jmp    inner             # goto inner
endInner:        irmovq $0x8, %rdi        # rdi = 4
                 addq   %rdi, %rcx        # a++
                 jmp    outer             # goto outer
done:            halt                     
.pos 0x1000
a:               .quad 0x00002000         # int *a
aSize:           .quad 0x0000000a         # int aSize
.pos 0x2000
aData:           .quad 0x00000007         # aData[0]
                 .quad 0x00000003         # aData[1]
                 .quad 0x00000004         # aData[2]
                 .quad 0x0000000a         # aData[3]
                 .quad 0x00000005         # aData[4]
                 .quad 0x00000008         # aData[5]
                 .quad 0x00000009         # aData[6]
                 .quad 0x00000001         # aData[7]
                 .quad 0x00000006         # aData[8]
                 .quad 0x00000002         # aData[9]
