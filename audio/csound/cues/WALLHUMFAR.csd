<CsoundSynthesizer>
<CsOptions>
-d -W -o WALLHUMFAR.wav
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 32
nchnls = 1
0dbfs  = 1

instr 1
    iAmp = p4
    iCf  = p5
    iBw  = p6

    ; Softer, lower click than WALLHUMNEAR so the two cues stay distinguishable.
    aSnapExc rand iAmp
    aBodyExc rand iAmp * 0.70

    aSnapEnv linseg 0, 0.0003, 1, 0.006, 0
    aBodyEnv linseg 0, 0.0010, 1, 0.024, 0.34, 0.026, 0

    aSnap = aSnapExc * aSnapEnv
    aBody = aBodyExc * aBodyEnv

    aClick reson aSnap, iCf, iBw
    aAir   reson aSnap, iCf * 1.55, iBw * 0.75
    aKnock reson aBody, iCf * 0.48, iBw * 2.10

    aOut = ((aClick * 0.52) + (aAir * 0.14) + (aKnock * 0.90)) * 0.18
    out aOut
endin
</CsInstruments>

<CsScore>
; Single lower wall click with a softer, boxier tail.
i 1 0.00 0.06 0.74 2550 1150
e 0.14
</CsScore>
</CsoundSynthesizer>
