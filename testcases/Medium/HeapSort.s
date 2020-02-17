.intel_syntax noprefix
.global Lmain
LHeap$init:
  PUSH ebp
  MOV ebp, esp
  SUB esp, 4
  MOV DWORD PTR [ ebp + -4 ], ebx
  Lt199:
  MOV ebx, DWORD PTR [ ebp + 8 ]
  ADD ebx, 4
  MOV ecx, DWORD PTR [ ebp + 12 ]
  MOV DWORD PTR [ ebx ], ecx
  MOV ebx, DWORD PTR [ ebp + 8 ]
  ADD ebx, 8
  MOV ecx, ebx
  MOV ebx, DWORD PTR [ ebp + 12 ]
  MOV ebx, DWORD PTR [ ebx ]
  MOV DWORD PTR [ ecx ], ebx
  PUSH DWORD PTR [ ebp + 8 ]
  CALL LHeap$heapify
  ADD esp, 4
  Lt3:
  JMP Lt200
  Lt200:
  MOV ebx, DWORD PTR [ ebp + -4 ]
  MOV esp, ebp
  POP ebp
  RET

LHeap$max:
  PUSH ebp
  MOV ebp, esp
  SUB esp, 4
  MOV DWORD PTR [ ebp + -4 ], ebx
  MOV ecx, esi
  Lt201:
  MOV ebx, DWORD PTR [ ebp + 8 ]
  ADD ebx, 4
  MOV ebx, DWORD PTR [ ebx ]
  MOV edx, 0
  CMP edx, DWORD PTR [1 * ebx + 0]
  JL Lt7
  LHeap$max$raise:
  PUSH 1
  CALL L_raise
  ADD esp, 4
  JMP LHeap$max$raise
  Lt7:
  MOV eax, DWORD PTR [1 * ebx + 4]
  Lt8:
  JMP Lt202
  Lt202:
  MOV ebx, DWORD PTR [ ebp + -4 ]
  MOV esi, ecx
  MOV esp, ebp
  POP ebp
  RET

LHeap$size:
  PUSH ebp
  MOV ebp, esp
  SUB esp, 0
  MOV ecx, esi
  MOV edx, edi
  Lt203:
  MOV esi, DWORD PTR [ ebp + 8 ]
  ADD esi, 8
  MOV eax, esi
  MOV eax, DWORD PTR [ eax ]
  Lt11:
  JMP Lt204
  Lt204:
  MOV esi, ecx
  MOV edi, edx
  MOV esp, ebp
  POP ebp
  RET

LHeap$pop:
  PUSH ebp
  MOV ebp, esp
  SUB esp, 4
  MOV DWORD PTR [ ebp + -4 ], ebx
  Lt205:
  MOV ebx, DWORD PTR [ ebp + 8 ]
  ADD ebx, 4
  MOV ebx, DWORD PTR [ ebx ]
  MOV ecx, 0
  CMP ecx, DWORD PTR [1 * ebx + 0]
  JL Lt19
  LHeap$pop$raise:
  PUSH 1
  CALL L_raise
  ADD esp, 4
  JMP LHeap$pop$raise
  Lt19:
  MOV ebx, DWORD PTR [1 * ebx + 4]
  MOV ecx, DWORD PTR [ ebp + 8 ]
  ADD ecx, 8
  MOV edx, ecx
  MOV ecx, DWORD PTR [ ebp + 8 ]
  ADD ecx, 8
  MOV ecx, DWORD PTR [ ecx ]
  SUB ecx, 1
  MOV DWORD PTR [ edx ], ecx
  MOV ecx, DWORD PTR [ ebp + 8 ]
  ADD ecx, 8
  MOV eax, ecx
  PUSH DWORD PTR [ eax ]
  PUSH 0
  PUSH DWORD PTR [ ebp + 8 ]
  CALL LHeap$swap
  ADD esp, 12
  PUSH 0
  PUSH eax
  CALL LHeap$sift_down
  ADD esp, 8
  MOV eax, ebx
  Lt20:
  JMP Lt206
  Lt206:
  MOV ebx, DWORD PTR [ ebp + -4 ]
  MOV esp, ebp
  POP ebp
  RET

