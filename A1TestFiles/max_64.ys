
# The person doing the conversion of this routine from
# a 32 bit machine to 64 bits has made an error and the program
# now crashes. See if you can figure out what the problem is.
# The fix involves changing the existing code. You do not have
# to add any code nor do you have to delete any code.

.pos 0x100
max:             irmovq $a, %rbx          # rbx = &a
                 mrmovq 0x0(%rbx), %rbx   # rbx = a
                 irmovq $aSize, %rcx      # rcx = &aSize
                 mrmovq 0x0(%rcx), %rcx   # rcx = aSize = cnt
                 irmovq $0x80000000, %rax # rax =small num = max
                 irmovq $0x4, %rdi        # rdi = 4
loop:            irmovq $0x1, %rdx        # rdx = 1
                 subq   %rdx, %rcx        # cnt -= 1
                 jl     done              # goto done if cnt < 0
                 mrmovq 0x0(%rbx), %rdx   # rdx = *a
                 addq   %rdi, %rbx        # a++
                 rrmovq %rdx, %rsi        # rsi = *a = t
                 subq   %rax, %rsi        # t = *a - max
                 jle    loop              # goto loop if  *a <= max
                 rrmovq %rdx, %rax        # max = *a
                 jmp    loop              # goto loop
done:            irmovq $result, %rbx     # rbx = &result
                 rmmovq %rax, 0x0(%rbx)   # result = min
                 halt                     
.pos 0x1000
a:               .quad 0x00002000         # int *a = aData
aSize:           .quad 0x0000000a         
result:          .quad 0x00000000         
.pos 0x2000
aData:           .quad 0x0000000e         # aData[0]
                 .quad 0x00000003         # aData[1]
                 .quad 0x0000001d         # aData[2]
                 .quad 0x0000000f         # aData[3]
                 .quad 0x00000010         # aData[4]
                 .quad 0x000002be         # aData[5]
                 .quad 0x00000141         # aData[6]
                 .quad 0x0000002b         # aData[7]
                 .quad 0xffffff9c         # aData[8]
                 .quad 0x00000020         # aData[9]
