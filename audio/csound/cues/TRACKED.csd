<CsoundSynthesizer>
<CsOptions>
-d -W -o TRACKED.wav
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
    iAtk  = 0.0008
    iBody = p3 * 0.18
    iTail = p3 - iAtk - iBody

    ; Short tracked-target ping: clean E4 core with a bright attack.
    aEnv     linseg 0, iAtk, 1, iBody, 0.78, iTail, 0
    kPitch   linseg iFreq * 1.045, 0.009, iFreq, p3 - 0.009, iFreq * 0.992
    aCore    oscili iAmp, kPitch, 1
    aUpper   oscili iAmp * 0.24, kPitch * 2, 1
    aAirExc  rand iAmp * 0.09
    aAirEnv  linseg 0, 0.0004, 1, 0.006, 0
    aAir     reson aAirExc * aAirEnv, 3600, 1100

    aOut = ((aCore * 0.88) + (aUpper * 0.22) + (aAir * 0.18)) * aEnv * 0.58
    out aOut
endin
</CsInstruments>

<CsScore>
; One short sharp tracked-object ping centered on E4.
i 1 0.00 0.11 0.82 329.63
e 0.18
</CsScore>
</CsoundSynthesizer>