LHeap$heapify:
  PUSH ebp
  MOV ebp, esp
  SUB esp, 4
  MOV DWORD PTR [ ebp + -4 ], ebx
  Lt207:
  MOV ebx, DWORD PTR [ ebp + 8 ]
  ADD ebx, 8
  MOV eax, ebx
  MOV eax, DWORD PTR [ eax ]
  MOV edx, eax
  SAR edx, 31
  MOV ebx, 2
  IDIV ebx
  MOV ebx, eax
  Lt27:
  MOV ecx, -1
  CMP ecx, ebx
  JL Lt28
  Lt29:
  MOV eax, DWORD PTR [ ebp + 8 ]
  Lt32:
  JMP Lt208
  Lt28:
  PUSH ebx
  PUSH DWORD PTR [ ebp + 8 ]
  CALL LHeap$sift_down
  ADD esp, 8
  MOV ecx, eax
  LEA ebx, DWORD PTR [1 * ebx + -1]
  JMP Lt27
  Lt208:
  MOV ebx, DWORD PTR [ ebp + -4 ]
  MOV esp, ebp
  POP ebp
  RET

LHeap$sift_down:
  PUSH ebp
  MOV ebp, esp
  SUB esp, 16
  MOV DWORD PTR [ ebp + -8 ], ebx
  MOV DWORD PTR [ ebp + -12 ], esi
  MOV DWORD PTR [ ebp + -16 ], edi
  Lt45:
  PUSH DWORD PTR [ ebp + 12 ]
  PUSH DWORD PTR [ ebp + 8 ]
  CALL LHeap$left
  ADD esp, 8
  MOV ecx, eax
  MOV ebx, DWORD PTR [ ebp + 8 ]
  ADD ebx, 8
  MOV ebx, DWORD PTR [ ebx ]
  CMP ecx, ebx
  JL Lt46
  Lt47:
  MOV eax, DWORD PTR [ ebp + 8 ]
  Lt90:
  JMP Lt210
  Lt46:
  PUSH DWORD PTR [ ebp + 12 ]
  PUSH DWORD PTR [ ebp + 8 ]
  CALL LHeap$left
  ADD esp, 8
  MOV ebx, eax
  PUSH DWORD PTR [ ebp + 12 ]
  PUSH DWORD PTR [ ebp + 8 ]
  CALL LHeap$right
  ADD esp, 8
  MOV esi, eax
  MOV ecx, DWORD PTR [ ebp + 12 ]
  MOV DWORD PTR [ ebp + -4 ], ecx
  MOV ecx, DWORD PTR [ ebp + 8 ]
  ADD ecx, 8
  MOV ecx, DWORD PTR [ ecx ]
  CMP ebx, ecx
  JL Lt53
  Lt51:
  Lt52:
  MOV ecx, esi
  MOV ebx, DWORD PTR [ ebp + 8 ]
  ADD ebx, 8
  MOV ebx, DWORD PTR [ ebx ]
  CMP ecx, ebx
  JL Lt69
  Lt67:
  Lt68:
  MOV ecx, DWORD PTR [ ebp + -4 ]
  MOV ebx, DWORD PTR [ ebp + 12 ]
  CMP ecx, ebx
  JL Lt82
  Lt85:
  MOV ecx, DWORD PTR [ ebp + 12 ]
  MOV ebx, DWORD PTR [ ebp + -4 ]
  CMP ecx, ebx
  JL Lt82
  Lt83:
  MOV ebx, DWORD PTR [ ebp + 8 ]
  ADD ebx, 8
  MOV ebx, DWORD PTR [ ebx ]
  MOV DWORD PTR [ ebp + 12 ], ebx
  Lt84:
  JMP Lt45
  Lt82:
  PUSH DWORD PTR [ ebp + 12 ]
  MOV eax, DWORD PTR [ ebp + -4 ]
  PUSH eax
  PUSH DWORD PTR [ ebp + 8 ]
  CALL LHeap$swap
  ADD esp, 12
  MOV ebx, eax
  MOV ebx, DWORD PTR [ ebp + -4 ]
  MOV DWORD PTR [ ebp + 12 ], ebx
  JMP Lt84
  Lt69:
  MOV ebx, DWORD PTR [ ebp + 8 ]
  ADD ebx, 4
  MOV ecx, DWORD PTR [ ebx ]
  MOV ebx, DWORD PTR [ ebp + -4 ]
  CMP ebx, 0
  JGE Lt76
  LHeap$sift_down$raise:
  PUSH 1
  CALL L_raise
  ADD esp, 4
  JMP LHeap$sift_down$raise
  Lt76:
  CMP ebx, DWORD PTR [1 * ecx + 0]
  JGE LHeap$sift_down$raise
  Lt77:
  MOV ecx, DWORD PTR [ecx + 4 * ebx + 4]
  MOV ebx, DWORD PTR [ ebp + 8 ]
  ADD ebx, 4
  MOV ebx, DWORD PTR [ ebx ]
  MOV edx, esi
  CMP edx, 0
  JL LHeap$sift_down$raise
  Lt80:
  CMP edx, DWORD PTR [1 * ebx + 0]
  JGE LHeap$sift_down$raise
  Lt81:
  MOV ebx, DWORD PTR [ebx + 4 * edx + 4]
  CMP ecx, ebx
  JGE Lt67
  Lt66:
  MOV DWORD PTR [ ebp + -4 ], esi
  JMP Lt68
  Lt53:
  MOV ecx, DWORD PTR [ ebp + 8 ]
  ADD ecx, 4
  MOV ecx, DWORD PTR [ ecx ]
  MOV edx, DWORD PTR [ ebp + -4 ]
  CMP edx, 0
  JL LHeap$sift_down$raise
  Lt60:
  CMP edx, DWORD PTR [1 * ecx + 0]
  JGE LHeap$sift_down$raise
  Lt61:
  MOV edx, DWORD PTR [ecx + 4 * edx + 4]
  MOV ecx, DWORD PTR [ ebp + 8 ]
  ADD ecx, 4
  MOV ecx, DWORD PTR [ ecx ]
  CMP ebx, 0
  JL LHeap$sift_down$raise
  Lt64:
  CMP ebx, DWORD PTR [1 * ecx + 0]
  JGE LHeap$sift_down$raise
  Lt65:
  MOV ecx, DWORD PTR [ecx + 4 * ebx + 4]
  CMP edx, ecx
  JGE Lt51
  Lt50:
  MOV DWORD PTR [ ebp + -4 ], ebx
  JMP Lt52
  Lt210:
  MOV ebx, DWORD PTR [ ebp + -8 ]
  MOV esi, DWORD PTR [ ebp + -12 ]
  MOV edi, DWORD PTR [ ebp + -16 ]
  MOV esp, ebp
  POP ebp
  RET

LHeap$swap:
  PUSH ebp
  MOV ebp, esp
  SUB esp, 8
  MOV DWORD PTR [ ebp + -8 ], ebx
  MOV DWORD PTR [ ebp + -4 ], esi
  Lt211:
  MOV ebx, DWORD PTR [ ebp + 8 ]
  ADD ebx, 4
  MOV ebx, DWORD PTR [ ebx ]
  MOV ecx, DWORD PTR [ ebp + 12 ]
  CMP ecx, 0
  JGE Lt97
  LHeap$swap$raise:
  PUSH 1
  CALL L_raise
  ADD esp, 4
  JMP LHeap$swap$raise
  Lt97:
  CMP ecx, DWORD PTR [1 * ebx + 0]
  JGE LHeap$swap$raise
  Lt98:
  MOV ebx, DWORD PTR [ebx + 4 * ecx + 4]
  MOV ecx, DWORD PTR [ ebp + 8 ]
  ADD ecx, 4
  MOV edx, DWORD PTR [ ecx ]
  MOV ecx, DWORD PTR [ ebp + 12 ]
  CMP ecx, 0
  JL LHeap$swap$raise
  Lt101:
  CMP ecx, DWORD PTR [1 * edx + 0]
  JGE LHeap$swap$raise
  Lt102:
  LEA ecx, DWORD PTR [4 * ecx + 4]
  ADD edx, ecx
  MOV esi, edx
  MOV ecx, DWORD PTR [ ebp + 8 ]
  ADD ecx, 4
  MOV ecx, DWORD PTR [ ecx ]
  MOV edx, DWORD PTR [ ebp + 16 ]
  CMP edx, 0
  JL LHeap$swap$raise
  Lt105:
  CMP edx, DWORD PTR [1 * ecx + 0]
  JGE LHeap$swap$raise
  Lt106:
  MOV ecx, DWORD PTR [ecx + 4 * edx + 4]
  MOV DWORD PTR [1 * esi + 0], ecx
  MOV ecx, DWORD PTR [ ebp + 8 ]
  ADD ecx, 4
  MOV ecx, DWORD PTR [ ecx ]
  MOV edx, DWORD PTR [ ebp + 16 ]
  CMP edx, 0
  JL LHeap$swap$raise
  Lt109:
  CMP edx, DWORD PTR [1 * ecx + 0]
  JGE LHeap$swap$raise
  Lt110:
  MOV DWORD PTR [ecx + 4 * edx + 4], ebx
  MOV eax, DWORD PTR [ ebp + 8 ]
  Lt111:
  JMP Lt212
  Lt212:
  MOV ebx, DWORD PTR [ ebp + -8 ]
  MOV esi, DWORD PTR [ ebp + -4 ]
  MOV esp, ebp
  POP ebp
  RET

