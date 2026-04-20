; ==========================================
; LittleC -> 8086 Assembly  (MASM format)
; ==========================================
.MODEL SMALL
.STACK 256

.DATA
    a DW 0        ; int
    b DW 0        ; int
    c DW 0        ; int

.CODE
MAIN PROC
    MOV AX, @DATA
    MOV DS, AX

    ; a = ...
    MOV AX, 1
    MOV a, AX
    ; b = ...
    MOV AX, 2
    MOV b, AX
    ; c = ...
    MOV AX, a
    PUSH AX
    MOV AX, b
    POP BX
    XCHG AX, BX
    ADD AX, BX
    PUSH AX
    MOV AX, b
    POP BX
    XCHG AX, BX
    ADD AX, BX
    MOV c, AX

    ; exit program
    MOV AH, 4CH
    INT 21H
MAIN ENDP

END MAIN
