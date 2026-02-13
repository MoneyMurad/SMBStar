.text
.align 4

.extern returnFromBitOverride
.extern powSongSwitch
.extern StopSoundRelated
.extern StartStarBGM
.extern getsoundblahblah
.extern FUN_8019F1D0

.global overrideBit
overrideBit:
    cmpwi r0, 0x8
    beq playMegaSong

    cmpwi r0, 0x10
    beq playMegaSong

    b returnFromBitOverride

callPowSongSwitch:
    lis r12, powSongSwitch@h
    ori r12, r12, powSongSwitch@l
    mtctr r12
    bctr

playMegaSong:
    lwz r3, -0x5218(r13)
    li r4, 0xf
    bl StopSoundRelated

    lwz r3, -0x5218(r13)

    li r4, 0x64

    bl StartStarBGM

    li r5, 0x74a
    li r3, 0x76
    li r0, 0x0

    li r4, 0x64

    stw r5, 0x58(r31)
    stw r5, 0x38(r31)

    stw r3, 0x3c(r31)
    stw r0, 0x28(r31)

    lwz r3, -0x5218(r13)
    lwz r3, 0x100(r3)

    bl getsoundblahblah

    rlwinm r3, r3, 0x0, 0x16, 0x1f
    
    bl FUN_8019F1D0
    stw r3, 0x2c(r31)

    lis r12, returnFromBitOverride@h
    ori r12, r12, returnFromBitOverride@l
    mtctr r12
    bctr

.data
.align 4
MegaStream:
    .string "BGM_MEGA"

.align 4
MegaStreamPtr:
    .long MegaStream