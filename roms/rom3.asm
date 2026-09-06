SECTION "entry", ROM0[$100]
    nop
    jp Start

SECTION "main", ROM0[$150]
Start:
.Display:
    ld a, $0
    ldh [$FF40], a                ; LCDC = 0 -> lcd & ppu disable

;; copy tile data to vram
    ld hl, TileData             ; hl = pattern adr
    ld de, $8000                ; de = dst (start of VRAM)
    ld bc, TileDataEnd - TileData ; bc = size(Pattern)
.CopyTiles:
    ld a, [hl+]                 ; a = mem[hl++] = pattern
    ld [de], a                  ; mem[de] = a = pattern
    inc de                      ; idx++
    dec bc                      ; size(left)--(sets no flags so we have o check)
    ld a, b
    or c                        ; a = b | c
    jr nz, .CopyTiles           ; loop if bc != 0

;; copy tile map
    ld hl, TileMap
    ld de, $9800
    ld bc, TileMapEnd - TileMap
.CopyTileMap:
    ld a, [hl+]
    ld [de], a
    inc de
    dec bc
    ld a, b
    or c
    jr nz, .CopyTileMap

;; def palette
    ld a, $E4                  
    ldh [$FF47], a                ; BGP = 11 10 01 00, normal
    ;; LCD on -> LCDC = 10010001
    ld a, $91
    ldh [$FF40], a

.MainLoop:
.WaitOut:                       ; wait until leave line 144
    ldh a, [$FF44]
    cp 144
    jr z, .WaitOut
.WaitVBlank:                    ; wait until re-enter vblank
    ldh a, [$FF44]
    cp 144
    jr nz, .WaitVBlank
    ;; in VBlank
    ldh a, [$FF43]
    inc a
    ldh [$FF43], a              ; scx++
    jr .MainLoop

TileData:
    db $3C, $7E, $42, $42, $42, $42, $42, $42, $7E, $5E, $7E, $0A, $7C, $56, $38, $7C
TileDataEnd:

TileMap:
REPT 32
    db 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0
    db 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0
ENDR
TileMapEnd:
