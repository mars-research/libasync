/********************************************************************************/
/* Direct path from AWE to AWE: 29-31 cycles
/* 16 memory accesses with push/pop/mov x 2 = 31 cycles
/* Specifically: 
     5x2 = 10 -- save and restore callee registers (r15, r14, r13, r12, rbx)
     save and restore eip, rbp, rsp (do we need rbp?)
/* 1 jump, 1 call, 1 return, 1 add = 4 
/* Total is in 31 range... 
/********************************************************************************/

Dump of assembler code for function THCYieldToAwe:
   0x00000000004011d0 <+0>:	push   %r15
   0x00000000004011d2 <+2>:	push   %r14
   0x00000000004011d4 <+4>:	push   %r13
   0x00000000004011d6 <+6>:	push   %r12
   0x00000000004011d8 <+8>:	push   %rbx
   0x00000000004011d9 <+9>:	callq  0x400ae0 <_thc_exec_awe_direct>
   0x00000000004011de <+14>:	pop    %rbx
   0x00000000004011df <+15>:	pop    %r12
   0x00000000004011e1 <+17>:	pop    %r13
   0x00000000004011e3 <+19>:	pop    %r14
   0x00000000004011e5 <+21>:	pop    %r15
   0x00000000004011e7 <+23>:	retq   
End of assembler dump.
(gdb) disas _thc_exec_awe_direct
Dump of assembler code for function _thc_exec_awe_direct:
   0x0000000000400ae0 <+0>:	mov    (%rsp),%rax            # save return eip of the awe_from (it's on the stack) into rax
   0x0000000000400ae4 <+4>:	mov    %rax,(%rdi)            # save eip into awe (it's in rdi)
   0x0000000000400ae7 <+7>:	mov    %rbp,0x8(%rdi)         # save rbp into awe->rbp
   0x0000000000400aeb <+11>:	mov    %rsp,0x10(%rdi)        # save rsp into awe->rsp
   0x0000000000400aef <+15>:	addq   $0x8,0x10(%rdi)
   0x0000000000400af4 <+20>:	mov    0x8(%rsi),%rbp         # restore rbp from awe_to (it's in rsi)
   0x0000000000400af8 <+24>:	mov    0x10(%rsi),%rsp        # restore rsp from awe_to
   0x0000000000400afc <+28>:	jmpq   *(%rsi)
   0x0000000000400afe <+30>:	int3   
   0x0000000000400aff <+31>:	nop


/********************************************************************************/
/* Direct path without checks for awe id in range
/* 21 memory accesses with push/pop/mov x 2 = 42
/* 10 register moves, adds, and so on... x1 = 10 
/* Total is in the 52 range... of course some execute 
/* in parallel, but in the end we're blocked on something
/********************************************************************************/

Dump of assembler code for function THCYieldToIdAndSaveNoDispatchDirectTrusted:
   0x0000000000401150 <+0>:	push   %r15
   0x0000000000401152 <+2>:	mov    %edi,%edi
   0x0000000000401154 <+4>:	mov    %esi,%esi
   0x0000000000401156 <+6>:	push   %r14
   0x0000000000401158 <+8>:	push   %r13
   0x000000000040115a <+10>:	push   %r12
   0x000000000040115c <+12>:	push   %rbx
   0x000000000040115d <+13>:	sub    $0x30,%rsp
   0x0000000000401161 <+17>:	mov    0x202f38(%rip),%rax        # 0x6040a0 <global_pts>
   0x0000000000401168 <+24>:	mov    0xb8(%rax),%rax
   0x000000000040116f <+31>:	mov    (%rax,%rdi,8),%rdx
   0x0000000000401173 <+35>:	mov    %rsp,(%rax,%rsi,8)
   0x0000000000401177 <+39>:	mov    %rdx,%rsi
   0x000000000040117a <+42>:	mov    %rsp,%rdi
   0x000000000040117d <+45>:	callq  0x400ac0 <_thc_exec_awe_direct>
   0x0000000000401182 <+50>:	add    $0x30,%rsp
   0x0000000000401186 <+54>:	xor    %eax,%eax
   0x0000000000401188 <+56>:	pop    %rbx
   0x0000000000401189 <+57>:	pop    %r12
   0x000000000040118b <+59>:	pop    %r13
   0x000000000040118d <+61>:	pop    %r14
   0x000000000040118f <+63>:	pop    %r15
   0x0000000000401191 <+65>:	retq   
End of assembler dump.
(gdb) disas _thc_exec_awe_direct
Dump of assembler code for function _thc_exec_awe_direct:
   0x0000000000400ac0 <+0>:	mov    (%rsp),%rax
   0x0000000000400ac4 <+4>:	mov    %rax,(%rdi)
   0x0000000000400ac7 <+7>:	mov    %rbp,0x8(%rdi)
   0x0000000000400acb <+11>:	mov    %rsp,0x10(%rdi)
   0x0000000000400acf <+15>:	addq   $0x8,0x10(%rdi)
   0x0000000000400ad4 <+20>:	mov    0x8(%rsi),%rbp
   0x0000000000400ad8 <+24>:	mov    0x10(%rsi),%rsp
   0x0000000000400adc <+28>:	jmpq   *(%rsi)
   0x0000000000400ade <+30>:	int3   
   0x0000000000400adf <+31>:	nop


/********************************************************************************/
/* Direct path */
/********************************************************************************/

Dump of assembler code for function THCYieldToIdAndSaveNoDispatchDirect:
   0x00000000004010f0 <+0>:	mov    0x201fa9(%rip),%rax        # 0x6030a0 <global_pts>
   0x00000000004010f7 <+7>:	cmp    $0x3ff,%edi
   0x00000000004010fd <+13>:	mov    0xb8(%rax),%rdx
   0x0000000000401104 <+20>:	mov    $0xffffffff,%eax
   0x0000000000401109 <+25>:	ja     0x401143 <THCYieldToIdAndSaveNoDispatchDirect+83>
   0x000000000040110b <+27>:	mov    %edi,%edi
   0x000000000040110d <+29>:	mov    (%rdx,%rdi,8),%rcx
   0x0000000000401111 <+33>:	test   %rcx,%rcx
   0x0000000000401114 <+36>:	je     0x401143 <THCYieldToIdAndSaveNoDispatchDirect+83>
   0x0000000000401116 <+38>:	push   %r15
   0x0000000000401118 <+40>:	mov    %esi,%esi
   0x000000000040111a <+42>:	push   %r14
   0x000000000040111c <+44>:	push   %r13
   0x000000000040111e <+46>:	push   %r12
   0x0000000000401120 <+48>:	push   %rbx
   0x0000000000401121 <+49>:	sub    $0x30,%rsp
   0x0000000000401125 <+53>:	mov    %rsp,(%rdx,%rsi,8)
   0x0000000000401129 <+57>:	mov    %rcx,%rsi
   0x000000000040112c <+60>:	mov    %rsp,%rdi
   0x000000000040112f <+63>:	callq  0x400ac0 <_thc_exec_awe_fast>
   0x0000000000401134 <+68>:	add    $0x30,%rsp
   0x0000000000401138 <+72>:	xor    %eax,%eax
   0x000000000040113a <+74>:	pop    %rbx
   0x000000000040113b <+75>:	pop    %r12
   0x000000000040113d <+77>:	pop    %r13
   0x000000000040113f <+79>:	pop    %r14
   0x0000000000401141 <+81>:	pop    %r15
   0x0000000000401143 <+83>:	repz retq 
End of assembler dump.
(gdb) disas _thc_exec_awe_direct
Dump of assembler code for function _thc_exec_awe_direct:
   0x0000000000400ac0 <+0>:	mov    (%rsp),%rax
   0x0000000000400ac4 <+4>:	mov    %rax,(%rdi)
   0x0000000000400ac7 <+7>:	mov    %rbp,0x8(%rdi)
   0x0000000000400acb <+11>:	mov    %rsp,0x10(%rdi)
   0x0000000000400acf <+15>:	addq   $0x8,0x10(%rdi)
   0x0000000000400ad4 <+20>:	mov    0x8(%rsi),%rbp
   0x0000000000400ad8 <+24>:	mov    0x10(%rsi),%rsp
   0x0000000000400adc <+28>:	jmpq   *(%rsi)
   0x0000000000400ade <+30>:	int3   
   0x0000000000400adf <+31>:	nop
End of assembler dump.




/********************************************************************************/
/* Call continuation path */
/********************************************************************************/

/* 28 memory instructions */
/* 5 arithm */
/* 5 jumps and 2 calls */

Dump of assembler code for function THCYieldToIdAndSaveNoDispatch:
   0x0000000000401070 <+0>:	mov    0x202029(%rip),%rax        # 0x6030a0 <global_pts>
   0x0000000000401077 <+7>:	cmp    $0x3ff,%edi
   0x000000000040107d <+13>:	mov    0xb8(%rax),%rcx
   0x0000000000401084 <+20>:	mov    $0xffffffff,%eax
   0x0000000000401089 <+25>:	ja     0x4010c5 <THCYieldToIdAndSaveNoDispatch+85>
   0x000000000040108b <+27>:	mov    %edi,%edi
   0x000000000040108d <+29>:	mov    (%rcx,%rdi,8),%rdx
   0x0000000000401091 <+33>:	test   %rdx,%rdx
   0x0000000000401094 <+36>:	je     0x4010c5 <THCYieldToIdAndSaveNoDispatch+85>
   0x0000000000401096 <+38>:	push   %r15
   0x0000000000401098 <+40>:	mov    %esi,%esi
   0x000000000040109a <+42>:	push   %r14
   0x000000000040109c <+44>:	push   %r13
   0x000000000040109e <+46>:	push   %r12
   0x00000000004010a0 <+48>:	push   %rbx
   0x00000000004010a1 <+49>:	sub    $0x30,%rsp
   0x00000000004010a5 <+53>:	mov    %rsp,(%rcx,%rsi,8)
   0x00000000004010a9 <+57>:	mov    $0x400c20,%esi
   0x00000000004010ae <+62>:	mov    %rsp,%rdi
   0x00000000004010b1 <+65>:	callq  0x400aa0 <_thc_callcont>
   0x00000000004010b6 <+70>:	add    $0x30,%rsp
   0x00000000004010ba <+74>:	xor    %eax,%eax
   0x00000000004010bc <+76>:	pop    %rbx
   0x00000000004010bd <+77>:	pop    %r12
   0x00000000004010bf <+79>:	pop    %r13
   0x00000000004010c1 <+81>:	pop    %r14
   0x00000000004010c3 <+83>:	pop    %r15
   0x00000000004010c5 <+85>:	repz retq 

Dump of assembler code for function _thc_callcont:
   0x0000000000400aa0 <+0>:	mov    (%rsp),%rax
   0x0000000000400aa4 <+4>:	mov    %rax,(%rdi)
   0x0000000000400aa7 <+7>:	mov    %rbp,0x8(%rdi)
   0x0000000000400aab <+11>:	mov    %rsp,0x10(%rdi)
   0x0000000000400aaf <+15>:	addq   $0x8,0x10(%rdi)
   0x0000000000400ab4 <+20>:	callq  0x400ea0 <_thc_callcont_c>
   0x0000000000400ab9 <+25>:	int3   
   0x0000000000400aba <+26>:	nopw   0x0(%rax,%rax,1)

Dump of assembler code for function _thc_callcont_c:
   0x0000000000400ea0 <+0>:	mov    %rsi,%rax
   0x0000000000400ea3 <+3>:	mov    %rdx,%rsi
   0x0000000000400ea6 <+6>:	jmpq   *%rax

Dump of assembler code for function thc_yieldto_with_cont_no_dispatch:
   0x0000000000400c20 <+0>:	mov    %rsi,%rdi
   0x0000000000400c23 <+3>:	jmpq   0x400a60 <thc_awe_execute_0>

Dump of assembler code for function thc_awe_execute_0:
   0x0000000000400a60 <+0>:	mov    0x8(%rdi),%rbp
   0x0000000000400a64 <+4>:	mov    0x10(%rdi),%rsp
   0x0000000000400a68 <+8>:	jmpq   *(%rdi)
   0x0000000000400a6a <+10>:	nopw   0x0(%rax,%rax,1)

/************************************************************/
/* The path before "one finish block, one PTS" optimization */
/************************************************************/