LHeap$parent:
  PUSH ebp
  MOV ebp, esp
  SUB esp, 4
  MOV DWORD PTR [ ebp + -4 ], esi
  MOV ecx, edi
  Lt213:
  MOV edx, DWORD PTR [ ebp + 12 ]
  SUB edx, 1
  MOV eax, edx
  MOV edx, eax
  SAR edx, 31
  MOV esi, 2
  IDIV esi
  Lt114:
  JMP Lt214
  Lt214:
  MOV esi, DWORD PTR [ ebp + -4 ]
  MOV edi, ecx
  MOV esp, ebp
  POP ebp
  RET

LHeap$left:
  PUSH ebp
  MOV ebp, esp
  SUB esp, 0
  MOV ecx, esi
  MOV edx, edi
  Lt215:
  MOV esi, 2
  IMUL esi, DWORD PTR [ ebp + 12 ]
  ADD esi, 1
  MOV eax, esi
  Lt117:
  JMP Lt216
  Lt216:
  MOV esi, ecx
  MOV edi, edx
  MOV esp, ebp
  POP ebp
  RET

LHeap$right:
  PUSH ebp
  MOV ebp, esp
  SUB esp, 0
  MOV ecx, esi
  MOV edx, edi
  Lt217:
  MOV esi, 2
  IMUL esi, DWORD PTR [ ebp + 12 ]
  ADD esi, 2
  MOV eax, esi
  Lt120:
  JMP Lt218
  Lt218:
  MOV esi, ecx
  MOV edi, edx
  MOV esp, ebp
  POP ebp
  RET

