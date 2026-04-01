<CsoundSynthesizer>
<CsOptions>
-d -W -o WHISPERINGNPC.wav
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
    iAtk  = 0.002
    iBody = p3 * 0.18
    iSoft = p3 * 0.28
    iTail = p3 - iAtk - iBody - iSoft

    ; Soft, short musical ping that should transpose cleanly for health cues.
    aEnv    linseg 0, iAtk, 1, iBody, 0.46, iSoft, 0.20, iTail, 0
    kPitch  linseg iFreq * 1.02, 0.010, iFreq, p3 - 0.010, iFreq * 0.998
    aFund   oscili iAmp, kPitch, 1
    aDetune oscili iAmp * 0.20, kPitch * 0.997, 1
    aOct    oscili iAmp * 0.20, kPitch * 2, 1
    aFifth  oscili iAmp * 0.10, kPitch * 3, 1
    aWarm   oscili iAmp * 0.18, kPitch * 0.5, 1

    ; A short filtered transient makes the note speak quickly without becoming sharp.
    aAirExc rand iAmp * 0.12
    aAirEnv linseg 0, 0.001, 1, 0.013, 0
    aAir    reson aAirExc * aAirEnv, iFreq * 2.2, iFreq * 0.95

    aOut = ((aFund * 0.78) + (aDetune * 0.35) + aOct + aFifth + (aWarm * 0.75) + (aAir * 0.30)) * aEnv * 0.70
    out aOut
endin
</CsInstruments>

<CsScore>
; One short soft ping on A4. Distance-based repetition and health mapping will be handled in code later.
i 1 0.00 0.16 0.42 440.00
e 0.22
</CsScore>
</CsoundSynthesizer>