Dump of assembler code for function THCYieldToIdAndSaveNoDispatch:
   0x00000000004011e0 <+0>:	mov    0x201ec1(%rip),%rax        # 0x6030a8 <global_pts>  # move pts into rax
   0x00000000004011e7 <+7>:	cmp    $0x3ff,%edi                                         # edi is id_to, check it's less 1024 
   0x00000000004011ed <+13>:	mov    0xe8(%rax),%rax                                     # move PTS->awe_map into rax
   0x00000000004011f4 <+20>:	ja     0x401240 <THCYieldToIdAndSaveNoDispatch+96>         # check edi (id_to) is in range
   0x00000000004011f6 <+22>:	mov    %edi,%edi                                           #  
   0x00000000004011f8 <+24>:	mov    (%rax,%rdi,8),%rdx                                  # move awe_to to rdx, or awe_map[id_to] (awe_map is rax, id_to is rdi) to rdx
   0x00000000004011fc <+28>:	test   %rdx,%rdx                                           # check awe_to is not null 
   0x00000000004011ff <+31>:	je     0x401240 <THCYieldToIdAndSaveNoDispatch+96>         
   0x0000000000401201 <+33>:	push   %r15                                                # Save callee registers
   0x0000000000401203 <+35>:	push   %r14
   0x0000000000401205 <+37>:	push   %r13
   0x0000000000401207 <+39>:	push   %r12
   0x0000000000401209 <+41>:	push   %rbx
   0x000000000040120a <+42>:	sub    $0x40,%rsp                                          # allocate space for the awe_from on the stack
   0x000000000040120e <+46>:	cmp    $0x3ff,%esi                                         # again... check is id_from (esi) is in range (not needed), assert was enabled
   0x0000000000401214 <+52>:	ja     0x401246 <THCYieldToIdAndSaveNoDispatch+102>      
   0x0000000000401216 <+54>:	mov    %esi,%esi
   0x0000000000401218 <+56>:	mov    %rsp,(%rax,%rsi,8)                                  # move awe_from (rsp) into awe_map (rax), rsi contains id_from
   0x000000000040121c <+60>:	mov    $0x400d10,%esi                                      # move &thc_yieldto_with_cont_no_dispatch into esi 
   0x0000000000401221 <+65>:	mov    %rsp,%rdi                                           # save awe_from (rsp) into rdi 
   0x0000000000401224 <+68>:	callq  0x400b50 <_thc_callcont>                            
   0x0000000000401229 <+73>:	add    $0x40,%rsp
   0x000000000040122d <+77>:	xor    %eax,%eax
   0x000000000040122f <+79>:	pop    %rbx
   0x0000000000401230 <+80>:	pop    %r12
   0x0000000000401232 <+82>:	pop    %r13
   0x0000000000401234 <+84>:	pop    %r14
   0x0000000000401236 <+86>:	pop    %r15
   0x0000000000401238 <+88>:	retq   

   0x0000000000401239 <+89>:	nopl   0x0(%rax)
   0x0000000000401240 <+96>:	mov    $0xffffffff,%eax
   0x0000000000401245 <+101>:	retq   
   0x0000000000401246 <+102>:	callq  0x400870 <awe_mapper_set_id>



   0x400b50 <_thc_callcont>                mov    (%rsp),%rax                  # save eip which is currently on the stack into rax   
   0x400b54 <_thc_callcont+4>              mov    %rax,(%rdi)                  # save eip into awe_from  
   0x400b57 <_thc_callcont+7>              mov    %rbp,0x8(%rdi)               # save ebp (hell do we need it?)
   0x400b5b <_thc_callcont+11>             mov    %rsp,0x10(%rdi)              # ESP after return + 8 
   0x400b5f <_thc_callcont+15>             addq   $0x8,0x10(%rdi)                                                                                                                   
   0x400b64 <_thc_callcont+20>             callq  0x400fb0 <_thc_callcont_c>    

   0x400fb0 <_thc_callcont_c>              mov    0x2020f1(%rip),%rax        # 0x6030a8 <global_pts>                                                                                
   0x400fb7 <_thc_callcont_c+7>            mov    %rsi,%rcx                                                                                                                         
   0x400fba <_thc_callcont_c+10>           mov    %rdx,%rsi                                                                                                                         
   0x400fbd <_thc_callcont_c+13>           mov    %rax,0x18(%rdi)                                                                                                                   
   0x400fc1 <_thc_callcont_c+17>           mov    0x70(%rax),%rax                                                                                                                   
   0x400fc5 <_thc_callcont_c+21>           mov    %rax,0x20(%rdi)                                                                                                                   
   0x400fc9 <_thc_callcont_c+25>           jmpq   *%rcx

   0x400d10 <thc_yieldto_with_cont_no_dispatch>    mov    0x18(%rsi),%rax                                                                                                           
   0x400d14 <thc_yieldto_with_cont_no_dispatch+4>  mov    0x20(%rsi),%rdx                                                                                                           
   0x400d18 <thc_yieldto_with_cont_no_dispatch+8>  mov    %rsi,%rdi                                                                                                                 
   0x400d1b <thc_yieldto_with_cont_no_dispatch+11> mov    %rdx,0x70(%rax)                                                                                                           
   0x400d1f <thc_yieldto_with_cont_no_dispatch+15> jmpq   0x400b10 <thc_awe_execute_0>

   0x400b10 <thc_awe_execute_0>            mov    0x8(%rdi),%rbp                                                                                                                    
   0x400b14 <thc_awe_execute_0+4>          mov    0x10(%rdi),%rsp                                                                                                                   
   0x400b18 <thc_awe_execute_0+8>          jmpq   *(%rdi)

   0x401224 <THCYieldToIdAndSaveNoDispatch+68>     callq  0x400b50 <_thc_callcont>                                                                                                  
 =>0x401229 <THCYieldToIdAndSaveNoDispatch+73>     add    $0x40,%rsp                                                                                                                
   0x40122d <THCYieldToIdAndSaveNoDispatch+77>     xor    %eax,%eax                                                                                                                 
   0x40122f <THCYieldToIdAndSaveNoDispatch+79>     pop    %rbx                                                                                                                      
   0x401230 <THCYieldToIdAndSaveNoDispatch+80>     pop    %r12                                                                                                                      
   0x401232 <THCYieldToIdAndSaveNoDispatch+82>     pop    %r13                                                                                                                      
   0x401234 <THCYieldToIdAndSaveNoDispatch+84>     pop    %r14                                                                                                                      
   0x401236 <THCYieldToIdAndSaveNoDispatch+86>     pop    %r15                                                                                                                      
   0x401238 <THCYieldToIdAndSaveNoDispatch+88>     retq