LHS$init:
  PUSH ebp
  MOV ebp, esp
  SUB esp, 8
  MOV DWORD PTR [ ebp + -4 ], ebx
  MOV DWORD PTR [ ebp + -8 ], esi
  Lt219:
  MOV ebx, DWORD PTR [ ebp + 8 ]
  ADD ebx, 8
  MOV ecx, DWORD PTR [ ebp + 12 ]
  MOV DWORD PTR [ ebx ], ecx
  MOV ebx, DWORD PTR [ ebp + 8 ]
  ADD ebx, 4
  MOV esi, ebx
  MOV ebx, DWORD PTR [ ebp + 12 ]
  LEA eax, DWORD PTR [4 * ebx + 4]
  PUSH eax
  CALL L_halloc
  ADD esp, 4
  MOV ecx, eax
  MOV DWORD PTR [1 * ecx + 0], ebx
  MOV DWORD PTR [1 * esi + 0], ecx
  MOV ebx, DWORD PTR [ ebp + 8 ]
  ADD ebx, 4
  MOV ebx, DWORD PTR [ ebx ]
  MOV ecx, 0
  CMP ecx, DWORD PTR [1 * ebx + 0]
  JL Lt126
  LHS$init$raise:
  PUSH 1
  CALL L_raise
  ADD esp, 4
  JMP LHS$init$raise
  Lt126:
  MOV DWORD PTR [1 * ebx + 4], 20
  MOV ebx, DWORD PTR [ ebp + 8 ]
  ADD ebx, 4
  MOV ecx, DWORD PTR [ ebx ]
  MOV ebx, 1
  CMP ebx, DWORD PTR [1 * ecx + 0]
  JGE LHS$init$raise
  Lt128:
  MOV DWORD PTR [1 * ecx + 8], 7
  MOV ebx, DWORD PTR [ ebp + 8 ]
  ADD ebx, 4
  MOV ecx, DWORD PTR [ ebx ]
  MOV ebx, 2
  CMP ebx, DWORD PTR [1 * ecx + 0]
  JGE LHS$init$raise
  Lt130:
  MOV DWORD PTR [1 * ecx + 12], 12
  MOV ebx, DWORD PTR [ ebp + 8 ]
  ADD ebx, 4
  MOV ecx, DWORD PTR [ ebx ]
  MOV ebx, 3
  CMP ebx, DWORD PTR [1 * ecx + 0]
  JGE LHS$init$raise
  Lt132:
  MOV DWORD PTR [1 * ecx + 16], 18
  MOV ebx, DWORD PTR [ ebp + 8 ]
  ADD ebx, 4
  MOV ecx, DWORD PTR [ ebx ]
  MOV ebx, 4
  CMP ebx, DWORD PTR [1 * ecx + 0]
  JGE LHS$init$raise
  Lt134:
  MOV DWORD PTR [1 * ecx + 20], 2
  MOV ebx, DWORD PTR [ ebp + 8 ]
  ADD ebx, 4
  MOV ecx, DWORD PTR [ ebx ]
  MOV ebx, 5
  CMP ebx, DWORD PTR [1 * ecx + 0]
  JGE LHS$init$raise
  Lt136:
  MOV DWORD PTR [1 * ecx + 24], 11
  MOV ebx, DWORD PTR [ ebp + 8 ]
  ADD ebx, 4
  MOV ecx, DWORD PTR [ ebx ]
  MOV ebx, 6
  CMP ebx, DWORD PTR [1 * ecx + 0]
  JGE LHS$init$raise
  Lt138:
  MOV DWORD PTR [1 * ecx + 28], 6
  MOV ebx, DWORD PTR [ ebp + 8 ]
  ADD ebx, 4
  MOV ecx, DWORD PTR [ ebx ]
  MOV ebx, 7
  CMP ebx, DWORD PTR [1 * ecx + 0]
  JGE LHS$init$raise
  Lt140:
  MOV DWORD PTR [1 * ecx + 32], 9
  MOV ebx, DWORD PTR [ ebp + 8 ]
  ADD ebx, 4
  MOV ecx, DWORD PTR [ ebx ]
  MOV ebx, 8
  CMP ebx, DWORD PTR [1 * ecx + 0]
  JGE LHS$init$raise
  Lt142:
  MOV DWORD PTR [1 * ecx + 36], 19
  MOV ebx, DWORD PTR [ ebp + 8 ]
  ADD ebx, 4
  MOV ecx, DWORD PTR [ ebx ]
  MOV ebx, 9
  CMP ebx, DWORD PTR [1 * ecx + 0]
  JGE LHS$init$raise
  Lt144:
  MOV DWORD PTR [1 * ecx + 40], 5
  MOV eax, DWORD PTR [ ebp + 8 ]
  Lt145:
  JMP Lt220
  Lt220:
  MOV ebx, DWORD PTR [ ebp + -4 ]
  MOV esi, DWORD PTR [ ebp + -8 ]
  MOV esp, ebp
  POP ebp
  RET

