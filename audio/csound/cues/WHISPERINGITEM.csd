<CsoundSynthesizer>
<CsOptions>
-d -W -o WHISPERINGITEM.wav
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 32
nchnls = 1
0dbfs  = 1

giSine ftgen 1, 0, 4096, 10, 1

instr 1
    iAmp  = p4
    iFreq = p5
    iAtk  = 0.001
    iBody = p3 * 0.30
    iTail = p3 - iAtk - iBody

    ; Thicker hollow-box knock: more low-mid body, less click-like edge.
    aEnv      linseg 0, iAtk, 1, iBody, 0.52, iTail, 0
    kPitch    linseg iFreq * 1.10, 0.014, iFreq, p3 - 0.014, iFreq * 0.92
    aCore     oscili iAmp, kPitch, 1
    aCoreDet  oscili iAmp * 0.18, kPitch * 0.992, 1
    aBoxLow   oscili iAmp * 0.42, kPitch * 0.68, 1
    aBoxMid   oscili iAmp * 0.28, kPitch * 1.34, 1
    aBoxHigh  oscili iAmp * 0.03, kPitch * 2.05, 1

    aKnockExc rand iAmp * 0.12
    aKnockEnv linseg 0, 0.0009, 1, 0.012, 0
    aCavityLo reson aKnockExc * aKnockEnv, 520, 320
    aCavityMd reson aKnockExc * aKnockEnv, 930, 440
    aCavityHi reson aKnockExc * aKnockEnv, 1620, 700

    aOut = ((aCore * 0.66) + (aCoreDet * 0.28) + (aBoxLow * 0.92) + (aBoxMid * 0.66) + (aBoxHigh * 0.04) +
           (aCavityLo * 0.92) + (aCavityMd * 0.56) + (aCavityHi * 0.06)) * aEnv * 0.42
    out aOut
endin
</CsInstruments>

<CsScore>
; One short thick hollow-box item knock centered around a lower G.
i 1 0.00 0.16 0.55 196.00
e 0.24
</CsScore>
</CsoundSynthesizer>
