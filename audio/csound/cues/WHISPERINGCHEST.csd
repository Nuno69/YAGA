<CsoundSynthesizer>
<CsOptions>
-d -W -o WHISPERINGCHEST.wav
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
    iBody = p3 * 0.20
    iTail = p3 - iAtk - iBody

    ; Small natural bell: stable partials and a clean ringing decay.
    aEnvA     linseg 0, iAtk, 1, 0.030, 0.34, p3 - 0.031, 0
    aEnvB     linseg 0, iAtk, 1, 0.024, 0.26, p3 - 0.025, 0
    aEnvC     linseg 0, iAtk, 1, 0.018, 0.20, p3 - 0.019, 0
    aStrike   linseg 0, 0.0008, 1, 0.008, 0

    aBellA    oscili iAmp * 0.15, iFreq, 1
    aBellB    oscili iAmp * 0.11, iFreq * 2.00, 1
    aBellC    oscili iAmp * 0.08, iFreq * 2.52, 1
    aBellD    oscili iAmp * 0.04, iFreq * 3.00, 1

    aStrikeN  rand iAmp * 0.03
    aStrikeHi reson aStrikeN * aStrike, iFreq * 7.0, iFreq * 2.0

    aOut = (aBellA * aEnvA) + (aBellB * aEnvA * 0.92) + (aBellC * aEnvB) + (aBellD * aEnvC)
    aOut = aOut + (aStrikeHi * 0.08)
    out aOut * 1.75
endin
</CsInstruments>

<CsScore>
; One short natural bell icon.
i 1 0.00 0.20 0.52 659.25
e 0.26
</CsScore>
</CsoundSynthesizer>
