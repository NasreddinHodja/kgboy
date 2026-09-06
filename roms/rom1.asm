SECTION "entry", ROM0[$100]
    nop
    jp Start

SECTION "main", ROM0[$150]
Start:
    ld hl, Pattern              ; hl = pattern adr
    ld de, $C000                ; de = dest (start of WRAM)
    ld bc, PatternEnd - Pattern ; bc = size(Pattern)
.copy:
    ld a, [hl+]                 ; a = mem[hl++] = pattern
    ld [de], a                  ; mem[de] = a = pattern
    inc de                      ; idx++
    dec bc                      ; size(left)--(sets no flags so we have o check)
    ld a, b
    or c                        ; a = b | c
    jr nz, .copy                ; loop if bc != 0
.done:
    jr .done

Pattern:    
    db $DE, $AD, $BE, $EF, $CA, $FE, $BA, $BE
PatternEnd:
