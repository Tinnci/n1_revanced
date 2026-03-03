; Minimal kexec test payload
; Runs in 32-bit protected mode with paging disabled (what relocate_kernel gives us)
; Writes magic string to pstore RAM (phys 0x0fde7000) to prove execution
; Then enters infinite HLT loop
;
; Build: nasm -f elf32 test_kexec_payload.asm -o test.o
;        ld -m elf_i386 -Ttext 0x100000 --entry=_start -o test_kexec test.o
;
BITS 32
section .text
global _start

_start:
    ; Disable interrupts
    cli
    
    ; Write "KEXEC_OK" magic to pstore RAM at physical 0x0fde7000
    ; pstore region: intel_mid_pstore_ram reserved at 0xfde7000, size 0x219000
    ; We write at the START of the region (overwriting old console-ramoops)
    mov edi, 0x0fde7000
    mov dword [edi],     0x45584B    ; "KXE" + null (little-endian: 'K','E','X','\0')
    ; Actually write clear ASCII
    mov byte [edi+0], 'K'
    mov byte [edi+1], 'E'
    mov byte [edi+2], 'X'
    mov byte [edi+3], 'E'
    mov byte [edi+4], 'C'
    mov byte [edi+5], '_'
    mov byte [edi+6], 'O'
    mov byte [edi+7], 'K'
    mov byte [edi+8], '!'
    mov byte [edi+9], 0x0A
    
    ; Also write a full message
    mov esi, msg
    mov edi, 0x0fde7010
    mov ecx, msg_end - msg
.copy:
    lodsb
    stosb
    loop .copy
    
    ; Try to feed SCU watchdog via IPC 
    ; (probably won't work without proper IPC setup, but try)
    
    ; Enter infinite halt loop
.halt_loop:
    hlt
    jmp .halt_loop

msg:
    db "KEXEC PAYLOAD EXECUTED SUCCESSFULLY", 0x0A
    db "If you see this in pstore, the kexec jump works!", 0x0A
    db "The problem is in the Linux kernel early startup.", 0x0A, 0
msg_end:
