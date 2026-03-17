# **Apple π: Alt. Firmware for Ciat-Lonbarde Cafeteria / Cafe Quantum**

An alternative firmware for the ESP32-based Ciat-Lonbarde Cafeteria / Cafe Quantum. Make sure to refer to the official [Ciat Lonbarde documentation](https://github.com/pblasser/esp_cafe/tree/main) for details on how to upload firmware to Cafe and add one extra step to remove "-main" from the downloaded folder name. This firmware expands the Cafe into a multi-fx all-you-can-eat buffet. It has loopers/delays, reverbs, resonators, samplers, synth voices, generative music, live processing and drum machines. Tying it all together is a preset selection system. Preset changes carry over the buffer into a new preset, so you can, for example, build a loop in coco, scramble it up in the scrambler and sync it to a clock in the sampler.

## **🍎 Preset Selection Mode**

This firmware contains 24 presets and a new preset selection system to navigate through selected presets in a preset playlist.

**How to change presets:**

1. **Enter Menu Mode:** Press and hold the hardware Button for \~1 second. The Lamp will begin to strobe rapidly to indicate it is ready.  
2. **Release the Button:** The Lamp will strobe a unique rhythmic flashing pattern. The control of the currently playing preset is paused while the audio plays in the background.  
3. **Punch-in the Preset:** Short-press the button the number of times corresponding to your desired preset index. *Important Note: count from zero\!*  
   * *Example: 0 presses \= Preset 0 (the first in your playlist). This is your home base preset, just long press twice in a row and you’re back to it. 1 press is the second preset in the list.* *The Lamp will flash the index with each button press to confirm the count.*   
4. **Load & Exit:** Press and hold the Button again. The Lamp will strobe rapidly. Release the button, and then the selected preset will load, with a frozen buffer if applicable, and resume the audio engine.

## **🥧 The Presets**

Here is an overview of all 24 included presets and how the hardware maps to their parameters.   
*(Note: **you will need a format jumbler** to take full advantage of these.  For example, many presets require Ash to hear the effect.)*

### **Loopers & Delays**

* **coco\_mod**: A version of the coco preset with a few additions. The first difference is that in this version Record mode (Lamp Off) is the boot state, so the buffer automatically overwrites the noise. Earth acts as a record on/off toggle. Yellow outputs a clock pulse (16ppqn). Ash outputs clean audio at half volume.  
* **scrambler**: A live stutter/glitch effect utilizing a dual-buffer architecture. The playback buffer is divided into segments. Earth selects the active playback segment. Skip toggles between 16 or 3 segment divisions. Flip enables random skipping between segments. Button freezes the buffer.  
* **formant**: A vowel filter bank applied to the audio buffer. Earth modulates the vowel tuning.  
* **external\_sync**: A delay synchronized to an external clock. Patch a clock (regular or irregular) into Skip to quantize the buffer length. Flip reverses the playhead. Earth is a record on/off toggle. Button freezes the buffer (which continues to quantize while frozen).  
* **phasing**: Records a loop and plays it back with 4 drifting playheads heads. Button toggles between Record/Play modes. Skip triggers a one-shot recording (in Rec mode) or randomizes the playheads (in Play mode). Flip reverses all playhead directions. Earth modulates phase alignment. Yellow is bit-crushed audio. Ash outputs clean audio at half volume.

### **Samplers & Granular**

* **sampler (one-shot)**: A record/playback engine. Button toggles modes (Lamp OFF \= Armed, Strobe \= Recording, Solid \= Playback). Skip triggers recording or playback. Flip reverses playback. Earth sets the start position of the playhead. Yellow is a trigger at the end of the buffer, useful for looping.
* **sampler\_4x**: A multi-slice sampler dividing the buffer into 4 segments. Earth selects the segment. Button toggles Record/Play modes. Skip triggers a re-triggerable one-shot. Flip triggers a non-re-triggerable one-shot. Yellow is a trigger at the end of the buffer segment, useful for looping each segment individually.
* **granular**: A 16-voice granular engine. Button toggles Record/Play modes. Skip triggers grains. Earth modulates grain start position. Flip toggles grain size. Yellow is bit-crushed audio.

### **Reverbs & Resonators**

* **echo\_mod**: A prime-number based delay/reverb with pitch shifting via Speed knob. Earth controls a low-pass filter. Skip switches room sizes (short vs. long primes). Flip reverses playback. Button freezes the buffer. Yellow is bit-crushed audio. Ash is audio at line level.  
* **reverb\_spring**: An experimental spring reverb tank. Earth controls dampening. Flip is a latching switch for modulation speed (Surf vs. Lush). Skip freezes the buffer momentarily. Button is a latching freeze, during which skip is ignored. Yellow is bit-crushed audio.
* **reverb\_granular**: A live granular processing mode. Earth controls grain size. Flip pitches the reverb an octave up (Shimmer). Skip momentarily freezes the buffer. Button is a latching freeze, during which skip is ignored. Yellow is bit-crushed audio.
* **resonator**: A 16-band sympathetic resonator. Earth controls the central pitch. Flip switches between Organ and Gong mode. Skip toggles an octave down. Patch audio or press the Button to ping/excite the resonator. Yellow is bit-crushed audio.
* **harmonizer**: Pitch-tracking harmonizer generating 3 delay taps of over/under tones. Earth controls pitch tracking stability vs. LFO modulation. Flip switches between harmonics and sub-harmonics. Skip switches the prime math. Button parameter-locks all controls. Yellow is a stepped harmonic LFO.

### **Live FX**

* **flanger**: A very short delay. Earth controls the tape head spread. Send LFO to Earth for basic effect. Skip acts as a momentary buffer freeze. Ash and Yellow provide stereo wet outputs. Ash is required to hear the effect when Main Output is mixed with Ash for classic flange effect. Turn up feedback to strengthen effect. Try patching the golden CV into earth and gesture as if pressing your finger on the flange of the tape wheel.   
* **saturator**: A multi-mode distortion featuring 9 modes. Press the Button to cycle through the 9 distortion modes. The Lamp will blink to indicate the current mode number (1 to 9). The lamp will otherwise follow the envelope or the input. When flip is switched on, because it is a latching switch, the lamp will turn on and override the envelope. The selected saturation mode will be recalled if switched to a different preset and back again. Use the mixer in the firmware to adjust levels between modes. Skip and flip are programmed to be latched, so you can set them up with a physical switch and play it like a stompbox. Ash output is required because the Feedback knob introduces feedback to the saturator which changes the effect. Ash outputs a warm line level signal at full volume. Yellow is the bit-crushed line level audio.

*See table below.*

| Mode | Name | Earth | Flip  | Skip  |
| :---- | :---- | :---- | :---- | :---- |
| **1** | **Tube** | Drive | Boost | Filter |
| **2** | **Fuzz** | Bias | Octave-up folding | Attack |
| **3** | **Fold** | Drive | Earth XOR Ring Mod | Hard Clipping |
| **4** | **Tape** | Drive | Flanger | Wobble  |
| **5** | **Vinyl** | Dust | Ham Radio | More Dust  |
| **6** | **MP3** | Sample Rate | Bit Reduction  | Glitch |
| **7** | **Radio** | Tuning Dial | Crossover Distortion | Static |
| **8** | **RAT** | Distortion | Turbo | Filter |

### **Synth Voices & Generators**

* **drone**: Navigates a tonal lattice of the first 31 prime numbers to generate complex chords. Earth moves the harmonic center. Button cycles through 4 geometric chord shapes. Skip toggles between dense and spread voicing. Flip toggles between overtones and undertones.  
* **wavetable**: Wavetable synthesizer with ring modulation against the audio input. Earth modulates pitch. Flip switches between wavetable banks. Skip selects a random waveform. Button freezes the current wave.  
* **karplus**: A Karplus-Strong string physical model. Button or Skip "plucks" the string. Flip toggles between quantized and fretless tracking. Earth modulates pitch over an 11-semitone range. Yellow is the button gate press so that the button can modulate other things. 

### **Byte Generators**

* **bytebeats\_mod**: A slowed-down version of the stock bytebeat equation. Earth controls the sample-rate / speed.  
* **megabytebeats**: 16 distinct algorithmic beats. Skip cycles through the 16 equations. Earth modulates variables within the math. Flip plays the beat in reverse. Button parameter-locks the beat.  
* **arcade**: Generative 8-bit music system with three voices: Lead or Arp, Bass, and Drum voices. Earth selects the game "Cartridge" (Genre/Tempo). Button toggles the B-side sequence. Skip enables Lead. Flip enables Arp. Skip+Flip enables Power mode. *(Cartridges: 1\. Hero, 2\. Dungeon, 3\. Techno, 4\. Boss, 5\. Forest, 6\. Space, 7\. Broken, 8\. Crash)*.  
* **FX**: A generator for 37 sound effects. Earth acts as the cursor to select the sound. Skip triggers the selected sound and allows re-triggering. Flip fires a random one-shot sound with no re-triggering. Button freezes the selected sound as last triggered by Skip and locks out Flip. This can be used to treat the FX sound as an oscillator if triggered at a high rate.

### **Drum Machines**

* **groovebox**: Topological drum machine playing sampled kits. Earth scans through the selected drum axis. The active axis is selected via jacks (Flip \= Snare, Skip \= Hat, Flip+Skip \= Kick). Button activates an energy "Fill" mode. Yellow outputs a clock pulse (16ppqn)  
* **polyrhythms**: Topographic sequencer based on overlapping polyrhythmic bar divisions. Navigate exactly like groovebox (Earth scans the axis selected by Flip/Skip). Button freezes the currently playing pattern configuration. Yellow outputs a clock pulse (16ppqn).

## **🧑‍🍳 Navigating the Presets**

Preset Playlists were created to give different recipes for Cafe. You can build custom **Playlists** by adding a new playlist. This makes navigation faster and lets you customize your Cafe for specific needs.

To use one of these lists, type its name into the `#define ACTIVE_PLAYLIST` line in `esp_cafe.ino` before flashing.

* **`playlist_classic`**: A modded Cafe experience.  
  * *(0) coco\_mod, (1) echo\_mod*  
* **`playlist_loopers`**: Focused on manipulating live audio buffers.  
  * *(0) coco\_mod, (1) formant, (2) scrambler, (3) sampler, (4) sampler\_4x, (5) granular, (6) phasing*  
* **`playlist_reverbs`**: Presets capable of syncing to or generating clock signals.  
  * *(0) echo\_mod, (1) reverb\_spring, (2) reverb\_granular*  
* **`playlist_sync`**: Presets capable of syncing to or generating clock signals.  
  * *(0) coco\_mod, (1) external\_sync, (2) sampler, (3) sampler\_4x*  
* **`playlist_all_delays`**: All time control.  
  * *(0) coco\_mod, (1) external\_sync, (2) formant, (3) scrambler, (4) sampler, (5)sampler\_4x,  (6) granular, (7) phasing, (8) echo\_mod, (9) reverb\_spring, (10)reverb\_granular, (11) flanger*  
* **`playlist_live_FX`**: Real time FX processing  
  * *(0) saturator, (1) flanger*  
* **`playlist_bytes`**: Generative, algorithmic, 8-bit nostalgia.  
  * *(0) bytebeats\_mod, (1) megabytebeats, (2) arcade, (3) FX*  
* **`playlist_synth_voices`**: Generative synthesizer voices.  
  * *(0) drone, (1) wavetable*  
* **`playlist_resonance`**: Physical modeling and sympathetic strings.  
  * *(0) karplus, (1) resonator*  
* **`playlist_drums`**: Generative beat-making.  
  * *(0) groovebox, (1) polyrhythms*  
* **`playlist_ambient`**: Spatial and atmospheric.  
  * *(0) reverb\_spring, (1) echo\_mod, (2) reverb\_granular, (3) drone, (4) phasing*  
* **`playlist_all`**: Loads all 24 presets sequentially for the complete collection. Recommend against using this unless experienced with navigating the preset selection menu system.

## **💻 Developer API & Macros (stuff.h)**

If you want to build your own presets, look in stuff.h for macros that give hardware access and serve common needs programming on the ESP32.

### **Lamp / LED Control**

The OS overrides the Lamp during the preset menu. Inside your presets, you can call these macros without conflicting with the menu system:

* LAMP\_ON: Safely forces the LED on.  
* LAMP\_OFF: Safely forces the LED off.

### **Audio Outputs (Ash & Yellow)**

The firmware uses ash and yellow for different purposes depending on the design of the preset, but often they provide audio or clocking, both for interfacing with the outside world, but just as well in the Ciat Lonbarde ecosystem as well. 

* CLEAN\_ASHWRITER(val): Outputs a 7-bit, symmetrical, unclipped half-volume signal to the Ash jack. Uses dithering and delta-sigma noise shaping. Boost 6db to match unity input gain.  
* WARM\_ASHWRITER(val): Outputs an 8-bit, asymmetrically clipped, full-volume signal to Ash. Shifts the DC center to clip the LM3900 chip.  
* YELLOW\_AUDIO(val): Acts as a 3.4-bit pseudo-DAC. Maps 12-bit audio to the 10 Yellow pins to create a bit-crushed output. Can be used for odd stereo image with ash.  
* YELLOW\_PULSE(val): Sends a square pulse. Ideal for sending clock/sync signals to external gear. Could be expanded for more square wave type synthesis.

### **Inputs**

* BUTTON\_PRESSED: A boolean read of the button state. It returns false if the OS preset menu is currently active, preventing the playing preset from triggering while you are trying to change presets.

### **Buffer Management**

To prevent memory crashes while changing between 8-bit tape loops and 16-bit synthesis algorithms, use the included buffer morphing functions at the very top the preset loop:

* morph\_to\_8bit(): Formats the shared RAM back into delaybuffa for native dellius() calls.  
* morph\_to\_16bit(owner\_id, length): Wipes the shared RAM and formats it into a high-resolution buffer\_16bit\[\] array.
