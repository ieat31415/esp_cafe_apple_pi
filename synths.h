#include "stuff.h"
#include "wavetables.h"

// ==========================================
// NEW FIRMWARE: BUFFER TRANSFER
// ==========================================
int current_buffer_owner = 0; // 0 = coco_mod
int current_buffer_len = 131072; 
int current_buffer_head = 0;

void morph_to_8bit() {
    if (current_buffer_owner == 0) return; // Already 8-bit
    
    int src_len = current_buffer_len;
    if (src_len <= 0 || src_len > 65536) src_len = 20000;
    
    // Convert audio (16-bit Signed AC to 12-bit Unsigned DC)
    // Write to the Safe Zone (Indices 0 to 65535 map to delaybuffb)
    for (int i = 0; i < src_len; i++) {
        int read_idx = (current_buffer_head + i) % src_len;
        
        int val_12bit = buffer_16bit[read_idx] + 2048;
        //int val_12bit = buffer_16bit[i] + 2048; 
        if (val_12bit > 4095) val_12bit = 4095;
        if (val_12bit < 0) val_12bit = 0;
        dellius(i, val_12bit, false); 
    }
    
    // Tile the Safe Zone across the rest of the buffer
    for (int i = src_len; i < 131072; i++) {
        int val = dellius(i % src_len, 0, true);
        dellius(i, val, false);
    }
    
    current_buffer_owner = 0;
    current_buffer_len = 131072;
}

void morph_to_16bit(int new_owner, int target_len) {
    if (current_buffer_owner == new_owner) return; 
    
    if (current_buffer_owner == 0) {
        // Morphing from 8-bit Tape to 16-bit
        // Read from 8-bit Safe Zone, convert to 16-bit AC, write to buffer_16bit
        for (int i = 0; i < target_len; i++) {
            int val_12bit = dellius(i, 0, true);
            buffer_16bit[i] = (int16_t)(val_12bit - 2048); 
        }
    } 
    else {
        // Tile from a short 16-bit preset to a long 16-bit preset
        int src_len = current_buffer_len;
        if (src_len <= 0) src_len = 1;
        for (int i = src_len; i < target_len; i++) {
            buffer_16bit[i] = buffer_16bit[i % src_len];
        }
    }
    
    current_buffer_owner = new_owner;
    current_buffer_len = target_len;
}

//////BEGIN PRESETS//////////////////////////////////////////////////////////////////////////////////

// ==========================================
// 1. COCO - modified
// ==========================================
// same as coco, just with changes below
// yellow is a clock pulse
// earth is record on/off switch
// ash is clean audio output at half volume of input

