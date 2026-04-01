<CsoundSynthesizer>
<CsOptions>
-d -W -o WALLHUMNEAR.wav
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

    ; Split the transient into a bright snap and a slightly slower body layer.
    aSnapExc rand iAmp
    aBodyExc rand iAmp * 0.60

    aSnapEnv linseg 0, 0.0002, 1, 0.005, 0
    aBodyEnv linseg 0, 0.0008, 1, 0.018, 0.30, 0.020, 0

    aSnap = aSnapExc * aSnapEnv
    aBody = aBodyExc * aBodyEnv

    ; The bright layer gives directional definition.
    aClick reson aSnap, iCf, iBw
    aAir   reson aSnap, iCf * 1.9, iBw * 0.65

    ; The lower band adds weight so the click feels fuller.
    aKnock reson aBody, iCf * 0.55, iBw * 1.8

    aOut = ((aClick * 0.75) + (aAir * 0.22) + (aKnock * 0.70)) * 0.19
    out aOut
endin
</CsInstruments>

<CsScore>
; Single fuller click with a short tail.
i 1 0.00 0.05 0.78 3400 950
e 0.12
</CsScore>
</CsoundSynthesizer>
