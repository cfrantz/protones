;==============================================================================
; Exported symbols
;==============================================================================

.export _cfplayer_init
.export _cfplayer_update_frame
.export _cfplayer_now_playing
.export _music_nmi_update

.export channel_delay
.export channel_seq_pos
.export channel_meas_pos
.export channel_volume

;==============================================================================
; Exports for debug
;==============================================================================
.export process_envelopes
.export process_envelope_state
.export load_envelope_ptr

;==============================================================================
; Imported symbols
;==============================================================================
.import _instruments_table

; Number of channels:
; NES:
;  0 = Pulse 0
;  1 = Pulse 1
;  2 = Triangle
;  3 = Noise 
;  4 = DMC
;  5 = MMC5 Pulse 0
;  6 = MMC5 Pulse 1
NUM_CHANNELS=7

;==============================================================================
; Opcodes in the measure "command list"
;==============================================================================
; $00: terminator (end of measure)
; $01 - $10: set volume
; $11: note off
; $12 - $1F: undefined
; $20 <byte>: set instrument to <byte>
; $21 - $77: note on
; $80 - $FF: delay (two's compliment, count up to zero).
NOTE_OFF = $11
PROGRAM_CHANGE = $20

;==============================================================================
; Channel envelope states
;==============================================================================
ENV_OFF = 0
ENV_ON = 1
ENV_RELEASE = 2
ENV_RELEASING = 3
;==============================================================================
; Envelope pointer indicies
;==============================================================================
ENV_VOLUME = 0
ENV_ARPEGGIO = 2
ENV_PITCH = 4
ENV_DUTY = 6

.ZEROPAGE
_cfplayer_now_playing:  .res 2
ptr1:                   .res 2
ptr2:                   .res 2
tmp1:                   .res 1
nr_tracks:              .res 1

.BSS
_bss_start:
channel_delay:          .res NUM_CHANNELS
channel_seq_pos:        .res NUM_CHANNELS
channel_meas_pos:       .res NUM_CHANNELS
channel_note:           .res NUM_CHANNELS
channel_volume:         .res NUM_CHANNELS
channel_instrument:     .res NUM_CHANNELS
channel_env_state:      .res NUM_CHANNELS   ; state: 0-noteoff, 1-on, 2-release
channel_env_vol:        .res NUM_CHANNELS   ; position within vol envelope
channel_env_arp:        .res NUM_CHANNELS   ; position within arp envelope
channel_env_pitch:      .res NUM_CHANNELS   ; position within pitch envelope
channel_env_duty:       .res NUM_CHANNELS   ; position within duty envelope

apu_shadow_ctrl:        .res NUM_CHANNELS
apu_shadow_sweep:       .res NUM_CHANNELS
apu_shadow_tlo:         .res NUM_CHANNELS
apu_shadow_thi:         .res NUM_CHANNELS
_bss_length = * - _bss_start
apu_thi_prev:           .res NUM_CHANNELS

.CODE
;==============================================================================
; Initialize the player
; 
;==============================================================================
.proc _cfplayer_init
    lda     #0
    sta     _cfplayer_now_playing
    sta     _cfplayer_now_playing+1
    ldx     #_bss_length
init_mem_loop:
    dex
    sta     _bss_start,x
    bne     init_mem_loop

    lda     #$FF
    ldx     #NUM_CHANNELS
init_thi_prev:
    dex
    sta     apu_thi_prev,x
    bne     init_thi_prev

    lda     #$1f                ; All APU channels enabled
    sta     $4015
    lda     #3                  ; MMC5 Pulse channels enabled
    sta     $5015
    rts

.endproc
;==============================================================================
; Update the sound engine for this frame
; 
;==============================================================================
_music_nmi_update:
.proc _cfplayer_update_frame
    jsr play_music_frame
    jsr set_apu_regs
    rts
.endproc

;==============================================================================
; Play music for this frame.
; 
;==============================================================================
.proc play_music_frame
    lda     _cfplayer_now_playing+1       ;high-byte of now_playing pointer
    beq     done                    ; if it's zero, it can't be a song
    ldy     #0
    ldx     #0
    lda     (_cfplayer_now_playing),y     ; number of tracks
    sta     nr_tracks
loop:    
    txa                             ; X is the track number.
    asl                             ; Convert track number to ptr offset
    tay                             ; (track = (track * 2) + 2
    iny                             ; 
    iny
    lda     (_cfplayer_now_playing),y     ; track ptr low
    sta     ptr1+0
    iny
    lda     (_cfplayer_now_playing),y     ; track ptr high
    sta     ptr1+1
    beq     next                    ; it it's zero, it can't be a valid track
    cpx     #4                      ; DMC channel?
    beq     next                    ; No DMC impl for now.
    jsr     play_music_channel      ; Play music for this track
next:
    inx
    cpx     nr_tracks
    bne     loop
done:
    rts
.endproc

;==============================================================================
; Play music for a particular channel.
; Channel number in X
; Destroys A, Y.
;==============================================================================
.proc play_music_channel
    inc     channel_delay,x         ; new event?
    bpl     load_sequence_pos       ; positive: process event
                                    ; negative: delay
done:
    jsr     process_envelopes
    rts
load_sequence_pos:
    ldy     #0                      ; Read seq ptr from track ptr.
    lda     (ptr1),y                ; seq ptr low
    sta     ptr2
    iny
    lda     (ptr1),y                ; seq ptr hi
    sta     ptr2+1
    lda     channel_seq_pos,x       ; where are we in the sequence?
load_sequence:
    tay
    lda     (ptr2),y                ; measure
    bpl     load_measure            ; positive values: measure number.
    and     #$7f                    ; negative value: loop to (value&0x7f).
    sta     channel_seq_pos,x       ; go to that sequence position
    jmp     load_sequence
load_measure:    
    beq     done                    ; zero terminator? yes: done.
    asl                             ; measure number to pointer offset
    tay
    lda     (ptr1),y                ; measure ptr low
    sta     ptr2
    iny
    lda     (ptr1),y                ; measure ptr hi
    sta     ptr2+1
    
load_music_event:
    ldy     channel_meas_pos,x
    inc     channel_meas_pos,x
    lda     (ptr2),y                ; get note or event.
    bpl     event_or_terminator
    sta     channel_delay,x         ; negative value: delay.
    bne     done
event_or_terminator:
    bne     process_event           ; zero terminator?
    inc     channel_seq_pos,x       ; yes: next sequence position
    lda     #0
    sta     channel_meas_pos,x      ; reset measure position to 0
    beq     load_sequence_pos
process_event:
    cmp     #$21                    ; is this a note event?
    bcs     note_event
    cmp     #PROGRAM_CHANGE         ; is this a program change event?
    bne     next1
    iny                             ; program to change to is next byte
    lda     (ptr2),y                ; get program number
    sta     channel_instrument,x
    inc     channel_meas_pos,x      ; increment measure pos (should never wrap)
    bne     load_music_event        ; get next music event
next1:
    cmp     #NOTE_OFF               ; NoteOff and ...
    bcs     note_off_event          ; ... all undefined events are NoteOff
    and     #$0f                    ; events $01-$10 are volume events
    sta     channel_volume,x
    bcc     load_music_event        ; carry should still be clear from prior cmp
note_off_event:
    lda     #ENV_RELEASE            ; Set the state to "release"
    sta     channel_env_state,x
    bne     load_music_event        ; get next music event
note_event:
    sta     channel_note,x          ; Save the note
    lda     #2                      ; Reset envelopes
    sta     channel_env_vol,x
    sta     channel_env_arp,x
    sta     channel_env_pitch,x
    sta     channel_env_duty,x
    lda     #ENV_ON                 ; Set the state to NoteOn
    sta     channel_env_state,x
    bne     load_music_event        ; get next music event
.endproc
    
;==============================================================================
; Play a musical note.
; Note in A
; Channel in X
; Destroys A, Y.
; TODO: need to handle noise and DMC channels.
;==============================================================================
.proc play_note
    tay
timer_low:
    lda     note_table_lsb,y        ; low byte of timer
    sta     apu_shadow_tlo,x
timer_high:
    lda     apu_shadow_thi,x        ; check if we should rewrite timer_hi
    and     #$F8                    ; clear length-counter bits
    cmp     note_table_msb,y        ; same as high byte of timer?
    beq     ctrl_reg                ; yes: skip timer_hi
    lda     note_table_msb,y        ; high byte of timer
    sta     apu_shadow_thi,x
ctrl_reg:    
    lda     channel_volume,x
    ora     #$30
    sta     apu_shadow_ctrl,x
sweep_reg:
    lda     #0
    sta     apu_shadow_sweep,x
    rts
.endproc    

;==============================================================================
; Process an instrument envelope
; Envelope number in A (0-3)
; Channel in X
;==============================================================================
.proc load_envelope_ptr
    ora     channel_instrument,x
    tay
    lda     _instruments_table,y     ; envelope ptr low
    sta     ptr2+0
    iny
    lda     _instruments_table,y     ; envelope ptr high
    beq     done                    ; it it's zero, it can't be a valid envelope
    sta     ptr2+1
done:
    rts
.endproc
    
;==============================================================================
; Advance envelope state
; Envelope pos in A
; Channel in X
; Returns new env pos in A, or zero if end of envelope.
;
; Note: envelope state: 0-off, 1-on, 2-rel, 3-releasing
; Envelope data layout: <len>, <loopidx>, <releaseidx>, <data...>
;==============================================================================
.proc process_envelope_state
    cmp     #2                      ; First valid env index is 3.
                                    ; We permit 2 because we start at index "-1"
    bcs     check_state
    lda     #0
    rts
check_state:    
    ldy     channel_env_state,x     ; state: 0-off, 1-on, 2-rel, 3-releasing
    beq     done                    ; state off, envelope done
check_on:
    cpy     #ENV_ON
    bne     check_release
    iny
    cmp     (ptr2),y                ; at release point?
    beq     loop_point              ; yes, go to loop point
    ldy     #0
    clc
    adc     #1                      ; increment envelope position
    cmp     (ptr2),y                ; at or beyond end point?
    bcs     loop_point              ; yes, go to loop point
    rts
loop_point:
    ldy     #1
    lda     (ptr2),y                ; load the loop point
    rts
check_release:
    cpy     #ENV_RELEASE
    bne     do_releasing
    lda     (ptr2),y                ; load release point
    rts
do_releasing:
    ldy     #0
    cmp     (ptr2),y                ; at or beyond envelope end?
    bcs     done                    ; yes, envelope done
    clc
    adc     #1                      ; increment envelope position
    rts
done:
    tya
    rts
.endproc
    
;==============================================================================
; Process envelopes and store music data to apu_shadow registers
; Channel in X
;==============================================================================
.proc process_envelopes
volume:
    lda     #ENV_VOLUME
    jsr     load_envelope_ptr       ; load the pointer
    bne     volume_envelope         ; not null, process
no_volume_env:
    ldy     channel_env_state,x     ; Is the state is "on"?
    cpy     #ENV_ON
    bne     volume_value            ; No, env volume value is zero
    lda     #$F0                    ; Yes, env volume value is max
    bne     volume_value
volume_envelope:
    lda     channel_env_vol,x
    jsr     process_envelope_state
    sta     channel_env_vol,x
    tay
    beq     volume_value            ; Invalid index -> volume 0.
    lda     (ptr2),y                ; get volume value
    asl                             ; todo: premultiply vol env in converter
    asl
    asl
    asl
volume_value:
    ora     channel_volume,x
    tay
    lda     volume_table,y          ; multiply env vol by chan vol
    cpx     #2                      ; triangle channel
    bne     volume_done
    tay                             ; save volume value
    lda     #$80                    ; triangle zero volume value
    cpy     #0
    beq     volume_done
    lda     #$FF                    ; triangle "on" volume value
volume_done:
    sta     apu_shadow_ctrl,x

duty:
    cpx     #2                      ; no duty for triangle
    beq     arpeggio
    cpx     #3                      ; no duty for noise
    beq     arpeggio
    lda     #ENV_DUTY
    jsr     load_envelope_ptr       ; load the pointer
    beq     duty_value              ; if null, no processing
duty_envelope:
    lda     channel_env_duty,x
    jsr     process_envelope_state
    sta     channel_env_duty,x
    beq     duty_value              ; Invalid index -> duty setting 0
duty_index:
    tay
    lda     (ptr2),y                ; get duty envelope value
duty_value:
    tay
    lda     duty_table,y            ; transform to apu regs pattern
    ora     apu_shadow_ctrl,x
    sta     apu_shadow_ctrl,x

arpeggio:
    lda     #ENV_ARPEGGIO
    jsr     load_envelope_ptr       ; load the pointer
    beq     arpeggio_value          ; if null, no processing
arpeggio_envelope:
    lda     channel_env_arp,x
    jsr     process_envelope_state
    sta     channel_env_arp,x
    beq     arpeggio_value          ; invalid arp index?
arpeggio_index:
    tay
    lda     (ptr2),y                ; get arp envelope value
arpeggio_value:
    clc
    adc     channel_note,x          ; arp value + note_value
    tay
    lda     note_table_lsb,y
    sta     apu_shadow_tlo,x
    lda     note_table_msb,y
    sta     apu_shadow_thi,x

pitch:
    lda     #ENV_PITCH
    jsr     load_envelope_ptr       ; load the pointer
    beq     pitch_done              ; if null, no processing
pitch_envelope:
    lda     channel_env_pitch,x
    jsr     process_envelope_state
    sta     channel_env_pitch,x
    beq     pitch_done              ; invalid pitch index?
pitch_index:
    tay
    lda     (ptr2),y                ; get pitch envelope value
pitch_value:
    clc
    adc     apu_shadow_tlo,x
    sta     apu_shadow_tlo,x
    lda     #0
    adc     apu_shadow_thi,x
    sta     apu_shadow_thi,x
pitch_done:

    lda     channel_env_state,x     ; Is the state is release?
    cmp     #ENV_RELEASE
    bne     done
    inc     channel_env_state,x     ; yes, release->releasing
done:
    rts
.endproc

;==============================================================================
; Set APU regs to the shadow values
; Destroys A, X, Y.
;==============================================================================
.proc set_apu_regs
    ldx     #0
    ldy     #0
apu_regs:
    lda     apu_shadow_ctrl,x
    sta     $4000,y
    lda     apu_shadow_sweep,x
    sta     $4001,y
    lda     apu_shadow_tlo,x
    sta     $4002,y
    lda     apu_shadow_thi,x
    cmp     apu_thi_prev,x
    beq     apu_skip_thi
    sta     $4003,y
    sta     apu_thi_prev,x
apu_skip_thi:
    iny
    iny
    iny
    iny
    inx
    cpx     #5
    bcc     apu_regs
    ldy     #0
mmc5_regs:
    lda     apu_shadow_ctrl,x
    sta     $5000,y
    lda     apu_shadow_sweep,x
    sta     $5001,y
    lda     apu_shadow_tlo,x
    sta     $5002,y
    lda     apu_shadow_thi,x
    cmp     apu_thi_prev,x
    beq     mmc5_skip_thi
    sta     $5003,y
    sta     apu_thi_prev,x
mmc5_skip_thi:
    iny
    iny
    iny
    iny
    inx
    cpx     #7
    bcc     mmc5_regs
    rts
.endproc
    
    

; Note tables
; The first 33 bytes of the note tables are unused.
; Note 33 (0x21) is MIDI note A0.
note_table_lsb:
        .byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00 ; Not Used
        .byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00 ; Not Used
        .byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $f1, $7f, $13 ; Octave 0
        .byte $ad, $4d, $f3, $9d, $4c, $00, $b8, $74, $34, $f8, $bf, $89 ; Octave 1
        .byte $56, $26, $f9, $ce, $a6, $80, $5c, $3a, $1a, $fb, $df, $c4 ; Octave 2
        .byte $ab, $93, $7c, $67, $52, $3f, $2d, $1c, $0c, $fd, $ef, $e1 ; Octave 3
        .byte $d5, $c9, $bd, $b3, $a9, $9f, $96, $8e, $86, $7e, $77, $70 ; Octave 4
        .byte $6a, $64, $5e, $59, $54, $4f, $4b, $46, $42, $3f, $3b, $38 ; Octave 5
        .byte $34, $31, $2f, $2c, $29, $27, $25, $23, $21, $1f, $1d, $1b ; Octave 6
        .byte $1a, $18, $17, $15, $14, $13, $12, $11, $10, $0f, $0e, $0d ; Octave 7

note_table_msb:
        .byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00 ; Not Used
        .byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00 ; Not Used
        .byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $07, $07, $07 ; Octave 0
        .byte $06, $06, $05, $05, $05, $05, $04, $04, $04, $03, $03, $03 ; Octave 1
        .byte $03, $03, $02, $02, $02, $02, $02, $02, $02, $01, $01, $01 ; Octave 2
        .byte $01, $01, $01, $01, $01, $01, $01, $01, $01, $00, $00, $00 ; Octave 3
        .byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00 ; Octave 4
        .byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00 ; Octave 5
        .byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00 ; Octave 6
        .byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00 ; Octave 7

; Duty lookup table.
duty_table:
    .byte $30
    .byte $70
    .byte $b0
    .byte $f0

; Precomputed volume multiplication table (rounded but never to zero unless one of the value is zero).
; Load the 2 volumes in the lo/hi nibble and fetch.

volume_table:
    .byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
    .byte $00, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01
    .byte $00, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $01, $02, $02, $02, $02
    .byte $00, $01, $01, $01, $01, $01, $01, $01, $02, $02, $02, $02, $02, $03, $03, $03
    .byte $00, $01, $01, $01, $01, $01, $02, $02, $02, $02, $03, $03, $03, $03, $04, $04
    .byte $00, $01, $01, $01, $01, $02, $02, $02, $03, $03, $03, $04, $04, $04, $05, $05
    .byte $00, $01, $01, $01, $02, $02, $02, $03, $03, $04, $04, $04, $05, $05, $06, $06
    .byte $00, $01, $01, $01, $02, $02, $03, $03, $04, $04, $05, $05, $06, $06, $07, $07
    .byte $00, $01, $01, $02, $02, $03, $03, $04, $04, $05, $05, $06, $06, $07, $07, $08
    .byte $00, $01, $01, $02, $02, $03, $04, $04, $05, $05, $06, $07, $07, $08, $08, $09
    .byte $00, $01, $01, $02, $03, $03, $04, $05, $05, $06, $07, $07, $08, $09, $09, $0a
    .byte $00, $01, $01, $02, $03, $04, $04, $05, $06, $07, $07, $08, $09, $0a, $0a, $0b
    .byte $00, $01, $02, $02, $03, $04, $05, $06, $06, $07, $08, $09, $0a, $0a, $0b, $0c
    .byte $00, $01, $02, $03, $03, $04, $05, $06, $07, $08, $09, $0a, $0a, $0b, $0c, $0d
    .byte $00, $01, $02, $03, $04, $05, $06, $07, $07, $08, $09, $0a, $0b, $0c, $0d, $0e
    .byte $00, $01, $02, $03, $04, $05, $06, $07, $08, $09, $0a, $0b, $0c, $0d, $0e, $0f