void IRAM_ATTR coco_mod() {

morph_to_8bit(); //needed for buffer translation

 //INTABRUPT
 //REG(GPIO_STATUS_W1TC_REG)[0]=0xFFFFFFFF; 

 DACWRITER(pout)
 gyo=ADCREADER // Audio Input signal is read here

//MODIFIED FIRMWARE
 int earth_cv = EARTHREAD;

// HYSTERESIS (Using earth_cv)
 if (earth_last_state == 0) {
     if (earth_cv > TRIGGER_ON_THRESHOLD) {
         lamp = !lamp; 
         audio_frozen_state = lamp;
        //  if (lamp) REG(GPIO_OUT1_W1TS_REG)[0] = BIT(1); //TO BE REPLACED BELOW FOR BUFFER TRANSFER
        //  else REG(GPIO_OUT1_W1TC_REG)[0] = BIT(1); 
        if (lamp) { LAMP_ON; } 
        else { LAMP_OFF; }
         earth_last_state = 1; 
     }
 } 
 else { 
     if (earth_cv < TRIGGER_OFF_THRESHOLD) {
         earth_last_state = 0; 
     }
 }
///////////END MODIFIED

 //pout=dellius(t,gyo,lamp); // ORIGINAL
 pout=dellius(t,gyo,audio_frozen_state); //disabing lamp during preset selection to allow buffer transfer
 if (FLIPPERAT) t--; //inverted to make work with sampler based presets
 else t++; 
 t=t&0x1FFFF;
 if (SKIPPERAT)  {
  if (lastskp==0) delayskp = t;
  lastskp = 1;
 } else {
  if (lastskp) t=delayskp;
  lastskp = 0;
 } 

//ORIGNAL FIRMWARE
//  adc_read = EARTHREAD;
//  ASHWRITER(adc_read); //rand()

//MODIFIED FIRMWARE
CLEAN_ASHWRITER(pout); //Sends wet audio through ASH. Swap out with "ASHWRITER" for a full volume line out
///////////END MODIFIED

//ORIGINAL FIRMWARE
//  YELLOWERS(t)

//MODIFIED FIRMWARE
// --- YELLOW 4 PPQN SYNC CLOCK ---
    // isolate the 16th note window (8192 samples)
    if ((t & 0x1FFF) < 2000) { 
        // if DOWNBEAT // to be used for clock reset in conjunction with comparator to extract this accent pule
        if (t < 8192) {
            YELLOW_AUDIO(4095); // 3.3V accent
        } else {
            YELLOW_AUDIO(3000); // 2.4V clock
        }
    } else {
        YELLOW_AUDIO(0); 
    }
///////////END MODIFIED
 
 // HEARTBEAT
 REG(I2S_CONF_REG)[0] &= ~(BIT(5)); 
 REG(I2S_INT_CLR_REG)[0] = 0xFFFFFFFF;
 REG(I2S_CONF_REG)[0] |= (BIT(5)); //start rx 
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////ORINGAL FIRMWARE
// int myNumbers[] = {32000, 31578, 22444, 25111};
// //you need to make a table that is 0,3000,5578
// int myPlacers[] = {0, 0, 0, 0};
// int tapsz=sizeof(myPlacers)>>2;

//int myNumbers[] = {12000, 11578, 14444, 15111,8900, 10278, 12004, 12111};
//you need to make a table that is 0,3000,5578
//int myPlacers[] = {0, 0, 0, 0,0,0,0,0};

//int tapsz=sizeof(myPlacers)>>2;


// ==========================================
// ECHO ORIGINAL - MODIFIED
// ==========================================
// // yellow is
// // earth is
// // ash is audio out at line level
// // speed is like pre-delay
// void IRAM_ATTR echo() {
//  //INTABRUPT
//  //REG(GPIO_STATUS_W1TC_REG)[0]=0xFFFFFFFF; 
//  DACWRITER(pout)
//  gyo=ADCREADER
//  pout =0;
//  for (int i=0; i<tapsz; i++) 
//   pout+=dellius((myPlacers[i]<<2)+i,gyo,lamp);
//  pout = pout>>2;
//  if (FLIPPERAT)
//   for (int i=0; i<tapsz; i++)  //sizeof(myPlacers)
//    myPlacers[i]++;
//  else 
//   for (int i=0; i<tapsz; i++) 
//    myPlacers[i]--;
//  for (int i=0; i<tapsz; i++) {
//   myPlacers[i] %= myNumbers[i];
//   if (myPlacers[i]<0) myPlacers[i] += myNumbers[i];
//  }
//  if (SKIPPERAT)  {} else {} 
//  REG(I2S_CONF_REG)[0] &= ~(BIT(5)); 
//  adc_read = EARTHREAD;
//  ASHWRITER(pout); //rand()
//  REG(I2S_INT_CLR_REG)[0]=0xFFFFFFFF;
//  REG(I2S_CONF_REG)[0] |= (BIT(5)); //start rx
//  YELLOWERS(myPlacers[0]+myPlacers[1]+myPlacers[2]+myPlacers[3]);
// }
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

// ==========================================
// ECHO MOD
// ==========================================
// based on the echo preset, this reverb is based on prime numbers
// it has howling feedback
// pitch shifting with speed modulation
// with tone control and room switching.
//
// earth is the input low pass filter
// skip is room size switch (long vs short primes)
// flip is playback direction
// ash is wet audio
// yellow is bit crushed audio
// can make stereo image with ash and yellow in a mixer
// button for freezing the buffer
//
// uses 16-bit buffer for low noise floor
// prime numbers prevent metallic ringing

// Buffer set up
//static int16_t echo_buffer[20000]; 
static int echo_head = 0;

// Setting up switching between reverb types
// Hall is active when grounded
const int taps_hall[] = {19001, 15013, 11003, 7001};

// Chamber is default/unplugged
const int taps_chamber[] = {5003, 4003, 3001, 2003};

// setting up filters
static int echo_tape_lpf = 0; 
static int yellow_lpf = 0;    

// SOFT LIMITER
int16_t echo_soft_limit(int32_t x) {
    // Clamp extremely loud signals first
    if (x > 6000) return 2047;
    if (x < -6000) return -2047;
    
    // Linear region (Clean sound)
    if (x > -1500 && x < 1500) return (int16_t)x;
    
    // Soft Knee (Saturation)
    // Compresses the top 500 values to fit into the DAC
    if (x > 0) return 1500 + ((x - 1500) / 3); 
    else       return -1500 + ((x + 1500) / 3);
}

// MAIN
void IRAM_ATTR echo_mod() {

    if (current_buffer_owner != 1) {
        morph_to_16bit(1, 20000);
        echo_head = 0;
    }

    // --- WAKE UP & CALIBRATION GLOBALS ---
    static int cal_min = 4095; 
    static int cal_max = 0; 
    static bool knob_moved = false;
    static int initial_earth = 0;
    static int s_earth = -1;
    static int boot_timer = 0;

    static bool was_in_menu = false;
    if (preset_mode) {
        was_in_menu = true;
    } else if (was_in_menu) {
        was_in_menu = false;
        cal_min = 4095;
        cal_max = 0;
        s_earth = -1;
        knob_moved = false;
        boot_timer = 0;
    }

    // INPUTS
    int raw_in = ADCREADER; 
    int earth_cv = EARTHREAD; 

    // --- EARTH AUTO-CALIBRATION ---
    REG(APB_SARADC_SAR1_PATT_TAB1_REG)[0] = (0x0C<<24) | (0x6C<<16);
    uint32_t fifo_data = REG(I2S_FIFO_RD_REG)[0];
    int channel = (fifo_data >> 12) & 0xF; 
    int value   = (fifo_data & 0xFFF);     
    
    if (channel == 0) {
        if (s_earth == -1) s_earth = value;
        else s_earth += (value - s_earth) >> 4;
    }

    if (boot_timer < 2000) {
        boot_timer++;
        initial_earth = s_earth;
    }

    if (!knob_moved && boot_timer >= 2000) {
        int drift = s_earth - initial_earth;
        if (drift < 0) drift = -drift;
        if (drift > 300) {
            knob_moved = true; 
        }
    }

    if (knob_moved) {
        if (s_earth < cal_min) cal_min = s_earth;
        if (s_earth > cal_max) cal_max = s_earth;
    } else {
        cal_min = initial_earth;
        cal_max = initial_earth;
    }

    int range = cal_max - cal_min;
    if (range < 50) range = 50;
    int reading = s_earth - cal_min;
    if (reading < 0) reading = 0;
    
    int earth_cal = (reading * 255) / range;
    if (!knob_moved) earth_cal = 0; // Default to 0 when unplugged

    // TONE (Inverted Filter: 0 = Open/Bright, 255 = dark and muffled
    int filter_alpha = 256 - ((earth_cal * 246) >> 8);

    // INPUT GAIN
    int ac_in = raw_in - 2048;
    
    // Write directly to the tape unfiltered
    int audio_write = echo_soft_limit(ac_in);
    
    // STATES
    bool use_hall = SKIPPERAT; // 0 (False) = Chamber Default
    bool reverse = FLIPPERAT;  // 0 (False) = Forward Default
    bool freeze = audio_frozen_state;

    // 4-VOICE REVERB ENGINE
    int32_t mix_sum = 0;
    
    for (int i=0; i<4; i++) {
        int delay_len;
        if (use_hall) delay_len = taps_hall[i];
        else delay_len = taps_chamber[i];
        
        int read_pos;
        if (reverse) read_pos = echo_head + delay_len; 
        else         read_pos = echo_head - delay_len;

        while (read_pos < 0) read_pos += 20000;
        while (read_pos >= 20000) read_pos -= 20000;
        
        mix_sum += buffer_16bit[read_pos];
    }
    
    // Controls
    if (!freeze) {
        buffer_16bit[echo_head] = audio_write;
    }
    
    if (reverse) echo_head--;
    else echo_head++;
    
    if (echo_head >= 20000) echo_head = 0;
    if (echo_head < 0) echo_head = 19999;

    // OUTPUT & FILTERING
    // Filter the summed output
    int32_t diff = mix_sum - echo_tape_lpf;
    echo_tape_lpf += (diff * filter_alpha) >> 8; 

    // filtered mix drives the limiter
    int16_t final_out = echo_soft_limit(echo_tape_lpf);
    
    pout = final_out + 2048;
    if (pout > 4095) pout = 4095;
    if (pout < 0) pout = 0;
    
    DACWRITER(pout);
    ASHWRITER(pout); 

    // YELLOW is a bit crushed wet output
    YELLOW_AUDIO(pout);
    
     // HEARTBEAT
    REG(I2S_CONF_REG)[0] &= ~(BIT(5)); 
    REG(I2S_INT_CLR_REG)[0] = 0xFFFFFFFF;
    REG(I2S_CONF_REG)[0] |= (BIT(5)); //start rx 
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

// ==========================================
// FORMANT (Vowel Filter) - NEW PRESET
// ==========================================
// based on the coco preset with a formant filter added
// earth modulates the vowels
// ash is wet signal at line level 
// yellow is buffer position

// 3 filters, each has 2 state variables (Band, Low)
// Values are scaled up by 4096 (12 bits) to simulate decimals and eliminate floating point computation
int f1_band=0, f1_low=0;
int f2_band=0, f2_low=0;
int f3_band=0, f3_low=0;

// State Variable Filter (high performance with ints)
// input: Audio sample
// f: Frequency coefficient (0.01 to 0.5)
// q: Resonance (Damping), lower is more resonant (0.05 to 0.5)
// f and q are fixed-point integers (1.0 = 4096)
int svf_bandpass_int(int input, int f, int q, int *band, int *low) {
    // low += f * band
    *low += (*band * f) >> 12;
    
    // high = input - low - q * band
    int high = input - *low - ((*band * q) >> 12);
    
    // band += f * high
    *band += (high * f) >> 12;
    
    return *band;
}

void IRAM_ATTR formant() {

morph_to_8bit(); //needed for buffer translation
 
 // read inputs
 int audio_in = ADCREADER;      // audio input
 int earth_cv = EARTHREAD;      // earth

 // get audio from delay
 //int raw_audio = dellius(t, audio_in, lamp); //TO BE REPLACED BELOW TO ALLOW FOR BUFFER TRANSFER
 int raw_audio = dellius(t, audio_in, audio_frozen_state);
 
// Center the audio (0-4095 -> -2048 to +2048)
 int signal = raw_audio - 2048;

 // FORMANT FILTER BANK
 // map earth_cv (0-255) to frequency coefficients (0.0 to 1.0)
 // these numbers tune the vowels
 
// quantized earth (0 to 255)
 int cv = earth_cv;

 // Formant 1 (Throat): Slides 200Hz -> 800Hz
 // Base 80 + (cv * 0.5) -> Range ~80 to 200
 int f1_freq = 80 + (cv >> 1);
 
 // Formant 2 (Mouth): Slides 800Hz -> 2200Hz
 // Base 300 + (cv * 1.5) -> Range ~300 to 700
 int f2_freq = 300 + ((cv * 3) >> 1);
 
 // Formant 3 (Teeth): Slides 2200Hz -> 3000Hz
 // Base 800 + (cv * 1.0) -> Range ~800 to 1055
 int f3_freq = 800 + cv;

 // Q Factor (Resonance): Fixed at 0.1 (approx 400 in 12-bit scale)
 int q = 400;

 // Apply the 3 Filters in Parallel
int out1 = svf_bandpass_int(signal, f1_freq, q, &f1_band, &f1_low);
 int out2 = svf_bandpass_int(signal, f2_freq, q, &f2_band, &f2_low);
 int out3 = svf_bandpass_int(signal, f3_freq, q, &f3_band, &f3_low);

 // Sum them up and scale back to integer
 // We multiply by 1.5 to boost the resonance volume
int filtered_mix = (out1 + out2 + out3);
 filtered_mix = filtered_mix + (filtered_mix >> 1);
 
 // Clip and re-center to 0-4095 for output
 int final_out = (int)filtered_mix + 2048;
 if (final_out > 4095) final_out = 4095;
 if (final_out < 0) final_out = 0;

 // Update Global Output
 pout = final_out;

 // 5. TIMING & OUTPUTS
 if (FLIPPERAT) t--;
 else t++; 
 t=t&0x1FFFF;
 
 if (SKIPPERAT)  {
  if (lastskp==0) delayskp = t;
  lastskp = 1;
 } else {
  if (lastskp) t=delayskp;
  lastskp = 0;
 } 
 
 DACWRITER(pout); // Send the "Talking" audio to speakers
 CLEAN_ASHWRITER(pout); // Monitor the Vowel Control on Ash
 YELLOW_BINARY(t) // Buffer position encoded

  // HEARTBEAT
 REG(I2S_CONF_REG)[0] &= ~(BIT(5)); 
 REG(I2S_INT_CLR_REG)[0] = 0xFFFFFFFF;
 REG(I2S_CONF_REG)[0] |= (BIT(5)); //start rx 
}

//////////////////END////////////////////////////////////////////////////////////////////////////////////////

// ==========================================
// FLANGER (Short Delay) - NEW PRESET
// ==========================================
// exploring short delay times
// stereo flanger
// main out in mono, ash and yellow for stereo
// ash is wet audio out at line level
// earth is the tape head spread, essential to send moduation here
// yellow is a bit crushed stereo channel at line level
// skip freezes buffer, like the button
// flip toggles Negative Flange (Phase Invert)
// Use with feedback knob up
// freezing buffer is like a wavetable scanner

static int flanger_head = 0; 

void IRAM_ATTR flanger() {

    morph_to_8bit(); //needed for buffer translation

    // --- STATES & WAKE UP ---
    static int skip_integrator = 0;
    static bool prev_skip_stable = false;
    static bool skip_toggled = false;
    
    static int flip_integrator = 0;
    static bool prev_flip_stable = false;
    static bool flip_toggled = false;
    
    static int32_t phase_acc = 256 << 16; 

    static bool button_locked = false;
    static bool last_frozen = false;

    // Auto-Calibration States
    static int cal_min = 255;
    static int cal_max = 0;

    static bool is_first_run = true;
    static bool was_in_menu = false;

    if (preset_mode) {
        was_in_menu = true;
    } else if (was_in_menu || is_first_run) {
        was_in_menu = false;
        is_first_run = false;
        
        // flanger to wakes up unfrozen
        audio_frozen_state = false;
        last_frozen = false;
        button_locked = false;
        skip_toggled = false;
        flip_toggled = false;  
        phase_acc = 256 << 16; 
        lamp = false; 

        // Reset Calibration
        cal_min = 255;
        cal_max = 0;

        // Sync to resting hardware state 
        bool skip_raw = SKIPPERAT;
        skip_integrator = skip_raw ? 300 : 0;
        prev_skip_stable = skip_raw;

        bool flip_raw = FLIPPERAT;
        flip_integrator = flip_raw ? 300 : 0;
        prev_flip_stable = flip_raw;
    }

    int audio_in = ADCREADER;

    // --- EARTH AUTO-CALIBRATION ---
    int earth_raw = EARTHREAD; 
    
    // Dynamically track the highest and lowest voltages 
    if (earth_raw < cal_min) cal_min = earth_raw;
    if (earth_raw > cal_max) cal_max = earth_raw;
    
    int spread = cal_max - cal_min;
    int earth_cv = earth_raw;
    
    // stretch CV range to 0-255
    if (spread > 10) {
        earth_cv = ((earth_raw - cal_min) * 255) / spread;
    }

    // write head inside the first 256 slots of memory
    // masking with 0xFF (255) makes the loop wrap
    flanger_head = (flanger_head + 1) & 0xFF; 
    
    // --- BUTTON SYNC (Latching, overrides skip) ---
    if (audio_frozen_state != last_frozen) {
        last_frozen = audio_frozen_state;
        
        if (skip_toggled) {
            skip_toggled = false;
            button_locked = false;
            audio_frozen_state = false; 
            lamp = false;
        } else {
            button_locked = audio_frozen_state;
        }
    }

    // --- SKIP DEBOUNCE (Edge-Triggered Toggle Freeze) ---
    bool skip_raw = SKIPPERAT;
    if (skip_raw) {
        if (skip_integrator < 300) skip_integrator++;
    } else {
        if (skip_integrator > 0) skip_integrator--;
    }
    
    bool skip_stable = (skip_integrator > 250);
    if (skip_stable && !prev_skip_stable) {
        if (!button_locked) skip_toggled = !skip_toggled;
    }
    prev_skip_stable = skip_stable;

    // --- FLIP DEBOUNCE (Edge-Triggered Phase Invert) ---
    bool flip_raw = FLIPPERAT;
    if (flip_raw) {
        if (flip_integrator < 300) flip_integrator++;
    } else {
        if (flip_integrator > 0) flip_integrator--;
    }
    
    bool flip_stable = (flip_integrator > 250);
    if (flip_stable && !prev_flip_stable) {
        flip_toggled = !flip_toggled;
    }
    prev_flip_stable = flip_stable;

    // Combine Button State and Skip State
    bool freeze_active = button_locked || skip_toggled;

    if (freeze_active) { LAMP_ON; } 
    else               { LAMP_OFF; }

    // write to buffer only if freeze_active is false
    dellius(flanger_head, audio_in, freeze_active);

    // read from a few steps behind the write head
    int read_target = flanger_head - 20 - (earth_cv >> 1);
    int read_pos = read_target & 0xFF;
    int wet_signal = dellius(read_pos, 0, true);

    // --- PHASE INVERT SMOOTHING ---
    int32_t target_phase = flip_toggled ? -(256 << 16) : (256 << 16);
    phase_acc += (target_phase - phase_acc) >> 12; 
    
    int32_t phase_multiplier = phase_acc >> 16; 

    int ac_in = audio_in - 2048;
    int ac_wet = wet_signal - 2048;
    
    int slewed_wet = (ac_wet * phase_multiplier) >> 8;
    int ac_mix = (ac_in + slewed_wet) >> 1; 

    pout = ac_mix + 2048;
    if (pout > 4095) pout = 4095;
    if (pout < 0) pout = 0;
    
    DACWRITER(pout);
    ASHWRITER(wet_signal); 
    YELLOW_AUDIO(wet_signal);

    // HEARTBEAT
    REG(I2S_CONF_REG)[0] &= ~(BIT(5)); 
    REG(I2S_INT_CLR_REG)[0] = 0xFFFFFFFF;
    REG(I2S_CONF_REG)[0] |= (BIT(5)); 
}


///////////////////////////////////////////////////////////////////////////////////////////////END

// ==========================================
// KARPLUS STRONG - NEW PRESET
// ==========================================
// Karplus strong on cafe
// Button is a trigger, play it like plucking a string
// Skip is a CV trigger to pluck
// Flip switches between quantized and unquantized
// Earth modulates the pitch by 11 semitones with a 0-7V CV (gold)
// Keep feedback knob up high for string to ring, find the sweet spot
// Speed can also modulate pitch
// Ash is the buffer signal
// Yellow is the button gate press so that the button can modulate other things

// Tuning
const int ji_nums[] = {1, 9, 5, 4, 3, 5, 15, 2}; 
const int ji_dens[] = {1, 8, 4, 3, 2, 3,  8, 1}; 

#define BASE_LEN 400

// Memory
static int ji_head = 0; 
static int ji_out_lpf = 0; 
static int current_len = BASE_LEN;
static int pluck_timer = 0;

// State Machine
static int trig_state = 0; 
static int lockout_timer = 0;

// Lamp setup
static int karplus_lamp_env = 0;
static int lamp_dc_tracker = 2048 << 6; 

// Skip debounce
static int skip_counter = 0;       // Counts continuous stable samples
static bool skip_stable_low = false; // The clean, verified state
static int skip_stuck_timeout = 0;   // Safety timer

void IRAM_ATTR karplus() {

    morph_to_8bit(); //needed for buffer translation
    
    // audio input 
    int audio_in = ADCREADER; 
    
    // EARTH CV (Pitch)
    static int pyth_smart_earth = 0;
    REG(APB_SARADC_SAR1_PATT_TAB1_REG)[0] = (0x0C<<24) | (0x6C<<16);
    uint32_t fifo_data = REG(I2S_FIFO_RD_REG)[0];
    int channel = (fifo_data >> 12) & 0xF; 
    int value   = (fifo_data & 0xFFF);     
    if (channel == 0) pyth_smart_earth += (value - pyth_smart_earth) >> 4;
    
    int raw_cv = pyth_smart_earth;
    int offset_cv = raw_cv - 180;
    if (offset_cv < 0) offset_cv = 0;
    int earth_cv = offset_cv / 5;
    if (earth_cv > 255) earth_cv = 255;

    // STRING LENGTH
    int target_len = BASE_LEN;
    bool fretless_mode = !(REG(GPIO_IN1_REG)[0] & 0x8); 

    // two modes: quantized or fretless controlled by flip
    if (fretless_mode) {
        int max_slide = (BASE_LEN * 19) >> 5; 
        int shorten = (earth_cv * max_slide) >> 8;
        target_len = BASE_LEN - shorten;
    } else {
        int index = earth_cv >> 5; 
        if (index > 7) index = 7;
        target_len = (BASE_LEN * ji_dens[index]) / ji_nums[index];
    }
    current_len += (target_len - current_len) >> 4; 

    // Button State
    //bool btn_raw = !(REG(GPIO_IN1_REG)[0] & 0x1); //TO BE REPLACED BELOW FOR BUFFER TRANSFER
    bool btn_raw = BUTTON_PRESSED;
    
    // Skip (GPIO 34) - Active Low, Floating Input
    // "High" (1) is Unplugged/Safe. "Low" (0) is Trigger.
    bool skip_is_low = !(REG(GPIO_IN1_REG)[0] & 0x4); 
    
    // DEBOUNCER:
    // If the signal is Low, count up. 
    // If it flickers High, reset count to 0.
    if (skip_is_low) {
        if (skip_counter < 1000) skip_counter++;
    } else {
        skip_counter = 0; // Immediate Reset on noise
    }
    
    // Threshold: Must be Low for ~20ms (600 samples) to count as real.
    if (skip_counter > 600) {
        skip_stable_low = true;
    } else {
        skip_stable_low = false;
    }

    // If Skip stays valid for >1 second, assume it's floating low
    // and ignore it so the button still works.
    if (skip_stable_low) {
        if (skip_stuck_timeout < 32000) skip_stuck_timeout++;
    } else {
        skip_stuck_timeout = 0;
    }
    
    bool ignore_skip = (skip_stuck_timeout >= 32000);
    bool effective_skip = skip_stable_low && !ignore_skip;
    bool input_active = btn_raw || effective_skip;

    // ENGINE
    if (trig_state == 0) { 
        if (input_active) {
            // PLUCK
            int smart_len = current_len; 
            if (smart_len < 150) smart_len = 150;
            if (smart_len > 1200) smart_len = 1200;
            pluck_timer = smart_len + (rand() & 127);
            
            lockout_timer = 2500; 
            trig_state = 1;
        }
    }
    else if (trig_state == 1) { // LOCKED
        lockout_timer--;
        if (lockout_timer <= 0) {
            if (input_active) trig_state = 2; 
            else trig_state = 0; 
        }
    }
    else if (trig_state == 2) { // WAIT FOR RELEASE
        if (!input_active) trig_state = 0;
    }

    // EXCITATION
    int write_val = 0;
    if (pluck_timer > 0) {
        int noise = (rand() & 4095) - 2048; 
        write_val = 2048 + noise;
        pluck_timer--;
    } else {
        write_val = audio_in; 
    }
    
    if (write_val > 4095) write_val = 4095;
    if (write_val < 0) write_val = 0;

    dellius(ji_head, write_val, false); 

    // READ STRING
    int read_pos = (ji_head - current_len) & 0x1FFF;
    int raw_string = dellius(read_pos, 0, true);

    // OUTPUT FILTER
    int ac_sig = raw_string - 2048;
    int ac_lpf = ji_out_lpf - 2048;
    int filtered = (ac_sig + (ac_lpf * 3)) >> 2; 
    ji_out_lpf = filtered + 2048;

    pout = filtered + 2048; 
    if (pout > 4095) pout = 4095;
    if (pout < 0) pout = 0;

    ASHWRITER(pout); 
    DACWRITER(pout);
    
    // LAMP is Envelope of sound
    lamp_dc_tracker += (pout - (lamp_dc_tracker >> 6));
    int current_dc = lamp_dc_tracker >> 6;
    
    int ac_pout = pout - current_dc;
    int abs_vol = (ac_pout > 0) ? ac_pout : -ac_pout;
    
    karplus_lamp_env += (abs_vol - karplus_lamp_env) >> 4;
    
    // if (karplus_lamp_env > 100) {  // TO BE REPLACED BELOW FOR BUFFER TRANSFER
    //     REG(GPIO_OUT1_W1TS_REG)[0] = BIT(1); 
    // } else {
    //     REG(GPIO_OUT1_W1TC_REG)[0] = BIT(1); 
    // }

    if (karplus_lamp_env > 100) { LAMP_ON; } else { LAMP_OFF; }
    
    // Yellow sends button gate
    if (btn_raw) { YELLOW_PULSE(4095); } else { YELLOW_PULSE(0); }

    // CLEANUP
    ji_head = (ji_head + 1) & 0x1FFF;
    REG(I2S_CONF_REG)[0] &= ~(BIT(5)); 
    REG(I2S_INT_CLR_REG)[0]=0xFFFFFFFF;
    REG(I2S_CONF_REG)[0] |= (BIT(5)); 
}

///////////////////////////////////////////////////////////////////////////////////////////////END

// ==========================================
// RESONATOR - NEW PRESET
// ==========================================
// 16 band resonator
// excite it through the audio inputs and button
// try sending it in a steady sine wave and modulate everything else
// flip button switches between organ and gong mode
// skip is an octave down toggle
// skip and flip are latching switches
// patch antenna to them and touch screw to switch
// earth is central pitch

// --- TUNING TABLES ---
const int ratios_prime[] = {
    100, 106, 119, 131, 137, 144, 162, 175, 
    181, 194, 212, 237, 275, 325, 350, 400 
};

typedef struct {
    int32_t low;
    int32_t band;
} svf_int;

#define NUM_BANDS 16
static svf_int bank[NUM_BANDS];
static bool bank_init = false;

static int res_yellow_lpf = 0;
// Damping: 12 = Long Ring
static int32_t global_damp = 12; 

// DC Blocker State
static int32_t dc_offset = 2048 << 12;

// DEBOUNCE & LOGIC MEMORY
static int res_skip_integrator = 0;
static bool res_skip_stable = false;
static int res_flip_integrator = 0;
static bool res_flip_stable = false;

// LATCHING MEMORY
static bool skip_latched = false;
static bool flip_latched = false;
static bool prev_skip_stable = false;
static bool prev_flip_stable = false;

// PING (BUTTON) MEMORY
static int ping_timer = 0;
static bool prev_btn_state = false;

// LAMP MEMORY
static int res_lamp_env = 0;
static int res_lamp_dc_tracker = 2048 << 6;

// Soft Limiter
int32_t res_limit(int32_t x) {
    if (x > 2000) return 2000 + (x - 2000) / 4;
    if (x < -2000) return -2000 + (x + 2000) / 4;
    return x;
}

void IRAM_ATTR resonator() {
    
    // INIT
    if (!bank_init) {
        for (int i=0; i<NUM_BANDS; i++) {
            bank[i].low = 0; bank[i].band = 0;
        }
        bank_init = true;
    }

    // --- WAKE UP BLOCK ---
    static bool was_in_menu = false;
    if (preset_mode) {
        was_in_menu = true;
    } else if (was_in_menu) {
        was_in_menu = false;
        
        // Sync flip and skip to prevent toggles on boot
        prev_skip_stable = SKIPPERAT;
        prev_flip_stable = FLIPPERAT;
        res_skip_integrator = SKIPPERAT ? 300 : 0;
        res_flip_integrator = FLIPPERAT ? 300 : 0;
        res_skip_stable = SKIPPERAT;
        res_flip_stable = FLIPPERAT;
    }
    
    // INPUTS & DC BLOCKING
    int raw_in = ADCREADER;
    
    // Slow DC Tracking
    int32_t current_dc = dc_offset >> 12;
    int32_t ac_raw = raw_in - current_dc;
    dc_offset += ac_raw >> 4; 
    
    // Input Gain
    int32_t ac_in = ac_raw << 1; 
    
    // 3. BUTTON PING LOGIC (IMPULSE + BOOST)
    bool btn_pressed = BUTTON_PRESSED;
    
    if (btn_pressed && !prev_btn_state) {
        ping_timer = 2500; // 80ms Sustain
        ac_in += 30000; 
    }
    prev_btn_state = btn_pressed;
    
    // Inject Sustaining Noise
    if (ping_timer > 0) {
        // Massive Noise Boost (<< 4) for high energy
        int32_t noise = ((rand() & 4095) - 2048) << 4;
        ac_in += noise; 
        ping_timer--;
    }
    
    // CONTROLS (DEBOUNCED & LATCHED)
    
    // SKIP (Octave Down Toggle)
    if (SKIPPERAT) {
        if (res_skip_integrator < 300) res_skip_integrator++;
    } else {
        if (res_skip_integrator > 0) res_skip_integrator--;
    }
    if (res_skip_integrator > 250) res_skip_stable = true;
    else if (res_skip_integrator < 50)  res_skip_stable = false; 
    
    if (res_skip_stable && !prev_skip_stable) {
        skip_latched = !skip_latched;
    }
    prev_skip_stable = res_skip_stable;
    
    // FLIP (Mode Select Toggle)
    if (FLIPPERAT) {
        if (res_flip_integrator < 300) res_flip_integrator++;
    } else {
        if (res_flip_integrator > 0) res_flip_integrator--;
    }
    if (res_flip_integrator > 250) res_flip_stable = true;
    else if (res_flip_integrator < 50)  res_flip_stable = false; 

    if (res_flip_stable && !prev_flip_stable) {
        flip_latched = !flip_latched;
    }
    prev_flip_stable = res_flip_stable;
    
    bool mode_gong = flip_latched;
    
    // EARTH (Pitch)
    int earth_cv = EARTHREAD; 
    
    // FREQUENCY CALC
    int32_t base_f = 60 + earth_cv; 
    if (skip_latched) base_f = base_f >> 1; // Octave Down
    if (base_f < 8) base_f = 8;
    
     // FILTER ENGINE
     int32_t out_accum = 0;
    
    for (int i=0; i<NUM_BANDS; i++) {
        int32_t f;
        
        if (mode_gong) {
            f = (base_f * ratios_prime[i]) / 100;
        } else {
            f = base_f * (i + 1); 
        }
        
        if (f > 2500) f = 2500; 

        // SVF Algorithm
        bank[i].low += (f * bank[i].band) >> 12;
        int32_t damp_term = (global_damp * bank[i].band) >> 12;
        int32_t high = ac_in - bank[i].low - damp_term;
        bank[i].band += (f * high) >> 12;
        
        out_accum += bank[i].band >> (i >> 3); 
    }
    
    // OUTPUT STAGE
    int32_t final = res_limit(out_accum >> 4); 
    
    if (final > 2047) final = 2047;
    if (final < -2047) final = -2047;
    
    pout = final + 2048;
    if (pout > 4095) pout = 4095;
    if (pout < 0) pout = 0;
    
    DACWRITER(pout);
    ASHWRITER(pout);
    
    // LAMP // Envelope Follower
    res_lamp_dc_tracker += (pout - (res_lamp_dc_tracker >> 6));
    int current_out_dc = res_lamp_dc_tracker >> 6;
    int ac_pout = pout - current_out_dc;
    int abs_vol = (ac_pout > 0) ? ac_pout : -ac_pout;
    res_lamp_env += (abs_vol - res_lamp_env) >> 4;
    if (res_lamp_env > 100) { LAMP_ON; } else { LAMP_OFF; }
    
    // YELLOW
    YELLOW_AUDIO(pout);        

    // CLEANUP
    REG(I2S_CONF_REG)[0] &= ~(BIT(5)); 
    REG(I2S_INT_CLR_REG)[0] = 0xFFFFFFFF;
    REG(I2S_CONF_REG)[0] |= (BIT(5)); 
}


//////////////NEW PRESET - SPRING REVERB///////////////////////////////////////////////////////////////////////////////

// ==========================================
// REVERB SPRING --- NEW PRESET
// ==========================================
// experimental spring reverb
// speed is the delay time, from slapback to galaxy
// ash is wet out
// yellow is bit crushed wet out
// for trad spring reverb, keep feedback knob down and mix ash and dry in mixer
// turn feedback up for self oscillation
// button freezes buffer. it is latching.
// earth controls the dampening amount
// flip controls modulation speed. it is a latching switch
// skip will freeze buffer too, but only momentarily and will be overridden by button

// Initialization
static int spring_head = 0; 
static int rev_yellow_lpf = 0;
static int rev_ash_lpf = 0;

// Filters
static int32_t spring_dc_tracker = 0; 
static int feedback_lpf = 0;

// LFO
static uint32_t lfo_accum = 0; 
static int lfo_dir = 1;

// TUBE SATURATOR
static int32_t tube_drive(int32_t x) {
    int32_t sign = 1;
    if (x < 0) { sign = -1; x = -x; }
    if (x <= 1200) return x * sign;
    int32_t delta = x - 1200;
    int32_t headroom = 847; 
    int32_t compressed = (delta * headroom) / (headroom + delta);
    return (1200 + compressed) * sign;
}

void IRAM_ATTR reverb_spring() {

    // Set up buffer transfer
    if (current_buffer_owner != 2) {
        morph_to_16bit(2, 16384);
        spring_head = 0;
    }

    // --- WAKE UP BLOCK ---
    static bool was_in_menu = false;
    static bool button_locked = false;
    static bool skip_toggled = false;
    static bool last_frozen = false;
    
    // Debouncer Memory
    static int skip_integrator = 0;
    static bool prev_skip_stable = false;
    static int flip_integrator = 0;
    static bool prev_flip_stable = false;
    static bool flip_toggled = false;

    // Calibration Globals
    static int cal_min = 4095; 
    static int cal_max = 0; 
    static bool knob_moved = false;
    static int initial_earth = 0;
    static int s_earth = -1;
    static int boot_timer = 0;

    if (preset_mode) {
        was_in_menu = true;
    } else if (was_in_menu) {
        was_in_menu = false;
        
        // Inherit the OS state so buffer transfers remain frozen!
        last_frozen = audio_frozen_state;
        button_locked = audio_frozen_state;     
        skip_toggled = false;
        flip_toggled = false; 
        
        // Sync the edge detectors to the resting hardware state!
        bool skip_raw = (REG(GPIO_IN1_REG)[0] & 0x4);
        skip_integrator = skip_raw ? 300 : 0;
        prev_skip_stable = skip_raw;

        bool flip_raw = (REG(GPIO_IN1_REG)[0] & 0x8);
        flip_integrator = flip_raw ? 300 : 0;
        prev_flip_stable = flip_raw;

        // Reset calibration on boot
        cal_min = 4095;
        cal_max = 0;
        s_earth = -1;
        knob_moved = false;
        boot_timer = 0;
    }

    // INPUTS
    int raw_in = ADCREADER; 
    
    // EARTH AUTO-CALIBRATION
    REG(APB_SARADC_SAR1_PATT_TAB1_REG)[0] = (0x0C<<24) | (0x6C<<16);
    uint32_t fifo_data = REG(I2S_FIFO_RD_REG)[0];
    int channel = (fifo_data >> 12) & 0xF; 
    int value   = (fifo_data & 0xFFF);     
    
    if (channel == 0) {
        if (s_earth == -1) s_earth = value;
        else s_earth += (value - s_earth) >> 4;
    }

    if (boot_timer < 2000) {
        boot_timer++;
        initial_earth = s_earth;
    }

    if (!knob_moved && boot_timer >= 2000) {
        int drift = s_earth - initial_earth;
        if (drift < 0) drift = -drift;
        if (drift > 300) {
            knob_moved = true; 
        }
    }

    if (knob_moved) {
        if (s_earth < cal_min) cal_min = s_earth;
        if (s_earth > cal_max) cal_max = s_earth;
    } else {
        cal_min = initial_earth;
        cal_max = initial_earth;
    }

    int range = cal_max - cal_min;
    if (range < 50) range = 50;
    int reading = s_earth - cal_min;
    if (reading < 0) reading = 0;
    
    int earth_cal = (reading * 255) / range;
    if (!knob_moved) earth_cal = 0; // Default to brightest if unplugged
    
    // EARTH = DAMPENING
    // Map the 0-255 calibrated value to the 1-4 bitshift range
    int damp_shift = 1 + (earth_cal >> 6); 

    // Pre-amp
    int ac_in = raw_in - 2048;
    ac_in = (ac_in * 3);

    // LFO (Surf vs Lush)
    // FLIP is a latching switch
    bool flip_raw = (REG(GPIO_IN1_REG)[0] & 0x8);
    if (flip_raw) { if (flip_integrator < 300) flip_integrator++; }
    else          { if (flip_integrator > 0) flip_integrator--; }
    
    bool flip_stable = (flip_integrator > 250);
    
    // Only fire exactly when the gate goes from LOW to HIGH
    if (flip_stable && !prev_flip_stable) {
        flip_toggled = !flip_toggled;
    }
    prev_flip_stable = flip_stable;

    bool surf_mode = flip_toggled;

    // Surf: Fast Drip
    int lfo_inc = surf_mode ? 800 : 50;

    if (lfo_dir == 1) {
        lfo_accum += lfo_inc;
        if (lfo_accum > 60000000) lfo_dir = -1; 
    } else {
        lfo_accum -= lfo_inc;
        if (lfo_accum < 1000) lfo_dir = 1;     
    }
    int lfo_val = lfo_accum >> 16; 

    // MULTI-TAP READ HEADS
    spring_head = (spring_head + 1) & 0x3FFF; 
    
    // LFO Modulation
    int total_mod = lfo_val;

    int depth_shift = surf_mode ? 2 : 4; 

    // Tap 1: Slap (Short)
    int t1_mod = total_mod >> depth_shift;
    int pos1 = (spring_head - 2500 - t1_mod) & 0x3FFF;
    //int16_t val1 = s_buf[pos1];
    int16_t val1 = buffer_16bit[pos1];

    // Tap 2: Body (Medium)
    int t2_mod = (total_mod >> depth_shift) + 50; // Slight offset
    int pos2 = (spring_head - 7000 - t2_mod) & 0x3FFF;
    //int16_t val2 = s_buf[pos2];
    int16_t val2 = buffer_16bit[pos2];

    // Tap 3: Tail (Long)
    int pos3 = (spring_head - 13000) & 0x3FFF;
    //int16_t val3 = s_buf[pos3];
    int16_t val3 = buffer_16bit[pos3];

    // Sum Taps
    int wet_sum = (val1 >> 2) + (val2 >> 2) + (val3 >> 1);

    // FEEDBACK LOOP
    
    // DAMPENING (Earth Control)
    feedback_lpf += (wet_sum - feedback_lpf) >> damp_shift; 
    
    // DC Block
    spring_dc_tracker += (feedback_lpf - (spring_dc_tracker >> 10));
    int clean_feedback = feedback_lpf - (spring_dc_tracker >> 10);

    // Tank Length
    // ~84% Feedback
    // Use Feddback Knob to add more feedback into self oscillation
    int regeneration = (clean_feedback * 215) >> 8;

    // Mix Input + Regeneration
    int mix_bus = ac_in + regeneration;

    // Tube Saturation
    int saturated_write = tube_drive(mix_bus);

    // BUTTON - LATCHING Overrides skip toggling
    if (audio_frozen_state != last_frozen) {
        last_frozen = audio_frozen_state;
        
        if (skip_toggled) {
            // If Skip had the buffer frozen, pressing the button unfreezes it
            skip_toggled = false;
            button_locked = false;
            audio_frozen_state = false; // Keep the variables in sync
            lamp = false;
        } else {
            button_locked = audio_frozen_state;
        }
    }

    // SKIP - MOMENTARY - Rising edge with debounce
    bool skip_raw = (REG(GPIO_IN1_REG)[0] & 0x4);
    if (skip_raw) { if (skip_integrator < 300) skip_integrator++; }
    else          { if (skip_integrator > 0) skip_integrator--; }
    
    bool skip_stable = (skip_integrator > 250);
    
    // Only fire from LOW to HIGH
    if (skip_stable && !prev_skip_stable) {
        // Only allow the toggle only if the Button isn't latched
        if (!button_locked) {
            skip_toggled = !skip_toggled;
        }
    }
    prev_skip_stable = skip_stable;

    // FREEZE LOGIC
    bool freeze = button_locked || skip_toggled;

    // LAMP
    if (freeze) { LAMP_ON; } 
    else        { LAMP_OFF; }

    // RECORD
    if (!freeze) {
        buffer_16bit[spring_head] = (int16_t)saturated_write;
    }

    // OUTPUT
    int wet_out = clean_feedback; 
    int final_out = ((raw_in - 2048) + wet_out) + 2048;
    
    if (final_out > 4095) final_out = 4095;
    if (final_out < 0) final_out = 0;

    DACWRITER(final_out);

    // ASH
    int ash_raw = wet_out + 2048;
    rev_ash_lpf += (ash_raw - rev_ash_lpf) >> 2;
    CLEAN_ASHWRITER(rev_ash_lpf);

    // YELLOW
    YELLOW_AUDIO(final_out);

    REG(I2S_CONF_REG)[0] &= ~(BIT(5)); 
    REG(I2S_INT_CLR_REG)[0]=0xFFFFFFFF;
    REG(I2S_CONF_REG)[0] |= (BIT(5)); 
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// ==========================================
// REVERB - GRANULAR -- NEW PRESET
// ==========================================
// a live granular processing mode
// turn up feedback via the hardware feedback knob
// change trigger rate with the speed knob
// the button is a latching freeze
// skip freezes the buffer momentarily while high
// flip is an octave up switch
// earth controls the grain size
// ash is wet audio at line level
// yellow is bit-crushed audio at line level

void IRAM_ATTR reverb_granular() {
    
    static int neb_write = 0;

    if (current_buffer_owner != 3) {
        morph_to_16bit(3, 30000);
        neb_write = 0;
    }

    // Grain States
    static int g1_start = 0;
    static int g1_timer = 0;
    static int g1_len = 1000;

    static int g2_start = 0;
    static int g2_timer = 0;
    static int g2_len = 1000;

    static int last_neb_y = -1;
    
    // Filter states
    static int32_t gran_mud_lpf = 0; 

    // Input state
    static int skip_integrator = 0;
    static bool skip_gate = false;
    
    // ============================
    // CONTROLS
    // ============================
    int raw_in = ADCREADER;
    int earth_raw = EARTHREAD;

    // SKIP (Freeze)
    if (SKIPPERAT) {
        if (skip_integrator < 2000) skip_integrator += 2;
    } else {
        if (skip_integrator > 0) skip_integrator -= 50; 
    }
    if (skip_integrator > 1500) skip_gate = true;
    else if (skip_integrator < 200) skip_gate = false;

    // FREEZE LOGIC (Global State + Skip Jack)
    bool freeze = audio_frozen_state || skip_gate;
    
    // FLIP (Octave Up)
    bool shimmer = FLIPPERAT;
    
    // LAMP
    if (freeze) { LAMP_ON; } 
    else        { LAMP_OFF; }

    // EARTH (Grain Size)
    int target_len = 500 + (earth_raw * 40); 

    // ============================
    // GRAIN ENGINE
    // ============================
    
    // --- VOICE 1 ---
    g1_timer++;
    if (g1_timer >= g1_len) {
        g1_timer = 0;
        g1_len = target_len; 
  
        // start at least 'g1_len' samples back to avoid catching the write head.
        int min_offset = 1000;
        if (shimmer) min_offset = g1_len + 2000; // Safety margin
        
        int r = (rand() % 14000) + min_offset;
        g1_start = neb_write - r;
        if (g1_start < 0) g1_start += 30000;
    }

    // Envelopes
    int fade_size = 400; 
    if (g1_len < 800) fade_size = g1_len / 2;

    int g1_gain = 4096; 
    if (g1_timer < fade_size) {
        g1_gain = (g1_timer * 4096) / fade_size;
    } else if (g1_timer > g1_len - fade_size) {
        g1_gain = ((g1_len - g1_timer) * 4096) / fade_size;
    }

    // --- VOICE 2 ---
    g2_timer++;
    if (g2_timer >= g2_len) {
        g2_timer = 0;
        g2_len = target_len;
        
        int min_offset = 1000;
        if (shimmer) min_offset = g2_len + 2000; 
        
        int r = (rand() % 14000) + min_offset;
        g2_start = neb_write - r;
        if (g2_start < 0) g2_start += 30000;
    }

    int g2_gain = 4096;
    if (g2_timer < fade_size) {
        g2_gain = (g2_timer * 4096) / fade_size;
    } else if (g2_timer > g2_len - fade_size) {
        g2_gain = ((g2_len - g2_timer) * 4096) / fade_size;
    }

    // ============================
    // READ HEADS
    // ============================
    
    int p1, p2;
    if (shimmer) {
        p1 = g1_start + (g1_timer * 2);
        p2 = g2_start + (g2_timer * 2);
    } else {
        p1 = g1_start + g1_timer;
        p2 = g2_start + g2_timer;
    }
    
    while (p1 >= 30000) p1 -= 30000;
    while (p2 >= 30000) p2 -= 30000;
    while (p1 < 0) p1 += 30000;
    while (p2 < 0) p2 += 30000;
    
    int16_t s1 = buffer_16bit[p1];
    int16_t s2 = buffer_16bit[p2];

    int32_t w1 = (s1 * g1_gain) >> 12;
    int32_t w2 = (s2 * g2_gain) >> 12;

    int32_t raw_wet_sum = (w1 + w2) >> 1; 


    // HIGH PASS FILTER
    gran_mud_lpf += (raw_wet_sum - gran_mud_lpf) >> 6;
    int32_t clean_wet_sum = raw_wet_sum - gran_mud_lpf;
    

    // WRITE BUFFER
    int ac_in = raw_in - 2048;
    ac_in = (ac_in * 3) >> 1; 

    if (!freeze) {
        int32_t mix_write = ac_in; 
        if (mix_write > 20000) mix_write = 20000;
        if (mix_write < -20000) mix_write = -20000;
        buffer_16bit[neb_write] = (int16_t)mix_write;
    }
    
    neb_write++;
    if (neb_write >= 30000) neb_write = 0;
    
    // ============================
    // OUTPUTS
    // ============================
    
    // MAIN (Dry + Wet)
    int32_t final_mix = ac_in + clean_wet_sum;
    if (final_mix > 20000) final_mix = 20000;
    if (final_mix < -20000) final_mix = -20000;
    
    pout = final_mix + 2048;
    if (pout > 4095) pout = 4095;
    if (pout < 0) pout = 0;
    DACWRITER(pout);

    // ASH (Wet Only)
    int32_t ash_wet = clean_wet_sum + 2048;
    if (ash_wet > 4095) ash_wet = 4095;
    if (ash_wet < 0) ash_wet = 0;
    ASHWRITER(ash_wet);
    
    // YELLOW
    YELLOW_AUDIO(ash_wet);

    REG(I2S_CONF_REG)[0] &= ~(BIT(5)); 
    REG(I2S_INT_CLR_REG)[0] = 0xFFFFFFFF;
    REG(I2S_CONF_REG)[0] |= (BIT(5)); 
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// ==========================================
// HARMONIZER -- NEW PRESET
// ==========================================
// Harmonizer that uses pitch tracking to generate harmonic over/under tones.
// Generates 3 delay taps based on harmonic ratios 
// A sine wave will have harmonic tones added based on flip/skip settings
// flip switches between harmonics (default) and subharmonics
// skip switches primes that generate the sub/harmonics
// earth controls pitch tracking: min (default) is clean tracking and increases LFO modulation towards max
// button is a latching parameter lock which ignores flip, skip and earth while lamp is lit
// yellow is a stepped harmonic LFO in time with the buffer and three taps

// BUFFER
//static int16_t prism_buffer[24000]; // 24k Buffer (~0.5 sec)
static int prism_head = 0;

// PITCH TRACKING STATE
static int prism_trk_val = 0;        // Low pass filter for tracking
static int prism_trk_timer = 0;      // Counter for period length
static int prism_stable_period = 400; // The detected pitch (in samples)
static int32_t prism_period_fine = 400 << 8; // High-res pitch (Fixed Point)
static bool prism_trk_state = false; // Zero-crossing state

// FILTERS
static int prism_input_dc = 2048; // DC Blocker state
static int prism_loop_dc = 0;     // Saturation DC Blocker
static int prism_out_lpf = 0;     // Smoothing output filter

// EARTH LFO
static int prism_shim_phase = 0;

// BUTTON
static bool prism_latch_active = false;
static bool last_frozen = false;

// Parameter Storage
static bool f_majestic = false;
static bool f_hell = false;
static int f_earth_val = 0;

// SATURATOR (Tanh)
int16_t prism_tanh(int32_t x) {
    if (x > 32000) return 32000;
    if (x < -32000) return -32000;
    if (x > -20000 && x < 20000) return (int16_t)x; // Linear region
    if (x > 0) return 20000 + ((x - 20000) / 3);    // Soft knee positive
    else return -20000 + ((x + 20000) / 3);         // Soft knee negative
}

// READ HEAD
// Handles wrapping and sub-sample interpolation
int16_t prism_read(int32_t delay_fine) {
    int32_t delay_int = delay_fine >> 8;
    int32_t frac = delay_fine & 0xFF; 
    
    // Calculate Read Position
    int32_t pos_a = prism_head - delay_int;
    
    // Safety Wrap
    while (pos_a < 0) pos_a += 24000;
    while (pos_a >= 24000) pos_a -= 24000;
    
    int32_t pos_b = pos_a - 1; 
    if (pos_b < 0) pos_b += 24000;
    
    int16_t val_a = buffer_16bit[pos_a];
    int16_t val_b = buffer_16bit[pos_b];
    
    // Linear Interpolation: val = A + (B-A)*frac
    int32_t mixed = (val_a * (256 - frac)) + (val_b * frac);
    return (int16_t)(mixed >> 8);
}

void IRAM_ATTR harmonizer() {

    // --- WAKE UP BLOCK ---
    static bool was_in_menu = false;
    if (preset_mode) {
        was_in_menu = true;
    } else if (was_in_menu) {
        was_in_menu = false;
        last_frozen = audio_frozen_state;
        prism_latch_active = audio_frozen_state;
    }

    if (current_buffer_owner != 4) {
        morph_to_16bit(4, 24000);
        prism_head = 0;
    }

    // ============================
    // HARDWARE INPUTS & SNAPSHOTS
    // ============================
    int audio_in = ADCREADER; 
    
    // Read Pins
    bool raw_btn = BUTTON_PRESSED;
    bool raw_majestic = SKIPPERAT; // Skip Jack = Primes
    bool raw_hell = FLIPPERAT;     // Flip Jack = Under/Over tones
    
    // Read Earth CV
    static int s_earth = 0;
    REG(APB_SARADC_SAR1_PATT_TAB1_REG)[0] = (0x0C<<24) | (0x6C<<16);
    uint32_t fifo_data = REG(I2S_FIFO_RD_REG)[0];
    int channel = (fifo_data >> 12) & 0xF; 
    int value   = (fifo_data & 0xFFF);     
    if (channel == 0) s_earth += (value - s_earth) >> 4; // Smoothing
    int raw_earth_val = (s_earth - 100);
    if (raw_earth_val < 0) raw_earth_val = 0;

    // Button Latch Logic
    if (audio_frozen_state != last_frozen) {
        last_frozen = audio_frozen_state;
        prism_latch_active = audio_frozen_state;
    }

    // LAMP
    if (prism_latch_active) { LAMP_ON; } else { LAMP_OFF; }

    // Parameter Multiplexer
    bool mode_majestic;
    bool mode_hell;
    int current_earth;

    if (prism_latch_active) {
        // Use Frozen Values
        mode_majestic = f_majestic;
        mode_hell = f_hell;
        current_earth = f_earth_val;
    } else {
        // Use Live Values & Update Freeze Buffer
        mode_majestic = raw_majestic;
        mode_hell = raw_hell;
        current_earth = raw_earth_val;
        
        f_majestic = raw_majestic;
        f_hell = raw_hell;
        f_earth_val = raw_earth_val;
    }

    int detune_depth = (current_earth * 1500) >> 12; // Scale Earth to Depth

    // ============================
    // PITCH TRACKING & LFO
    // ============================

    // Update LFO
    prism_shim_phase += 14; 
    if (prism_shim_phase > 4095) prism_shim_phase = 0;
    int lfo_tri = (prism_shim_phase > 2048) ? (4096 - prism_shim_phase) : prism_shim_phase;
    int lfo_signed = (lfo_tri << 1) - 2048;

    // Input Filtering (Low Pass for better tracking)
    prism_trk_val += (audio_in - prism_trk_val) >> 2; 
    int ac_sig = prism_trk_val - 2048;
    prism_trk_timer++;
    
    // Zero Crossing Detector with Hysteresis
    if (!prism_trk_state) {
        if (ac_sig > 60) { // Trigger High
            prism_trk_state = true;
            // Valid Audio Range: 20 samples (2.2kHz) to 1600 samples (27Hz)
            if (prism_trk_timer > 20 && prism_trk_timer < 1600) {
                // Hysteresis to reject noise
                int diff = prism_trk_timer - prism_stable_period;
                if (diff < 0) diff = -diff;
                if (diff > 2) prism_stable_period = prism_trk_timer;
            }
            prism_trk_timer = 0;
        }
    } else {
        if (ac_sig < -60) prism_trk_state = false; // Trigger Low
    }

    // ============================
    // ADAPTIVE INERTIA (Slew Limiting)
    // ============================
    int32_t target_fine = prism_stable_period << 8;
    if (mode_hell) {
        // HELL MODE: Heavy Slew (>>11). 
        // Multipliers amplify jitter, so need slow liquid movement
        prism_period_fine += (target_fine - prism_period_fine) >> 11;
    } else {
        // HEAVEN MODE: Moderate Slew (>>9).
        // Responsive tracking for divisors.
        prism_period_fine += (target_fine - prism_period_fine) >> 9;
    }

    // ============================
    // HARMONIC CALCULATOR
    // ============================
    int32_t len_a, len_b, len_c;
    const int32_t MAX_LEN = 5632000; // 22,000 samples (Safe Ceiling)

    if (mode_hell) {
        // --- Sub-Harmonics ---
        // Mystic: x2, x7, x11
        // Majestic: x2, x3, x5
        int m1 = 2;
        int m2 = mode_majestic ? 3 : 7;
        int m3 = mode_majestic ? 5 : 11;

        len_a = prism_period_fine * m1;
        len_b = prism_period_fine * m2;
        len_c = prism_period_fine * m3;
        
        // STAGGERED CEILINGS
        // Set voices to different maximums so they don't unison.
        if (len_a > MAX_LEN) len_a = MAX_LEN;
        if (len_b > MAX_LEN - 128000) len_b = MAX_LEN - 128000; // -500 samples
        if (len_c > MAX_LEN - 256000) len_c = MAX_LEN - 256000; // -1000 samples

    } else {
        // --- Upper Harmonics ---
        // Mystic: /2, /7, /11
        // Majestic: /2, /3, /5
        int d1 = 2;
        int d2 = mode_majestic ? 3 : 7;
        int d3 = mode_majestic ? 5 : 11;

        len_a = prism_period_fine / d1;
        len_b = prism_period_fine / d2;
        len_c = prism_period_fine / d3;
        
        // STAGGERED FLOORS
        // Ensure voices clamp to different minimums so they don't unison.
        if (len_a < 32) len_a = 32; 
        if (len_b < 64) len_b = 64; 
        if (len_c < 96) len_c = 96; 
    }

    // Apply Shimmer (Detune)
    int32_t mod_a = (lfo_signed * detune_depth) >> 4;
    int32_t mod_b = (-lfo_signed * detune_depth) >> 4; 
    int32_t mod_c = ((lfo_signed / 2) * detune_depth) >> 4; 

    // ============================
    // AUDIO IO
    // ============================
    
    // Read Taps
    int16_t samp_a = prism_read(len_a + mod_a);
    int16_t samp_b = prism_read(len_b + mod_b);
    int16_t samp_c = prism_read(len_c + mod_c);

    // Process Input & Write to Buffer
    prism_input_dc += (audio_in - prism_input_dc) >> 10;
    int16_t ac_in = (int16_t)(audio_in - prism_input_dc);
    
    // Soft Saturation before writing
    int16_t sat_in = prism_tanh(ac_in);
    prism_loop_dc += (sat_in - prism_loop_dc) >> 10; 
    int16_t clean_write = sat_in - prism_loop_dc;
    //prism_buffer[prism_head] = clean_write;
    buffer_16bit[prism_head] = clean_write;

    // Mix Output (100% Wet)
    int32_t wet_mix = samp_a + samp_b + samp_c;
    wet_mix = (wet_mix * 3600) >> 12; // Gain staging
    prism_out_lpf += (wet_mix - prism_out_lpf) >> 1; // Smooth highs
    
    int out = (prism_out_lpf >> 1) + 2048; 
    if (out > 4095) out = 4095;
    if (out < 0) out = 0;

    pout = out;
    CLEAN_ASHWRITER(pout); 
    DACWRITER(pout);
    
    // Yellow
    // Stepped wave for syncing with tap positions as steps
    // Use as modulation source
    int y_val = 0;
    int pos_a = (prism_head - (len_a>>8)); while(pos_a < 0) pos_a += 24000;
    int pos_b = (prism_head - (len_b>>8)); while(pos_b < 0) pos_b += 24000;
    int pos_c = (prism_head - (len_c>>8)); while(pos_c < 0) pos_c += 24000;
    
    if (pos_a < 12000) y_val += 1300;
    if (pos_b < 12000) y_val += 1300;
    if (pos_c < 12000) y_val += 1300;
    
    if (prism_latch_active && (y_val > 3000)) y_val = 4095; 

    YELLOW_AUDIO(y_val); 

    // Increment Write Head
    prism_head++;
    if (prism_head >= 24000) prism_head = 0;

    // Audio Interrupt Reset
    REG(I2S_CONF_REG)[0] &= ~(BIT(5)); 
    REG(I2S_INT_CLR_REG)[0]=0xFFFFFFFF;
    REG(I2S_CONF_REG)[0] |= (BIT(5)); 
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// ==========================================
// SATURATOR -- FINAL VERSION
// ==========================================
// 8 different distortion modes
// use button to cycle through modes
// lamp flashes the mode index selected
// skip and flip are toggle switches for different states
// use the mixing console below to balance volume between modes

void IRAM_ATTR saturator() { 
    
    // --- MIXING CONSOLE  ---
    // 128 = 1.0x (Unity) 
    // 256 = 2.0x (Boost)
    // 64  = 0.5x (Cut)
    const int16_t gain_compensation[8] = { 
        96,  // 1. Tube  
        72,  // 2. Fuzz  
        64,  // 3. Fold  
        256, // 4. Tape  
        320, // 5. Vinyl 
        200, // 6. MP3   
        384, // 7. Radio 
        72   // 8. RAT  
    };

    // --- STATE ---
    static int dist_mode = 0;   
    static bool last_frozen = false; 
    static int blink_queue = 0;
    static int blink_timer = 0;
    static int blink_state = 0;
    
    static int32_t dc_slow = 0; 
    static bool servo_ready = false;

    static int32_t dist_lpf = 0; 
    static uint32_t t = 0; 
    static int hp_mem = 0;
    static int lp_mem = 0;
    
    // TAPE STATE 
    static int16_t tape_flange[256]; 
    static uint8_t tape_f_ptr = 0;

    // RADIO STATE
    static int32_t svf_low = 0;
    static int32_t svf_band = 0;

    // RAT STATE
    static int32_t rat_slew = 0;
    static int32_t rat_tone = 0;

    // Earth Auto-Calibration States
    static int cal_min = 255;
    static int cal_max = 0;

    // Stompbox State
    static int flip_latched = 0; 
    static int skip_latched = 0;
    static bool flip_was_high = false;
    static bool skip_was_high = false;

    // INPUTS
    int audio_in = ADCREADER; 
    t++; 

    // --- EARTH AUTO-CALIBRATION ---
    int earth_raw = EARTHREAD; 
    
    if (earth_raw < cal_min) cal_min = earth_raw;
    if (earth_raw > cal_max) cal_max = earth_raw;
    
    int spread = cal_max - cal_min;
    int cv = earth_raw;
    
    if (spread > 10) {
        cv = ((earth_raw - cal_min) * 255) / spread;
    }

    // DC SERVO
    if (!servo_ready || dc_slow == 0) {
        dc_slow = audio_in << 12;
        servo_ready = true;
    } else {
        dc_slow += (audio_in << 12) - dc_slow >> 10;
    }
    int32_t signal = audio_in - (dc_slow >> 12);

    // MODE SELECTOR 
    if (audio_frozen_state != last_frozen) {
        last_frozen = audio_frozen_state;
        
        dist_mode++;
        if (dist_mode > 7) dist_mode = 0;
        
        blink_queue = dist_mode + 1;
        blink_timer = 0; blink_state = 0;
    }

    // STOMPBOX LOGIC
    bool flip_is_high = FLIPPERAT;
    if (flip_is_high && !flip_was_high) flip_latched = !flip_latched;
    flip_was_high = flip_is_high;

    bool skip_is_high = SKIPPERAT;
    if (skip_is_high && !skip_was_high) skip_latched = !skip_latched;
    skip_was_high = skip_is_high;

    // WAKE UP BLOCK
    static bool was_in_menu = false;
    if (preset_mode) {
        was_in_menu = true;
    } else if (was_in_menu) {
        was_in_menu = false;
        last_frozen = audio_frozen_state;
        flip_was_high = FLIPPERAT;
        skip_was_high = SKIPPERAT;
        cal_min = 255;
        cal_max = 0;
    }

    int32_t out = 0;
    bool flip_active = flip_latched; 
    bool skip_active = skip_latched; 

    switch (dist_mode) {
        
        // MODE 0: TUBE
        case 0: {
            int64_t gain = 256 + (cv * 150); 
            int64_t driven = (signal * gain) >> 8;
            int64_t limit = 1950; 
            int64_t abs_d = (driven > 0) ? driven : -driven;
            int64_t stage1 = (driven * limit) / (limit + abs_d);

            if (flip_active) {
                int64_t turbo = stage1 * 3; 
                if (turbo > 2000) turbo = 2000;
                if (turbo < -2000) turbo = -2000;
                out = (int32_t)turbo;
            } else {
                out = (int32_t)stage1;
            }
            if (skip_active) { lp_mem += (out - lp_mem) >> 2; out = lp_mem; }
            break;
        }

        // MODE 1: FUZZ
        case 1: {
            if (skip_active) { lp_mem += (signal - lp_mem) >> 3; signal = signal + lp_mem; }
            int bias = (cv - 128) * 32; 
            int fuzz_sig = (signal * 32) + bias; 
            int limit = 2048; 
            if (fuzz_sig > limit) fuzz_sig = limit;
            if (fuzz_sig < -limit) fuzz_sig = -limit;
            
            if (flip_active) { 
                if (fuzz_sig < 0) fuzz_sig = -fuzz_sig; 
                fuzz_sig -= 1024;  
                fuzz_sig = (fuzz_sig * 3) >> 1;   
            }
            out = fuzz_sig;
            break;
        }

        // MODE 2: FOLD
        case 2: {
            int drive = 256 + (cv * 64);
            int folded = (signal * drive) >> 8;
            int threshold = 2000; 
            for (int i=0; i<2; i++) {
                if (folded > threshold) folded = threshold - (folded - threshold); 
                if (folded < -threshold) folded = -threshold - (folded + threshold);
            }
            if (flip_active) folded = folded ^ ((cv << 4) & 0xFFF); 
            if (skip_active) { 
                if (folded > 500) folded = 1000; 
                else if (folded < -500) folded = -1000; 
                else folded = 0; 
            }
            out = folded;
            break;
        }

        // MODE 3: TAPE 
        case 3: {
            signal = signal << 1; 
            int drive = 300 + cv;
            int tape_sig = (signal * drive) >> 8;
            
            if (tape_sig > 1500) tape_sig = 1500 + ((tape_sig - 1500) >> 2);
            if (tape_sig < -1500) tape_sig = -1500 + ((tape_sig + 1500) >> 2);
            
            if (skip_active) {
                int lfo = (t >> 10) & 0xFF; 
                if ((t >> 18) & 1) lfo = 255 - lfo; 
                int drop_gain = 256 - (lfo >> 1); 
                tape_sig = (tape_sig * drop_gain) >> 8; 
            }
            if (flip_active) { 
                tape_flange[tape_f_ptr] = (int16_t)tape_sig;
                tape_f_ptr++; 
                int lfo = (t >> 9) & 0xFF;
                if ((t >> 17) & 1) lfo = 255 - lfo;
                uint8_t read_ptr = tape_f_ptr - (lfo >> 1); 
                int delayed = tape_flange[read_ptr];
                tape_sig = (tape_sig + delayed) >> 1; 
            }
            dist_lpf += (tape_sig - dist_lpf) >> 2;
            out = dist_lpf;
            break;
        }

        // MODE 4: VINYL
        case 4: {
            int vinyl_sig = signal;
            int threshold = 4095 - (cv >> 2); 
            if (skip_active) threshold -= 500; 
            
            int crackle_vol = 32; 
            if ((rand() & 4095) > threshold) { 
                int raw_crackle = (rand() & 1024) - 512;
                vinyl_sig += (raw_crackle * crackle_vol) >> 7; 
            }

            if (flip_active) { 
                hp_mem += (vinyl_sig - hp_mem) >> 2; 
                int32_t hp_sig = vinyl_sig - hp_mem;
                lp_mem += (hp_sig - lp_mem) >> 1; 
                vinyl_sig = lp_mem;
                
                vinyl_sig = (vinyl_sig * 3) >> 1; 
                if (vinyl_sig > 1500) vinyl_sig = 1500 + ((vinyl_sig - 1500) >> 3);
                if (vinyl_sig < -1500) vinyl_sig = -1500 - ((vinyl_sig + 1500) >> 3);
                
                vinyl_sig *= 4; 
            } else {
                hp_mem += (vinyl_sig - hp_mem) >> 4; vinyl_sig = vinyl_sig - hp_mem;
                lp_mem += (vinyl_sig - lp_mem) >> 2; vinyl_sig = lp_mem;
            }

            out = vinyl_sig * 2; 
            break;
        }

        // MODE 5: MP3
        case 5: {
            static int hold_sample = 0;
            static int timer = 0;
            static int packet_loss_timer = 0;
            static int16_t micro_buffer[3]; 
            static uint8_t micro_ptr = 0;

            int rate = 1 + (cv >> 5); 
            
            if (skip_active) {
                if (packet_loss_timer > 0) {
                    packet_loss_timer--;
                    micro_ptr = (micro_ptr + 1) % 3; 
                    out = micro_buffer[micro_ptr] >> 1; 
                    break; 
                } else if ((rand() & 255) < 3) { 
                    packet_loss_timer = 100 + (rand() & 2000); 
                }
            }

            timer++;
            if (timer >= rate) {
                hold_sample = signal;
                if (flip_active) hold_sample = (hold_sample / 64) * 64; 
                micro_buffer[micro_ptr] = hold_sample; 
                micro_ptr = (micro_ptr + 1) % 3;
                timer = 0;
            }
            out = hold_sample * 2; 
            break;
        }

        // MODE 6: RADIO
        case 6: {
            int32_t noisy_input = signal;

            if (flip_active) {
                noisy_input *= 16; 
                if (noisy_input > 3000) noisy_input = 3000;
                if (noisy_input < -3000) noisy_input = -3000;
                if (noisy_input > -800 && noisy_input < 800) noisy_input = 0; 
                noisy_input = noisy_input >> 2;
            }

            int tuning = 150 + (cv * 4); 
            int damp = 40; 
            
            svf_low  += (tuning * svf_band) >> 12;
            int32_t svf_high = noisy_input - svf_low - ((damp * svf_band) >> 8);
            svf_band += (tuning * svf_high) >> 12;

            int32_t radio_out = svf_band * 2; 

            if (skip_active) {
                if ((rand() & 255) > 230) {
                    radio_out += ((rand() & 4095) - 2048) >> 2; 
                }
            }

            out = radio_out;
            break;
        }

        // MODE 7: RAT
        case 7: {
            int32_t gain = 20 + (cv >> 4); 
            int32_t target = signal * gain;

            int32_t max_slew = 2000; 
            int32_t delta = target - rat_slew;
            if (delta > max_slew) delta = max_slew;
            if (delta < -max_slew) delta = -max_slew;
            
            int32_t op_amp_out = rat_slew + delta;
            rat_slew = op_amp_out; 

            int32_t threshold = flip_active ? 6000 : 2048;
            
            if (op_amp_out > threshold) op_amp_out = threshold;
            if (op_amp_out < -threshold) op_amp_out = -threshold;

            if (skip_active) {
                rat_tone += (op_amp_out - rat_tone) >> 3; 
                op_amp_out = rat_tone;
            }

            // Removed the `out = out * 2` blowout logic here!
            out = op_amp_out;
            break;
        }
    }

    // MIXING STAGE
    int32_t compensated = (out * gain_compensation[dist_mode]) >> 7;

    // OUTPUT CLIPPING
    int32_t wet_out_dac = compensated + 2048;
    if (wet_out_dac > 4095) wet_out_dac = 4095;
    if (wet_out_dac < 0) wet_out_dac = 0;

    // LAMP
    bool lamp_busy = false;
    if (blink_queue > 0) {
        lamp_busy = true;
        blink_timer++;
        if (blink_state == 0) { 
             if (blink_timer > 3000) { blink_state = 1; blink_timer = 0; LAMP_ON; } 
             else { LAMP_OFF; }
        } else { 
             if (blink_timer > 3000) { blink_state = 0; blink_timer = 0; LAMP_OFF; blink_queue--; } 
             else { LAMP_ON; }
        }
    }
    
    if (!lamp_busy) {
        if (flip_latched) {
            LAMP_ON;
        } else if (abs(out) > 500) {
            LAMP_ON;
        } else {
            LAMP_OFF;
        }
    }

    DACWRITER(wet_out_dac);   
    ASHWRITER(wet_out_dac); 
    YELLOW_AUDIO(wet_out_dac); 
    
    REG(I2S_CONF_REG)[0] &= ~(BIT(5)); 
    REG(I2S_INT_CLR_REG)[0]=0xFFFFFFFF;
    REG(I2S_CONF_REG)[0] |= (BIT(5)); 
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////END

// ==========================================
// EXTERNAL SYNC -- NEW PRESET
// ==========================================
// a synced delay to an external clock
// quantizes the buffer based on clock
// skip is the clock, patch a clock signal here
// clock doesn't have to be regular signal
// flip reverses the play head
// button freezes the buffer
// buffer continues to quantize while frozen
// use as way to quantize loop from other preset
// earth is a record on/off toggle
// yellow passes the external clock
// ash is the wet audio

void IRAM_ATTR external_sync() {

   // --- STATE VARIABLES ---
    static int sync_head = 0;

    if (current_buffer_owner != 5) {
        morph_to_16bit(5, 65536);
        sync_head = 0;
    }

    // Clock Logic
    static int samples_since_clock = 0;
    static int sync_loop_len = 27562; // 0.625s / 96 BPM Default
    static bool gate_state = false;
    
    // Filter States
    static int sync_tape_lpf = 0;
    static int sync_earth_lpf = 0; 

    // Yellow Pulse State
    static int yellow_timer = 0;

    // Input States
    static int skip_integrator = 0;
    static bool is_frozen = false;
    static bool last_frozen = false;
    static int earth_last_state = 0;

    // CROSSFADE TAIL STATES
    static bool tail_active = false;
    static int tail_pos = 0;
    static int tail_timer = 0;
    static int tail_buffer_len = 65536;

    // SPLICE FADES
    static int fade_state = 0; 
    static int fade_timer_btn = 0;

    // --- WAKE UP BLOCK ---
    static bool was_in_menu = false;
    if (preset_mode) {
        was_in_menu = true;
    } else if (was_in_menu) {
        was_in_menu = false;
        last_frozen = audio_frozen_state;
        is_frozen = audio_frozen_state; 
        skip_integrator = 0;
        tail_active = false;
        fade_state = 0; // Reset start/stop fades
        
        // --- NEW: POWER OF TWO QUANTIZATION (Boot) ---
        int max_mult = 65536 / sync_loop_len;
        int power_of_two = 1;
        while ((power_of_two * 2) <= max_mult) {
            power_of_two *= 2;
        }
        current_buffer_len = power_of_two * sync_loop_len;
    }

    // ============================
    // INPUT READING
    // ============================
    int raw_in = ADCREADER;
    int earth_raw = EARTHREAD;

    // ============================
    // CONTROLS
    // ============================
    
    // Earth freezes buffer at 3.7V with hysteresis
    if (earth_last_state == 0) {
        if (earth_raw > TRIGGER_ON_THRESHOLD) {
            audio_frozen_state = !audio_frozen_state; // Toggle global freeze
            if (audio_frozen_state) { LAMP_ON; } else { LAMP_OFF; }
            earth_last_state = 1; 
        }
    } 
    if (earth_last_state == 1) {
        if (earth_raw < TRIGGER_OFF_THRESHOLD) {
            earth_last_state = 0; 
        }
    }

    // BUTTON
    if (audio_frozen_state != last_frozen) {
        last_frozen = audio_frozen_state;
        is_frozen = audio_frozen_state;

        // --- NEW: TRIGGER TAPE SPLICE FADES ---
        if (is_frozen) {
            fade_state = 1; // Fading OUT live audio, IN looped audio
            fade_timer_btn = 4000; // 90ms Tail (Post-roll)
        } else {
            fade_state = 2; // Fading IN live audio, OUT looped audio
            fade_timer_btn = 4000; // 90ms Pre-roll
        }
    }
    
    // LAMP (Freeze)
    if (is_frozen) { LAMP_ON; } 
    else           { LAMP_OFF; }

    // 2. EARTH SMOOTHING
    sync_earth_lpf += (earth_raw - sync_earth_lpf) >> 5; 

    // ============================
    // CLOCK SYNC (Integrator + Measure)
    // ============================
    
    samples_since_clock++;
    bool trigger_clock = false;

    // Integrator: Filter out noise on Skip Pin
    if (SKIPPERAT) {
        if (skip_integrator < 2000) skip_integrator += 10; // Fast Charge
    } else {
        if (skip_integrator > 0) skip_integrator -= 50;   // Fast Discharge
    }

    // Schmitt Trigger
    if (skip_integrator > 1000) {
        if (!gate_state) {
            gate_state = true;
            trigger_clock = true; // RISING EDGE
        }
    } else if (skip_integrator < 100) {
        gate_state = false;
    }

    // HANDLE CLOCK TRIGGER
    if (trigger_clock) {
        // Measure Interval (if valid speed)
        if (samples_since_clock > 150) { 
            int new_loop_len = samples_since_clock;
            
            // Safety Limits
            if (new_loop_len > 29900) new_loop_len = 29900;
            if (new_loop_len < 100) new_loop_len = 100;

            // --- JITTER HYSTERESIS & CROSSFADE ---
            // Only jump if the tempo changed by more than 10 samples (~0.2ms)
            int diff = new_loop_len - sync_loop_len;
            if (diff < 0) diff = -diff; // Absolute value
            
            if (diff > 10) {
                tail_active = true;
                tail_timer = 2000; // ~45ms fade window
                tail_buffer_len = current_buffer_len; // Save the old wrap boundary
                
                // Calculate exactly where the read head WAS going to be before the jump
                int offset = (sync_earth_lpf * 16);
                tail_pos = sync_head - sync_loop_len - offset;
                while (tail_pos < 0) tail_pos += tail_buffer_len;
                while (tail_pos >= tail_buffer_len) tail_pos -= tail_buffer_len;
                
                sync_loop_len = new_loop_len; // Apply the new tempo
            }
        }
        samples_since_clock = 0;
        
        // Trigger Yellow Pulse
        yellow_timer = 2000; // ~40ms Pulse
    }
    
    // Safety Limits
    if (sync_loop_len > 29900) sync_loop_len = 29900;
    if (sync_loop_len < 100) sync_loop_len = 100;

    // --- QUANTIZE THE LOOP BOUNDARY ---
    if (trigger_clock) {
        // Find the maximum subdivision (1, 2, 4, 8, 16) that fits the 65k limit
        int max_mult = 65536 / sync_loop_len;
        int power_of_two = 1;
        while ((power_of_two * 2) <= max_mult) {
            power_of_two *= 2;
        }
        current_buffer_len = power_of_two * sync_loop_len;
    }

    // ============================
    // AUDIO ENGINE
    // ============================
    
    int ac_in = raw_in - 2048;
    ac_in = (ac_in * 3) >> 1; 
    
    int read_pos = sync_head - sync_loop_len;
    
    // Wrap Read Head
    while (read_pos < 0) read_pos += current_buffer_len;
    while (read_pos >= current_buffer_len) read_pos -= current_buffer_len;
    
    //int16_t delayed_sample = sync_buffer[read_pos];
    int16_t delayed_sample = buffer_16bit[read_pos];

    // APPLY CROSSFADE
    // --- APPLY CROSSFADE ---
    if (tail_active) {
        int32_t t_samp = buffer_16bit[tail_pos];
        int32_t d_samp = delayed_sample;
        
        // Linear crossfade
        delayed_sample = (int16_t)(((t_samp * tail_timer) + (d_samp * (4000 - tail_timer))) / 4000);
        
        // Advance the tail playhead
        if (FLIPPERAT) tail_pos++; else tail_pos--;
        while (tail_pos < 0) tail_pos += tail_buffer_len;
        while (tail_pos >= tail_buffer_len) tail_pos -= tail_buffer_len;
        
        tail_timer--;
        if (tail_timer <= 0) tail_active = false;
    }
    
    // Write if not frozen OR recording crossfade tail
    if (!is_frozen || fade_state == 1) {
        sync_tape_lpf += (ac_in - sync_tape_lpf) >> 2;
        int16_t new_audio = (int16_t)sync_tape_lpf;
        
        if (fade_state == 1 && fade_timer_btn > 0) {
            // Freezing (Post-roll): Fade from Live down to Loop
            int16_t old_audio = buffer_16bit[sync_head];
            buffer_16bit[sync_head] = (int16_t)(((new_audio * fade_timer_btn) + (old_audio * (4000 - fade_timer_btn))) / 4000);
            
            fade_timer_btn--;
            if (fade_timer_btn <= 0) fade_state = 0;
        } 
        else if (fade_state == 2 && fade_timer_btn > 0) {
            // Unfreezing (Pre-roll): Fade from Loop up to Live
            int16_t old_audio = buffer_16bit[sync_head];
            buffer_16bit[sync_head] = (int16_t)(((new_audio * (4000 - fade_timer_btn)) + (old_audio * fade_timer_btn)) / 4000);
            
            fade_timer_btn--;
            if (fade_timer_btn <= 0) fade_state = 0;
        } 
        else {
            // Normal live overwrite
            buffer_16bit[sync_head] = new_audio;
        }
    }

    // Move Head
    if (FLIPPERAT) sync_head++;
    else sync_head--;

    // --- QUANTIZE LOOP LENGTH ---
    if (sync_head >= current_buffer_len) sync_head = 0;
    if (sync_head < 0) sync_head = current_buffer_len - 1;
    
    // ============================
    // OUTPUTS
    // ============================
    
    int32_t mix_out = delayed_sample;
    mix_out = (mix_out * 3) >> 1; 
    if (mix_out > 20000) mix_out = 20000;
    if (mix_out < -20000) mix_out = -20000;
    
    pout = mix_out + 2048;
    if (pout > 4095) pout = 4095;
    if (pout < 0) pout = 0;
    
    DACWRITER(pout);
    ASHWRITER(pout);
    
    // ============================
    // YELLOW (CLOCK OUT)
    // ============================
    
    if (yellow_timer > 0) {
        YELLOW_PULSE(4095);
        yellow_timer--;
    } else {
        YELLOW_PULSE(0); 
    }

    current_buffer_head = sync_head; // Update the OS with the tape splice location!

    REG(I2S_CONF_REG)[0] &= ~(BIT(5)); 
    REG(I2S_INT_CLR_REG)[0] = 0xFFFFFFFF;
    REG(I2S_CONF_REG)[0] |= (BIT(5)); 
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////END

// ==========================================
// SCRAMBLER --- NEW PRESET
// ==========================================
// 
// a live stutter/glitch effect
// dual buffer architecture that writes to one and plays the other
// playback buffer is divided into segments
// earth selects the segment
// the button freezes the buffer
// skip switches between 16 (default) and 3 segments of the buffer
// flip switched on (when high) random skipping 
// ash is the wet audio out at line level
// yellow is a bit crushed timbre to mix with ash


void IRAM_ATTR scrambler() { 

    morph_to_8bit(); //needed for buffer translation
    
    // --- CONFIG ---
    #define SCRAM_MAX 131000 //changed from 96000 
    
    // GEOMETRY
    static int active_segs = 16;   
    static int pending_segs = 16;  
    static int startup_seg = 0;
    static int current_seg = 0;
    static int next_seg = -1;      
    
    // AUDIO
    static int write_head = 0;
    static int local_head = 0; 
    static int main_vol = 256;      
    static uint32_t lock_timer = 0;

    // YELLOW STATE
    static int yellow_err = 0; 

    // FLIP CHAOS
    static uint32_t chaos_timer = 0;
    static uint32_t chaos_next = 1000;
    static bool chaos_gate = true; 
    static int chaos_vol = 256;    
    static uint32_t seed = 12345;  
    
    // Calibration
    static int cal_min = 4095;
    static int cal_max = 0;
    static bool knob_moved = false;
    static int initial_earth = 0;

    // Controls
    static bool last_frozen = false;
    static bool freeze = false;
    static bool prev_skip = false;
 

    // --- WAKE UP  ---
    // --- INPUTS ---
    static int s_earth = -1;
    static int boot_timer = 0;

    // --- WAKE UP  ---
    static bool was_in_menu = false;
    if (preset_mode) {
        was_in_menu = true;
    } else if (was_in_menu) {
        was_in_menu = false;
        last_frozen = audio_frozen_state;
        freeze = audio_frozen_state; // Inherit the buffer protection!
        
        cal_min = 4095;
        cal_max = 0;
        s_earth = -1;
        knob_moved = false;
        boot_timer = 0;

        // Randomly pick a segment (0-15) and force it as the starting point!
        startup_seg = rand() % 16;
        current_seg = startup_seg;
        next_seg = -1;
        
        prev_skip = SKIPPERAT;
    }

    // INPUTS
    int audio_in = ADCREADER; 

    REG(APB_SARADC_SAR1_PATT_TAB1_REG)[0] = (0x0C<<24) | (0x6C<<16);
    uint32_t fifo_data = REG(I2S_FIFO_RD_REG)[0];
    int channel = (fifo_data >> 12) & 0xF;
    int value = (fifo_data & 0xFFF);

    if (channel == 0) {
        if (s_earth == -1) s_earth = value;
        else s_earth += (value - s_earth) >> 4;
    }

    if (boot_timer < 2000) {
        boot_timer++;
        initial_earth = s_earth;
    }

    // --- CONTROL LOGIC ---
    
    // FREEZE
    if (audio_frozen_state != last_frozen) {
        last_frozen = audio_frozen_state;
        freeze = audio_frozen_state;
    }

    // SKIP (MODE TOGGLE)
    if (SKIPPERAT && !prev_skip) {
        if (pending_segs == 16) pending_segs = 3;
        else                    pending_segs = 16;
        next_seg = 0; 
        lock_timer = 0;
    }
    prev_skip = SKIPPERAT;

    // EARTH LOGIC
    if (!knob_moved && boot_timer >= 2000) {
        int drift = s_earth - initial_earth;
        if (drift < 0) drift = -drift;
        if (drift > 300) {
            knob_moved = true;
        }
    }

    if (knob_moved) {
        if (s_earth < cal_min) cal_min = s_earth;
        if (s_earth > cal_max) cal_max = s_earth;
    } else {
        cal_min = initial_earth;
        cal_max = initial_earth;
    }

    int range = cal_max - cal_min;
    if (range < 50) range = 50; 
    int reading = s_earth - cal_min;
    if (reading < 0) reading = 0;

    int earth_norm = (reading * 255) / range;

    lock_timer++;
    if (lock_timer > 1500) { 
        int candidate = 0;
        if (earth_norm < 12) candidate = 0; 
        else {
            candidate = (earth_norm * (active_segs + 1)) >> 8; 
            if (candidate >= active_segs) candidate = active_segs - 1;
        }

        // Force the random segment until the knob is physically turned
        if (!knob_moved) {
            candidate = startup_seg; 
        }

        if (candidate != current_seg && next_seg == -1) {
            next_seg = candidate;
            lock_timer = 0; 
        }
    }
    
    // CHAOS LOGIC
    if (FLIPPERAT) {
        chaos_timer++;
        if (chaos_timer > chaos_next) {
            chaos_timer = 0;
            seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
            if (chaos_gate) {
                chaos_gate = false;
                chaos_next = (seed % 3000) + 500; 
            } else {
                chaos_gate = true;
                chaos_next = (seed % 10000) + 2000; 
            }
        }
    } else {
        chaos_gate = true; 
    }
    
    if (chaos_gate) { if (chaos_vol < 256) chaos_vol += 8; } 
    else            { if (chaos_vol > 0) chaos_vol -= 8; }


    // --- AUDIO ENGINE ---
    
    int seg_len = SCRAM_MAX / active_segs;
    int play_len = seg_len - 100;
    const int XFADE = 1600;

    // TRANSITION DUCKING
    if (next_seg != -1) {
        main_vol -= 4; 
        if (main_vol <= 0) {
            main_vol = 0;
            active_segs = pending_segs;
            if (next_seg >= active_segs) next_seg = active_segs - 1;
            current_seg = next_seg; 
            next_seg = -1;
            local_head = 0; 
        }
    } else {
        if (main_vol < 256) main_vol += 4;
        if (main_vol > 256) main_vol = 256;
    }

    // WRITE
    if (!freeze) {
        dellius(write_head, audio_in, false); 
        write_head++;
        if (write_head >= SCRAM_MAX) write_head = 0;
    }

    // READ (Crossfade)
    int read_base = current_seg * seg_len; 
    int raw_out = 2048;

    if (local_head > (play_len - XFADE)) {
        int dist = local_head - (play_len - XFADE);
        int t = (dist * 255) / XFADE;
        int pos_a = read_base + local_head;
        int pos_b = read_base + dist; 
        
        if (pos_a >= SCRAM_MAX) pos_a = 0;
        if (pos_b >= SCRAM_MAX) pos_b = 0;

        int samp_a = dellius(pos_a, 0, true) - 2048;
        int samp_b = dellius(pos_b, 0, true) - 2048;
        
        int mix_ac = ((samp_a * (255-t)) + (samp_b * t)) >> 8;
        raw_out = mix_ac + 2048;
    } else {
        int pos = read_base + local_head;
        if (pos >= SCRAM_MAX) pos = 0;
        raw_out = dellius(pos, 0, true);
    }

    // OUTPUT
    int final_ac = raw_out - 2048;
    final_ac = (final_ac * main_vol) >> 8;
    final_ac = (final_ac * chaos_vol) >> 8;
    int wet_out = final_ac + 2048;

    // ADVANCE
    local_head++;
    if (local_head >= play_len) local_head = XFADE;

    // --- OUTPUTS ---
    
    DACWRITER(wet_out);

    // ASH OUT
    ASHWRITER(wet_out);

    // --- YELLOW OUTPUT ---
    YELLOW_AUDIO(wet_out);

    // --- LAMP VISUALS ---
    // hold ON if frozen
    // OFF when recording to buffer segments
    if (freeze) {
        LAMP_ON;  // Solid ON when locked
    } else {
        LAMP_OFF; // Dark when live
    }

    // CLEANUP
    REG(I2S_CONF_REG)[0] &= ~(BIT(5)); 
    REG(I2S_INT_CLR_REG)[0] = 0xFFFFFFFF;
    REG(I2S_CONF_REG)[0] |= (BIT(5)); 
}


/////////////////////////////////////////////////////////////////////////END

// ==========================================
// SAMPLER (ONE-SHOT) --- NEW PRESET
// ==========================================
//
// in coco pressing the button defines where the loop ends
// sampler is designed to set where the loop starts
// there are two modes: record and playback
// the lamp indicates the mode, off for armed to record, on for playback, and flashing while recording
// the button toggles between modes
// in recording mode, skip triggers the recording start
// in playback mode, skip triggers playback and will re-trigger
// when skip is high, it triggers recording/playback depending on mode
// the buffer will play through once and stop, unless retriggered before the stopping point
// patch antenna to skip, turn down sensitivity, and touch screw to trigger
// earth sets the playhead start position, by default it starts at the beginning of the buffer
// speed modulates the record and playback heads (varispeed recording)
// flip reverses the playback direction
// yellow is a pulse at the end of buffer, can be used for patching a loop with skip

#define PRE_ROLL_LEN 1000 
#define FADE_LEN 1000     

void IRAM_ATTR sampler() { 

    morph_to_8bit(); //needed for buffer translation

    int audio_in = ADCREADER; 
    
    // --- STATES ---
    static int skip_integrator = 0;
    static int skip_latch = 0;
    static int btn_latch = 0;
    static int btn_timer = 0;
    
    static int yellow_pulse_timer = 0;
    static int latched_start = 0; 

    // --- WAKE UP BLOCK --- // NEEDED FOR BUFFER TRANSFER
    static bool was_in_menu = false;
    if (preset_mode) {
        was_in_menu = true; // We are in the selector menu
    } else if (was_in_menu) {
        // We JUST exited the menu!
        was_in_menu = false;
        system_mode = true;  // Force Safe Playback mode!
        is_active = false;   // Kill any active recording
        btn_timer = 20000;   // Lockout the button for 0.5s to kill the Ghost Click!
        skip_integrator = 0; // Dump any floating noise
    }

    static int master_vol = 0; 
    static bool retrigger_pending = false; 

    // FLIP LATCH STATE
    static int flip_integrator = 0;
    static bool flip_is_reverse = false; 
    static bool flip_gate_active = false; 

    // PRE-ROLL
    static int16_t pre_roll[PRE_ROLL_LEN]; 
    static int pr_head = 0;



    // --- INPUTS ---
    int earth_raw = EARTHREAD;

    // --- EARTH CALIBRATION ---
    // optimized for quantum 2-6V
    int constrained_earth = earth_raw;
    if (constrained_earth < 56) constrained_earth = 56;
    if (constrained_earth > 160) constrained_earth = 160;
    int calculated_start = (constrained_earth - 56) * 1133;

    // --- CONTROLS ---

    // BUTTON 
    // Press to change between play/record mode
    if (BUTTON_PRESSED) {
        if (btn_latch == 0 && btn_timer == 0) {
            system_mode = !system_mode; 
            btn_latch = 1;
            btn_timer = 5000; 
        }
    } else {
        btn_latch = 0;
    }
    if (btn_timer > 0) btn_timer--;

    // SKIP (Trigger to Record/Play)
    if (SKIPPERAT) {
        if (skip_integrator < 2000) skip_integrator += 20;
    } else {
        if (skip_integrator > 0) skip_integrator -= 50; 
        if (skip_integrator < 0) skip_integrator = 0;
    }

    bool trigger_event = false;
    if (skip_integrator > 1500) {
        if (skip_latch == 0) {
            trigger_event = true;
            skip_latch = 1;
        }
    } else if (skip_integrator < 100) {
        skip_latch = 0;
    }

    // FLIP (reverse playback)
    if (FLIPPERAT) { if (flip_integrator < 2000) flip_integrator += 10; } 
    else           { if (flip_integrator > 0) flip_integrator -= 10; }

    bool current_flip_gate = (flip_integrator > 1500); 
    if (current_flip_gate && !flip_gate_active) {
        flip_is_reverse = !flip_is_reverse; 
        flip_gate_active = true; 
    } 
    else if (flip_integrator < 500) {
        flip_gate_active = false; 
    }


    // --- STATE MACHINE ---

    if (trigger_event) {
        latched_start = calculated_start;

        if (!is_active) {
            // START
            is_active = true;
            current_action_is_play = system_mode;
            
            if (current_action_is_play) {
                // PLAY START
                if (!flip_is_reverse) {
                    offset = latched_start;
                } else {
                    offset = (SAMPLE_LEN - 1) - latched_start;
                }
            } else {
                // RECORD START
                int r_idx = pr_head; 
                for(int i = 0; i < PRE_ROLL_LEN; i++) {
                    dellius(i, pre_roll[r_idx], false);
                    r_idx++;
                    if (r_idx >= PRE_ROLL_LEN) r_idx = 0;
                }
                offset = PRE_ROLL_LEN;
            }
            master_vol = 0; 
            retrigger_pending = false;

        } else {
            // RETRIGGER
            retrigger_pending = true;
        }
    }

    int output_sample = 2048; 

    // PROCESS EVENT
    if (is_active) {
        
        // --- DUCKING (for clicks) ---
        if (retrigger_pending) {
            master_vol -= 8; 
            if (master_vol <= 0) {
                master_vol = 0;
                if (current_action_is_play) {
                     if (!flip_is_reverse) offset = latched_start;
                     else                  offset = (SAMPLE_LEN - 1) - latched_start;
                } else {
                    offset = 0; 
                }
                retrigger_pending = false;
            }
        } else {
            if (master_vol < 256) master_vol += 8; 
        }

        // PLAYBACK
        if (current_action_is_play) {
            int raw_audio = dellius(offset, 0, true);
            int pos_gain = 256;
            
            // --- SYMMETRICAL FADES ---
            if (!flip_is_reverse) {
                // FORWARD
                if (offset < (latched_start + FADE_LEN)) {
                    if (offset >= latched_start) 
                        pos_gain = ((offset - latched_start) * 256) / FADE_LEN;
                }
                else if (offset > (SAMPLE_LEN - FADE_LEN)) {
                    pos_gain = ((SAMPLE_LEN - offset) * 256) / FADE_LEN;
                }
            } else {
                // REVERSE
                int r_start = (SAMPLE_LEN - 1) - latched_start;
                if (offset > (r_start - FADE_LEN)) {
                    if (offset <= r_start)
                        pos_gain = ((r_start - offset) * 256) / FADE_LEN;
                }
                else if (offset < FADE_LEN) {
                    pos_gain = (offset * 256) / FADE_LEN;
                }
            }
            
            int total_gain = (pos_gain * master_vol) >> 8;
            int signal_ac = raw_audio - 2048;
            output_sample = ((signal_ac * total_gain) >> 8) + 2048;

            // --- MOVEMENT ---
            // for flipping around earth in flip mode
            if (!flip_is_reverse) { 
                offset++; 
                if (offset >= SAMPLE_LEN) {
                    if (!retrigger_pending) { is_active = false; yellow_pulse_timer = 3000; }
                }
            } else {
                offset--; 
                if (offset < 0) {
                    if (!retrigger_pending) { is_active = false; yellow_pulse_timer = 3000; }
                }
            }
        } 
        
        // RECORDING
        else {
            output_sample = audio_in; 
            dellius(offset, audio_in, false); 
            
            offset++;
            if (offset >= SAMPLE_LEN) {
                is_active = false; 
                yellow_pulse_timer = 3000; 
            }
        }
    } else {
        master_vol = 0;
        
        if (!system_mode) {
            pre_roll[pr_head] = audio_in;
            pr_head++;
            if (pr_head >= PRE_ROLL_LEN) pr_head = 0;
        }
    }

    // --- LAMP VISUALS ---
    // Off = Armed for Recording
    // Strobe = Recording
    // Solid = Playback mode
    if (is_active) {
        if (current_action_is_play) {
            LAMP_ON; // Playing = Solid
        } else {
            // Recording = Strobe
            if ((offset >> 13) & 1) LAMP_ON; else LAMP_OFF;
        }
    } else {
        if (system_mode) LAMP_ON;  // Play Standby
        else             LAMP_OFF; // Rec Standby
    }

    // OUTPUTS
    pout = output_sample;
    ASHWRITER(pout); 
    DACWRITER(pout);
    
    // YELLOW
    // end of buffer trigge
    if (yellow_pulse_timer > 0) {
        YELLOW_PULSE(4095);
        yellow_pulse_timer--;
    } else {
        YELLOW_PULSE(0); 
    }
    
    REG(I2S_CONF_REG)[0] &= ~(BIT(5)); 
    REG(I2S_INT_CLR_REG)[0]=0xFFFFFFFF;
    REG(I2S_CONF_REG)[0] |= (BIT(5)); 
}

//////////////////////////////////////////////////////////////////////////////////////////END/////////////////////

// // ==========================================
// // MULTI-SLICE SAMPLER - NEW PRESET
// // ==========================================
// // based on the one-shot sampler
// // the buffer is divided into 4 segments
// // earth selects the segment of the buffer
// // stack antenna cv into earth
// // use patterns of antenna touching to select buffer segment
// // lamp shows which segment is selected (1-4) while in record mode
// // button toggles between record and playback mode
// // skip triggers a one shot record/playback and can be retriggered
// // flip triggers a one shot playback that cannot be re-triggered
// // ash is the wet audio at line level
// // yellow is the end of slice trigger

#define S4_TOTAL 131000 
#define S4_CNT 4
#define S4_LEN (S4_TOTAL / S4_CNT) 
#define XFADE_LEN 500 

// Auto-Cue
#define TRIM_THRESH 80   
#define PRE_BREATH  500  

// Thresholds
#define T_1_2  69
#define T_2_3  108
#define T_3_4  134
#define HYST   5 

static int32_t s4_earth_acc = 0; 
static int32_t dc_sum = 2048 * 4096; 
static int slice_starts[S4_CNT] = {0,0,0,0}; 

void IRAM_ATTR sampler_4x() { 

    morph_to_8bit(); 

    int audio_in = ADCREADER; 
    
    // DC Tracking
    dc_sum = dc_sum - (dc_sum >> 12) + audio_in;
    int clean_audio = (audio_in - (dc_sum >> 12)) + 2048;

    // --- STATES ---
    static int skip_integrator = 2000;
    static int skip_latch = 1;
    static int flip_integrator = 2000;
    static int flip_latch = 1;

    static int yellow_pulse_timer = 0;

    // Audio State
    static bool is_active = false;
    static int offset = 0; 
    static bool last_system_mode = true; 
    static bool one_shot_locked = false; 

    // Auto-Cue State
    static bool detection_armed = false; 
    static int play_timer = 0;           

    // Slice State
    static int current_slice = 0;   
    static int preview_slice = 0;   
    
    // Crossfade State
    static bool tail_active = false;
    static int tail_offset = 0;
    static int tail_slice = 0;
    static int tail_timer = 0; 

    // --- INPUTS ---
    int earth_raw = EARTHREAD;
    
    // EARTH SMOOTHING 
    int32_t target = earth_raw << 8;
    s4_earth_acc += (target - s4_earth_acc) >> 6;
    int earth_val = s4_earth_acc >> 8;
    
    if (preview_slice == 0) {
        if (earth_val > (T_1_2 + HYST)) preview_slice = 1;
    }
    else if (preview_slice == 1) {
        if (earth_val < (T_1_2 - HYST)) preview_slice = 0;
        else if (earth_val > (T_2_3 + HYST)) preview_slice = 2;
    }
    else if (preview_slice == 2) {
        if (earth_val < (T_2_3 - HYST)) preview_slice = 1;
        else if (earth_val > (T_3_4 + HYST)) preview_slice = 3;
    }
    else if (preview_slice == 3) {
        if (earth_val < (T_3_4 - HYST)) preview_slice = 2;
    }

    // --- CONTROLS ---

    // SYSTEM MODE (Tied directly to the OS Freeze State)
    bool system_mode = audio_frozen_state;

    if (system_mode != last_system_mode) {
        last_system_mode = system_mode;
        is_active = false;
        tail_active = false;
        one_shot_locked = false;
        yellow_pulse_timer = 0;
        
        bool s_raw = SKIPPERAT;
        skip_integrator = s_raw ? 2000 : 0;
        skip_latch = s_raw ? 1 : 0;
        
        bool f_raw = FLIPPERAT;
        flip_integrator = f_raw ? 2000 : 0;
        flip_latch = f_raw ? 1 : 0;
    }

    // SKIP
    if (SKIPPERAT) { if (skip_integrator < 2000) skip_integrator += 20; }
    else           { if (skip_integrator > 0) skip_integrator -= 50; }
    
    bool skip_rising = false;
    if (skip_integrator > 1500) {
        if (skip_latch == 0) { skip_rising = true; skip_latch = 1; }
    } else if (skip_integrator < 100) { skip_latch = 0; }

    // FLIP
    if (FLIPPERAT) { if (flip_integrator < 2000) flip_integrator += 2; } 
    else           { if (flip_integrator > 0) flip_integrator -= 50; }

    bool flip_rising = false;
    if (flip_integrator > 1500) {
        if (flip_latch == 0) { flip_rising = true; flip_latch = 1; }
    } else if (flip_integrator < 100) { flip_latch = 0; }

    // --- STATE MACHINE ---
    bool trigger_valid = false;
    bool is_one_shot = false;

    if (flip_rising) {
        if (!one_shot_locked) {
            trigger_valid = true;
            is_one_shot = true;
        }
    } 
    else if (skip_rising) {
        if (!one_shot_locked) {
            trigger_valid = true;
            is_one_shot = false;
        }
    }

    if (trigger_valid) {
        int old_slice = current_slice;
        current_slice = preview_slice;

        one_shot_locked = is_one_shot;

        if (!system_mode) {
            // RECORD START
            is_active = true;
            offset = 0;
            tail_active = false;
            slice_starts[current_slice] = 0; 
            detection_armed = true; 
        } 
        else {
            // PLAY START
            if (is_active) {
                tail_active = true;
                tail_slice = old_slice; 
                tail_offset = offset;       
                tail_timer = XFADE_LEN; 
            }
            is_active = true;
            offset = slice_starts[current_slice]; 
            play_timer = 0; 
        }
    }

    int output_sample = 2048; 

    // --- PROCESS AUDIO ---
    
    // A. RECORD MODE
    if (!system_mode && is_active) {
        if (detection_armed) {
            int signal_level = clean_audio - 2048;
            if (signal_level < 0) signal_level = -signal_level;
            
            if (signal_level > TRIM_THRESH) {
                int start_point = offset - PRE_BREATH;
                if (start_point < 0) start_point = 0;
                slice_starts[current_slice] = start_point;
                detection_armed = false; 
            }
        }

        int write_addr = (current_slice * S4_LEN) + offset;
        dellius(write_addr, clean_audio, false);
        output_sample = clean_audio; 
        
        offset++;
        if (offset >= S4_LEN) {
            is_active = false; 
            one_shot_locked = false;
            yellow_pulse_timer = 3000; 
        }
    }
    
    // B. PLAY MODE
    else if (system_mode) {
        int32_t mix_accumulator = 0;

        // MAIN VOICE
        if (is_active) {
            int read_addr = (current_slice * S4_LEN) + offset;
            int raw = dellius(read_addr, 0, true);
            
            int attack_gain = 256;
            if (play_timer < XFADE_LEN) attack_gain = (play_timer * 256) / XFADE_LEN;
            
            int release_gain = 256;
            if (offset > (S4_LEN - XFADE_LEN)) release_gain = ((S4_LEN - offset) * 256) / XFADE_LEN;
            
            int pos_gain = (attack_gain * release_gain) >> 8;
            mix_accumulator += ((raw - 2048) * pos_gain) >> 8;

            offset++; 
            play_timer++; 
            
            if (offset >= S4_LEN) {
                is_active = false;
                one_shot_locked = false; 
                yellow_pulse_timer = 3000; 
            }
        }

        // TAIL VOICE (for crossfade)
        if (tail_active) {
            int read_addr = (tail_slice * S4_LEN) + tail_offset;
            int raw = dellius(read_addr, 0, true);
            
            int tail_gain = (tail_timer * 256) / XFADE_LEN;
            
            int win_gain = 256;
            if (tail_offset > (S4_LEN - XFADE_LEN)) 
                win_gain = ((S4_LEN - tail_offset) * 256) / XFADE_LEN;
            
            int final_gain = (tail_gain * win_gain) >> 8;
            mix_accumulator += ((raw - 2048) * final_gain) >> 8;

            tail_offset++; 
            if (tail_offset >= S4_LEN) tail_active = false;
            
            tail_timer--;
            if (tail_timer <= 0) tail_active = false;
        }

        output_sample = mix_accumulator + 2048;
        if (output_sample > 4095) output_sample = 4095;
        if (output_sample < 0) output_sample = 0;
    }
    
    if (!is_active && !tail_active && system_mode) output_sample = 2048;
    
    // --- VISUALS ---
    if (!system_mode) {
        if (is_active) {
            if ((offset >> 10) & 1) LAMP_ON; else LAMP_OFF;
        } else {
            // Idle Blink
            offset++; 
            int cycle = (offset >> 13) & 0x1F; 
            if (cycle < ((preview_slice + 1) * 2)) {
                if (cycle & 1) LAMP_OFF; else LAMP_ON;
            } else {
                LAMP_OFF;
            }
        }
    } else {
        LAMP_ON; 
    }

    // --- OUTPUTS ---
    DACWRITER(output_sample);
    ASHWRITER(output_sample); 
    
    // YELLOW 
    if (yellow_pulse_timer > 0) {
        YELLOW_PULSE(4095);
        yellow_pulse_timer--;
    } else {
        YELLOW_PULSE(0); 
    }

    REG(I2S_CONF_REG)[0] &= ~(BIT(5)); 
    REG(I2S_INT_CLR_REG)[0]=0xFFFFFFFF;
    REG(I2S_CONF_REG)[0] |= (BIT(5)); 
}

//////////////////////////////////////////////////////////////////////////////////////////END/////////////////////

// ==========================================
// GRANULAR --- NEW PRESET
// ==========================================
// Sixteen polyphony granular engine based on sampler preset
// record into buffer
// button switches between record/play mode
// lamp indicates state. armed = off, recording = blinking, play = on
// trigger with skip
// send audio rate triggers with LFO mod to earth for slow motion
// earth modulates the grain start position
// flip switches the grain size, short grains when high
// ash is the wet audio out
// yellow is the bit crushed audio

// --- LOCAL SETTINGS ---
#define MAX_GRAINS 16       
#define GRAIN_CORE  3000    
#define FADE_SIZE   512     
#define GRAIN_SHORT (GRAIN_CORE + (FADE_SIZE * 2)) 
#define GRAIN_LONG  15000   

// Auto-Cue Settings
// Trims silence at beginning of buffer
#define TRIM_THRESH 80      // Silence Threshold
#define PRE_BREATH  1000    // Pre-roll for the Auto-Cue

struct Grain {
    bool active;     
    int position;    
    int life;        
    int total_life;  
};

static struct Grain grains[MAX_GRAINS];  

void IRAM_ATTR granular() { 

    // CALIBRATION GLOBALS
    static int cal_min = 4095; 
    static int cal_max = 0; 
    static int cue_start_idx = 0;  
    static bool seeking_cue = false; 
    static bool knob_moved = false;
    static int initial_earth = 0;
    static int s_earth = -1;
    static int boot_timer = 0;

morph_to_8bit(); //needed for buffer translation

 int audio_in = ADCREADER; 

// ------------------------------------------
// CONTROLS & WAKE UP
// ------------------------------------------
static int prev_skip = 0;
static bool last_frozen = false;

// NEW: Flip Latch States
static int flip_integrator = 0;
static bool prev_flip_stable = false;
static bool flip_latched = false;

// --- WAKE UP BLOCK ---
static bool was_in_menu = false;
if (preset_mode) {
    was_in_menu = true;
} else if (was_in_menu) {
    was_in_menu = false;
    
    // Inherit the global freeze state
    last_frozen = audio_frozen_state;
    system_mode = audio_frozen_state; 
    
    is_active = false;
    seeking_cue = false;
    
    cal_min = 4095;
    cal_max = 0;
    s_earth = -1;
    knob_moved = false;
    boot_timer = 0;
    
    // Sync hardware resting states
    prev_skip = SKIPPERAT;
    
    bool flip_raw = FLIPPERAT;
    flip_integrator = flip_raw ? 300 : 0;
    prev_flip_stable = flip_raw;
    flip_latched = false;
}

 // ------------------------------------------
 // EARTH CALIBRATION
 // ------------------------------------------
 REG(APB_SARADC_SAR1_PATT_TAB1_REG)[0] = (0x0C<<24) | (0x6C<<16);
 uint32_t fifo_data = REG(I2S_FIFO_RD_REG)[0];
 int channel = (fifo_data >> 12) & 0xF; 
 int value   = (fifo_data & 0xFFF);     
 
 if (channel == 0) {
     if (s_earth == -1) s_earth = value;
     else s_earth += (value - s_earth) >> 4;
 }

 if (boot_timer < 2000) {
     boot_timer++;
     initial_earth = s_earth;
 }
 
// ------------------------------------------
// FREEZE - Needed for Buffer Transfer
// ------------------------------------------
if (audio_frozen_state != last_frozen) {
    last_frozen = audio_frozen_state;
    system_mode = audio_frozen_state; 
    
    for(int i=0; i<MAX_GRAINS; i++) grains[i].active = false;
    if (!system_mode) offset = 0;
}

// ------------------------------------------
// TRIGGER LOGIC
// ------------------------------------------
int curr_skip = SKIPPERAT; 
bool skip_rising = (curr_skip && !prev_skip);
prev_skip = curr_skip;

// --- FLIP - LATCHING SWITCH - TOGGLE GRAIN SIZE
bool flip_raw = FLIPPERAT;

if (boot_timer < 2000) {
    flip_integrator = flip_raw ? 300 : 0;
    prev_flip_stable = flip_raw;
} else {
    if (flip_raw) { if (flip_integrator < 300) flip_integrator++; }
    else          { if (flip_integrator > 0) flip_integrator--; }
    
    bool flip_stable = (flip_integrator > 250);
    
    if (flip_stable && !prev_flip_stable) {
        flip_latched = !flip_latched;
    }
    prev_flip_stable = flip_stable;
}


 // ------------------------------------------
 // RECORD MODE (ARMED = LAMP OFF, RECORDING = BLINKING)
 // ------------------------------------------
 if (system_mode == false) {
     if (skip_rising && !is_active) {
         is_active = true; 
         offset = 0;
         
         // RESET AUTO-CUE
         cue_start_idx = 0; 
         seeking_cue = true; 
     }
     
     if (is_active) {
         // AUTO-CUE LOGIC
         if (seeking_cue) {
             int signal_level = audio_in - 2048;
             if (signal_level < 0) signal_level = -signal_level;
             
             if (signal_level > TRIM_THRESH) {
                 cue_start_idx = offset - PRE_BREATH;
                 if (cue_start_idx < 0) cue_start_idx = 0;
                 seeking_cue = false;
             }
         }

         pout = audio_in; 
         dellius(offset, audio_in, false); 
         if ((offset >> 11) & 1) LAMP_ON; else LAMP_OFF;
         offset++;
         if (offset >= SAMPLE_LEN) {
             offset = 0;
             is_active = false; 
             seeking_cue = false;
             LAMP_OFF;
         }
     } else {
         LAMP_OFF;
     }
 } 
 
 // ------------------------------------------
 // PLAY MODE
 // ------------------------------------------
 else {
     LAMP_ON; 
     
     // EARTH AUTO-CALIBRATION
     if (!knob_moved && boot_timer >= 2000) {
         int drift = s_earth - initial_earth;
         if (drift < 0) drift = -drift;
         if (drift > 300) {
             knob_moved = true; 
         }
     }

     if (knob_moved) {
         if (s_earth < cal_min) cal_min = s_earth;
         if (s_earth > cal_max) cal_max = s_earth;
     } else {
         cal_min = initial_earth;
         cal_max = initial_earth;
     }
     
     // SKIP TRIGGER GRAIN
     if (skip_rising) { 
         for (int i = 0; i < MAX_GRAINS; i++) {
             if (!grains[i].active) {
                 grains[i].active = true;
                //  if (FLIPPERAT) grains[i].total_life = GRAIN_SHORT; 
                //  else           grains[i].total_life = GRAIN_LONG;  

                if (flip_latched) grains[i].total_life = GRAIN_SHORT; 
                 else              grains[i].total_life = GRAIN_LONG;
                 grains[i].life = grains[i].total_life;
                 
                 // CALCULATE MAPPED POSITION
                 int range = cal_max - cal_min;
                 if (range < 50) range = 50; 
                 
                 int rel_earth = s_earth - cal_min;
                 if (rel_earth < 0) rel_earth = 0;
                 
                 // Available Audio Length
                 int usable_len = SAMPLE_LEN - cue_start_idx - 1000; 
                 if (usable_len < 1000) usable_len = 1000;

                 // Target = Start_Cue + (Percent * Length)
                 int target_pos = cue_start_idx + ((long)rel_earth * usable_len / range);

                 // If knob hasn't moved, bring to the cue point
                 if (!knob_moved) target_pos = cue_start_idx;

                 // ADD JITTER
                 int jitter = (t & 0x1FF) << 3; 
                 target_pos += jitter;

                 // APPLY FADE PRE-ROLL
                 grains[i].position = target_pos - FADE_SIZE;
                 
                 // Wrap Safety
                 if (grains[i].position < 0) grains[i].position += SAMPLE_LEN;
                 while (grains[i].position >= SAMPLE_LEN) grains[i].position -= SAMPLE_LEN;
                 
                 break; 
             }
         }
     }

     // MIX
     int32_t mix_accumulator = 0; 
     int active_count = 0;

     for (int i = 0; i < MAX_GRAINS; i++) {
         if (grains[i].active) {
             active_count++;
             int raw = dellius(grains[i].position, 0, true);
             int gain = 0;
             
             // ENVELOPE
             if (grains[i].total_life == GRAIN_SHORT) {
                 int elapsed = grains[i].total_life - grains[i].life;
                 if (elapsed < FADE_SIZE) gain = (elapsed * 256) / FADE_SIZE;
                 else if (grains[i].life < FADE_SIZE) gain = (grains[i].life * 256) / FADE_SIZE;
                 else gain = 256;
             } 
             else {
                 int halfway = grains[i].total_life / 2;
                 if (grains[i].life > halfway) gain = (grains[i].total_life - grains[i].life) * 256 / halfway; 
                 else gain = grains[i].life * 256 / halfway;
             }

             // AC MIX
             int ac_sample = raw - 2048;
             mix_accumulator += (ac_sample * gain) >> 8;
             
             grains[i].position++;
             if (grains[i].position >= SAMPLE_LEN) grains[i].position = 0;
             grains[i].life--;
             if (grains[i].life <= 0) grains[i].active = false;
         }
     }

// OUTPUT STAGE
     if (active_count > 0) {
         // 1. High-Pass Filter / DC Blocker
         // Carves out the low-end mud that builds up when 16 grains overlap
         static int32_t gran_dc_tracker = 0;
         gran_dc_tracker += (mix_accumulator - gran_dc_tracker) >> 5; 
         int32_t clean_mix = mix_accumulator - gran_dc_tracker;

         // 2. Gain Staging
         // Scale down slightly (75% volume) so 1-3 grains are perfectly clean
         int32_t signal = (clean_mix * 3) >> 2; 
         
         int32_t sat = signal;
         
         // 3. Asymptotic Tube Limiter (Threshold pushed higher to 1600)
         // Leaves quiet grains 1:1, perfectly asymptotes dense clouds to 2047
         if (signal > 1600) {
             int32_t over = signal - 1600;
             int32_t headroom = 447; // 2047 (DAC limit) - 1600 (Threshold)
             int32_t compressed = (over * headroom) / (headroom + over); 
             sat = 1600 + compressed;
         } else if (signal < -1600) {
             int32_t over = (-signal) - 1600;
             int32_t headroom = 447;
             int32_t compressed = (over * headroom) / (headroom + over);
             sat = -1600 - compressed;
         }
         
         pout = sat + 2048;
         if (pout > 4095) pout = 4095;
         if (pout < 0) pout = 0;
     } else {
         pout = 2048; 
     }
 }

 // ------------------------------------------
 // HARDWARE OUTPUT
 // ------------------------------------------

 // for jitter
 t++; 
 t = t & 0xFFFF;


 DACWRITER(pout); 
 CLEAN_ASHWRITER(pout);
 YELLOW_AUDIO(pout);

 REG(I2S_CONF_REG)[0] &= ~(BIT(5)); 
 REG(I2S_INT_CLR_REG)[0]=0xFFFFFFFF;
 REG(I2S_CONF_REG)[0] |= (BIT(5)); 
}

//////////////////////////////////////////////////////////////////////////////////////////END/////////////////////

// ==========================================
// PHASING --- NEW PRESET
// ==========================================
// record a loop and 4 playheads will slowly drift out of phase
// button switched between record and playback modes
// lamp indicates mode (off = armed, blinking = recording, on = playback)
// skip will trigger a one shot recording when in record mode
// skip will randomize the playheads in playback mode
// flip will reverse playback
// earth will modulate the phase alignment of the playheads
// feedback volume is clean audio output through main line out
// ash is the wet audio output at line level
// yellow is bit crushed audio at line level

#define PH_VOICES 4
#define PH_MAX_LEN 131000
#define PH_XFADE   1000   
#define PH_MAX_DELTA 4000 
#define SAT_LIMIT 2047      
#define SAT_FACTOR 1431655 

struct PhaseHead {
    int pos;         
    bool tail_active;
    int tail_pos;
    int tail_timer;
};

static struct PhaseHead p_heads[PH_VOICES];
static int ph_rec_len = 0; 
static int32_t ph_dc_sum = 2048 * 4096;

void IRAM_ATTR phasing() {

    morph_to_8bit(); //needed for buffer translation

    int audio_in = ADCREADER;

    // DC TRACKING
    ph_dc_sum = ph_dc_sum - (ph_dc_sum >> 12) + audio_in;
    int clean_audio = (audio_in - (ph_dc_sum >> 12)) + 2048;

    // --- STATES ---
    static int skip_integrator = 0;
    static int skip_latch = 0;
    static bool last_frozen = false;

    static bool ph_recording = true; 
    static bool ph_active = false;   
    static int offset = 0;           

    // --- WAKE UP ---
    static int flip_integrator = 0;
    static bool prev_flip_stable = false;
    static bool flip_latched = false;
    static int boot_timer = 0;

    static bool was_in_menu = false;
    if (preset_mode) {
        was_in_menu = true;
    } else if (was_in_menu) {
        was_in_menu = false;
        
        // Audio Engine Transfer Resets
        ph_recording = false;   
        ph_active = false;
        ph_rec_len = PH_MAX_LEN;
        skip_integrator = 0;
        skip_latch = 0;
        
        // Reset playheads 
        for(int i=0; i<PH_VOICES; i++) {
            p_heads[i].pos = 0;
            p_heads[i].tail_active = false;
        }
        
        last_frozen = audio_frozen_state;
        boot_timer = 2000;
        flip_latched = false;   
    }

    // --- INPUTS ---
    int earth_raw = EARTHREAD;

    // --- FLIP ---
    // Latching toggle
    bool flip_raw = FLIPPERAT;
    
    if (boot_timer > 0) {
        boot_timer--;
        // Sync integrator silently without allowing toggles during boot
        flip_integrator = flip_raw ? 300 : 0;
        prev_flip_stable = flip_raw;
    } else {
        // Debounce & Latch
        if (flip_raw) { if (flip_integrator < 300) flip_integrator++; }
        else          { if (flip_integrator > 0) flip_integrator--; }
        
        bool flip_stable = (flip_integrator > 250);
        
        // fire on rising edge
        if (flip_stable && !prev_flip_stable) {
            flip_latched = !flip_latched;
        }
        prev_flip_stable = flip_stable;
    }

    // latched state controls the playheads
    bool reverse_mode = flip_latched;

    // --- CONTROLS ---

    // BUTTON (Rec/Play Toggle)
    if (audio_frozen_state != last_frozen) {
        last_frozen = audio_frozen_state;
        
        if (ph_recording) {
            // REC -> PLAY
            ph_recording = false;
            if (offset == 0) ph_rec_len = PH_MAX_LEN; 
            else ph_rec_len = offset;
            
            if (ph_rec_len < PH_XFADE * 3) ph_rec_len = PH_XFADE * 3; 
            
            // Initialize Heads
            for(int i=0; i<PH_VOICES; i++) {
                p_heads[i].pos = 0;
                p_heads[i].tail_active = false;
            }
        } else {
            // PLAY -> REC
            ph_recording = true;
            ph_active = false; 
            offset = 0;
        }
    }

    // SKIP (Trigger)
    if (SKIPPERAT) { if (skip_integrator < 2000) skip_integrator += 2; }
    else           { if (skip_integrator > 0) skip_integrator -= 50; }
    
    bool trigger_rising = false;
    if (skip_integrator > 1500) {
        if (skip_latch == 0) { trigger_rising = true; skip_latch = 1; }
    } else if (skip_integrator < 100) { skip_latch = 0; }

    // --- PHASE LOGIC ---

    int delta = 1 + earth_raw; 
    if (delta > PH_MAX_DELTA) delta = PH_MAX_DELTA;

    int output_sample = 2048;

    // --- PROCESS ---

    if (ph_recording) {
        // --- RECORD MODE ---
        if (trigger_rising && !ph_active) {
            ph_active = true;
            offset = 0;
            flip_latched = false;
        }
        
        if (ph_active) {
            dellius(offset, clean_audio, false);
            if ((offset >> 11) & 1) LAMP_ON; else LAMP_OFF;
            
            offset++;
            if (offset >= PH_MAX_LEN) {
                ph_active = false;
                offset = 0; 
                LAMP_OFF;
            }
        } else {
            LAMP_OFF;
        }
        output_sample = clean_audio; 

    } else {
        // --- PLAY MODE ---
        
        // RANDOMIZE PLAYHEADS
        if (trigger_rising) {
            static uint32_t seed = 0xCAFEBABE;
            for(int i=0; i<PH_VOICES; i++) {
                 int my_len = ph_rec_len - (i * delta);
                 if (my_len < 2000) my_len = 2000;
                 seed = (seed * 1664525 + 1013904223); 
                 p_heads[i].pos = seed % my_len;
                 p_heads[i].tail_active = false; 
            }
        }

        int32_t mix = 0;
        
        for (int i=0; i<PH_VOICES; i++) {
            int my_len = ph_rec_len - (i * delta);
            if (my_len < 2000) my_len = 2000; 

            // MAIN VOICE
            int raw = dellius(p_heads[i].pos, 0, true);
            int gain = 256;
            
            // FADE IN
            if (p_heads[i].pos < PH_XFADE) {
                gain = (p_heads[i].pos * 256) / PH_XFADE;
            }
            
            mix += ((raw - 2048) * gain) >> 8;

            // TAIL VOICE
            if (p_heads[i].tail_active) {
                if (p_heads[i].tail_pos >= ph_rec_len) p_heads[i].tail_pos = 0;
                
                int t_raw = dellius(p_heads[i].tail_pos, 0, true);
                int t_gain = (p_heads[i].tail_timer * 256) / PH_XFADE;
                mix += ((t_raw - 2048) * t_gain) >> 8;
                
                if (reverse_mode) p_heads[i].tail_pos--; 
                else              p_heads[i].tail_pos++;
                
                if (p_heads[i].tail_pos >= ph_rec_len) p_heads[i].tail_pos = 0;
                if (p_heads[i].tail_pos < 0) p_heads[i].tail_pos = ph_rec_len - 1;

                p_heads[i].tail_timer--;
                if (p_heads[i].tail_timer <= 0) p_heads[i].tail_active = false;
            }

            // ADVANCE
            if (!reverse_mode) {
                p_heads[i].pos++;
                if (p_heads[i].pos >= my_len) {
                    p_heads[i].tail_active = true;
                    p_heads[i].tail_pos = p_heads[i].pos; 
                    if (p_heads[i].tail_pos >= ph_rec_len) p_heads[i].tail_pos = 0;
                    p_heads[i].tail_timer = PH_XFADE;
                    p_heads[i].pos = 0; 
                }
            } else {
                p_heads[i].pos--;
                if (p_heads[i].pos < 0) {
                    p_heads[i].tail_active = true;
                    p_heads[i].tail_pos = p_heads[i].pos; 
                    if (p_heads[i].tail_pos < 0) p_heads[i].tail_pos = ph_rec_len - 1;
                    p_heads[i].tail_timer = PH_XFADE;
                    p_heads[i].pos = my_len - 1;
                }
            }
        }
        
        // --- SATURATOR ---
        int32_t signal = mix; 
        if (signal > 6000) signal = 6000;
        if (signal < -6000) signal = -6000;
        int32_t abs_sig = (signal > 0) ? signal : -signal;
        int32_t sat = signal - ((signal * abs_sig) >> 15);
        output_sample = sat + 2048;
        if (output_sample > 4095) output_sample = 4095;
        if (output_sample < 0) output_sample = 0;
        
        LAMP_ON;
    }

    // --- OUTPUTS ---
    DACWRITER(output_sample);
    CLEAN_ASHWRITER(output_sample);
    YELLOW_AUDIO(output_sample);

    REG(I2S_CONF_REG)[0] &= ~(BIT(5)); 
    REG(I2S_INT_CLR_REG)[0]=0xFFFFFFFF;
    REG(I2S_CONF_REG)[0] |= (BIT(5)); 
}

//////////////////////////////////////////////////////////////////////////////////////////END/////////////////////

// ==========================================
// BYTEBEATS ORIGINAL
// ==========================================
// void IRAM_ATTR bytebeats() {
//  INTABRUPT
// // DACWRITER(((pout<<4)&0xFF0))
 
//  DACWRITER(((pout&0xFFF)))
//  gyo=ADCREADER
 
//  pout =BYTECODES;

//  if (FLIPPERAT) t++;
//  else t--;
//  if (SKIPPERAT)  {} else {} 
//  REG(I2S_CONF_REG)[0] &= ~(BIT(5)); 
//  adc_read = EARTHREAD;
//  ASHWRITER(adc_read); //rand()
//  REG(I2S_INT_CLR_REG)[0]=0xFFFFFFFF;
//  REG(I2S_CONF_REG)[0] |= (BIT(5)); //start rx
//  YELLOWERS(pout);
// }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

// ==========================================
// BYTEBEATS -- MODIFIED
// ==========================================
// Modified Bytebeats 
// slowed down
// added earth modulation
// added ash audio output
// button just toggles the lamp but has no sonic effect

void IRAM_ATTR bytebeats_mod() {
  
 // --- WAKE UP BLOCK ---
 static bool was_in_menu = false;
 if (preset_mode) {
     was_in_menu = true;
 } else if (was_in_menu) {
     was_in_menu = false;
     lamp = false; //set lamp to OFF on boot
     audio_frozen_state = false;
     REG(GPIO_OUT1_W1TC_REG)[0] = BIT(1); 
 }

 // CONTROLS
 int cv = EARTHREAD; // Controls Speed (Sample Rate)
 
 // CLOCK DIVIDER
 // set to 8kHz
 // Cafe runs at ~32kHz. Need to slow 't' down.
 // earth CV: low (unplugged) = 8kHz, high = 32kHz (Modern/Fast)
 static int speed_accumulator = 0;
 int speed_div = 4 - (cv >> 6); // maps earth to dividers 4, 3, 2, 1
 if (speed_div < 1) speed_div = 1;
 
 speed_accumulator++;
 if (speed_accumulator >= speed_div) {
     speed_accumulator = 0;
     // only advance time 't' periodically
     if (FLIPPERAT) t++; else t--; 
 }

 // GENERATE AUDIO
 // calculate the byte, but cast to int to prevent overflow issues during scaling
 int raw_byte = BYTECODES; 
 
 // BIT DEPTH SCALING
 // map 8-bit (0-255) to 12-bit (0-4095)
 // use (x & 0xFF) to ensure clean wrapping, then shift left by 4
 pout = (raw_byte & 0xFF) << 4; 

 // OUTPUT
 DACWRITER(pout);
 
 gyo=ADCREADER; // dummy read to keep timing
 adc_read = EARTHREAD;
 ASHWRITER(pout); 
 
 // Yellow bit crush
 YELLOW_AUDIO(pout);

  // HEARTBEAT
 REG(I2S_CONF_REG)[0] &= ~(BIT(5)); 
 REG(I2S_INT_CLR_REG)[0] = 0xFFFFFFFF;
 REG(I2S_CONF_REG)[0] |= (BIT(5));  
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

// ==========================================
// MEGABYTEBEATS --- NEW PRESET
// ==========================================
// 16 algorithmic beats
// earth controls variable inside bytebeat equations
// skip cycles through the equations
// flip plays the beat in reverse
// press button to parameter-lock flip and skip (freeze the beat)
// lamp indicates button toggled on
// ash is the wet audio out at line level
// yellow is the bit crushed audio 

void IRAM_ATTR megabytebeats() { 
    
    // --- STATE VARIABLES ---
    static int cal_min = 4095; 
    static int cal_max = 0; 
    static bool knob_moved = false;
    static int initial_earth = 0;
    static int s_earth = -1;
    static int boot_timer = 0;
    
    static int mode = 0;
    static int prev_skip = 0;
    static int speed_acc = 0;
    
    // Parameter Lock State
    static bool param_lock = false;
    static int btn_latch = 0;
    static bool locked_flip = false; 

    // --- WAKE UP BLOCK ---
    static bool was_in_menu = false;
    if (preset_mode) {
        was_in_menu = true;
    } else if (was_in_menu) {
        was_in_menu = false;
        
        cal_min = 4095;
        cal_max = 0;
        s_earth = -1;
        knob_moved = false;
        boot_timer = 0;
        
        // Sync to resting hardware state to prevent phantom equation skips!
        prev_skip = SKIPPERAT;
    }

    // INPUTS & CALIBRATION
    REG(APB_SARADC_SAR1_PATT_TAB1_REG)[0] = (0x0C<<24) | (0x6C<<16);
    uint32_t fifo_data = REG(I2S_FIFO_RD_REG)[0];
    int channel = (fifo_data >> 12) & 0xF; 
    int value   = (fifo_data & 0xFFF);     
    
    if (channel == 0) {
        if (s_earth == -1) s_earth = value;
        else s_earth += (value - s_earth) >> 4;
    }

    if (boot_timer < 2000) {
        boot_timer++;
        initial_earth = s_earth;
    }

    // --- EARTH AUTO-CALIBRATION (With Deadzone) ---
    if (!knob_moved && boot_timer >= 2000) {
        int drift = s_earth - initial_earth;
        if (drift < 0) drift = -drift;
        if (drift > 300) {
            knob_moved = true; 
        }
    }

    if (knob_moved) {
        if (s_earth < cal_min) cal_min = s_earth;
        if (s_earth > cal_max) cal_max = s_earth;
    } else {
        cal_min = initial_earth;
        cal_max = initial_earth;
    }

    // Normalize to 0-255 (Bytebeat Range)
    int range = cal_max - cal_min;
    if (range < 50) range = 50;
    int reading = s_earth - cal_min;
    if (reading < 0) reading = 0;
    
    int cv = (reading * 255) / range;
    if (!knob_moved) cv = 0; // Force to base math until deliberately moved

    // PARAMETER LOCK LOGIC
    if (BUTTON_PRESSED) {
        if (btn_latch == 0) {
            param_lock = !param_lock; // Toggle Lock
            
            // If locking, capture the current Flip state to freeze it
            if (param_lock) {
                locked_flip = (FLIPPERAT);
            }
            
            btn_latch = 10000; // Debounce
        }
    } else {
        if (btn_latch > 0) btn_latch--;
    }

    // MODE SELECTOR (Gated by Lock)
    // Only allow mode changes if NOT locked
    if (!param_lock) {
        if (SKIPPERAT && !prev_skip) {
            mode++;
            if (mode > 15) mode = 0;
        }
    }
    prev_skip = SKIPPERAT;

    // CLOCK & DIRECTION
    // Determine Direction based on Lock State
    bool direction_forward = true;
    
    if (param_lock) {
        // If locked, use the frozen state
        direction_forward = locked_flip;
    } else {
        // If unlocked, read hardware switch
        direction_forward = (FLIPPERAT);
    }

    speed_acc++;
    if (speed_acc >= 4) { // 8kHz Divider
        speed_acc = 0;
        if (direction_forward) t++; else t--; 
    }

    // THE 16 EQUATIONS
    int output = 0;
 
    switch (mode) {
        // --- CLASSICS ---
        case 0: output = t * ((t>>12 | t>>8) & (63 + (cv>>2)) & t>>4); break;
        case 1: output = (t & (t >> (4 + (cv >> 5)))) * (t >> 4); break;
        case 2: { int p = (cv >> 4) + 1; output = (t >> 6 | t | t >> (t >> 16)) * p + ((t >> 11) & 7); } break;
        case 3: output = ((t * (cv >> 4)) & (t >> 10)) | (t * 3 & t >> 7); break;

        // --- MELODIC ---
        case 4: output = t * ((42 + (cv>>2)) & t >> 10); break;
        case 5: output = (t >> (10 + (cv>>6)) ^ t >> 11) % 5 * t; break;
        case 6: output = (t * 5 & t >> 7) | (t * 3 & t >> 10) | (t * (cv >> 4) & t >> 13); break;
        case 7: { int gate = 9 + (cv >> 4); output = (t*gate & t>>4 | t*5 & t>>7 | t*3 & t/1024) - 1; } break;

        // --- RHYTHMIC ---
        case 8: output = (t * (t>>5 | t>>8)) >> (t>>(16 - (cv>>5))); break;
        case 9: output = (t>>14 ? t : t*(2+(cv>>6))) & (t>>7); break;
        case 10: output = (t & 4096) ? ((t * (t ^ t % (255 - (cv>>1))) / 256)) : 0; break;
        case 11: output = (t>>6 | t | t>>(t>>(16 - (cv>>5)))) * 10 + ((t>>11) & 7); break;

        // --- CHAOS ---
        case 12: output = (t * 5) & (t >> 7) | (t * (3+(cv>>4))) & (t >> 10); break;
        case 13: output = t * ((t>>5 | t>>8) & (63 - (cv>>3))); break;
        case 14: output = ((t*(t>>8 | t>>9) & 46 & t>>8)) ^ (t & t>>13 | t>>6); break;
        case 15: output = (t * (t >> 4 & t >> 8) & (127 - (cv >> 2))) ^ (t * 3); break;
    }

    // OUTPUT
    int pout = (output & 0xFF) << 4; // Scale 8-bit to 12-bit
 
    CLEAN_ASHWRITER(pout); 
    DACWRITER(pout);

    // Yellow Out
    YELLOW_AUDIO(pout);
    
    // Lamp Logic: ON if Locked, OFF if Unlocked
    if (param_lock) { LAMP_ON; } else { LAMP_OFF; }
 
    REG(I2S_CONF_REG)[0] &= ~(BIT(5)); 
    REG(I2S_INT_CLR_REG)[0]=0xFFFFFFFF;
    REG(I2S_CONF_REG)[0] |= (BIT(5)); 
}

//////////////////////////////////////////////////////////////////////////////////////////END/////////////////////

// ==========================================
// ARCADE --- NEW PRESET
// ==========================================
// Generative 8-bit music system
// the music system includes drums, bass, an arp and a lead voice
// earth selects the game cartridge (4 available)
// button switches to alt music mode of the cartridge
// skip switches on melody
// flip switches on arpeggiator
// when both flip and skip are high, a lead melody is enabled
// ash is wet audio at line level
// yellow is bit crushed audio


// --- MIXER CONSOLE ---
// Adjust these values (0 - 128) to balance the parts
// 64 = 100% Volume
#define MIX_LEAD  64
#define MIX_ARP   64
#define MIX_BASS  64
#define MIX_DRUM  32  

// --- GLOBAL SCALES ---
const int scale_maj[] = {65, 73, 82, 98, 110, 130, 146, 164}; 
const int scale_min[] = {65, 77, 82, 92, 103, 116, 123, 130}; 
const int scale_blues[] = {65, 77, 87, 92, 97, 116, 130, 154}; 
const int scale_dim[] = {65, 77, 82, 92, 103, 110, 123, 130}; 
const int scale_dorian[] = {65, 73, 77, 87, 98, 110, 120, 130}; 
const int scale_whole[] = {65, 73, 82, 92, 103, 116, 130, 146}; 
const int scale_chrom[] = {65, 69, 73, 77, 82, 87, 92, 98};   

// --- STATE GLOBALS ---
static uint32_t arcade_t = 0;    
static int arcade_cart = 0;      
static int arcade_yellow_lpf = 0;

static uint32_t p1_phase = 0;
static uint32_t p2_phase = 0;
static uint32_t tri_phase = 0;
static uint16_t noise_seed = 12345;

// --- UTILITIES ---
int16_t arcade_pulse(uint32_t phase, int duty) {
    int pos = phase >> 20; 
    int threshold = (duty == 0) ? 512 : (duty == 1) ? 1024 : (duty == 3) ? 3072 : 2048;
    return (pos < threshold) ? 2000 : -2000;
}

int16_t arcade_triangle(uint32_t phase) {
    int step = (phase >> 27) & 31; 
    int val = (step < 16) ? step : 31 - step;
    return (val - 7) * 300; 
}

int16_t arcade_noise(uint16_t *seed) {
    uint16_t bit = ((*seed >> 0) ^ (*seed >> 1)) & 1;
    *seed = (*seed >> 1) | (bit << 14);
    return bit ? 2000 : -2000;
}

int16_t arcade_limit(int32_t x) {
    if (x > 4095) return 4095;
    if (x < -4095) return -4095;
    return (int16_t)x;
}

void IRAM_ATTR arcade() {
    
    // --- STATE VARIABLES ---
    static int cal_min = 4095; 
    static int cal_max = 0; 
    static bool knob_moved = false;
    static int initial_earth = 0;
    static int s_earth = -1;
    static int boot_timer = 0;
    
    static int btn_latch = 0;
    static int btn_state = 0; 

    // --- WAKE UP BLOCK ---
    static bool was_in_menu = false;
    if (preset_mode) {
        was_in_menu = true;
    } else if (was_in_menu) {
        was_in_menu = false;
        
        cal_min = 4095;
        cal_max = 0;
        s_earth = -1;
        knob_moved = false;
        boot_timer = 0;
        btn_latch = 20000;
    }

    // --- ADC & DEADZONE ---
    REG(APB_SARADC_SAR1_PATT_TAB1_REG)[0] = (0x0C<<24) | (0x6C<<16);
    uint32_t fifo_data = REG(I2S_FIFO_RD_REG)[0];
    int channel = (fifo_data >> 12) & 0xF; 
    int value   = (fifo_data & 0xFFF);     
    
    if (channel == 0) {
        if (s_earth == -1) s_earth = value;
        else s_earth += (value - s_earth) >> 4;
    }

    if (boot_timer < 2000) {
        boot_timer++;
        initial_earth = s_earth;
    }

    if (!knob_moved && boot_timer >= 2000) {
        int drift = s_earth - initial_earth;
        if (drift < 0) drift = -drift;
        if (drift > 300) {
            knob_moved = true; 
        }
    }

    if (knob_moved) {
        if (s_earth < cal_min) cal_min = s_earth;
        if (s_earth > cal_max) cal_max = s_earth;
    } else {
        cal_min = initial_earth;
        cal_max = initial_earth;
    }

    int range = cal_max - cal_min;
    if (range < 50) range = 50;
    int reading = s_earth - cal_min;
    if (reading < 0) reading = 0;
    
    int earth_cal = (reading * 255) / range;
    if (!knob_moved) earth_cal = 0; // Anchor to 'Hero' Cartridge until earth change

    // --- CONTROLS ---
    bool pin_skip = SKIPPERAT; 
    bool pin_flip = FLIPPERAT; 
    
    bool bass_en = true;       
    bool lead_en = false;      
    bool arp_en = false;       
    bool power_en = false;     
     
    // If nothing is patched, 'power_en' will be active by default
    if (pin_skip && pin_flip) power_en = true;       
    else if (pin_skip) lead_en = true;        
    else if (pin_flip) arp_en = true;         

    // BUTTON (Toggle A/B)
    bool btn_pressed = BUTTON_PRESSED;
    
    if (btn_pressed) {
        if (btn_latch == 0) {
            btn_state = !btn_state; 
            btn_latch = 10000; 
        }
    } else if (btn_latch > 0) btn_latch--;

    // EARTH BANK SELECT
    int bank = earth_cal >> 6; 
    if (bank > 3) bank = 3;
    arcade_cart = (bank * 2) + btn_state;

    // LAMP
    if (btn_state) { LAMP_ON; } else { LAMP_OFF; }

    // CARTRIDGES
    static int clock_div = 0;
    int tempo = 3000; 
    
    static int lead_idx = 0;
    static int bass_idx = 0;
    static int noise_trig = 0;
    
    if (arcade_cart == 2 || arcade_cart == 3) tempo = 2200; 
    if (arcade_cart == 4 || arcade_cart == 5) tempo = 4500; 
    if (arcade_cart == 7) tempo = 1500; 

    clock_div++;
    if (clock_div > tempo) {
        clock_div = 0;
        arcade_t++; 
        
        uint32_t t = arcade_t;
        int section_b = (t >> 10) & 1; 

        switch (arcade_cart) {
            case 0: // HERO 
                lead_idx = (section_b) ? ((t * 3) | (t >> 5)) : ((t * 5) & (t >> 7));
                bass_idx = (t >> 4); 
                noise_trig = ((t >> 3) & 1) & ((t >> 8) != 0xFF); 
                break;
            case 1: // DUNGEON 
                lead_idx = (t * (t >> 9) & 15) + (t >> 3);
                bass_idx = (t >> 5) | (t >> 4);
                noise_trig = (t * 5) & (t >> 3) & 16; 
                break;
            case 2: // TECHNO 
                lead_idx = t & (t >> 12); 
                bass_idx = (t >> 3); 
                noise_trig = ((t >> 2) & 9) | ((t & 63) == 0); 
                break;
            case 3: // BOSS 
                lead_idx = (t * (t >> 8 | t >> 3)); 
                bass_idx = t % 7; 
                noise_trig = ((t >> 1) & 1) & (t >> 5); 
                break;
            case 4: // FOREST 
                lead_idx = (t * 3) & (t >> 5); 
                bass_idx = (t >> 6); 
                noise_trig = (t >> 4) & (t >> 8) & 1; 
                break;
            case 5: // SPACE 
                lead_idx = t | (t >> 9); 
                bass_idx = (t >> 5) & (t >> 7);
                noise_trig = (t & 255) == 0; 
                break;
            case 6: // BROKEN 
                lead_idx = (t * t) >> 5;
                bass_idx = t % 13; 
                noise_trig = t & 42; 
                break;
            case 7: // CRASH 
                lead_idx = t * ((t>>12|t>>8)&63&t>>4); 
                bass_idx = t >> 2;
                noise_trig = (t >> 2) | 1; 
                break;
        }
    }
    
    // SOUND GENERATION
    lead_idx &= 7; bass_idx &= 7;
    int32_t p1_out = 0;
    
    int note = 65; 
    switch(arcade_cart) {
        case 0: note = scale_maj[lead_idx]; break;
        case 1: note = scale_min[lead_idx]; break;
        case 2: note = scale_blues[lead_idx]; break;
        case 3: note = scale_dim[lead_idx]; break;
        case 4: note = scale_dorian[lead_idx]; break;
        case 5: note = scale_whole[lead_idx]; break;
        default: note = scale_chrom[lead_idx]; break;
    }

    // LEAD
    if (lead_en) {
        p1_phase += (note << 19); 
        int duty = (arcade_cart >= 6) ? 0 : ((arcade_t >> 10) & 1) ? 2 : 1;
        p1_out = arcade_pulse(p1_phase, duty); 
    }
    else if (power_en) {
        p1_phase += (note << 20); 
        p1_out = arcade_pulse(p1_phase, 3); 
    }
    
    // ARP
    int32_t p2_out = 0;
    if (arp_en) {
        int note_arp = scale_maj[(lead_idx + 3) & 7]; 
        p2_phase += (note_arp << 19); 
        p2_out = arcade_pulse(p2_phase, 0); 
    }
    
    // BASS
    int32_t tri_out = 0;
    if (bass_en) {
        int note_bass = scale_maj[bass_idx] >> 1; 
        tri_phase += (note_bass << 18); 
        tri_out = arcade_triangle(tri_phase); 
    }
    
    // DRUMS
    int32_t noise_out = 0;
    static int noise_env = 0;
    
    if (noise_trig && clock_div == 0) noise_env = 16; 
    if (noise_env > 0) {
        if (clock_div % 150 == 0) noise_env--;
        int16_t n_val = arcade_noise(&noise_seed);
        noise_out = (n_val * noise_env) >> 3;
    }
    
    // MIXER
    int32_t m_p1 = (p1_out * MIX_LEAD) >> 6;
    int32_t m_p2 = (p2_out * MIX_ARP) >> 6;
    int32_t m_tri = (tri_out * MIX_BASS) >> 6;
    int32_t m_noi = (noise_out * MIX_DRUM) >> 6;

    int32_t mix = (m_p1 + m_p2 + m_tri + m_noi);
    int16_t final = arcade_limit(mix);

    int pout = final + 2048;
    if (pout > 4095) pout = 4095;
    if (pout < 0) pout = 0;
    
    DACWRITER(pout);
    CLEAN_ASHWRITER(pout);
    
    // YELLOW
    YELLOW_AUDIO(pout);           

    REG(I2S_CONF_REG)[0] &= ~(BIT(5)); 
    REG(I2S_INT_CLR_REG)[0] = 0xFFFFFFFF;
    REG(I2S_CONF_REG)[0] |= (BIT(5)); 
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

// ==========================================
// FX --- NEW PRESET
// ==========================================
// fx sound generator
// earth is cursor to select sound
// flip is a random one shot trigger
// skip triggers the sound selected by cursor
// button freezes skip sound and ignores flip
// turn a sound into an oscillator with high trigger rate to skip while frozen
// ash is audio out at line level
// yellow is audio out at line level

#include "samples.h" 

void IRAM_ATTR FX() { 
    
    // --- STATE ---
    static int ui_cursor = 0;    
    static int play_voice = 0;   
    static bool is_playing = false;
    static bool freeze_active = false;
    
    // Calibration
    static int cal_min = 4000; 
    static int cal_max = 100;  
    static int cal_flash = 0;  
    static bool knob_moved = false;    
    static int initial_earth = 0;

    // Input History
    //static bool prev_btn = false;
    static bool prev_skip = false;
    static bool prev_flip = false;
    static bool last_frozen = false;

    // Synthesis Vars
    static uint32_t phase = 0;    
    static int32_t timer = 0;     
    static int32_t val_a = 0, val_b = 0, val_c = 0;
    static int32_t period_a = 0, counter_a = 0;
    static uint32_t play_head = 0; 
    static uint32_t rng = 123456789; 

    const uint32_t INC_1HZ = 95444; 

    // --- INPUTS ---
    static int s_earth = -1;
    static int boot_timer = 0;

    // --- WAKE UP BLOCK ---
    static bool was_in_menu = false;
    if (preset_mode) {
        was_in_menu = true;
    } else if (was_in_menu) {
        was_in_menu = false;
        cal_min = 4095;
        cal_max = 0;
        s_earth = -1; 
        knob_moved = false;
        boot_timer = 0; 
        last_frozen = audio_frozen_state;
        freeze_active = audio_frozen_state;
    }

    REG(APB_SARADC_SAR1_PATT_TAB1_REG)[0] = (0x0C<<24) | (0x6C<<16);
    uint32_t fifo_data = REG(I2S_FIFO_RD_REG)[0];
    int channel = (fifo_data >> 12) & 0xF;
    int value = (fifo_data & 0xFFF);   

    // select earth channel
    if (channel == 0) {
        if (s_earth == -1) s_earth = value; 
        else s_earth += (value - s_earth) >> 4; 
    }

    // --- Cache flush ---
    // wait ~45ms for old preset data to clear out of the ADC queue
    if (boot_timer < 2000) {
        boot_timer++;
        initial_earth = s_earth;
    }

    //bool trig_btn = BUTTON_PRESSED;
    bool trig_cv  = SKIPPERAT;
    bool trig_rnd = FLIPPERAT; 
    
    // --- CONTROL LOGIC ---
    
    //FREEZE TOGGLE
    if (audio_frozen_state != last_frozen) {
        last_frozen = audio_frozen_state;
        freeze_active = audio_frozen_state;
    }

    // AUTO-CALIBRATION EARTH
    // always starts on the coin sound with nothing patched into earth
    // only check for after the boot timer finishes
    if (!knob_moved && boot_timer >= 2000) {
        int drift = s_earth - initial_earth;
        if (drift < 0) drift = -drift;
        if (drift > 300) {
            knob_moved = true; 
        }
    }

    bool limits_changed = false;
    if (knob_moved) {
        if (s_earth < cal_min) { cal_min = s_earth; limits_changed = true; }
        if (s_earth > cal_max) { cal_max = s_earth; limits_changed = true; }
    } else {
        cal_min = initial_earth;
        cal_max = initial_earth;
    }

    if (freeze_active) { 
        LAMP_ON;
    } else if (limits_changed || cal_flash > 0) { 
        if (limits_changed) cal_flash = 2000; 
        cal_flash--;
        LAMP_ON;
    } else { 
        LAMP_OFF;
    }

    // CURSOR MAPPING
    int range = cal_max - cal_min;
    if (range < 50) range = 50; 
    int reading = s_earth - cal_min; 
    if (reading < 0) reading = 0;
    
    int candidate = (reading * 37) / range;
    if (candidate > 36) candidate = 36;

    // SELECTION UPDATE
    if (!freeze_active) {
        ui_cursor = candidate;
    }

    bool trigger_now = false;

    // FLIP (random one shot) - ignored if frozen
    if (trig_rnd && !prev_flip) {
        if (!freeze_active) {
            rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
            int rnd_slot = rng % 37;
            play_voice = rnd_slot; 
            ui_cursor = rnd_slot; // Sync UI
            trigger_now = true;
        }
    }
    prev_flip = trig_rnd;

    // SKIP (Trigger Earth Selected) - Always Active
    if (trig_cv && !prev_skip) {
        play_voice = ui_cursor; 
        trigger_now = true;
    }
    prev_skip = trig_cv;

    // --- RESET ENGINE ---
    if (trigger_now) {
        is_playing = true;
        timer = 0; phase = 0; play_head = 0;
        val_a = 0; val_b = 0; val_c = 0; 
        counter_a = 0; period_a = 0;
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; 
    }

    int32_t out_sig = 0;

    // --- AUDIO ENGINE ---
    if (is_playing) {
        timer++;
        int mod = 1; 
        switch(play_voice) {
            
            case 0: // COIN
                if (timer < 3000) phase += (987 * mod) * INC_1HZ; 
                else phase += (1318 * mod) * INC_1HZ;              
                if (timer > 6000) is_playing = false;
                out_sig = ((phase >> 20) & 4095) - 2048; 
                if (out_sig > 0) out_sig = 2000; else out_sig = -2000; 
                break;
                
            case 1: { // JUMP
                if (timer == 1) period_a = 380; 
                if ((timer % 700) == 0) {
                    period_a -= (period_a >> 3); 
                    if (period_a < 30) period_a = 30; 
                }
                counter_a += mod; 
                if (counter_a >= period_a) counter_a = 0;
                out_sig = (counter_a < (period_a / 2)) ? 2000 : -2000;
                if (timer > 8000) is_playing = false; 
                break;
            }

            case 2: { // MUSHROOM
                int step = timer / 900; 
                int note = 0; int amp = 2000;
                switch(step) {
                    case 0: note = 261; break; case 1: note = 329; break; 
                    case 2: note = 392; break; case 3: note = 523; break; 
                    case 4: note = 659; break; case 5: note = 783; break; 
                    case 6: note = 1046; break; case 7: note = 1318; break; 
                    case 8: note = 1568; break; case 9: note = 2093; break; 
                    case 10: note = 2637; break; case 11: note = 3136; break; 
                    default: 
                        note = 3136;
                        int decay = (timer - (12*900)); 
                        amp = 2000 - decay; if (amp < 0) amp = 0;
                        break;
                }
                phase += note * mod * INC_1HZ;
                out_sig = (phase & 0x80000000) ? amp : -amp;
                if (timer > 13500) is_playing = false;
                break;
            }

            case 3: { // DAMAGE
                rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
                val_a = 4000 - (timer >> 1); 
                if (val_a < 0) { is_playing = false; break; }
                phase += (300 - (timer>>4)) * mod * INC_1HZ; 
                out_sig = ((phase >> 20) & 4095) - 2048; 
                if ((rng & 255) > 128) out_sig += 2000; else out_sig -= 2000; 
                out_sig = (out_sig * val_a) >> 12;
                break;
            }

            case 4: // LASER
                val_a = 2000 - (timer >> 3); 
                if (val_a < 100) is_playing = false;
                phase += val_a * mod * INC_1HZ;
                out_sig = ((phase >> 20) & 4095) - 2048; 
                if (out_sig > 0) out_sig = 2000; else out_sig = -2000;
                break;

            case 5: // EXPLOSION
                rng ^= rng << 13; rng ^= rng >> 17;
                val_a = 22000 - timer; 
                if (val_a < 0) { is_playing = false; break; }
                if ((timer % (1 + (timer >> 10))) == 0) val_b = (rng & 4095) - 2048;
                out_sig = (val_b * val_a) >> 15;
                break;

            case 6: // MENU
                if (timer > 2000) is_playing = false;
                phase += 2200 * mod * INC_1HZ; 
                val_a = (phase >> 20) & 2047;
                out_sig = (val_a * (2048 - val_a)) >> 9;
                if (phase & 0x80000000) out_sig = -out_sig;
                break;

            case 7: // FLAGPOLE
                val_a = 800 - (timer >> 6); 
                if (val_a < 50) is_playing = false;
                phase += val_a * mod * INC_1HZ;
                out_sig = ((phase >> 20) & 4095) - 2048; 
                if (phase & 0x80000000) out_sig = -out_sig;
                break;
            
            case 8: // 1-UP
                val_c = timer / 3000; 
                if (val_c > 5) is_playing = false;
                if (val_c == 0) phase += 659 * INC_1HZ; 
                else if (val_c == 1) phase += 783 * INC_1HZ; 
                else if (val_c == 2) phase += 1318 * INC_1HZ;
                else if (val_c == 3) phase += 1046 * INC_1HZ; 
                else if (val_c == 4) phase += 1174 * INC_1HZ;
                else phase += 1567 * INC_1HZ;
                out_sig = (phase & 0x80000000) ? 2000 : -2000;
                break;

            case 9: // LOW HEALTH
                phase += 800 * INC_1HZ;
                if (timer > 45000) is_playing = false;
                out_sig = (phase & 0x80000000) ? 2000 : -2000;
                if ((timer % 7500) > 3750) out_sig = 0;
                break;

            case 10: { // BOING
                val_a = 150 + (timer >> 7); 
                val_b = phase * 2; 
                val_c = 1000 - (timer >> 5); if (val_c < 0) val_c = 0;
                int32_t fm = (val_b & 0x80000000) ? val_c : -val_c;
                phase += (val_a + fm) * mod * INC_1HZ; 
                if (timer > 22000) is_playing = false;
                out_sig = ((phase >> 20) & 4095) - 2048; 
                break;
            }

            case 11: // ZIP
                if (timer == 1) val_a = 200;
                val_a += (val_a >> 10) + 1; 
                if (val_a > 5000) is_playing = false;
                phase += val_a * mod * INC_1HZ;
                out_sig = ((phase >> 20) & 4095) - 2048; 
                if (out_sig > 0) out_sig = 2000; else out_sig = -2000;
                break;

            case 12: // WHISTLE
                val_a = 800 + (timer >> 6); 
                phase += val_a * mod * INC_1HZ;
                val_b = (phase >> 20) & 2047;
                out_sig = (val_b * (2048 - val_b)) >> 9;
                if (phase & 0x80000000) out_sig = -out_sig;
                rng ^= rng << 13; rng ^= rng >> 17;
                out_sig += ((rng & 511) - 256);
                if (timer > 20000) is_playing = false;
                break;

            case 13: { // BONK
                val_a = 4000 - (timer >> 2); 
                if (val_a < 0) { is_playing = false; break; }
                val_b += 1410 * (INC_1HZ / 2); 
                int32_t m = (val_b & 0x80000000) ? 2000 : -2000;
                phase += (400 * INC_1HZ) + ((m * val_a) >> 8); 
                int32_t raw = (phase >> 20) & 2047;
                out_sig = (raw * (2048 - raw)) >> 9;
                if (phase & 0x80000000) out_sig = -out_sig; 
                break;
            }

            case 14: { // ALERT
                if (timer > 30000) is_playing = false;
                val_a += 60 * INC_1HZ; 
                val_b += 65 * INC_1HZ; 
                int p1 = (val_a & 0x80000000) ? 2000 : -2000;
                int p2 = (val_b & 0x80000000) ? 2000 : -2000;
                if ((timer % 1000) < 50) {
                     rng ^= rng << 13; rng ^= rng >> 17;
                     if ((rng & 255) > 128) p1 = 0;
                }
                out_sig = (p1 + p2) >> 1;
                int env = 30000 - timer;
                out_sig = (out_sig * env) >> 15;
                break;
            }

            case 15: // ALARM
                rng ^= rng << 13; rng ^= rng >> 17;
                val_a = (rng & 4095) - 2048;
                if ((timer % 3000) > 1500) val_a = 0;
                out_sig = val_a;
                if (timer > 20000) is_playing = false;
                break;

            case 16: // TIP-TOE
                if (timer > 4000) is_playing = false;
                phase += 2000 * mod * INC_1HZ; 
                val_a = 4000 - timer; 
                val_b = (phase >> 20) & 2047;
                out_sig = (val_b * (2048 - val_b)) >> 9;
                if (phase & 0x80000000) out_sig = -out_sig;
                out_sig = (out_sig * val_a) >> 12;
                break;

            case 17: // FALL
                val_a = 1500 - (timer >> 5); 
                if (val_a < 100) is_playing = false;
                phase += val_a * mod * INC_1HZ;
                val_b = (phase >> 20) & 2047; 
                out_sig = (val_b * (2048 - val_b)) >> 9;
                if (phase & 0x80000000) out_sig = -out_sig;
                break;

            case 18: // DIZZY
                if (timer > 20000) is_playing = false;
                phase += 600 * INC_1HZ; 
                val_a += 605 * INC_1HZ; 
                out_sig = (((phase>>20)&4095)+((val_a>>20)&4095))-4096;
                break;

            case 19: // SHAKE
                if ((timer % 3000) < 500) { 
                     rng ^= rng << 13; rng ^= rng >> 17;
                     out_sig = (rng & 4095) - 2048;
                } else out_sig = 0;
                if (timer > 15000) is_playing = false;
                break;

            case 20: // SIREN
                val_a = (timer & 16383); 
                if (val_a > 8192) val_a = 16384 - val_a; 
                phase += (600 + (val_a >> 4)) * mod * INC_1HZ;
                out_sig = ((phase >> 20) & 4095) - 2048; 
                if (out_sig > 0) out_sig = 2000; else out_sig = -2000;
                if (timer > 45000) is_playing = false;
                break;

            case 21: // HORN
                phase += 349 * mod * INC_1HZ; 
                val_a += 440 * mod * INC_1HZ; 
                val_b = ((phase >> 20) & 4095) - 2048;
                val_c = ((val_a >> 20) & 4095) - 2048;
                out_sig = (val_b + val_c) >> 1;
                if (out_sig > 0) out_sig = 2000; else out_sig = -2000;
                if (timer > 15000) is_playing = false;
                break;

            case 22: { // PIPE
                val_a = 2500 - (timer >> 3); 
                if (val_a < 100) is_playing = false;
                int stepped_pitch = val_a & 0xFF80; 
                phase += stepped_pitch * mod * INC_1HZ;
                out_sig = ((phase >> 20) & 4095) - 2048; 
                if (phase & 0x80000000) out_sig = -out_sig;
                break;
            }

            case 23: { // CLOCK
                if (timer == 1) val_a = 4000; 
                val_a -= (val_a >> 8); 
                val_b += (val_a - val_b) >> 6; 
                val_c = val_a - val_b; 
                phase += 400 * INC_1HZ;
                static uint32_t mp = 0; mp += 600 * INC_1HZ;
                int m = (((mp >> 20) & 4095) - 2048) * (val_c >> 2) >> 12;
                out_sig = ((( (phase + (m << 10)) >> 20) & 4095) - 2048) * val_c >> 12;
                if (timer > 8000) is_playing = false;
                break;
            }

            case 24: { // HEART
                val_a = timer; 
                if (val_a < 4000) { 
                    phase += 60 * mod * INC_1HZ; 
                    val_b = (phase >> 20) & 2047; 
                    out_sig = (val_b * (2048 - val_b)) >> 9;
                    if (phase & 0x80000000) out_sig = -out_sig;
                    int env = (val_a < 500) ? val_a*8 : 4000-val_a;
                    if (env < 0) env = 0;
                    out_sig = (out_sig * env) >> 12;
                } else if (val_a > 12000 && val_a < 15000) { 
                     phase += 80 * mod * INC_1HZ; 
                     val_b = (phase >> 20) & 2047;
                     out_sig = (val_b * (2048 - val_b)) >> 9;
                     if (phase & 0x80000000) out_sig = -out_sig;
                     int env = (val_a < 12500) ? (val_a-12000)*8 : 15000-val_a;
                     if (env < 0) env = 0;
                     out_sig = (out_sig * env) >> 12;
                } else out_sig = 0;
                if (timer > 30000) is_playing = false;
                break;
            }

            case 25: // GEIGER
                rng ^= rng << 13; rng ^= rng >> 17;
                if ((rng & 255) < 5) out_sig = 4000; 
                else if ((rng & 255) < 10) out_sig = -4000;
                else out_sig = 0;
                if (timer > 20000) is_playing = false;
                break;

            case 26: // SONAR
                val_a = 2000;
                if (timer > 100) val_a = 2000 - (timer >> 4);
                if (val_a < 0) { is_playing = false; break; }
                phase += 1000 * INC_1HZ;
                val_b = (phase >> 20) & 2047; 
                out_sig = (val_b * (2048 - val_b)) >> 9;
                if (phase & 0x80000000) out_sig = -out_sig;
                out_sig = (out_sig * val_a) >> 11;
                break;

            case 27: // METAL
                val_a = 3000 - (timer >> 3); 
                if (val_a < 0) { is_playing=false; break; }
                phase += 800 * INC_1HZ;
                val_b += 1340 * INC_1HZ;
                val_c += 2110 * INC_1HZ;
                out_sig = (((phase>>20)&4095)+((val_b>>20)&4095)+((val_c>>20)&4095))-6144;
                out_sig = (out_sig * val_a) >> 12;
                break;

            case 28: { // WATER DROP
                int pitch = 800 + (timer >> 3); 
                phase += pitch * INC_1HZ;
                int raw = (phase >> 20) & 2047;
                int sig_main = (raw * (2048 - raw)) >> 9;
                if (phase & 0x80000000) sig_main = -sig_main;
                int env_main = 1700 - timer; 
                if (env_main < 0) env_main = 0;
                sig_main = (sig_main * env_main) >> 11;

                int sig_plop = 0;
                if (timer < 500) {
                    uint32_t p_plop = timer * 140 * INC_1HZ;
                    int r_plop = (p_plop >> 20) & 2047;
                    int s_plop = (r_plop * (2048 - r_plop)) >> 9;
                    if (p_plop & 0x80000000) s_plop = -s_plop;
                    int env_plop = 500 - timer;
                    sig_plop = (s_plop * env_plop) >> 10;
                }
                out_sig = sig_main + sig_plop;
                if (timer > 1700) is_playing = false;
                break;
            }

            case 29: { // GAMEOVER
                int n = 0;
                if (timer < 10000) n = 523;
                else if (timer < 20000) n = 392;
                else if (timer < 30000) n = 329;
                else if (timer < 45000) n = 220;
                else if (timer < 60000) n = 246;
                else if (timer < 75000) n = 261;
                else is_playing = false;
                if (n > 0) phase += n * INC_1HZ;
                out_sig = ((phase >> 20) & 4095) - 2048; 
                if (phase & 0x80000000) out_sig = -out_sig; 
                break;
            }

            case 30: { // MEOW
                val_a = 400 - abs((timer/50) - 400); 
                phase += (800 + val_a) * INC_1HZ;
                int32_t s = ((phase >> 20) & 4095) - 2048;
                val_b = 200 + (val_a >> 1); 
                val_c += (s - val_c) * val_b >> 13;
                out_sig = val_c * 4;
                if (timer > 40000) is_playing = false;
                break;
            }
            
            case 31: { // QUACK
                play_head += mod; 
                uint32_t idx = play_head >> 2; 
                if (idx >= QUACK_LEN) { is_playing = false; out_sig = 0; }
                else { out_sig = quack_sample[idx] * 16; }
                break;
            }

            case 32: // CRICKET
                if ((timer % 3000) < 500) { 
                    phase += 4500 * INC_1HZ; 
                    out_sig = (phase & 0x80000000) ? 2000 : -2000;
                } else out_sig = 0;
                if (timer > 10000) is_playing = false;
                break;

            case 33: { // PHONE
                phase += 440 * INC_1HZ; 
                val_a += 480 * INC_1HZ; 
                val_b += 20 * INC_1HZ; 
                int32_t ra = (phase >> 20) & 2047;
                int32_t sa = (ra * (2048 - ra)) >> 9;
                if (phase & 0x80000000) sa = -sa;
                int32_t rb = (val_a >> 20) & 2047;
                int32_t sb = (rb * (2048 - rb)) >> 9;
                if (val_a & 0x80000000) sb = -sb;
                int32_t rl = (val_b >> 20) & 2047;
                int32_t sl = (rl * (2048 - rl)) >> 9;
                int32_t mix = (sa + sb) >> 1;
                out_sig = (mix * (2048 + sl)) >> 12;
                if (timer > 45000) is_playing = false;
                break;
            }

            case 34: { // ROBOT
                phase += 200 * INC_1HZ; 
                val_a += 50 * INC_1HZ; 
                int32_t vs = ((phase >> 20) & 4095) - 2048;
                int32_t vn = ((val_a >> 20) & 2047); 
                vn = (vn * (2048 - vn)) >> 9;
                out_sig = (vs * vn) >> 10; 
                if (timer > 20000) is_playing = false;
                break;
            }

            case 35: { // SABER
                val_a += 2 * INC_1HZ; 
                int32_t pwm = 2048 + (((val_a >> 20) & 4095) - 2048) / 2;
                phase += 100 * INC_1HZ; 
                int32_t ramp = (phase >> 20) & 4095;
                out_sig = (ramp > pwm) ? 2000 : -2000;
                if (timer > 20000) is_playing = false;
                break;
            }

            case 36: // POWER OFF
                val_a = 1000 - (timer >> 4); 
                if (val_a < 0) { is_playing = false; break; }
                phase += val_a * INC_1HZ;
                out_sig = ((phase >> 20) & 4095) - 2048;
                out_sig = (out_sig & 0xFC00); 
                break;
        }

    } else {
        out_sig = 0;
    }
    
    // OUTPUT
    int32_t dac_out = out_sig + 2048;
    if (dac_out > 4095) dac_out = 4095; if (dac_out < 0) dac_out = 0;
    DACWRITER(dac_out);
    ASHWRITER(dac_out);

    // YELLOW PSEUDO-DAC
    YELLOW_AUDIO(dac_out);

    // Interrupt reset
    REG(I2S_CONF_REG)[0] &= ~(BIT(5)); 
    REG(I2S_INT_CLR_REG)[0] = 0xFFFFFFFF;
    REG(I2S_CONF_REG)[0] |= (BIT(5)); 
}

//////////////////////////////////////////////////////////////////////////////////////////END////////////

// ==========================================
// WAVETABLE -- NEW PRESET
// ==========================================
// wavetable synth with ring modulation
// flip switches between wavetable banks
// skip selects a random waveform when triggered
// earth modulates the pitch
// ash is the audio output at line level
// yellow is the bit crushed audio output at line level
// the feedback knob controls the audio output of the main output
// the dry knob controls the modulator volume of the dry signal to the ring mod
// button is a freeze on the currently playing waveform
// lamp indicates freeze state (on = frozen)

#include "wavetables.h" 

void IRAM_ATTR wavetable() {
    
    // --- STATE ---
    static uint32_t ph_phase = 0;
    static uint32_t rng_wt = 123456789; 
    
    static int32_t active_inc = 10737418; 
    static int active_z_index = 0;
    static int current_wave_idx = 0; 
    
    static bool latched_freeze = false;
    static bool last_frozen = false;
    static bool prev_flip = false;
    static bool prev_skip = false; 

    // --- EARTH LPF ---
    static int s_earth = -1;

    // --- WAKE UP BLOCK ---
    static bool was_in_menu = false;
    if (preset_mode) {
        was_in_menu = true;
    } else if (was_in_menu) {
        was_in_menu = false;
        
        // Sync to the global freeze state
        last_frozen = audio_frozen_state;
        latched_freeze = audio_frozen_state;
        
        // Sync inputs to hardware resting state
        prev_flip = FLIPPERAT;
        prev_skip = SKIPPERAT;
        s_earth = -1;
    }

    // INPUTS
    int audio_in = ADCREADER;          
    bool skip_active = SKIPPERAT; 
    bool flip_active = FLIPPERAT; 
    
    // EARTH 
    REG(APB_SARADC_SAR1_PATT_TAB1_REG)[0] = (0x0C<<24) | (0x6C<<16);
    uint32_t fifo_data = REG(I2S_FIFO_RD_REG)[0];
    int channel = (fifo_data >> 12) & 0xF; 
    int value = (fifo_data & 0xFFF);     
    
    if (channel == 0) {
        if (s_earth == -1) s_earth = value;
        else s_earth += (value - s_earth) >> 4;
    }

    // --- CONTROL LOGIC ---

    // FREEZE 
    if (audio_frozen_state != last_frozen) {
        last_frozen = audio_frozen_state;
        latched_freeze = audio_frozen_state;
    }

    // PITCH & SELECTION
    if (!latched_freeze) {
        
        // EARTH 8-BIT QUANTIZATION
        // Shift 12-bit (4096) down to 8-bit (256)
        int cv_8bit = s_earth >> 4; 

        // FLOOR CLAMP
        if (cv_8bit < 10) cv_8bit = 0;

        // PITCH SCALING
        // Base + (Steps * Multiplier)
        // 0 * X = 0, preserving the deep base tone.
        active_inc = 10737418 + (cv_8bit * 150000); 
        
        // Safety Floor
        if (active_inc < 500000) active_inc = 500000;

        // BANK SELECT (Flip)
        if (flip_active && !prev_flip) {
            active_z_index++;
            if (active_z_index > 7) active_z_index = 0;
        }
        prev_flip = flip_active;

        // RANDOM WAVE (Skip)
        if (skip_active && !prev_skip) {
            rng_wt ^= rng_wt << 13;
            rng_wt ^= rng_wt >> 17;
            rng_wt ^= rng_wt << 5;
            current_wave_idx = rng_wt & 63; 
        }
        prev_skip = skip_active;
    }

    // --- VISUALS ---
    if (latched_freeze) { LAMP_ON; } else { LAMP_OFF; }

    // --- AUDIO ENGINE ---
    ph_phase += active_inc;
    int idx = (active_z_index * 16384) + (current_wave_idx * 256) + ((ph_phase >> 24) & 0xFF);
    
    #ifdef WAVETABLES_H
        int16_t carrier = factory_waves[idx];
    #else
        int16_t carrier = ((ph_phase >> 24) & 0xFF > 128) ? 2000 : -2048; 
    #endif

    // RING MOD MIXER
    int32_t synth_part = carrier; 
    int32_t mod_in = (audio_in - 2048) << 1; 
    int32_t ring_part = (carrier * mod_in) >> 11; 
    int32_t final_out = synth_part + ring_part;
    
    int out_dac = final_out + 2048;
    if (out_dac > 4095) out_dac = 4095;
    if (out_dac < 0) out_dac = 0;

    // --- OUTPUTS ---
    pout = out_dac;
    CLEAN_ASHWRITER(pout); 
    DACWRITER(pout);
    
    // YELLOW PSEUDO-DAC
    YELLOW_AUDIO(pout);

    // Interrupt reset
    REG(I2S_CONF_REG)[0] &= ~(BIT(5)); 
    REG(I2S_INT_CLR_REG)[0]=0xFFFFFFFF;
    REG(I2S_CONF_REG)[0] |= (BIT(5)); 
}


//////////////////////////////////////////////////////////////////////////////////////////END/////////////////////

// ==========================================
// DRONE --- NEW PRESET
// ==========================================
// create a drone from navigating a tonal lattice via earth
// drones are created from the first 31 primes, in 4 chord shapes, with over or under tones, in 2 spreads
// this allows for 496 unique drone states
// use speed knob to tune drone (use lamp mode 4 for tuning)
// sequence from speed modulation
// earth modulates the harmonic center in the lattice and therefore the tonal complexity, with higher CV moving up to higher primes
// there are four geometric chord shapes built from the earth's position in the lattice
// the lamp flashes in the pattern of the selected chord shape
// the cycle of chords is cathedral (3 octave span), fist (nearest prime neighbors on lattice), comb (more spread prime selection), organ (octave stack)
// skip swtiches between two spreads: dense when low (default) or spread when high (voices spread to both lower and upper registers)
// flip switches between overtones where the lattice is built by multiples of the primes (default) and undertones where it is built from division
// ash is the wet audio out at line level
// yellow is the bit crushed audio at line level


// ------------------------------------------
// SET UP
// ------------------------------------------
#define DR_VOICES 32
#define DR_PRIME_CNT 31
#define DR_ONE 4096      

// When speed set to 44.1kHz (~ 2 0'clock) the base freq is C5 (~523Hz)
#define DR_BASE_INC 59224800 

// Limits
#define DR_MAX_PRESCALE 32 // 32 = ~3.5s
#define DR_MIN_PRESCALE 1 // 1  = ~100ms (Fast/Reaction)

#define DR_FADE_STEP 3

// S&H Rate for handling fast CV to earth
#define DR_SH_SPEED 35

// Dead Zone to earth for CV that doesn't go fully to ground
#define DR_DEAD_ZONE 200

// Primes
// the foundation of the tonal lattice
const uint8_t raw_primes[DR_PRIME_CNT] = {
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 
    59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127
};

// Sine Table
const int8_t dr_sine[256] = {
    0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 
    48, 51, 54, 57, 59, 62, 65, 67, 70, 73, 75, 78, 80, 82, 85, 87, 
    89, 91, 93, 95, 97, 99, 100, 102, 104, 105, 107, 108, 109, 111, 112, 113, 
    114, 115, 116, 117, 118, 119, 119, 120, 120, 121, 121, 121, 121, 121, 121, 120, 
    120, 119, 119, 118, 117, 116, 115, 114, 113, 112, 111, 109, 108, 107, 105, 104, 
    102, 100, 99, 97, 95, 93, 91, 89, 87, 85, 82, 80, 78, 75, 73, 70, 
    67, 65, 62, 59, 57, 54, 51, 48, 45, 42, 39, 36, 33, 30, 27, 24, 
    21, 18, 15, 12, 9, 6, 3, 0, -3, -6, -9, -12, -15, -18, -21, -24, 
    -27, -30, -33, -36, -39, -42, -45, -48, -51, -54, -57, -59, -62, -65, -67, -70, 
    -73, -75, -78, -80, -82, -85, -87, -89, -91, -93, -95, -97, -99, -100, -102, -104, 
    -105, -107, -108, -109, -111, -112, -113, -114, -115, -116, -117, -118, -119, -119, -120, -120, 
    -121, -121, -121, -121, -121, -121, -120, -120, -119, -119, -118, -117, -116, -115, -114, -113, 
    -112, -111, -109, -108, -107, -105, -104, -102, -100, -99, -97, -95, -93, -91, -89, -87, 
    -85, -82, -80, -78, -75, -73, -70, -67, -65, -62, -59, -57, -54, -51, -48, -45, 
    -42, -39, -36, -33, -30, -27, -24, -21, -18, -15, -12, -9, -6, -3
};

// Equal Power Volume Curve for transitions
const int16_t dr_vol_curve[65] = {
    0, 100, 201, 301, 402, 502, 603, 703, 803, 903, 1003, 1103, 1202, 1301, 1400, 1499, 
    1597, 1695, 1792, 1889, 1985, 2081, 2176, 2270, 2364, 2457, 2550, 2642, 2733, 2823, 2913, 3002, 
    3090, 3177, 3263, 3348, 3433, 3517, 3599, 3681, 3762, 3842, 3920, 3998, 4074, 4150, 4224, 4298, 
    4370, 4441, 4511, 4580, 4647, 4713, 4778, 4841, 4903, 4964, 5023, 5081, 5137, 5192, 5246, 5298, 5348
};

// ------------------------------------------
// SHAPES
// ------------------------------------------
// based on the selected tonal center by earth, these are the coodinates on the prime list relative to it
struct Coord { int8_t x; int8_t y; };

const struct Coord shape_cathedral[12] = {
    {0,-2}, {0,-1}, {0,0}, {0,1}, {1,-1}, {1,0}, {1,1}, {2,0}, {3,0}, {4,1}, {5,2}, {7,2} 
};
const struct Coord shape_fist[12] = {
    {0,0}, {1,0}, {2,0}, {3,0}, {0,1}, {1,1}, {2,1}, {3,1}, {0,-1}, {1,-1}, {2,-1}, {3,-1}
};
const struct Coord shape_comb[12] = {
    {0,-2}, {2,-2}, {4,-1}, {6,-1}, {0,0}, {3,0}, {6,0}, {9,0}, {1,1}, {4,1}, {7,2}, {10,2}
};
const struct Coord shape_tuner[12] = {
    {0,-2}, {0,-1}, {0,0}, {0,1}, {0,2},  
    {1,-1}, {1,0}, {1,1},                 
    {0,-1}, {0,0}, {0,1}, {0,2}           
};

struct DroneVoice {
    uint32_t phase;
    uint32_t increment;
    int32_t amp;        
    int32_t target_amp; 
    uint16_t id;         
    bool updated_this_frame;
};

static struct DroneVoice d_voices[DR_VOICES];
static int32_t prime_ratios[DR_PRIME_CNT]; 
static bool dr_initialized = false;

// ------------------------------------------
// MAIN
// ------------------------------------------
void IRAM_ATTR drone() {

    // CALIBRATION GLOBALS
    static int cal_min = 4095; 
    static int cal_max = 0; 
    static bool knob_moved = false;
    static int initial_earth = 0;
    static int s_earth = -1;
    static int boot_timer = 0;
    
    // --- SETUP ---
    if (!dr_initialized) {
        for(int i=0; i<DR_PRIME_CNT; i++) {
            int32_t val = raw_primes[i] * DR_ONE;
            while (val >= (DR_ONE * 2)) val >>= 1; 
            prime_ratios[i] = val;
        }
        for(int i=0; i<DR_VOICES; i++) {
            d_voices[i].amp = 0;
            d_voices[i].target_amp = 0;
            d_voices[i].id = 0xFFFF;
        }
        dr_initialized = true;
    }

    // --- WAKE UP BLOCK ---
    static bool was_in_menu = false;
    if (preset_mode) {
        was_in_menu = true;
    } else if (was_in_menu) {
        was_in_menu = false;
        
        cal_min = 4095;
        cal_max = 0;
        s_earth = -1;
        knob_moved = false;
        boot_timer = 0;
    }

    // --- ADC ---
    REG(APB_SARADC_SAR1_PATT_TAB1_REG)[0] = (0x0C<<24) | (0x6C<<16);
    uint32_t fifo_data = REG(I2S_FIFO_RD_REG)[0];
    int channel = (fifo_data >> 12) & 0xF; 
    int value   = (fifo_data & 0xFFF);     
    
    if (channel == 0) {
        if (s_earth == -1) s_earth = value;
        else s_earth += (value - s_earth) >> 4;
    }

    if (boot_timer < 2000) {
        boot_timer++;
        initial_earth = s_earth;
    }

    // --- CONTROL LOOP ---
    static int ctrl_timer = 0;
    static int current_shape_idx = 0;
    static int btn_latch = 0;
    
    static int lamp_pattern_counter = 0;
    static int lamp_timer = 0;

    static int sh_timer = 0;
    static int held_earth_val = 0;
    
    static int last_earth_val = 0;
    static int dynamic_prescaler = DR_MAX_PRESCALE;
    
    static uint32_t global_t = 0;
    global_t++;

    // Button Logic
    if (BUTTON_PRESSED) {
        if (btn_latch == 0) {
            current_shape_idx++;
            if (current_shape_idx > 3) current_shape_idx = 0; 
            
            // Visual Reset
            lamp_pattern_counter = 0;
            lamp_timer = 0;
            
            btn_latch = 10000; 
        }
    } else {
        if (btn_latch > 0) btn_latch--;
    }

    ctrl_timer++;
    if (ctrl_timer > 256) {
        ctrl_timer = 0;
        
        bool utonal = (FLIPPERAT); 
        bool spread = (SKIPPERAT); 

        // --- EARTH ADAPTIVE KINETICS ---
        int velocity = s_earth - last_earth_val;
        if (velocity < 0) velocity = -velocity;
        last_earth_val = s_earth;
        
        int target_pre = DR_MAX_PRESCALE;
        if (velocity > 40) target_pre = DR_MIN_PRESCALE;
        else if (velocity > 10) target_pre = 8;
        
        if (dynamic_prescaler < target_pre) dynamic_prescaler++;
        else if (dynamic_prescaler > target_pre) dynamic_prescaler--;

        // --- EARTH AUTO-CALIBRATION (With Deadzone) ---
        if (!knob_moved && boot_timer >= 2000) {
            int drift = s_earth - initial_earth;
            if (drift < 0) drift = -drift;
            if (drift > 300) {
                knob_moved = true; 
            }
        }

        if (knob_moved) {
            if (s_earth < cal_min) cal_min = s_earth;
            if (s_earth > cal_max) cal_max = s_earth;
        } else {
            cal_min = initial_earth;
            cal_max = initial_earth;
        }

        // --- EARTH SAMPLE & HOLD ---
        sh_timer++;
        if (sh_timer > DR_SH_SPEED) {
            sh_timer = 0;
            
            int range = cal_max - cal_min;
            if (range < 50) range = 50; 
            
            int rel = s_earth - cal_min;
            if (rel < DR_DEAD_ZONE) rel = 0;
            if (rel < 0) rel = 0;
            if (rel > range) rel = range;
            
            if (!knob_moved) {
                held_earth_val = 0; // Force to lowest lattice point until moved
            } else {
                held_earth_val = (rel * 4095) / range;
            }
        }

        int center_idx = (held_earth_val * (DR_PRIME_CNT - 1)) >> 12;

        // Reset Flags
        for(int i=0; i<DR_VOICES; i++) d_voices[i].updated_this_frame = false;

        const struct Coord* shape_ptr;
        if (current_shape_idx == 0) shape_ptr = shape_cathedral;
        else if (current_shape_idx == 1) shape_ptr = shape_fist;
        else if (current_shape_idx == 2) shape_ptr = shape_comb;
        else shape_ptr = shape_tuner; 

        // Iterate Points
        for(int i=0; i<12; i++) {
            
            int p_idx = center_idx + shape_ptr[i].x;
            if (p_idx < 0) p_idx = 0;
            if (p_idx >= DR_PRIME_CNT) p_idx = DR_PRIME_CNT - 1;

            int y_mult = spread ? 2 : 1; 
            int oct = shape_ptr[i].y * y_mult;

            uint16_t polarity_bit = utonal ? 0x8000 : 0;
            uint16_t note_id = (p_idx << 8) | (oct + 128) | polarity_bit; 

            int32_t ratio = prime_ratios[p_idx];
            uint64_t calc_freq = (uint64_t)DR_BASE_INC;

            if (!utonal) {
                calc_freq = (calc_freq * ratio) >> 12; 
            } else {
                calc_freq = (calc_freq << 12) / ratio; 
            }
            uint32_t freq = (uint32_t)calc_freq; 

            if (oct > 0) freq <<= oct;      
            else if (oct < 0) freq >>= -oct; 

            int voice_idx = -1;

            for(int v=0; v<DR_VOICES; v++) {
                if (d_voices[v].id == note_id && d_voices[v].amp > 0) {
                    voice_idx = v;
                    break;
                }
            }

            if (voice_idx == -1) {
                for(int v=0; v<DR_VOICES; v++) {
                    if (d_voices[v].amp == 0 && d_voices[v].target_amp == 0) {
                        voice_idx = v;
                        d_voices[voice_idx].phase = 0; 
                        d_voices[voice_idx].id = note_id;
                        d_voices[voice_idx].increment = freq; 
                        break;
                    }
                }
            }

            if (voice_idx != -1) {
                d_voices[voice_idx].target_amp = 4096; 
                d_voices[voice_idx].increment = freq; 
                d_voices[voice_idx].updated_this_frame = true;
            }
        }

        // Cleanup
        for(int i=0; i<DR_VOICES; i++) {
            if (!d_voices[i].updated_this_frame) {
                d_voices[i].target_amp = 0;
            }
        }
    }

    // --- AUDIO LOOP ---
    int32_t mix = 0;
    
    // Adaptive Fade Prescaler
    static int fade_tick = 0;
    bool do_fade = false;
    fade_tick++;
    if (fade_tick >= dynamic_prescaler) { 
        fade_tick = 0;
        do_fade = true;
    }

    for (int i=0; i<DR_VOICES; i++) {
        if (do_fade) {
            if (d_voices[i].amp < d_voices[i].target_amp) d_voices[i].amp += DR_FADE_STEP;
            else if (d_voices[i].amp > d_voices[i].target_amp) d_voices[i].amp -= DR_FADE_STEP;
        }

        if (d_voices[i].amp > 0) {
            d_voices[i].phase += d_voices[i].increment;
            int32_t wave = dr_sine[d_voices[i].phase >> 24];
            
            // Equal Power Lookup
            int curve_idx = d_voices[i].amp >> 6;
            if (curve_idx > 64) curve_idx = 64;
            int32_t gain = dr_vol_curve[curve_idx];
            
            mix += (wave * gain) >> 12; 
        }
    }

    // --- LIMITER ---
    int32_t signal = mix * 2; 
    if (signal > 6000) signal = 6000;
    if (signal < -6000) signal = -6000;
    int32_t abs_sig = (signal > 0) ? signal : -signal;
    int32_t sat = signal - ((signal * abs_sig) >> 15);
    int output_sample = sat + 2048;
    if (output_sample > 4095) output_sample = 4095;
    if (output_sample < 0) output_sample = 0;

    // --- HARDWARE OUT ---
    DACWRITER(output_sample);
    CLEAN_ASHWRITER(output_sample);
    
    // YELLOW PSEUDO-DAC
    YELLOW_AUDIO(output_sample);

    // --- LAMP ---
    // Always blink mode pattern
    lamp_timer++;
    if (lamp_timer > 5000) {
        lamp_timer = 0;
        lamp_pattern_counter++;
        if (lamp_pattern_counter > 20) lamp_pattern_counter = 0; 
    }
    
    int blinks = current_shape_idx + 1;
    bool light_on = false;
    if (lamp_pattern_counter < (blinks * 4)) {
        if ((lamp_pattern_counter % 4) < 2) light_on = true;
    }
    if (light_on) LAMP_ON; else LAMP_OFF;

    REG(I2S_CONF_REG)[0] &= ~(BIT(5)); 
    REG(I2S_INT_CLR_REG)[0]=0xFFFFFFFF;
    REG(I2S_CONF_REG)[0] |= (BIT(5)); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// ==========================================
// GROOVEBOX  --- NEW PRESET
// ==========================================
// topological drum machine
// three drum parts: kick, snare and hi hat
// navigate through axis of drum pattern parts
// earth scans through the drum axis in focus
// each drum axis is a list of 16 step patterns for that drum part
// flip and skip combinations set the earth focus
// only flip = snare
// only skip = hi hat
// both flip and skip = kick
// when focus is changed, the previously playing pattern is held
// the drum pattern changes in time, mid-pattern
// hard code your desired tempo below
// the bar length is constant across patterns
// yellow is a 16th note clock out for syncing external gear
// lamp flashes at the quarter note pulse
// button activates fill mode which increases the energy and inverts the lamp
// ash is downbeat of the bit-crushed kick audio out at line level
// change BPM directly in firmware "target_BPM" variable

// --- MIX CONSOLE ---
// Adjust these values (0 - 128) to balance the parts
// 64 = 100% Volume
// Mix is sent to a saturating limiter
#define MIX_KICK  60   
#define MIX_SNARE 45   
#define MIX_HAT   40

// --- SET BPM ---
#define TARGET_BPM 120 // <<<<<<<<<<< CHANGE TEMPO IN BPM HERE
#define ACTIVE_BPM ((44100 * 15) / TARGET_BPM) // translation to sample ticks

// --- PATTERN BANKS ---
const uint16_t map_kick[16][4] = {
    {0x0001, 0x0001, 0x0001, 0x0001}, {0x0101, 0x0101, 0x0101, 0x0105}, 
    {0x1111, 0x1111, 0x1111, 0x1111}, {0x1111, 0x1111, 0x1111, 0x1313},
    {0x1311, 0x1311, 0x1311, 0x1331}, {0x1113, 0x1113, 0x1113, 0x1133},
    {0x1313, 0x1313, 0x1313, 0x3333}, {0x5555, 0x5555, 0x5555, 0x5555},
    {0x1113, 0x1113, 0x1113, 0x5113}, {0x9111, 0x9111, 0x9111, 0x9151},
    {0x5115, 0x5115, 0x5115, 0x5515}, {0x9595, 0x9595, 0x9595, 0xF595},
    {0x1111, 0x1111, 0x1111, 0x1111}, {0x0411, 0x0411, 0x0411, 0x0451},
    {0x1012, 0x1012, 0x1012, 0x5012}, {0x4111, 0x1141, 0x4111, 0x5141}
};
const uint16_t map_snare[16][4] = {
    {0x0000, 0x0000, 0x0000, 0x0100}, {0x1010, 0x1010, 0x1010, 0x1010},
    {0x1010, 0x1010, 0x1010, 0x5010}, {0x1050, 0x1050, 0x1050, 0x7050},
    {0x1010, 0x1010, 0x1010, 0x5410}, {0x1111, 0x1111, 0x1111, 0x5555},
    {0x0000, 0x1000, 0x0000, 0x1010}, {0x1010, 0x1010, 0x5050, 0xF0F0},
    {0x0400, 0x0400, 0x0400, 0x0410}, {0x1000, 0x1000, 0x1000, 0x1020},
    {0x5014, 0x5014, 0x5014, 0x5054}, {0x5411, 0x5411, 0x5411, 0x5555},
    {0x0000, 0x0000, 0x0000, 0x0000}, {0x1000, 0x1000, 0x1000, 0x5000},
    {0x0000, 0x0400, 0x0000, 0x0400}, {0x0002, 0x2000, 0x0002, 0x2020}
};
const uint16_t map_hat[16][4] = {
    {0x0001, 0x0001, 0x0001, 0x1111}, {0x1111, 0x1111, 0x1111, 0x3111},
    {0x2222, 0x2222, 0x2222, 0x2222}, {0x3333, 0x3333, 0x3333, 0x7333},
    {0x1111, 0x2222, 0x1111, 0x2222}, {0x5555, 0x5555, 0x5555, 0x5555},
    {0x9999, 0x9999, 0x9999, 0xFFFF}, {0xAAAA, 0xAAAA, 0xAAAA, 0xAAAA},
    {0x5151, 0x5151, 0x5151, 0x5555}, {0x1313, 0x1313, 0x1313, 0x1F1F},
    {0xF0F0, 0xF0F0, 0xF0F0, 0xFFFF}, {0x8080, 0x8880, 0x8080, 0x8888},
    {0x2222, 0x2222, 0x2222, 0x2222}, {0x5555, 0x5555, 0x5555, 0x7555},
    {0x1010, 0x1010, 0x1010, 0x1010}, {0x0808, 0x0808, 0x0808, 0x0808}
};

// --- FILL PATTERNS ---
// press button to switch to the fill for the pattern
// button is latching, lamp will indicate fill mode

const uint16_t fill_kick[16][4] = {
    {0x1111, 0x1111, 0x1111, 0x5555}, {0x0101, 0x0505, 0x1515, 0x5555}, 
    {0x1111, 0x1111, 0x1111, 0xFFFF}, {0x1111, 0x1111, 0x1313, 0x3333},
    {0x1311, 0x1311, 0x1331, 0x3333}, {0x1113, 0x1113, 0x1133, 0x3333},
    {0x1313, 0x1313, 0x3333, 0x7777}, {0x5555, 0x5555, 0x5555, 0xFFFF},
    {0x1113, 0x1113, 0x5113, 0x5555}, {0x9111, 0x9111, 0x9151, 0x5555},
    {0x5115, 0x5115, 0x5515, 0x5555}, {0x9595, 0x9595, 0xF595, 0xFFFF},
    {0x1111, 0x1111, 0x1111, 0x5555}, {0x0411, 0x0411, 0x0451, 0x5555},
    {0x1012, 0x1012, 0x5012, 0x5555}, {0x4111, 0x4111, 0x5141, 0xFFFF}
};

const uint16_t fill_snare[16][4] = {
    {0x0000, 0x0000, 0x0100, 0x5555}, {0x1010, 0x1010, 0x1010, 0xF0F0},
    {0x1010, 0x1010, 0x5010, 0xFFFF}, {0x1050, 0x1050, 0x7050, 0xFFFF},
    {0x1010, 0x1010, 0x5410, 0x5555}, {0x1111, 0x1111, 0x5555, 0xFFFF},
    {0x1000, 0x1000, 0x1010, 0x5550}, {0x1010, 0x5050, 0xF0F0, 0xFFFF},
    {0x0400, 0x0400, 0x0410, 0x5555}, {0x1000, 0x1000, 0x1020, 0x5550},
    {0x5014, 0x5014, 0x5054, 0xFFFF}, {0x5411, 0x5411, 0x5555, 0xFFFF},
    {0x0000, 0x0000, 0x5000, 0x5555}, {0x1000, 0x1000, 0x5000, 0x5550},
    {0x0400, 0x0400, 0x0400, 0x5555}, {0x2000, 0x2000, 0x2020, 0x5550}
};

const uint16_t fill_hat[16][4] = {
    {0x0001, 0x0001, 0x1111, 0xFFFF}, {0x1111, 0x1111, 0x3111, 0xFFFF},
    {0x2222, 0x2222, 0x2222, 0xAAAA}, {0x3333, 0x3333, 0x7333, 0xFFFF},
    {0x1111, 0x2222, 0x2222, 0xAAAA}, {0x5555, 0x5555, 0x5555, 0xFFFF},
    {0x9999, 0x9999, 0xFFFF, 0xFFFF}, {0xAAAA, 0xAAAA, 0xAAAA, 0xFFFF},
    {0x5151, 0x5151, 0x5555, 0xFFFF}, {0x1313, 0x1313, 0x1F1F, 0xFFFF},
    {0xF0F0, 0xF0F0, 0xFFFF, 0xFFFF}, {0x8080, 0x8880, 0x8888, 0xFFFF},
    {0x2222, 0x2222, 0x2222, 0xAAAA}, {0x5555, 0x5555, 0x7555, 0xFFFF},
    {0x1010, 0x1010, 0x1010, 0xFFFF}, {0x0808, 0x0808, 0x0808, 0xAAAA}
};

const uint8_t accents[] = { 5, 1, 3, 1,   4, 1, 3, 2,   5, 1, 3, 1,   4, 2, 3, 1 };

void IRAM_ATTR groovebox() {
    
    // --- MAIN ENGINE STATE ---
    static int32_t k_env = 0, h_env = 0;
    static uint32_t k_phase = 0;
    static int32_t k_pitch_env = 0;
    static uint32_t rng_state = 123456789; 
    static int k_vel = 0, h_vel = 0;
    static int hat_hpf1 = 0, hat_hpf2 = 0;
    static int32_t s_env_body = 0, s_env_noise = 0, s_env_click = 0, s_env_ring = 0;
    static uint32_t s_phase_body = 0, s_phase_ring = 0;
    static int32_t s_pitch_body = 0;
    static int s_noise_hpf = 0, s_vel = 0, s_flam_timer = 0;
    static int32_t eq_low_band = 0, eq_wide_band = 0;

    // --- ASH ENGINE STATE ---
    static int32_t ak_env = 0;       
    static uint32_t ak_phase = 0;    
    static int32_t ak_pitch_env = 0; 
    static int32_t as_env = 0;       
    static int32_t ah_env = 0;       
    static uint16_t lfsr = 1;     

    // --- SEQUENCER STATE ---
    static int env_timer = 0;
    static uint32_t dm_step_timer = 0;
    static int dm_step = 0, bar_counter = 0, lamp_flash_timer = 0; 
    static int active_kick_idx = 2, active_snare_idx = 1, active_hat_idx = 2;   
    static int next_coord_x = 100, next_coord_y = 50, next_coord_z = 50;  
    static bool fill_mode_latched = false; // Button latches for Fill Mode 
    static bool prev_btn_state = false; // Button latches for Fill Mode 

    // --- EARTH CALIBRATION STATES ---
    static int cal_min = 4095; 
    static int cal_max = 0; 
    static bool knob_moved = false;
    static int initial_earth = 0;
    static int s_earth = -1;
    static int boot_timer = 0;

    // --- WAKE UP BLOCK ---
    static bool was_in_menu = false;
    if (preset_mode) {
        was_in_menu = true;
    } else if (was_in_menu) {
        was_in_menu = false;
        
        cal_min = 4095;
        cal_max = 0;
        s_earth = -1;
        knob_moved = false;
        boot_timer = 0;
        fill_mode_latched = false;  // prevents entering preset in fill mode 
        prev_btn_state = BUTTON_PRESSED;
    }

    // --- EARTH CALIBRATION ---
    REG(APB_SARADC_SAR1_PATT_TAB1_REG)[0] = (0x0C<<24) | (0x6C<<16);
    uint32_t fifo_data = REG(I2S_FIFO_RD_REG)[0];
    int channel = (fifo_data >> 12) & 0xF; 
    int value   = (fifo_data & 0xFFF);     
    
    if (channel == 0) {
        if (s_earth == -1) s_earth = value;
        else s_earth += (value - s_earth) >> 4;
    }

    if (boot_timer < 2000) {
        boot_timer++;
        initial_earth = s_earth;
    }

    if (!knob_moved && boot_timer >= 2000) {
        int drift = s_earth - initial_earth;
        if (drift < 0) drift = -drift;
        if (drift > 300) {
            knob_moved = true; 
        }
    }

    if (knob_moved) {
        if (s_earth < cal_min) cal_min = s_earth;
        if (s_earth > cal_max) cal_max = s_earth;
    } else {
        cal_min = initial_earth;
        cal_max = initial_earth;
    }

    int range = cal_max - cal_min;
    if (range < 50) range = 50;
    int reading = s_earth - cal_min;
    if (reading < 0) reading = 0;
    
    int earth_cal = (reading * 255) / range;
    if (!knob_moved) earth_cal = 100;  

    // --- SEQUENCER STATE ---

    // CONTROLS
    bool pin_skip = SKIPPERAT; bool pin_flip = FLIPPERAT; bool btn = BUTTON_PRESSED; 

    // LATCHING FILL TOGGLE
    if (btn && !prev_btn_state) {
        fill_mode_latched = !fill_mode_latched;
    }
    prev_btn_state = btn;

    
    if (pin_skip && pin_flip) next_coord_x = earth_cal; 
    else if (pin_flip)        next_coord_y = earth_cal; 
    else if (pin_skip)        next_coord_z = earth_cal; 
    

    // SEQUENCER
    dm_step_timer++;
    if (dm_step_timer >= ACTIVE_BPM) {
        dm_step_timer = 0;
        
        if ((dm_step & 3) == 0) lamp_flash_timer = 2000; 

        if (btn) {
            active_kick_idx = 4; active_snare_idx = 7; active_hat_idx = 7;
        } else {
            int cx = next_coord_x; if(cx>255) cx=255;
            int cy = next_coord_y; if(cy>255) cy=255;
            int cz = next_coord_z; if(cz>255) cz=255;
            active_kick_idx = cx >> 4; if (active_kick_idx > 15) active_kick_idx = 15;
            active_snare_idx = cy >> 4; if (active_snare_idx > 15) active_snare_idx = 15;
            active_hat_idx = cz >> 4; if (active_hat_idx > 15) active_hat_idx = 15;
        }

        dm_step = (dm_step + 1) & 15; 
        if (dm_step == 0) bar_counter = (bar_counter + 1) & 3; 

        int acc = accents[dm_step]; 

        // --- TRIGGER LOGIC ---
        
        // KICK

        // KICK
        uint16_t current_k_pat = (fill_mode_latched) ? fill_kick[active_kick_idx][bar_counter] : map_kick[active_kick_idx][bar_counter];
        if (current_k_pat & (1 << dm_step)) {
            // MAIN: Always fires on trigger
            k_env = (acc >= 4) ? 3800 : 3200; k_pitch_env = 3000; k_phase = 0; k_vel = (acc >= 4) ? 2 : 1; 
            
            // ASH: ONLY fires on Velocity 5 (Beats 1 & 3)
            if (acc == 5) {
                ak_env = 4095; ak_pitch_env = 4000; ak_phase = 0;
            }
        }

        // SNARE
        bool s_trig = false;
        uint16_t current_s_pat = (fill_mode_latched) ? fill_snare[active_snare_idx][bar_counter] : map_snare[active_snare_idx][bar_counter];
        if (current_s_pat & (1 << dm_step)) s_trig = true;
        
        if (s_trig) {
            switch(acc) {
                case 5: s_env_body=2500; s_env_noise=3000; s_env_click=0; s_env_ring=0; s_pitch_body=800; s_phase_body=0; s_vel=3; s_flam_timer=40; break;
                case 4: s_env_body=4095; s_env_noise=4095; s_env_click=4095; s_env_ring=2000; s_pitch_body=1300; s_phase_body=0; s_vel=2; break;
                case 3: s_env_body=4095; s_env_noise=4095; s_env_click=3000; s_env_ring=1000; s_pitch_body=1100; s_phase_body=0; s_vel=2; break;
                case 2: s_env_body=3000; s_env_noise=3500; s_env_click=1000; s_env_ring=0; s_pitch_body=800; s_phase_body=0; s_vel=2; break;
                case 1: s_env_body=1500; s_env_noise=2500; s_env_click=0; s_env_ring=0; s_pitch_body=400; s_phase_body=0; s_vel=1; break;
            }
            // Ash Snare
            as_env = (acc >= 4) ? 4095 : (acc >= 2) ? 3000 : 1500;
        }

        // HAT
        uint16_t current_h_pat = (fill_mode_latched) ? fill_hat[active_hat_idx][bar_counter] : map_hat[active_hat_idx][bar_counter];
        if (current_h_pat & (1 << dm_step)) {
            int syn_vel = 1; if (acc >= 4) syn_vel = 3; else if (acc == 3) syn_vel = 2;
            h_env = (acc >= 3) ? 4095 : 2000; h_vel = syn_vel;
            // Ash Hat
            ah_env = (acc >= 3) ? 2000 : 1000; 
        }
    }
    
    // --- SYNTHESIS ---
    bool update_grit = false;
    env_timer++;
    if (env_timer >= 16) { 
        env_timer = 0; update_grit = true;
        
        if (s_flam_timer > 0) { s_flam_timer--; if (s_flam_timer == 0) { s_env_body=4095; s_env_noise=4095; s_env_click=4095; s_env_ring=3000; s_pitch_body=1500; s_phase_body=0; } }
        
        // DYNAMIC LAMP VISUALS
        if (fill_mode_latched) {
            // Fill Mode: Mostly ON, flashes OFF on the pulse
            if (lamp_flash_timer > 0) { lamp_flash_timer -= 16; LAMP_OFF; } else { LAMP_ON; }
        } else {
            // Normal Mode: Mostly OFF, flashes ON on the pulse
            if (lamp_flash_timer > 0) { lamp_flash_timer -= 16; LAMP_ON; } else { LAMP_OFF; }
        }

        if (ak_env > 0) { ak_env -= 10; ak_pitch_env -= 60; if (ak_pitch_env < 0) ak_pitch_env = 0; }
        if (as_env > 0) as_env -= 20; 
        if (ah_env > 0) ah_env -= 40; 
    }
    
    // MAIN OUTPUT SOUNDS
    int32_t kick_out = 0;
    if (k_env > 0) {
        if (update_grit) { int dec = (k_vel >= 2) ? 18 : 16; k_env -= dec; k_pitch_env -= 45; if (k_pitch_env < 0) k_pitch_env = 0; }
        k_phase += (1200 + k_pitch_env) << 12; 
        int pos = k_phase >> 20; int tri = (pos > 2048) ? (4096 - pos) : pos; tri -= 1024; 
        kick_out = (tri * k_env * 2) >> 12;
    }
    
    int32_t snare_out = 0;
    if (s_env_body > 0 || s_env_noise > 0) {
        if (update_grit) {
            s_env_click -= 400; s_env_body -= 20; s_env_noise -= 15; s_env_ring -= 10;
            if (s_env_click < 0) s_env_click = 0; if (s_env_body < 0) s_env_body = 0;
            if (s_env_noise < 0) s_env_noise = 0; if (s_env_ring < 0) s_env_ring = 0;
            s_pitch_body -= 60; if (s_pitch_body < 0) s_pitch_body = 0;
        }
        s_phase_body += (5500 + s_pitch_body) << 12; 
        int pos = s_phase_body >> 20; int tri = (pos > 2048) ? (4096 - pos) : pos; tri -= 1024;
        int body_sig = (tri * s_env_body) >> 12;
        rng_state = (rng_state * 1664525) + 1013904223;
        int noise_raw = (rng_state & 0x80000000) ? 4000 : -4000;
        s_noise_hpf += (noise_raw - s_noise_hpf) >> 2; 
        int noise_sig = ((noise_raw - s_noise_hpf) * s_env_noise) >> 12;
        int click_sig = (noise_raw * s_env_click) >> 12;
        s_phase_ring += (12000) << 12; 
        int r_pos = s_phase_ring >> 20;
        int ring_sig = ((r_pos > 2048 ? 4096-r_pos : r_pos) - 1024) * s_env_ring >> 12;
        snare_out = body_sig + noise_sig + click_sig + ring_sig;
    }
    
    int32_t hat_out = 0;
    if (h_env > 0) {
        if (update_grit) { int decay = (h_vel == 3) ? 5 : (h_vel == 2) ? 12 : 24; h_env -= decay; }
        rng_state = (rng_state * 1664525) + 1013904223;
        int raw_noise = (rng_state & 0x80000000) ? 2500 : -2500;
        hat_hpf1 += (raw_noise - hat_hpf1) >> 1; int stage1 = raw_noise - hat_hpf1;
        hat_hpf2 += (stage1 - hat_hpf2) >> 1; int stage2 = stage1 - hat_hpf2;
        hat_out = (stage2 * h_env) >> 12;
    }
    
    int32_t mk = (kick_out * MIX_KICK) >> 6;
    int32_t ms = (snare_out * MIX_SNARE) >> 6;
    int32_t mh = (hat_out * MIX_HAT) >> 6;
    int32_t sum = mk + ms + mh;

    eq_low_band += (sum - eq_low_band) >> 3;
    eq_wide_band += (sum - eq_wide_band) >> 1; 
    int32_t highs = sum - eq_wide_band;
    int32_t lows = eq_low_band;
    int32_t mids = sum - lows - highs; 
    int32_t eq_sum = ((lows * 320) >> 8) + ((mids * 192) >> 8) + ((highs * 280) >> 8); 

    int32_t limit_out = eq_sum;
    if (eq_sum > 2048) { int32_t over = eq_sum - 2048; limit_out = 2048 + over - ((over * over) >> 15); } 
    else if (eq_sum < -2048) { int32_t under = -2048 - eq_sum; limit_out = -2048 - (under - ((under * under) >> 15)); }
    if (limit_out > 4095) limit_out = 4095; if (limit_out < -4095) limit_out = -4095;

    DACWRITER(limit_out + 2048); 
    
    // ASH SOUNDS
    int32_t ash_k_out = 0;
    if (ak_env > 0) {
        ak_phase += (1500 + ak_pitch_env) << 12;
        ash_k_out = ((ak_phase >> 20) & 2048) ? ak_env : -ak_env;
    }

    uint16_t lfsr_bit = ((lfsr >> 0) ^ (lfsr >> 2)) & 1;
    lfsr = (lfsr >> 1) | (lfsr_bit << 14);
    int32_t ash_noise = (lfsr & 1) ? 4000 : -4000;
    int32_t ash_s_out = (as_env > 0) ? (ash_noise * as_env) >> 12 : 0;
    int32_t ash_h_out = (ah_env > 0) ? (ash_noise * ah_env) >> 12 : 0;
    
    int32_t ash_sum = ash_k_out + ash_s_out + ash_h_out;
    if (ash_sum > 2048) ash_sum = 2048;
    if (ash_sum < -2048) ash_sum = -2048;
    
    // ASH OUTPUT
    CLEAN_ASHWRITER(ash_sum + 2048);

    // YELLOW SYNC OUT
    // 4 PPQN CLOCK
    if (dm_step_timer < (ACTIVE_BPM >> 1)) {
        // Accent the very first step of the sequence
        if (dm_step == 0) {
            YELLOW_AUDIO(4095); // 3.3V ACCENT
        } else {
            YELLOW_AUDIO(3000); // 2.4V clock
        }
    } else {
        YELLOW_AUDIO(0);
    }

    
    REG(I2S_CONF_REG)[0] &= ~(BIT(5)); 
    REG(I2S_INT_CLR_REG)[0]=0xFFFFFFFF;
    REG(I2S_CONF_REG)[0] |= (BIT(5)); 
}

/////////////////////////////////////////////////////////END//////////////////////////////////////////////////////

// ==========================================
// POLYRHYTHMS --- NEW PRESET
// ==========================================
// topographic drum sequencer based on different polyrhythmic divisions of a bar
// sampled drum sound engine. Make sure to include drums.h 
// there are three drums: kick, snare and hi hat, each with three sampled velocities
// there are 29 unique patterns per drum voice
// patterns are conscruted by different divisions of the bar
// all patterns are the same length and overlap with each other in one bar
// in total, there are 24,389 distinct pattern combinations
// navigate the drum pattern topology with earth
// direction of navigation controlled by flip and skip
// flip is ON then earth scans the snare patterns
// skip is ON then earth scans the hi hat patterns
// BOTH flip AND skip are ON then earth scans the kick patterns
// when both flip and skip are OFF, earth signal is ignored
// the button freezes the selected patterns
// yellow is a 16th note clock pulse to sync external devices
// lamp flashes on the start of every bar

//#include "drums.h" // drum samples
// make your own drum samples with wav2header_multi.py
// run in folder with samples named kick1.wav, kick2....snare1.wav....hat3.wav
// it will generate drums.h
// sample must be short and low res
// 8 bit mono 22kHz
// each drum has three velocities
// kick ~0.5 sec per velocity
// snare ~0.2 sec per velocity
// hi hate ~0.05 sec per velocity

/////-------SAMPLE MANAGEMENT
// global pointers defined in stuff.h
extern uint8_t *current_kick[3];
extern uint8_t *current_snare[3];
extern uint8_t *current_hat[3];
extern int len_kick[3];
extern int len_snare[3];
extern int len_hat[3];
///////////END

// --- SETTINGS ---

#define ACTIVE_BPM 120  // Change TEMPO BPM here 

// --- MIXER CONSOLE ---
// volume mix (0-31)
#define MIX_KICK   35
#define MIX_SNARE  32
#define MIX_HAT    40

// --- LOGIC CONSTANTS ---
#define TRANSITION_LEN 6930000 // duration of pattern crossfade (1 Beat)
#define SAMPLE_PERIOD 1732500   // rate of earth sampling (1/16th Note)
#define ZONE_HYST 40         // earth stability threshold (higher stabler)   

// ARCHITECTURE: the grid is based on the least common multiple (3360 ticks) supports divisions of 
// 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 20, 24, 28, 32.
// this allows polyrhythms (e.g. 5 against 7) to align and weave together

// --- GRID MATH ---
#define TICKS_PER_BAR 3360
#define TOTAL_LOOP    13440 
#define GRID_SCALER   8250  
#define LCM_BAR       110880000

// Interval Look-Up Table: How many ticks per step for each Zone?
// e.g. Zone 0 (Whole Note) = 3360 ticks. Zone 15 (32nd) = 105 ticks.
const uint16_t INTERVALS[16] = {
    3360, 1680, 1120, 840, 840, 560, 480, 420, 
    420, 280, 280, 168, 210, 210, 140, 105
};

// Step Count Look-Up Table: How many steps in 4 Bars?
const uint8_t STEPS_COUNT[16] = {
    4, 8, 12, 16, 16, 24, 28, 32, 32, 48, 48, 80, 64, 64, 96, 128
};

// Pattern Bank Count: How many variations does this Zone have?
const uint8_t PAT_VARS[16] = {
    1, 1, 1, 1, 2, 2, 3, 2, 2, 2, 2, 2, 3, 3, 1, 1
};

// --- PATTERNS ---
// 3=Accent, 2=Med, 1=Soft , 0=Rest

// KICK
const uint8_t t_kick[16][192] = {
    {3,3,3,3}, 
    {3,0,3,2, 3,0,3,2}, 
    {3,0,2, 3,0,2, 3,0,2, 3,2,0}, 
    {3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3},
    {3,0,0,3, 0,0,3,0, 3,0,0,3, 0,0,3,2, 3,0,0,0, 3,0,0,3, 0,0,3,0, 0,0,3,0}, 
    {3,0,2,0,2,0, 3,0,2,0,2,0, 3,0,2,0,2,0, 3,2,0,3,0,0, 3,0,0,0,2,0, 0,0,3,0,0,0, 3,0,0,0,2,0, 3,0,2,0,3,0}, 
    // Septuplet
    {3,0,2,0,2,0,0, 3,0,2,0,2,0,0, 3,0,2,0,2,0,0, 3,2,3,2,3,2,0, 
     3,0,0,2,0,0,0, 3,0,0,0,2,0,0, 3,0,0,2,0,0,0, 3,0,0,0,3,0,0, 
     3,0,2,0,3,0,0, 2,0,3,0,2,0,0, 3,0,2,0,3,0,0, 2,0,1,3,2,1,0},
    // 8ths 
    {3,0,2,0,3,0,2,0, 3,0,2,0,3,0,2,0, 3,0,2,0,3,0,2,0, 3,2,3,2,3,2,0,0, 3,0,2,0,3,0,2,0, 3,0,0,0,3,0,2,0, 3,0,2,0,3,0,0,0, 3,2,0,2,3,2,0,0},
    {3,0,0,0,3,0,2,0, 0,0,3,0,2,0,0,0, 3,0,0,0,3,0,2,0, 3,2,0,2,3,0,2,0, 
     3,0,1,0,0,0,3,0, 0,0,2,0,3,0,1,0, 3,0,1,0,0,0,3,0, 3,2,0,2,3,0,0,0},
    // 12/8
    {3,0,0,2,0,0, 3,0,0,2,0,0, 3,0,0,2,0,0, 3,0,0,2,0,0, 3,0,0,2,0,0, 3,0,0,2,0,0, 3,0,0,2,0,0, 3,2,0,3,2,0, 
     3,0,0,0,0,0, 3,0,0,0,0,0, 3,0,0,2,0,0, 3,0,0,0,0,0, 3,0,0,0,0,0, 3,0,0,0,0,0, 3,0,0,2,0,0, 3,0,1,3,0,1}, // Var 2: Blues shuffle ghosts
    {3,0,2,0,3,0, 2,0,3,0,2,0, 3,0,2,0,3,0, 2,0,3,0,2,0, 3,0,2,0,3,0, 2,0,3,0,2,0, 3,0,2,0,3,0, 2,0,3,0,2,0, 3,0,0,2,0,0, 3,0,0,2,0,0, 3,0,0,2,0,0, 3,0,0,2,0,0, 3,0,0,2,0,0, 3,0,0,2,0,0, 3,0,0,2,0,0, 3,0,0,2,0,0},
    {3,0,0,2,0, 3,0,0,2,0, 3,0,0,2,0, 3,0,0,2,0, 3,0,0,2,0, 3,0,0,2,0, 3,0,0,2,0, 3,0,0,2,0, 3,0,0,2,0, 3,0,0,2,0, 3,0,0,2,0, 3,0,0,2,0, 3,0,0,2,0, 3,0,0,2,0, 3,0,0,2,0, 3,2,0,3,2, 3,0,0,0,0, 0,0,0,3,0, 0,0,0,0,0, 3,0,0,0,0, 3,0,0,0,0, 0,0,0,3,0, 0,0,0,0,0, 3,0,0,0,0, 3,0,0,0,0, 0,0,0,3,0, 0,0,0,0,0, 3,0,0,0,0, 3,0,0,0,0, 0,0,0,3,0, 0,0,0,0,0, 3,0,0,0,0},
    // 16ths 
    {3,0,2,0,3,0,2,0, 3,0,2,0,3,0,2,0, 3,0,2,0,3,0,2,0, 3,0,2,0,3,0,2,0, 3,0,2,0,3,0,2,0, 3,0,2,0,3,0,2,0, 3,0,2,0,3,0,2,0, 3,2,0,2,3,2,0,2, 
     3,0,2,0,2,0,2,0, 3,0,2,0,2,0,2,0, 3,0,2,0,2,0,2,0, 3,0,2,0,2,0,2,0, 3,0,2,0,2,0,2,0, 3,0,2,0,2,0,2,0, 3,0,2,0,2,0,2,0, 3,0,2,0,2,0,2,0, 
     3,0,0,0,3,0,0,0, 2,0,0,0,3,0,0,0, 3,0,0,0,3,0,0,0, 2,0,0,0,3,0,0,0, 3,0,0,0,3,0,0,0, 2,0,0,0,3,0,0,0, 3,0,0,0,3,0,0,0, 3,2,3,2,3,2,3,2},
    {3,0,0,0,2,0,0,2, 0,0,3,0,0,0,2,0, 3,0,0,0,2,0,0,2, 0,0,3,0,0,0,2,0, 3,0,0,0,2,0,0,2, 0,0,3,0,0,0,2,0, 3,0,0,0,2,0,0,2, 3,2,0,3,2,0,2,0, 
     3,0,1,0,0,0,0,2, 0,0,3,0,0,0,1,0, 3,0,1,0,0,0,0,2, 0,0,3,0,0,0,1,0, 3,0,1,0,0,0,0,2, 0,0,3,0,0,0,1,0, 3,0,1,0,0,0,0,2, 0,0,3,0,0,0,1,0,
     3,0,2,0,0,0,3,0, 0,0,2,0,3,0,0,0, 3,0,2,0,0,0,3,0, 0,0,2,0,3,0,0,0, 3,0,2,0,0,0,3,0, 0,0,2,0,3,0,0,0, 3,0,2,0,0,0,3,0, 0,0,2,0,3,0,0,0},
    {3,2,3,2,3,2, 3,2,3,2,3,2, 3,2,3,2,3,2, 3,2,3,2,3,2, 3,2,3,2,3,2, 3,2,3,2,3,2, 3,2,3,2,3,2, 3,2,3,2,3,2, 3,2,3,2,3,2, 3,2,3,2,3,2, 3,2,3,2,3,2, 3,2,3,2,3,2, 3,2,3,2,3,2, 3,2,3,2,3,2, 3,2,3,2,3,2, 3,3,3,3,3,3},
    {3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3}
};

// SNARE
const uint8_t t_snare[16][192] = {
    {0,0,0,0}, 
    {0,3,0,0, 0,3,0,2}, 
    {0,3,0, 0,3,0, 0,3,0, 2,3,0}, 
    {0,3,0,3, 0,3,0,3, 0,3,0,3, 0,3,2,3}, 
    {0,0,3,0, 0,0,3,0, 0,0,3,0, 0,0,3,0, 0,0,3,0, 0,0,3,0, 0,0,3,0, 0,2,3,2}, 
    {0,0,3,0,0,0, 0,0,3,0,0,0, 0,0,3,0,0,0, 0,0,3,0,0,0, 0,0,3,0,0,0, 0,0,3,0,0,0, 0,0,3,0,0,0, 0,0,3,0,0,0}, 
    {0,0,0,3,0,0,0, 0,0,0,3,0,0,0, 0,0,0,3,0,0,0, 0,0,2,3,2,0,0, 0,0,0,3,0,0,0, 0,0,0,3,0,0,0, 0,0,0,3,0,0,0, 0,0,0,3,0,0,0, 0,0,0,3,0,0,0, 0,0,0,3,0,0,0, 0,0,0,3,0,0,0, 0,2,0,3,0,2,0}, 
    {0,0,3,0,0,0,3,0, 0,0,3,0,0,0,3,0, 0,0,3,0,0,0,3,0, 0,0,3,0,0,0,3,0, 
     0,0,3,0,0,0,3,0, 0,0,3,0,0,0,3,0, 0,0,3,0,0,0,3,0, 0,2,3,0,2,0,3,0}, 
    // Break 
    {0,0,3,0,0,1,3,0, 0,2,3,0,0,1,3,0, 0,0,3,0,0,2,3,0, 0,1,3,0,2,0,3,2, 
     0,0,3,0,0,0,3,0, 0,0,3,0,0,0,3,0, 0,0,3,0,0,0,3,0, 0,0,3,0,2,0,3,0}, 
    {0,0,0,3,0,0, 0,0,0,3,0,0, 0,0,0,3,0,0, 0,0,0,3,0,0, 0,0,0,3,0,0, 0,0,0,3,0,0, 0,0,0,3,0,0, 0,0,0,3,0,0, 0,0,0,3,0,0, 0,0,0,3,0,0, 0,0,0,3,0,0, 0,0,0,3,0,0, 0,0,0,3,0,0, 0,0,0,3,0,0, 0,0,0,3,0,0, 0,0,0,3,0,0}, 
    {0,0,0,3,0,0, 0,0,0,3,0,0, 0,0,0,3,0,0, 0,0,0,3,0,0, 0,0,0,3,0,0, 0,0,0,3,0,0, 0,0,0,3,0,0, 0,2,0,3,0,0, 0,0,0,3,0,0, 0,0,0,3,0,0, 0,0,0,3,0,0, 0,0,0,3,0,0, 0,0,0,3,0,0, 0,0,0,3,0,0, 0,0,0,3,0,0, 0,0,0,3,0,0}, 
    {0,0,0,0,0, 3,0,0,0,0, 0,0,0,0,0, 3,0,0,0,0, 0,0,0,0,0, 3,0,0,0,0, 0,0,0,0,0, 3,0,0,0,0, 0,0,0,0,0, 3,0,0,0,0, 0,0,0,0,0, 3,0,0,0,0, 0,0,0,0,0, 3,0,0,0,0, 0,0,0,0,0, 3,0,0,0,0, 0,0,0,0,0, 3,0,0,0,0, 0,0,0,0,0, 3,0,0,0,0, 0,0,0,0,0, 3,0,0,0,0, 0,0,0,0,0, 3,0,0,0,0, 0,0,0,0,0, 3,0,0,0,0},
    {0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,0},
    // 16ths Funky 
    {0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,2, 0,0,0,0,3,0,0,0, 0,0,2,0,3,0,2,0, 0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,2, 0,0,0,0,3,0,0,0, 0,0,2,0,3,0,2,0, 
     0,1,0,1,3,0,1,0, 0,1,0,0,3,0,0,2, 0,1,0,1,3,0,1,0, 0,0,2,0,3,0,2,0, 0,1,0,1,3,0,1,0, 0,1,0,0,3,0,0,2, 0,1,0,1,3,0,1,0, 0,0,2,0,3,0,2,0, 
     0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,0, 0,0,0,0,3,0,0,0},
    {0}, {0}
};

// HAT
const uint8_t t_hat[16][192] = {
    {1,2,1,2}, 
    {1,2,1,2, 1,2,1,2}, 
    {1,2,1, 1,2,1, 1,2,1, 1,2,3},
    {1,2,1,2, 1,2,1,2, 1,2,1,2, 1,2,3,2}, 
    {1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1}, 
    {1,2,1,2,1,2, 1,2,1,2,1,2, 1,2,1,2,1,2, 1,2,1,2,1,2, 1,2,1,2,1,2, 1,2,1,2,1,2, 1,2,1,2,1,2, 1,2,1,2,1,2}, 
    {1,2,1,2,1,2,1, 1,2,1,2,1,2,1, 1,2,1,2,1,2,1, 1,2,3,2,1,2,1, 1,2,1,2,1,2,1, 1,2,1,2,1,2,1, 1,2,1,2,1,2,1, 1,2,3,2,1,2,1, 1,2,1,2,1,2,1, 1,2,1,2,1,2,1, 1,2,1,2,1,2,1, 1,2,3,2,1,2,1}, 
    {1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 3,2,3,2,3,2,3,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 3,2,3,2,3,2,3,2}, 
    {1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2}, 
    {1,2,1,2,1,2, 1,2,1,2,1,2, 1,2,1,2,1,2, 1,2,1,2,1,2, 1,2,1,2,1,2, 1,2,1,2,1,2, 1,2,1,2,1,2, 3,2,3,2,3,2, 1,2,1,2,1,2, 1,2,1,2,1,2, 1,2,1,2,1,2, 1,2,1,2,1,2, 1,2,1,2,1,2, 1,2,1,2,1,2, 1,2,1,2,1,2, 3,2,3,2,3,2}, 
    {1,2,1,2,1,2, 1,2,1,2,1,2, 1,2,1,2,1,2, 1,2,1,2,1,2, 1,2,1,2,1,2, 1,2,1,2,1,2, 1,2,1,2,1,2, 1,2,1,2,1,2, 1,2,1,2,1,2, 1,2,1,2,1,2, 1,2,1,2,1,2, 1,2,1,2,1,2, 1,2,1,2,1,2, 1,2,1,2,1,2, 1,2,1,2,1,2, 1,2,1,2,1,2}, 
    {1,2,1,2,1, 1,2,1,2,1, 1,2,1,2,1, 1,2,1,2,1, 1,2,1,2,1, 1,2,1,2,1, 1,2,1,2,1, 1,2,1,2,1, 
     1,2,1,2,1, 1,2,1,2,1, 1,2,1,2,1, 1,2,1,2,1, 1,2,1,2,1, 1,2,1,2,1, 1,2,1,2,1, 1,2,1,2,1,
     1,2,1,2,1, 1,2,1,2,1, 1,2,1,2,1, 1,2,1,2,1, 1,2,1,2,1, 1,2,1,2,1, 1,2,1,2,1, 1,2,1,2,1},
    {1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 3,2,3,2,3,2,3,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 3,2,3,2,3,2,3,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 3,2,3,2,3,2,3,2},
    {1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2, 1,2,1,2,1,2,1,2},
    {1,1,1,1,1,1, 1,1,1,1,1,1, 1,1,1,1,1,1, 1,1,1,1,1,1, 1,1,1,1,1,1, 1,1,1,1,1,1, 1,1,1,1,1,1, 1,1,1,1,1,1, 1,1,1,1,1,1, 1,1,1,1,1,1, 1,1,1,1,1,1, 1,1,1,1,1,1, 1,1,1,1,1,1, 1,1,1,1,1,1, 1,1,1,1,1,1, 3,3,3,3,3,3},
    {1}
};

void IRAM_ATTR polyrhythms() {

    //SAMPLE MANAGEMENT POINTER
    static uint8_t *k_ptr = current_kick[0]; 
    static uint8_t *s_ptr = current_snare[0]; 
    static uint8_t *h_ptr = current_hat[0];
    //END

    static uint32_t k_len=0, s_len=0, h_len=0;
    static uint32_t k_idx=0, s_idx=0, h_idx=0;
    static bool k_play=false, s_play=false, h_play=false;
    static int32_t k_vel=0, s_vel=0, h_vel=0;

    static uint32_t ash_xor = 123456789;
    static int ash_lpf=0;
    static uint8_t ctrl_div=0;
    static int timer=0; 
    
    // --- EARTH STATE ---
    static int cal_min = 4095; 
    static int cal_max = 0; 
    static bool knob_moved = false;
    static int initial_earth = 0;
    static int s_earth = -1;
    static int boot_timer = 0;
    
    static int earth_zone_kick = 3;
    static int earth_zone_snare = 3;
    static int earth_zone_hat = 3;
    static int next_zone_kick = 3;
    static int next_zone_snare = 3;
    static int next_zone_hat = 3;
    
    static int k_trans_timer = 0;
    static int s_trans_timer = 0;
    static int h_trans_timer = 0;
    static int earth_sample_timer = 0;
    static int last_earth_val = 0;

    // Master Clock State
    static uint32_t master_bar = 0;
    static int last_idx_k=-1, last_idx_s=-1, last_idx_h=-1;
    
    static bool is_frozen = false;
    static bool last_frozen = false;

    // --- WAKE UP BLOCK --- 
    static bool was_in_menu = false;
    if (preset_mode) {
        was_in_menu = true;
    } else if (was_in_menu) {
        was_in_menu = false;

        // Generate a random beat on boot
        earth_zone_kick = rand() % 16;
        earth_zone_snare = rand() % 16;
        earth_zone_hat = rand() % 16;
        next_zone_kick = earth_zone_kick;
        next_zone_snare = earth_zone_snare;
        next_zone_hat = earth_zone_hat;
        
        cal_min = 4095;
        cal_max = 0;
        s_earth = -1;
        knob_moved = false;
        boot_timer = 0;
        last_frozen = audio_frozen_state;
        is_frozen = audio_frozen_state;
    }

    // --- EARTH CALIBRATION ---
    REG(APB_SARADC_SAR1_PATT_TAB1_REG)[0] = (0x0C<<24) | (0x6C<<16);
    uint32_t fifo_data = REG(I2S_FIFO_RD_REG)[0];
    int channel = (fifo_data >> 12) & 0xF; 
    int value   = (fifo_data & 0xFFF);     
    
    if (channel == 0) {
        if (s_earth == -1) s_earth = value;
        else s_earth += (value - s_earth) >> 4;
    }

    if (boot_timer < 2000) {
        boot_timer++;
        initial_earth = s_earth;
    }

    if (!knob_moved && boot_timer >= 2000) {
        int drift = s_earth - initial_earth;
        if (drift < 0) drift = -drift;
        if (drift > 300) {
            knob_moved = true; 
        }
    }

    if (knob_moved) {
        if (s_earth < cal_min) cal_min = s_earth;
        if (s_earth > cal_max) cal_max = s_earth;
    } else {
        cal_min = initial_earth;
        cal_max = initial_earth;
    }

    int range = cal_max - cal_min;
    if (range < 50) range = 50;
    int reading = s_earth - cal_min;
    if (reading < 0) reading = 0;
    
    int earth_cal = (reading * 255) / range;
    if (!knob_moved) earth_cal = last_earth_val; // Anchor!

    // ============================
    // CONTROL LOGIC
    // ============================
    ctrl_div++;
    if (ctrl_div >= 16) {
        ctrl_div = 0;
        timer++;
        
        // DECAY
        if (k_vel > 0) k_vel -= 8; 
        if (s_vel > 0) s_vel -= 8; 
        if (h_vel > 0) h_vel -= 12;

        earth_sample_timer += 1; 
        if (earth_sample_timer > 344 && !is_frozen) {
            earth_sample_timer = 0; 
            int diff = earth_cal - last_earth_val;
            if (diff < 0) diff = -diff;
            if (diff > ZONE_HYST) {
                last_earth_val = earth_cal;
                int new_zone = (earth_cal * 16) / 256;
                if (new_zone > 15) new_zone = 15;
                bool pin_skip = SKIPPERAT;
                bool pin_flip = FLIPPERAT;
                if (!pin_skip && !pin_flip) { }
                else {
                    if (pin_skip && pin_flip) { if (new_zone != earth_zone_kick) { next_zone_kick = new_zone; k_trans_timer = 1000; } }
                    else if (pin_skip) { if (new_zone != earth_zone_hat) { next_zone_hat = new_zone; h_trans_timer = 1000; } }
                    else if (pin_flip) { if (new_zone != earth_zone_snare) { next_zone_snare = new_zone; s_trans_timer = 1000; } }
                }
            }
        }

        if (k_trans_timer > 0) { k_trans_timer--; if (k_trans_timer <= 0) earth_zone_kick = next_zone_kick; }
        if (s_trans_timer > 0) { s_trans_timer--; if (s_trans_timer <= 0) earth_zone_snare = next_zone_snare; }
        if (h_trans_timer > 0) { h_trans_timer--; if (h_trans_timer <= 0) earth_zone_hat = next_zone_hat; }


        // BUTTON (Mode Toggle via OS Short Press)
        if (audio_frozen_state != last_frozen) {
            last_frozen = audio_frozen_state;
            is_frozen = audio_frozen_state;
        }

        if (is_frozen) { LAMP_ON; } 
        else { if ((master_bar % 27720000) < 3465000) { LAMP_ON; } else { LAMP_OFF; } }

        // --- YELLOW SYNC OUT (4 PPQN) ---
        // isolate the 16th Note Window (1,732,500 ticks)
        if ((master_bar % 1732500) < 500000) { 
            
            // if DOWNBEAT (1 Bar = 27,720,000) accent pulse
            if ((master_bar % 27720000) < 1732500) { 
                YELLOW_AUDIO(4095); // 3.3V ACCENT (for reset)
            } else {
                YELLOW_AUDIO(3000); // 2.4V clock
            }
        } else {
            YELLOW_AUDIO(0);
        }
    } 

    // ============================
    // AUDIO PROCESSING
    // ============================
    master_bar += (ACTIVE_BPM * 262) / 100;
    if (master_bar >= LCM_BAR) {
        master_bar -= LCM_BAR;
        last_idx_k = -1; last_idx_s = -1; last_idx_h = -1; 
    }

    static uint32_t last_grid_tick = 0xFFFFFFFF;
    uint32_t grid_tick = master_bar / GRID_SCALER;

    if (grid_tick != last_grid_tick) {
        last_grid_tick = grid_tick;
        uint32_t bar_count = master_bar / 27720000;

        // KICK
        int k_int = INTERVALS[earth_zone_kick];
        int k_step = (grid_tick / k_int) % STEPS_COUNT[earth_zone_kick];
        if (k_step != last_idx_k) {
            last_idx_k = k_step;
            int pat_len = STEPS_COUNT[earth_zone_kick];
            int pat_var = bar_count % PAT_VARS[earth_zone_kick];
            int acc = t_kick[earth_zone_kick][(pat_var * pat_len) + (k_step % pat_len)];
            if (acc > 0) {
                int v = (acc==3)?4095:(acc==2)?3000:1500;
                int ptr_idx = (acc==3)?2:(acc==2)?1:0;

                if (k_trans_timer > 0) {
                    int k_int_B = INTERVALS[next_zone_kick];
                    int k_step_B = (grid_tick / k_int_B) % STEPS_COUNT[next_zone_kick];
                    int acc_B = t_kick[next_zone_kick][k_step_B % 128]; 
                    if (acc_B > acc) { acc=acc_B; v=(acc==3)?4095:3000; 
                    ptr_idx = (acc==3)?2:1; // Update index
                    }
                }
                k_vel = v; 
                k_ptr = current_kick[ptr_idx];
                k_len = (acc==3)?KICK3_RAW_LEN:(acc==2)?KICK2_RAW_LEN:KICK1_RAW_LEN; 
                k_idx = 0; k_play = true;
            }
        }

        // SNARE
        int s_int = INTERVALS[earth_zone_snare];
        int s_step = (grid_tick / s_int) % STEPS_COUNT[earth_zone_snare];
        if (s_step != last_idx_s) {
            last_idx_s = s_step;
            int pat_len = STEPS_COUNT[earth_zone_snare];
            int pat_var = bar_count % PAT_VARS[earth_zone_snare];
            int acc = t_snare[earth_zone_snare][(pat_var * pat_len) + (s_step % pat_len)];
            if (earth_zone_snare >= 12 && acc == 0 && (s_step%2==0)) acc = 2;
            if (acc > 0) {
                int v = (acc==3)?4095:(acc==2)?3000:1500;
                int ptr_idx = (acc==3)?2:(acc==2)?1:0; 
                if (s_trans_timer > 0) {
                    int s_int_B = INTERVALS[next_zone_snare];
                    int s_step_B = (grid_tick / s_int_B) % STEPS_COUNT[next_zone_snare];
                    int acc_B = t_snare[next_zone_snare][s_step_B % 128];
                    if (acc_B > acc) { acc=acc_B; v=4095; 
                    ptr_idx = (acc==3)?2:1;
                    }
                }
                s_vel = v; 
                s_ptr = current_snare[ptr_idx];
                s_len = (acc==3)?SNARE3_RAW_LEN:(acc==2)?SNARE2_RAW_LEN:SNARE1_RAW_LEN;
                s_idx = 0; s_play = true;
            }
        }

        // HAT
        int h_int = INTERVALS[earth_zone_hat];
        int h_step = (grid_tick / h_int) % STEPS_COUNT[earth_zone_hat];
        if (h_step != last_idx_h) {
            last_idx_h = h_step;
            int pat_len = STEPS_COUNT[earth_zone_hat];
            int pat_var = bar_count % PAT_VARS[earth_zone_hat];
            int acc = t_hat[earth_zone_hat][(pat_var * pat_len) + (h_step % pat_len)];
            if (earth_zone_hat >= 12 && acc == 0) acc = 1;
            if (acc > 0) {
                int v = (acc==3)?4095:(acc==2)?3000:1500;
                int ptr_idx = (acc==3)?2:(acc==2)?1:0;
                if (h_trans_timer > 0) {
                    int h_int_B = INTERVALS[next_zone_hat];
                    int h_step_B = (grid_tick / h_int_B) % STEPS_COUNT[next_zone_hat];
                    int acc_B = t_hat[next_zone_hat][h_step_B % 128];
                    if (acc_B > acc) { acc=acc_B; v=4095; }
                }
                h_vel = v; 
                h_ptr = current_hat[ptr_idx];
                h_len = (acc==3)?HAT3_RAW_LEN:(acc==2)?HAT2_RAW_LEN:HAT1_RAW_LEN;
                h_idx = 0; h_play = true;
            }
        }
    }

    int output_sample = 2048; 
    uint32_t speed = 2048; 

    int32_t kick_out = 0;
    if (k_play) {
        k_idx += speed;
        uint32_t pos = k_idx >> 12;
        if (pos >= k_len) k_play = false; else kick_out = (int32_t)k_ptr[pos] - 128;
    }

    int32_t snare_out = 0;
    if (s_play) {
        s_idx += speed;
        uint32_t pos = s_idx >> 12;
        if (pos >= s_len) s_play = false; else snare_out = (int32_t)s_ptr[pos] - 128;
    }

    int32_t hat_out = 0;
    if (h_play) {
        h_idx += speed;
        uint32_t pos = h_idx >> 12;
        if (pos >= h_len) h_play = false; else hat_out = (int32_t)h_ptr[pos] - 128;
    }
    
    // --- OUTPUT ---
    int32_t sum = (kick_out * k_vel * MIX_KICK >> 12) + (snare_out * s_vel * MIX_SNARE >> 12) + (hat_out * h_vel * MIX_HAT >> 12); 
    
    output_sample += sum;
    if (output_sample > 4095) output_sample = 4095;
    if (output_sample < 0) output_sample = 0;
    
    DACWRITER(output_sample);
    WARM_ASHWRITER(output_sample);

    
    // HEARTBEAT
    REG(I2S_CONF_REG)[0] &= ~(BIT(5)); 
    REG(I2S_INT_CLR_REG)[0] = 0xFFFFFFFF;
    REG(I2S_CONF_REG)[0] |= (BIT(5)); 
}


