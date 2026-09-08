SECTION "entry", ROM0[$100]
    nop
    jp Start

SECTION "main", ROM0[$150]
Start:
    ld hl, Pattern
    ld a, h
    ldh [$FF46], a
.done:    
    jp .done


SECTION "data", ROM0[$200]
Pattern:    
    db $DE, $AD, $BE, $EF, $CA, $FE, $BA, $BE
PatternEnd:
