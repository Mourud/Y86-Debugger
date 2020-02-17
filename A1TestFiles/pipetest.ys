.pos 0x100
fwdOrder:        irmovq $0x1, %rax        
                 irmovq $0x2, %rax        
                 rrmovq %rax, %rcx        # data-forwarding order (1)
                 irmovq $0x1, %rdx        
                 irmovq $0x2, %rdx        
                 irmovq $0x3, %rdx        
                 rrmovq %rdx, %rbx        # data-forwarding order (2)
                 halt                     
.pos 0x200
srcAHzd:         irmovq $0x0, %rax        
                 irmovq $0x0, %rcx        
                 irmovq $0x0, %rdx        
                 irmovq $0x0, %rbx        
                 irmovq $0x1, %rax        # r-r data hazard on srcA (1)
                 addq   %rax, %rbx        
                 irmovq $0x2, %rcx        # r-r data hazard on srcA (2)
                 nop                      
                 addq   %rcx, %rbx        
                 irmovq $0x3, %rdx        # r-r data hazard on srcA (3)
                 nop                      
                 nop                      
                 addq   %rdx, %rbx        
                 halt                     
.pos 0x300
sBHazard:        irmovq $0x0, %rax        
                 irmovq $0x0, %rcx        
                 irmovq $0x0, %rdx        
                 irmovq $0x10, %rbx       
                 irmovq $0x1, %rax        # r-r data hazard on srcB (1)
                 addq   %rbx, %rax        
                 irmovq $0x2, %rcx        # r-r data hazard on srcB (2)
                 nop                      
                 addq   %rbx, %rcx        
                 irmovq $0x3, %rdx        # r-r data hazard on srcB (3)
                 nop                      
                 nop                      
                 addq   %rbx, %rdx        
                 halt                     
.pos 0x400
aLoadUse:        irmovq $0x0, %rax        
                 irmovq $0x0, %rcx        
                 irmovq $0x0, %rdx        
                 irmovq $0x0, %rbx        
                 irmovq $x, %rdi          
                 mrmovq 0x0(%rdi), %rax   # load-use data hazard on srcA (1)
                 addq   %rax, %rbx        
                 mrmovq 0x0(%rdi), %rcx   # load-use data hazard on srcA (2)
                 nop                      
                 addq   %rcx, %rbx        
                 mrmovq 0x0(%rdi), %rdx   # load-use data hazard on srcA (3)
                 nop                      
                 nop                      
                 addq   %rdx, %rbx        
                 halt                     
.pos 0x500
bLoadUse:        irmovq $0x0, %rax        
                 irmovq $0x0, %rcx        
                 irmovq $0x0, %rdx        
                 irmovq $0x10, %rbx       
                 irmovq $x, %rdi          
                 mrmovq 0x0(%rdi), %rax   # load-use data hazard on srcB (1)
                 addq   %rbx, %rax        
                 mrmovq 0x0(%rdi), %rcx   # load-use data hazard on srcB (2)
                 nop                      
                 addq   %rbx, %rcx        
                 mrmovq 0x0(%rdi), %rdx   # load-use data hazard on srcB (3)
                 nop                      
                 nop                      
                 addq   %rbx, %rdx        
                 halt                     
.pos 0x600
takenJmp:        irmovq $0x0, %rax        
                 irmovq $0x0, %rcx        
                 irmovq $0x0, %rdx        
                 irmovq $0x0, %rbx        
                 irmovq $0x0, %rsp        
                 andq   %rax, %rax        
                 je     JT                # taken jump
                 irmovq $0x1, %rcx        # should not execute
                 irmovq $0x1, %rdx        # should not execute
                 nop                      
                 halt                     
JT:              irmovq $0x1, %rbx        # should execute
                 irmovq $0x1, %rsp        # should execute
                 halt                     
.pos 0x700
notTkJmp:        irmovq $0x0, %rax        
                 irmovq $0x0, %rcx        
                 irmovq $0x0, %rdx        
                 irmovq $0x0, %rbx        
                 irmovq $0x0, %rsp        
                 andq   %rax, %rax        
                 jne    JNT               # not-taken jump
                 irmovq $0x1, %rcx        # should execute
                 irmovq $0x1, %rdx        # should execute
                 halt                     
JNT:             irmovq $0x1, %rbx        # should not execute
                 irmovq $0x1, %rsp        # should not execute
                 halt                     
.pos 0x800
callRtn:         irmovq $stack, %rsp      
                 irmovq $0x0, %rax        
                 irmovq $0x0, %rcx        
                 irmovq $0x0, %rdx        
                 call   CR                
                 irmovq $0x1, %rcx        # should execute
                 halt                     
CR:              irmovq $0x1, %rax        
                 ret                      # return
                 irmovq $0x1, %rdx        # should not execute
                 halt   
.pos 0x900
cmov:            irmovq $1, %rax
                 irmovq $2, %rbx
                 xorq   %rcx, %rcx
                 cmovne %rax, %rbx
                 addq   %rbx, %rbx
                 halt                  
.pos 0x1000
x:               .quad 0x0000000a         
.pos 0xf000
                 .quad 0x00000000         # runtime stack
                 .quad 0x00000000         
                 .quad 0x00000000         
                 .quad 0x00000000         
                 .quad 0x00000000         
stack:           .quad 0x00000000         