LHS$sort:
  PUSH ebp
  MOV ebp, esp
  SUB esp, 12
  MOV DWORD PTR [ ebp + -12 ], ebx
  MOV DWORD PTR [ ebp + -8 ], esi
  MOV DWORD PTR [ ebp + -4 ], edi
  Lt221:
  MOV ebx, DWORD PTR [ ebp + 8 ]
  ADD ebx, 4
  MOV eax, ebx
  MOV ebx, DWORD PTR [ eax ]
  PUSH 12
  CALL L_halloc
  ADD esp, 4
  PUSH ebx
  PUSH eax
  CALL LHeap$init
  ADD esp, 8
  MOV edi, eax
  Lt154:
  XOR ebx, ebx
  PUSH edi
  CALL LHeap$size
  ADD esp, 4
  MOV ecx, eax
  CMP ebx, ecx
  JL Lt155
  Lt156:
  MOV eax, DWORD PTR [ ebp + 8 ]
  Lt163:
  JMP Lt222
  Lt155:
  PUSH edi
  CALL LHeap$pop
  ADD esp, 4
  MOV ebx, eax
  MOV ecx, DWORD PTR [ ebp + 8 ]
  ADD ecx, 4
  MOV eax, ecx
  MOV esi, DWORD PTR [ eax ]
  PUSH edi
  CALL LHeap$size
  ADD esp, 4
  MOV ecx, eax
  CMP ecx, 0
  JGE Lt161
  LHS$sort$raise:
  PUSH 1
  CALL L_raise
  ADD esp, 4
  JMP LHS$sort$raise
  Lt161:
  CMP ecx, DWORD PTR [1 * esi + 0]
  JGE LHS$sort$raise
  Lt162:
  MOV DWORD PTR [esi + 4 * ecx + 4], ebx
  JMP Lt154
  Lt222:
  MOV ebx, DWORD PTR [ ebp + -12 ]
  MOV esi, DWORD PTR [ ebp + -8 ]
  MOV edi, DWORD PTR [ ebp + -4 ]
  MOV esp, ebp
  POP ebp
  RET

LHS$print:
  PUSH ebp
  MOV ebp, esp
  SUB esp, 4
  MOV DWORD PTR [ ebp + -4 ], ebx
  Lt223:
  XOR ebx, ebx
  PUSH 10
  CALL L_write
  ADD esp, 4
  MOV ecx, eax
  Lt169:
  MOV ecx, DWORD PTR [ ebp + 8 ]
  ADD ecx, 8
  MOV ecx, DWORD PTR [ ecx ]
  CMP ebx, ecx
  JL Lt170
  Lt171:
  MOV eax, DWORD PTR [ ebp + 8 ]
  Lt179:
  JMP Lt224
  Lt170:
  MOV ecx, DWORD PTR [ ebp + 8 ]
  ADD ecx, 4
  MOV ecx, DWORD PTR [ ecx ]
  CMP ebx, 0
  JGE Lt177
  LHS$print$raise:
  PUSH 1
  CALL L_raise
  ADD esp, 4
  JMP LHS$print$raise
  Lt177:
  CMP ebx, DWORD PTR [1 * ecx + 0]
  JGE LHS$print$raise
  Lt178:
  PUSH DWORD PTR [ecx + 4 * ebx + 4]
  CALL L_println_int
  ADD esp, 4
  MOV ecx, eax
  LEA ebx, DWORD PTR [1 * ebx + 1]
  JMP Lt169
  Lt224:
  MOV ebx, DWORD PTR [ ebp + -4 ]
  MOV esp, ebp
  POP ebp
  RET

LHS$dummy:
  PUSH ebp
  MOV ebp, esp
  SUB esp, 0
  MOV ecx, esi
  MOV edx, edi
  Lt225:
  MOV eax, 10
  Lt182:
  JMP Lt226
  Lt226:
  MOV esi, ecx
  MOV edi, edx
  MOV esp, ebp
  POP ebp
  RET

Lmain:
  PUSH ebp
  MOV ebp, esp
  SUB esp, 0
  Lt227:
  PUSH 12
  CALL L_halloc
  ADD esp, 4
  PUSH 10
  PUSH eax
  CALL LHS$init
  ADD esp, 8
  PUSH eax
  CALL LHS$print
  ADD esp, 4
  PUSH eax
  CALL LHS$sort
  ADD esp, 4
  PUSH eax
  CALL LHS$print
  ADD esp, 4
  PUSH eax
  CALL LHS$dummy
  ADD esp, 4
  PUSH eax
  CALL L_write
  ADD esp, 4
  XOR eax, eax
  Lt186:
  JMP Lt228
  Lt228:
  MOV esp, ebp
  POP ebp
  RET

