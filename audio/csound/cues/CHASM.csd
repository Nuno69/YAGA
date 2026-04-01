<CsoundSynthesizer>
<CsOptions>
-d -W -o CHASM.wav
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
    iAtk  = 0.0015
    iMid  = p3 * 0.42
    iTail = p3 - iAtk - iMid

    ; Very short alert beep shaped to fit the note duration.
    aEnv   linseg 0, iAtk, 1, iMid, 0.55, iTail, 0
    aTone  oscili iAmp, iFreq, 1
    aUpper oscili iAmp * 0.22, iFreq * 2, 1
    aSub   oscili iAmp * 0.12, iFreq * 0.5, 1

    aOut = (aTone + aUpper + aSub) * aEnv
    out aOut
endin
</CsInstruments>

<CsScore>
; Tempo-based score: rapid trill between F#5 and G5 at 120 BPM.
t 0 120
i 1 0.00 0.0625 0.48 739.99
i 1 0.0625 0.0625 0.48 783.99
i 1 0.1250 0.0625 0.48 739.99
i 1 0.1875 0.0625 0.48 783.99
i 1 0.2500 0.0625 0.48 739.99
i 1 0.3125 0.0625 0.48 783.99
e 0.50
</CsScore>
</CsoundSynthesizer>
