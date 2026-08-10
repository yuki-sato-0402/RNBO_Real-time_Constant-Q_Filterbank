# RNBO_Real-time_Constant-Q_Filterbank
**A real-time polyphonic pitch (F0) detector plugin based on a Constant-Q filter bank (VST3 / AU / Standalone)**

This system is implemented in `gen` (RNBO) and uses a **Constant-Q filter bank** to compute, in real time, the amplitude of each of the 88 piano keys (A0–C8) from the input signal, and to detect the notes with the largest amplitude as fundamental-frequency (F0) candidates. Unlike simple zero-crossing or FFT-based methods, each key has its own dedicated correlation (Goertzel-like) filter, which gives a musically natural frequency resolution matched to the semitone scale. A dedicated overtone-masking algorithm also suppresses harmonics of a detected fundamental (octaves, fifths, etc.) so they aren't mistaken for separate notes, improving accuracy for polyphonic (chord) input.

## Key Features
- **Constant-Q Filter Bank**: Each of the 88 keys (A0–C8) has its own correlation filter, giving real-time amplitude estimation with a frequency resolution matched to the equal-tempered semitone spacing.
- **Polyphonic Pitch Detection**: Continuously tracks the top 6 amplitude candidates across all keys, keeping multiple fundamental-frequency candidates available for chords rather than just a single peak.
- **Overtone Masking Algorithm**: Detects and masks harmonics of an identified fundamental (one octave, one octave + perfect 5th, two octaves, two octaves + major 3rd) so a single note's own overtones aren't reported as separate detected notes, improving polyphonic detection accuracy.
- **50% Overlapping Hanning Windows**: Each key's analysis window runs as two interleaved 50%-overlapped systems (A/B) that alternate computation and output, effectively improving time resolution (detection latency).
- **Noise Floor Gate**: Amplitudes below a configurable noise floor are zeroed out, suppressing false detections from low-level noise.
- **JUCE Integration**: Uses `AudioProcessorValueTreeState` for robust parameter management and DAW automation.

Built from the [rnbo.example.juce](https://github.com/Cycling74/rnbo.example.juce) template.   
The C++ source exported from RNBO is in `export/`, and the original Max patch is in `patches/`.

## Demonstration
[Youtube<img width="848" height="574" alt="Screenshot 2026-08-07 at 23 32 13" src="https://github.com/user-attachments/assets/50fd372b-4811-49fc-be41-fe5e8bf1d8b8" />](https://youtu.be/WEppLL6FAo8)

## Algorithm Details
> Please refer to the `gen~` code alongside this section.

### 1. Pitch Analysis via a Constant-Q Filter Bank
At the core of this system is a **Constant-Q filter bank**, with one filter centered on each of the 88 equal-tempered key frequencies `freq = mtof(k + 21)`. 

A key motivation for this design is to solve a well-known limitation of Max's `fft~`: because `fft~` uses a single, fixed window length across the entire spectrum, low frequencies end up with poor frequency resolution (the bin spacing is too coarse to cleanly separate low notes) unless the window is made very long, which then hurts time resolution everywhere. Here, each key has its own window length, sized specifically for its own frequency — so low notes automatically get the long window they need for good frequency resolution, and high notes automatically get a short window for fast response, without one fixed window length forcing a compromise across the whole range.

The analysis window length (in samples) for each key is given by:

$$
Q = \frac{1}{2^{1/12} - 1} \cdot Q_{\text{factor}}
$$

$$
\text{winLength} = Q \cdot \frac{f_s}{f_{\text{key}}}
$$

where $f_s$ is the sample rate and $f_{\text{key}} = \mathrm{mtof}(k + 21)$ is the center frequency of key $k$.

$\dfrac{1}{2^{1/12} - 1}$ is a baseline Q value derived from the frequency ratio between adjacent semitones in equal temperament, $2^{1/12}$. It scales each key's filter bandwidth to roughly match the spacing to its neighboring semitone. The $Q_{\text{factor}}$ parameter is a multiplier on top of this baseline, letting the user tune how sharply selective (narrow-band) the detection is.

- **Frequency resolution vs. time resolution trade-off**: since $\text{winLength}$ is proportional to $Q$ and inversely proportional to frequency,
  - Increasing `Q_factor` → longer analysis windows → better frequency resolution (cleaner separation between adjacent semitones), but higher latency per detection, i.e. worse time resolution.
  - Decreasing `Q_factor` → shorter windows → faster response (better time resolution), but wider filter bandwidth, which increases bleed between neighboring notes.

  This is the classic trade-off inherent to any constant-Q filter bank, and `Q_factor` exposes it as a single tunable parameter so it can be balanced for the performance context.

  Put simply: each key's window is set to span **$Q$ periods of the frequency it's trying to detect**. Since one period is $f_s / f_{\text{key}}$ samples, a window of $Q$ such periods is exactly $\text{winLength} = Q \cdot (f_s / f_{\text{key}})$ samples — the formula above.


### 2. Per-Key Correlation Accumulation
Rather than transforming the whole spectrum at once as FFT does, each key `k` incrementally accumulates its correlation (dot product) with its own center-frequency cosine and sine waves, sample by sample.

$$
S_{\cos}[k] \mathrel{+}= x[n] \cdot w(n) \cdot \cos(\phi_k[n])
$$

$$
S_{\sin}[k] \mathrel{+}= x[n] \cdot w(n) \cdot \sin(\phi_k[n])
$$

where $x[n]$ is the input sample, $w(n)$ is the Hanning window value at the current sample count, and $\phi_k[n]$ is the running phase for key $k$'s center frequency.

This accumulation continues every sample until that key's own $\text{winLength}$ (its analysis window length) is reached. At that point the accumulated sums are converted to amplitude/phase via `cartopol`,

$$
A[k] = \frac{4}{\text{winLength}} \sqrt{S_{\cos}[k]^2 + S_{\sin}[k]^2}
$$

and written into `outputBuff`.

Crucially, **because lower notes have longer periods (and therefore longer `winLength`), their amplitude updates less frequently, while higher notes — with shorter windows — update much more often.** This is an inherent property of the constant-Q approach: it naturally allocates more frequency resolution (semitone separation) to low notes and more time resolution (faster response) to high notes, matching how pitch perception scales across the musical range.

### 3. Improving Time Resolution with 50% Overlapping Windows
Each key runs its correlation accumulation through two parallel systems, A and B. System B starts its phase and counter offset by half a window (`winLength / 2`) relative to A, and both apply a Hanning window (`0.5 - 0.5*cos(...)`) to reduce spectral leakage (sidelobes).

Each system, A and B, independently writes to `outputBuff` and updates the top-6 search (see below) whenever its own window completes. This effectively **doubles the output update rate per key** compared to a single-window design, roughly halving detection latency without sacrificing frequency resolution.

### 4. Top-6 Amplitude Tracking (`updateMax` / `commitMax`)
Across all 88 keys, the system continuously maintains a sorted list of the 6 largest-amplitude candidates.

- Whenever a key's window completes, `updateMax` is called with its new amplitude, and the 6-slot working list (`workingData`) is updated accordingly.
- If the same key index is already present anywhere in the list, its rank isn't shifted — the value is simply updated in place. (This is the fix for the original bug, where the same key could end up occupying both the 1st- and 2nd-place slots.)
- If a new value surpasses an existing rank, all lower ranks are cascaded down by one slot, and whatever falls off the 6th slot is discarded.
- When `k == 0` (i.e. one full pass over all 88 keys has completed), `commitMax` is called: it snapshots `workingData` into `currentData` and resets `workingData` to zero for the next cycle. This commit happens on both the A and B window completions, so the published top-6 list is also refreshed at roughly twice the base rate.

This gives the system access to more than just the single loudest note — a set of top candidates that can represent a chord — which the overtone-masking stage (below) then refines further, and which leaves headroom for extending polyphony detection in the future.

## Parameter Explanation
- **Q_factor** : Controls the selectivity (bandwidth) of the constant-Q filter bank. Higher values improve frequency resolution but reduce responsiveness (time resolution); lower values respond faster but blur separation between neighboring notes.

- **noiseFloor** : Amplitudes below this value (%) are treated as zero, filtering out false detections caused by low-level noise or spectral leakage.

- **overtoneThreshold / polyTolerance (Overtone Masking Algorithm)** : These two parameters drive the mechanism that improves polyphonic detection accuracy. For each of the top detected fundamental candidates, the `overtoneMask` function checks the amplitude at four harmonic offsets — one octave, one octave + perfect 5th, two octaves, and two octaves + major 3rd.
  - **overtoneThreshold** : The minimum amplitude a harmonic bin must reach, as a percentage of the fundamental's amplitude, to be considered a candidate for masking.
  - **polyTolerance** : A tolerance (%) that protects against masking a harmonic-position bin that is actually an independent, real note of near-equal amplitude — masking is only applied when the harmonic bin's amplitude differs from the fundamental's by more than this tolerance.

  A harmonic bin at offset $o$ (e.g. $o = 12$ for one octave) is masked when both conditions below hold:

$$
A_{\text{fund}} \cdot \left(\frac{\text{overtoneThreshold}}{100}\right) < A_{\text{fund}+o}
$$

$$
\frac{\left| A_{\text{fund}} - A_{\text{fund}+o} \right|}{A_{\text{fund}+o}} > \frac{\text{polyTolerance}}{100}
$$

  Together, these parameters let the system distinguish between "a note's own overtone" and "a coincidentally harmonic but genuinely separate note," reducing false detections caused by harmonics during chord playing while avoiding accidentally erasing real notes that happen to sit at a harmonic interval.

## Challenges
- **CPU load in Max**: When running the patch inside Max, CPU usage can reach around 50% at peak. Lighter-weight processing needs to be explored to make this more practical for real-time / performance use.
- **Fundamental vs. 2nd harmonic ambiguity**: For some input signals, the amplitude of the fundamental and its 2nd harmonic (one octave up) is nearly identical — in some cases the 2nd harmonic is even louder than the fundamental. In these cases, even with the overtone masking algorithm, it's difficult to reliably determine which bin is the true fundamental and which is a harmonic.


## Reference
- [定Q変換のリアルタイム向けPython実装の解説（音高・コード・メロディの分析向け）](https://www.wizard-notes.com/entry/music-analysis/cqt-implementation)
- [rnbo.example.juce](https://github.com/Cycling74/rnbo.example.juce)
