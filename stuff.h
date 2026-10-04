#pragma once
#include "setup.h"
#include "drums.h"
#include <esp_heap_caps.h>

// =========================================================
// TAPE SAVE SETUP--- NEW FIRMWARE
// =========================================================
volatile int tape_index = BOOT_TAPE_SLOT;
volatile bool tape_save_flag = false;
volatile bool tape_load_flag = false;

// =========================================================
// SAMPLE MANAGEMENT STUFF --- NEW FIRMWARE
// =========================================================
// DRUM ENGINE MEMORY (9-Sample architecture)

// Swap Buffer
// reserve 32KB of RAM to hold the current active kit only when needed
//#define DRUM_RAM_SIZE 32000 
//uint8_t *drum_ram_buffer;

// 30KB shared pool for any preset that needs heavy variables
#define PRESET_POOL_SIZE 4096 
uint8_t *preset_volatile_pool;

// pointers: Index 0=Soft, 1=Med, 2=Loud
// uint8_t *current_kick[3];
// uint8_t *current_snare[3];
// uint8_t *current_hat[3];
const uint8_t *current_kick[3]  = {KICK1_RAW, KICK2_RAW, KICK3_RAW};
const uint8_t *current_snare[3] = {SNARE1_RAW, SNARE2_RAW, SNARE3_RAW};
const uint8_t *current_hat[3]   = {HAT1_RAW, HAT2_RAW, HAT3_RAW};

// TRACK LENGTHS
// to know when to stop playing each sample
int len_kick[3];
int len_snare[3];
int len_hat[3];
// ---------------------------------------------------------

#define ENNEAGRAM simpleHelpers
#define SETUPPERS initDEL();

// Sampler Engine States
volatile bool system_mode = true; // NEW FIRMWARE: DEFAULT TO PLAYBACK //needed for buffer transfer
volatile bool is_active = false; 
volatile bool current_action_is_play = false; 
static int offset = 0;

// =========================================================
// BUTTON SETUP
// =========================================================
#define BUTTONEST REG(GPIO_IN1_REG)[0] & 0x1
#define CLICKETTE(c) attachInterrupt(32, c, CHANGE);
#define DOUBLECLK CLICKETTE(doubleclicker);
#define LONGPRESS
#define TRIPLECLK
#define BUTTON_PRESSED (is_pressed && !preset_mode) //NEW FIRMWARE
// ---------------------------------------------------------

// =========================================================
// CROSSFADE
// =========================================================
// Added as per Peter's crossfade update
// Peter defaults the Crossbite to 8
// 8 means about 1/512 of the loop
// At base clockrate of 48kHz a countdown of 256 (2^8 = 256) is a 5.3 millisecond crossfade. 
// Increasing this by one doubles the crossfade time or vice versa.
// For seemless crossfades of lower pitch audio, the wave cycle can be > 30ms (for C1)
// Setting Crossbite to 10 is a 21ms crossfade at base CPU speed
// The CPU speed controls the crossfade time, so slower clock speeds will have slower fade times
// If the CPU speed is set faster, then the crossfade time will be shorter 
// but either way the number of samples is the same, so the fade has the same effect
// Set to -1 for no crossfade, this cancels out the crossfade
#define CROSSBITE 8  // Change this for longer/shorter crossfades
#define CROSSFADE (1<<(CROSSBITE)) 
int xfado = 0;  // Fade out for switching from delay to looping
int yfado = 0;  // Fade in for switching from looping to delay

// CROSSFADE to be called by presets to avoid clicks on loops. Added per Peter's crossfade update
#define TRIGGER_CROSSFADE(is_freezing) \
  if (is_freezing) { \
      if (xfado == 0) xfado = CROSSFADE; \
  } else { \
      if (yfado == 0) yfado = CROSSFADE; \
  }
// ---------------------------------------------------------

#define PRESETTER(p) attachInterrupt(2, p, FALLING);

// =========================================================
// LAMP
// =========================================================
bool lamp; // declare the lamp variable for lampaflip

// GLOBAL LAMP CONTROL (Stateful control that ties lamp control to preset mode change)
// When BUTTONEST pressed, audio interrupt is locked out so that entering preset mode flashes lamp
#define LAMPLIGHT \
  if (BUTTONEST) { \
      if (lamp) REG(GPIO_OUT1_W1TS_REG)[0] = BIT(1); \
      else REG(GPIO_OUT1_W1TC_REG)[0] = BIT(1); \
  }

// Main Loop Override LED Control (Forces LED regardless of button state)
#define LAMPLIGHT_OVERRIDE \
  if (lamp) REG(GPIO_OUT1_W1TS_REG)[0] = BIT(1); \
  else REG(GPIO_OUT1_W1TC_REG)[0] = BIT(1);

// DIRECT LAMP CONTROL
// for UI use of lamp 
// bypasses the stateful lamplight control
#define LAMP_ON  do { if(!preset_mode && BUTTONEST) REG(GPIO_OUT1_W1TS_REG)[0] = BIT(1); } while(0) 
#define LAMP_OFF do { if(!preset_mode && BUTTONEST) REG(GPIO_OUT1_W1TC_REG)[0] = BIT(1); } while(0)

//ORIGINAL FIRMWARE
// #define LAMPLIGHT \ //REPLACED ABOVE
//  REG(GPIO_OUT_REG)[3]=((lamp?1:0)<<1);

#define LAMPAFLIP \
  lamp = !lamp; \
  LAMPLIGHT
// ---------------------------------------------------------


// =========================================================
// EARTH
// =========================================================
#define EARTHREAD (REG(I2S_FIFO_RD_REG)[0] & 0x7FF) >> 3 // converts earth to 8bit from its raw 12 bit

volatile int earth_last_state = 0; 

//hysteresis thresholds for when earth is a switch
// const int TRIGGER_ON_THRESHOLD = 160; // ~2.8V
// const int TRIGGER_OFF_THRESHOLD = 60; // ~1.8V
// Calibrated for a 4V midpoint (using a 2V-6V onboard LFO)
const int TRIGGER_ON_THRESHOLD = 100;  // Triggers ON just above 4V was 115
const int TRIGGER_OFF_THRESHOLD = 85; // Triggers OFF just below 4V was 100
// ---------------------------------------------------------


// =========================================================
// MASTER FADE --- NEW FIRMWARE
// =========================================================
// Smooths out preset changes from the preset selection menu
// Old way: the clock got unplugged and the DAC froze on its last sample, then ramped
// to 2048. The loop just stops dead mid-wave and that is the click.
// New way: the old preset keeps running while every output fades down,
// then the new preset gets plugged in and every output fades back up.
// The fade is counted in clock ticks, so the speed knob changes how long it takes.
// 10 = 1024 ticks, about 21ms at 48kHz. Add one to double it, take one away to halve it.
#define MASTER_FADE_BITS 10  // Change this for longer/shorter preset change fades
#define MASTER_FADE_MAX (1 << (MASTER_FADE_BITS))

volatile int master_gain = MASTER_FADE_MAX; // MASTER_FADE_MAX = full volume, 0 = resting
volatile int master_fade_dir = 0;           // -1 fading out, 1 fading in, 0 sitting still
volatile int last_dac = 2048;               // what actually went out the DAC last (pout isn't always it)
volatile int32_t dac_rest = 2048 << 12;     // slow average of the DAC, where the fade comes to rest

// Pulls a signal towards its resting point by the master gain
// At full gain the signal goes through untouched so the presets sound exactly as before
static inline int IRAM_ATTR master_fade_level(int val, int rest) {
    int g = master_gain;
    if (g >= MASTER_FADE_MAX) return val;
    return rest + (((val - rest) * g) >> MASTER_FADE_BITS);
}

// Called by DACWRITER, which every preset calls once per clock tick
// so this is where the fade steps along. Ash and yellow just follow master_gain.
static inline int IRAM_ATTR master_fade_dac(int p) {
    // track the DAC's resting level so the fade lands on the buffer's DC and not a hard 2048
    dac_rest += ((p << 12) - dac_rest) >> 12;

    // step the fade
    int g = master_gain;
    if (master_fade_dir < 0) {
        if (g > 0) g--; else master_fade_dir = 0;
    } else if (master_fade_dir > 0) {
        if (g < MASTER_FADE_MAX) g++; else master_fade_dir = 0;
    }
    master_gain = g;

    p = master_fade_level(p, dac_rest >> 12);
    last_dac = p;
    return p;
}
// ---------------------------------------------------------


// =========================================================
// ASH OUTPUT MENU
// =========================================================

// ---------------------------------------------------------
// ASH RESTING LEVEL GLIDE --- NEW FIRMWARE
// ---------------------------------------------------------
// The ash writers don't all rest at the same spot
// compressed sits at 128, clean / cleaner / warm sit at 64
// so going between presets that use different ones
// jumps the ash DC a little
// Now Ashes glide between each other
// Starts at 64 since that's where setup pre-charges ash
// 7 = about 170ms to cross from 64 to 128 at 48kHz. Add one to double it.
#define ASH_GLIDE_BITS 7  // Change this for a slower/faster ash level glide
static int32_t ash_glide = 64 << ASH_GLIDE_BITS;

static inline void IRAM_ATTR ash_out(int32_t out_8bit, int rest) {
    int32_t target = rest << ASH_GLIDE_BITS;
    if (ash_glide < target) ash_glide++;
    else if (ash_glide > target) ash_glide--;

    // move the sample from the writer's resting level to wherever the glide is right now
    int32_t final_out = out_8bit - rest + (ash_glide >> ASH_GLIDE_BITS);

    if (final_out > 255) final_out = 255;
    if (final_out < 0) final_out = 0;

    REG(ESP32_RTCIO_PAD_DAC1)[0] = BIT(10) | BIT(17) | BIT(18) | ((final_out & 0xFF) << 19);
}

// ---------------------------------------------------------
// CLEAN ASH AUDIO OUTPUT (7-Bit Symmetrical, Half-Volume)
// ---------------------------------------------------------
inline void write_ash_clean(int raw_val) {
    // DC Blocker (centers post-ADC to 2048)
    static int32_t dc_tracker = 2048 << 12;
    dc_tracker += ((raw_val << 12) - dc_tracker) >> 12; 
    int32_t ac_centered = raw_val - (dc_tracker >> 12) + 2048;

    // Master fade for preset changes (does nothing at full gain)
    ac_centered = master_fade_level(ac_centered, 2048);
    
    // TPDF Dither & Delta-Sigma Accumulation
    int32_t dither = (rand() & 15) - (rand() & 15);
    static int32_t error_accumulator = 0;
    int32_t target = ac_centered + dither + error_accumulator;
    
    // Scale for 128 peak-to-peak
    int32_t out_8bit = target >> 5;
    
    // Save discarded bits
    error_accumulator = target - (out_8bit << 5);
    
    // Output
    // int32_t final_out = out_8bit; //ORIGINAL, clipping and writing moved into ash_out
    // rests at 64 (2048 >> 5)
    ash_out(out_8bit, 64);
}
#define CLEAN_ASHWRITER(a) write_ash_clean(a)

// ---------------------------------------------------------
// WARM ASH AUDIO OUTPUT (8-Bit asymmetrically clipped, full-volume)
// ---------------------------------------------------------
inline void write_ash_warm(int raw_val) {
    // DC Blocker
    static int32_t dc_tracker = 2048 << 12;
    dc_tracker += ((raw_val << 12) - dc_tracker) >> 12; 
    int32_t ac_centered = raw_val - (dc_tracker >> 12) + 2048;

    // Master fade for preset changes (does nothing at full gain)
    ac_centered = master_fade_level(ac_centered, 2048);
    
    // TPDF Dither & Delta-Sigma Accumulation
    int32_t dither = (rand() & 7) - (rand() & 7);
    static int32_t error_accumulator = 0;
    int32_t target = ac_centered + dither + error_accumulator;
    
    // Scale for 255 peak-to-peak
    int32_t out_8bit = target >> 4;
    
    // ave discarded bits
    error_accumulator = target - (out_8bit << 4);
    
    // Shift down by 64 to force center to rest at LM3900 diode conductance level
    int32_t final_out = out_8bit - 64; 
    
    // Asymmetrical Clipping (now in ash_out)
    // rests at 64 ((2048 >> 4) - 64)
    ash_out(final_out, 64);
}
#define WARM_ASHWRITER(a) write_ash_warm(a)

// ---------------------------------------------------------
// CLEANER WARM ASH AUDIO OUTPUT (8-Bit asymmetrically clipped, full-volume, no dithering)
// ---------------------------------------------------------
inline void write_ash_cleaner(int raw_val) {
    // DC Blocker (centers post-ADC to 2048)
    static int32_t dc_tracker = 2048 << 12;
    dc_tracker += ((raw_val << 12) - dc_tracker) >> 12; 
    int32_t ac_centered = raw_val - (dc_tracker >> 12) + 2048;

    // Master fade for preset changes (does nothing at full gain)
    ac_centered = master_fade_level(ac_centered, 2048);
    
    // Scale for 128 peak-to-peak directly (no dither or error accumulation)
    int32_t out_8bit = ac_centered >> 5;
    
    // Hard limit clipping bounds and hardware write (now in ash_out)
    // rests at 64 (2048 >> 5)
    ash_out(out_8bit, 64);
}
#define CLEANER_ASHWRITER(a) write_ash_cleaner(a)


// ---------------------------------------------------------
// COMPRESSED ASH AUDIO OUTPUT (8-Bit, Driven into Limiter, Symmetrical Clipping)
// ---------------------------------------------------------
inline void write_ash_compressed(int raw_val) {
    // DC Blocker  and extracting AC signal
    static int32_t dc_tracker = 2048 << 12;
    dc_tracker += ((raw_val << 12) - dc_tracker) >> 12; 
    int32_t ac_only = raw_val - (dc_tracker >> 12); 

    // Master fade for preset changes (does nothing at full gain)
    ac_only = master_fade_level(ac_only, 0);
    
    // Boosting by 2 to pull quiet sounds up
    ac_only = ac_only * 2; 
    
    // Hard Limiting (clamp at 12-bit cieling)
    if (ac_only > 2047) ac_only = 2047;
    if (ac_only < -2048) ac_only = -2048;
    
    // Scale to full 8-bit range
    int32_t out_8bit = (ac_only + 2048) >> 4; 
    
    // Safety Clipping and hardware write (now in ash_out)
    // rests at 128 (2048 >> 4)
    ash_out(out_8bit, 128);
}
#define COMPRESSED_ASHWRITER(a) write_ash_compressed(a)

//ORIGINAL FIRMWARE // REPLACED ABOVE
// #define ASHWRITER(a) \
//  REG(ESP32_RTCIO_PAD_DAC1)[0]= \
//  BIT(10)|BIT(17)|BIT(18)|((a&0xFF)<<19);

// ---------------------------------------------------------
// SET DEFAULT ASH BELOW
// ---------------------------------------------------------
// most presets use a generic "ashwriter" 
// so this is where you can define which verions of ash that points to
// just change "warm_ashwriter" to "clean_ashwriter" to swap them
#define ASHWRITER(a) COMPRESSED_ASHWRITER(a)


// ---------------------------------------------------------
// ---------------------------------------------------------

#define INTABRUPT \
  REG(GPIO_STATUS_W1TC_REG) \
  [0] = 0xFFFFFFFF; \
  REG(GPIO_STATUS1_W1TC_REG) \
  [0] = 0xFFFFFFFF;
#define SPIWRITER(d) \
  REG(SPI3_W8_REG) \
  [0] = (d) << 16; \
  REG(SPI3_CMD_REG) \
  [0] = BIT(18);
//#define DACWRITER(p) SPIWRITER(0x9000 | p) //ORIGINAL, replaced below
#define DACWRITER(p) SPIWRITER(0x9000 | master_fade_dac(p)) // NEW FIRMWARE: master fade for preset changes
#define ADCREADER ((REG(SPI3_W0_REG)[0]) >> 16) & 0xFFF;
#define I2S_START
#define I2SFINISH

// =========================================================
// YELLOW OUTPUT MENU
// =========================================================
#define YELLOW_MASK (BIT(12) | BIT(13) | BIT(14) | BIT(15) | BIT(16) | BIT(17) | BIT(21) | BIT(22) | BIT(26) | BIT(27))

// ---------------------------------------------------------
// YELLOW_BINARY (Original Cocoquantus Buffer Position Binary Code)
// ---------------------------------------------------------
// a binary counter in voltage of the buffer position
// refer to the guide for crucFX SLM for more details
#define YELLOW_BINARY(b) \
 REG(GPIO_OUT_REG)[0]=((uint32_t)(b)<<12);

// ---------------------------------------------------------
// YELLOW CLOCK PULSE
// ---------------------------------------------------------
// Creates square pulse from Yellow for clocking external equipment
// Drives all 10 pins of the Yellow Ladder simultaneously
#define YELLOW_PULSE(b) \
  if (b > 2048) REG(GPIO_OUT_W1TS_REG) \
                [0] = YELLOW_MASK; \
  else REG(GPIO_OUT_W1TC_REG) \
       [0] = YELLOW_MASK;

// ---------------------------------------------------------
// YELLOW AUDIO OUTPUT (BIT-CRUSHED)
// ---------------------------------------------------------
// yellow as a psuedo DAC
// can be used as a complement to ash for a weird stereo image
// since all pins are equal weight, map amplitude -> pin count
// input 'b' is 0-1023. scale it to 0-10 steps for 10 pins

inline void write_yellow(int raw_val) {
    static int32_t yellow_error = 0;
    
    // TPDF Dither
    int32_t dither = (rand() & 127) - (rand() & 127);
    int32_t target = raw_val + dither + yellow_error;
    
    // Map 12 bit audio (4095 steps) to 10 hardware pins (4095 / 10 = ~409)
    int32_t pins_on = target / 409;
    
    if (pins_on > 10) pins_on = 10;
    if (pins_on < 0) pins_on = 0;
    
    // Save discarded remainder
    yellow_error = target - (pins_on * 409);
    
    uint32_t mask = 0;
    if (pins_on >= 1) mask |= BIT(12);
    if (pins_on >= 2) mask |= BIT(13);
    if (pins_on >= 3) mask |= BIT(14);
    if (pins_on >= 4) mask |= BIT(15);
    if (pins_on >= 5) mask |= BIT(16);
    if (pins_on >= 6) mask |= BIT(17);
    if (pins_on >= 7) mask |= BIT(21);
    if (pins_on >= 8) mask |= BIT(22);
    if (pins_on >= 9) mask |= BIT(26);
    if (pins_on >= 10) mask |= BIT(27);
    
    REG(GPIO_OUT_W1TC_REG)[0] = YELLOW_MASK;
    REG(GPIO_OUT_W1TS_REG)[0] = mask;
}

// NEW FIRMWARE: yellow audio follows the master fade on preset changes
// It tracks its own resting level, same idea as the ash DC blockers
inline void write_yellow_audio(int raw_val) {
    static int32_t yellow_rest = 2048 << 12;
    yellow_rest += ((raw_val << 12) - yellow_rest) >> 12;
    write_yellow(master_fade_level(raw_val, yellow_rest >> 12));
}
#define YELLOW_AUDIO(a) write_yellow_audio(a)

// YELLOW CLOCK
// Same pins as yellow audio but skips the master fade
// Use this when yellow is a clock/pulse (4095 accent, 3000 clock, 0 off)
// so a sync clock doesn't sag into a fake trigger level during a preset change
#define YELLOW_CLOCK(a) write_yellow(a)

// ---------------------------------------------------------
// ---------------------------------------------------------

#define FLIPPERAT REG(GPIO_IN1_REG)[0] & 0x8
#define SKIPPERAT REG(GPIO_IN1_REG)[0] & 0x4

#define DELAYSIZE (1 << 17) // Original buffer size is 131000
#define SAMPLE_LEN 131000 // NEW FIRMWARE For sampler buffer
#define FADE_LEN 1000 // NEW FIRMWARE for sampler crossfades

#define FILLNOISE \
  for (int i = 0; i < DELAYSIZE; i++) dellius(i, rand(), false);


int tima;
int timahi;
int preset;
//void (*presets[PRESETAMT])(); //updated to tie to the PRESETAMT in .ino
// PLAYLIST MEMORY
int active_preset_count = 1; 
void (*presets[32])(); // hardware ceiling of 32 presets per playlist

// =========================================================
// DRUM SAMPLE SETUP --- NEW FIRMWARE
// =========================================================
#include "drums.h"

// void load_drum_kit(int kit_id) {
//   // STOP AUDIO
//   REG(I2S_CONF_REG)
//   [0] &= ~(BIT(5));

//   int head = 0;

//   // LOAD KICKS
//   // Mapping: v0=KICK1 (Soft), v1=KICK2 (Med), v2=KICK3 (Loud)
//   for (int v = 0; v < 3; v++) {
//     current_kick[v] = &drum_ram_buffer[head];

//     const uint8_t *source_data;
//     int source_len;

//     if (kit_id == 0) {
//       if (v == 0) {
//         source_data = KICK1_RAW;
//         source_len = sizeof(KICK1_RAW);
//       }
//       if (v == 1) {
//         source_data = KICK2_RAW;
//         source_len = sizeof(KICK2_RAW);
//       }
//       if (v == 2) {
//         source_data = KICK3_RAW;
//         source_len = sizeof(KICK3_RAW);
//       }
//     }

//     memcpy(current_kick[v], source_data, source_len);
//     len_kick[v] = source_len;
//     head += source_len;
//   }

//   // LOAD SNARES
//   for (int v = 0; v < 3; v++) {
//     current_snare[v] = &drum_ram_buffer[head];

//     const uint8_t *source_data;
//     int source_len;

//     if (kit_id == 0) {
//       // FIXED NAMES HERE:
//       if (v == 0) {
//         source_data = SNARE1_RAW;
//         source_len = sizeof(SNARE1_RAW);
//       }
//       if (v == 1) {
//         source_data = SNARE2_RAW;
//         source_len = sizeof(SNARE2_RAW);
//       }
//       if (v == 2) {
//         source_data = SNARE3_RAW;
//         source_len = sizeof(SNARE3_RAW);
//       }
//     }

//     memcpy(current_snare[v], source_data, source_len);
//     len_snare[v] = source_len;
//     head += source_len;
//   }

//   // LOAD HATS
//   for (int v = 0; v < 3; v++) {
//     current_hat[v] = &drum_ram_buffer[head];

//     const uint8_t *source_data;
//     int source_len;

//     if (kit_id == 0) {
//       // FIXED NAMES HERE:
//       if (v == 0) {
//         source_data = HAT1_RAW;
//         source_len = sizeof(HAT1_RAW);
//       }
//       if (v == 1) {
//         source_data = HAT2_RAW;
//         source_len = sizeof(HAT2_RAW);
//       }
//       if (v == 2) {
//         source_data = HAT3_RAW;
//         source_len = sizeof(HAT3_RAW);
//       }
//     }

//     memcpy(current_hat[v], source_data, source_len);
//     len_hat[v] = source_len;
//     head += source_len;
//   }

//   // RESTART AUDIO
//   REG(I2S_CONF_REG)
//   [0] |= (BIT(5));
// }
// ---------------------------------------------------------

// =========================================================
// BUTTON --- MODIFIED FIRMWARE
// =========================================================
// Modified doubleclicker to allow for preset selection mode

// --- PRESET SELECTION VARIABLES ---
volatile bool preset_mode = false;
volatile bool exit_menu_request = false;
volatile uint32_t press_time = 0;
volatile uint32_t release_time = 0;
volatile bool is_pressed = false;
volatile int preset_counter = 0;
volatile bool audio_frozen_state = false;

void IRAM_ATTR doubleclicker() {
  int buttnow = BUTTONEST; // 0 is pressed, 1 is released

  // Force timer update and read the lower 32 bits
  REG(TIMG0_T0UPDATE_REG)[0] = BIT(1); 
  uint32_t current_time = REG(TIMG0_T0LO_REG)[0]; 

  if (buttnow == 0) { 
    // === PRESS ===
    // register a new press if it has been released for 100ms (250,000 ticks)
    if (!is_pressed && (current_time - release_time > 250000)) { 
       is_pressed = true;
       press_time = current_time;
    }
  } else { 
    // === RELEASE ===
    // register a release if it is currently pressed AND has been held for 50ms (125,000 ticks)
    if (is_pressed && (current_time - press_time > 125000)) { 
       is_pressed = false;
       release_time = current_time;
       
       // Unsigned 32-bit math automatically handles timer overflow
       uint32_t hold_time = current_time - press_time;

       // Check if held for more than 0.8 seconds (2,000,000 ticks)
       if (hold_time > 2000000) { 
          // LONG PRESS DETECTED
          //preset_mode = !preset_mode; 
          if (!preset_mode) {
              preset_mode = true; 
              preset_counter = 0; 
          } else {
              // Exiting mode: Apply the new preset.
              preset = preset_counter % active_preset_count;
              audio_frozen_state = true; 

              // NEW FIRMWARE: start fading the old preset out right now, on the release
              // the main loop waits for it to hit bottom before swapping presets
              master_fade_dir = -1;
              
              // Signal the main OS loop to execute the teardown crossfade safely
              exit_menu_request = true; 
          }
          
          // if (preset_mode) {
          //   preset_counter = 0; 
          // } else {
          //   // Exiting mode: Apply the new preset.
          //   preset = preset_counter % active_preset_count;
          //   // set audio_frozen_state so preset changing doesn't 
          //   // record over transferred buffers
          //   audio_frozen_state = true; 
          //   // Signal the main OS loop to execute the teardown crossfade safely
          //     exit_menu_request = true;
          // }
          
       } else if (preset_mode) {
          // SHORT PRESS (While in Preset Mode)
          preset_counter++;
          lamp = true; 
          LAMPLIGHT; 
       } else {
          // NORMAL SHORT PRESS (Toggles Lamp)
          lamp = !lamp; 
          audio_frozen_state = lamp;
          LAMPLIGHT; 
       }
    }
  }
}
// ---------------------------------------------------------

// =========================================================
// BUFFER --- MODIFIED FIRMWARE
// =========================================================
// SETTING UP THE BUFFER
// dellius packs 12 bits into two bytes
// uint8_t *delaybuffa;
// uint8_t *delaybuffb;
#define CRUMB_BITS 10       // 1024 samples per crumb
#define CRUMBS (DELAYSIZE >> CRUMB_BITS) // 128 crumbs total
#define CRUMB_BYTES (((1 << CRUMB_BITS) * 3) >> 1) // 1536 bytes per crumb
//uint8_t *dcrumb[CRUMBS]; // Global Pointers for the linker
// Hardcoded to 128 to fix the compiler scope error
extern uint8_t *dcrumb[128];

// ==========================================
// SHARED 16-BIT DSP BUFFER --- NEW FIRMWARE
//
// Needed for resolution on audio computations
// 60KB shared memory pool for 16-bit delay and reverb presets
// Presets check 'current_16bit_owner' and wipe the buffer if they are newly loaded.
#define SHARED_16BIT_LEN 30000

struct B16 {
    inline int16_t& operator[](uint32_t i) const {
        uint32_t c = (i * 43691) >> 25; // i / 768
        uint32_t o = i - (c * 768);     // i % 768
        return ((int16_t *)dcrumb[c])[o];
    }
};
const B16 buffer_16bit = {};

// Cast the 8-bit delaybuffa array into a 16-bit array
// 131,072 bytes of 8-bit audio equals 65,536 slots of 16-bit audio.
// #define buffer_16bit ((int16_t *)delaybuffa) //replaced below
extern uint8_t *dcrumb[CRUMBS];
//RTC_NOINIT_ATTR static uint8_t dcrumb_rtc[CRUMB_BYTES];
// ==========================================

// uint8_t *delptr;
int t;
static int delayskp;
static int lastskp;
int adc_read;
int gyo;
volatile int pout; //persistent_red // updated to volatile per Peter's FW

// optimizes memory use for 12 bit ADC
int dellius(int ptr, int val, bool but) {
  int zut, biz, forsh;

  // 1024 samples per crumb
  uint8_t *delptr = dcrumb[(ptr >> 10) & 127];

  // Bounds Check: Bail out if pointer is uninitialized or outside safe SRAM
  if ((uint32_t)delptr < 0x3F000000 || (uint32_t)delptr >= 0x40000000) return 0;

  // Local pointer inside the crumb
  int local_ptr = (ptr & 1023) * 3;
  biz = local_ptr & 1;
  forsh = biz << 2;

  // if (ptr & 0x10000) //replaced above to deal with crumb packets of data
  //   delptr = delaybuffa;
  // else
  //   delptr = delaybuffb;
  // ptr = ptr & 0xFFFF;
  // ptr = ptr * 3;
  // biz = ptr & 1;
  // forsh = biz << 2;

  // unpack buffer audio
  zut = delptr[(local_ptr >> 1) + biz] << 4;
  zut |= (delptr[(local_ptr >> 1) + 1 - biz] & (0xF << (forsh))) >> (forsh);

  // When button is pressed to loop, keeping recording during crossfade time
  if ((!but) || (but && (xfado > 0))) {

    // Linear Crossfade of the live signal and the buffer 
    if (xfado > 0) {   
        val = (val * xfado) >> CROSSBITE;   
        val += (zut * (CROSSFADE - xfado)) >> CROSSBITE;      
        xfado--;  
    }

    // Linear Crossfade of the buffer signal and the live signal when unfreezing
        if (yfado > 0) {
            val = (val * (CROSSFADE - yfado)) >> CROSSBITE;
            val += (zut * yfado) >> CROSSBITE;
            yfado--;
        }

    // re-pack the buffer to 12 bits by splitting the second byte of the 12bit read input across the end of each 8 bit buffer
    delptr[(local_ptr >> 1) + biz] = (uint8_t)(val >> 4);
    delptr[(local_ptr >> 1) + 1 - biz] &= (uint8_t)(0xF << (4 - forsh));
    delptr[(local_ptr >> 1) + 1 - biz] |= (uint8_t)((val & 0xF) << forsh);

    // delptr[(ptr >> 1) + biz] = (uint8_t)(val >> 4); // replaced above
    // delptr[(ptr >> 1) + 1 - biz] &= (uint8_t)(0xF << (4 - forsh));
    // delptr[(ptr >> 1) + 1 - biz] |= (uint8_t)((val & 0xF) << forsh);
  }
  return zut;
}
// ---------------------------------------------------------

// Read-only 12-bit ring buffer for k.odk presets
int IRAM_ATTR dread(int ptr) {
  uint8_t *dp = dcrumb[(ptr >> 10) & 127];
  if ((uint32_t)dp < 0x3F000000 || (uint32_t)dp >= 0x40000000) return 0;
  int local_ptr = (ptr & 1023) * 3;
  int biz = local_ptr & 1;
  int forsh = biz << 2;
  int zut = dp[(local_ptr >> 1) + biz] << 4;
  zut |= (dp[(local_ptr >> 1) + 1 - biz] & (0xF << forsh)) >> forsh;
  return zut;
}

// Write-only 12-bit ring buffer for k.odk presets
void IRAM_ATTR dwrite(int ptr, int val) {
  if (val < 0) val = 0; if (val > 4095) val = 4095;
  uint8_t *dp = dcrumb[(ptr >> 10) & 127];
  if ((uint32_t)dp < 0x3F000000 || (uint32_t)dp >= 0x40000000) return;
  int local_ptr = (ptr & 1023) * 3;
  int biz = local_ptr & 1;
  int forsh = biz << 2;
  dp[(local_ptr >> 1) + biz] = (uint8_t)(val >> 4);
  dp[(local_ptr >> 1) + 1 - biz] &= (uint8_t)(0xF << (4 - forsh));
  dp[(local_ptr >> 1) + 1 - biz] |= (uint8_t)((val & 0xF) << forsh);
}

void initDEL() {

  // Total free memory in the heap
  Serial.print("Total Free Heap: ");
  Serial.print(ESP.getFreeHeap() / 1024);
  Serial.println(" KB");

  // largest single block of memory available for malloc()
  Serial.print("Largest Contiguous Block: ");
  Serial.print(ESP.getMaxAllocHeap() / 1024);
  Serial.println(" KB");


  Serial.println("    -> initDEL: Allocating delay crumbs...");
  bool dok = true;

  // Loop through all crumbs and assign them to byte-addressable SRAM
  for (int i = 0; i < CRUMBS; i++) { 
      dcrumb[i] = (uint8_t *)heap_caps_malloc(CRUMB_BYTES, MALLOC_CAP_8BIT); 
      if (!dcrumb[i]) dok = false; 
  }

  if (!dok) {
    Serial.println("    -> FATAL: Malloc failed! Entering infinite loop.");
    while (1) {
      REG(GPIO_OUT1_W1TS_REG)[0] = BIT(1); delay(50);
      REG(GPIO_OUT1_W1TC_REG)[0] = BIT(1); delay(50);
    }
  }
  Serial.println("    -> Success! 128 Crumbs allocated.");

  preset_volatile_pool = (uint8_t *)malloc(PRESET_POOL_SIZE);
  if (preset_volatile_pool == NULL) {
      Serial.println("    -> FATAL: volatile pool failed!");
      while (1) {
          REG(GPIO_OUT1_W1TS_REG)[0] = BIT(1); delay(100);
          REG(GPIO_OUT1_W1TC_REG)[0] = BIT(1); delay(100);
      }
  }

  // Serial.println("    -> initDEL: Allocating delay buffers..."); //For Debugging // replaced by crumbs below
  // delaybuffa = (uint8_t *)malloc((DELAYSIZE >> 2) + (DELAYSIZE >> 1)); 
  // delaybuffb = (uint8_t *)malloc((DELAYSIZE >> 2) + (DELAYSIZE >> 1)); 


  // NEW FIRMWARE
  // safety check
  // if (delaybuffa == NULL || delaybuffb == NULL) {
  //   // buffer set up fails, flash the orange lamp rapidly 
  //   Serial.println("    -> FATAL: Malloc failed! Entering infinite loop.");
  //   while (1) {
  //     REG(GPIO_OUT1_W1TS_REG)[0] = BIT(1);
  //     delay(50);
  //     REG(GPIO_OUT1_W1TC_REG)[0] = BIT(1);
  //     delay(50);
  //   }
  // }
  //Serial.printf("    -> Success! Buffer A at: %p | Buffer B at: %p\n", delaybuffa, delaybuffb); //For Debugging
 

  /////NEW FIRMWARE
  // needed for sample management
  //drum_ram_buffer = (uint8_t*)malloc(DRUM_RAM_SIZE); //not enough spare memory for seperate drum buffer
  // drum_ram_buffer = delaybuffa; //would use the shared buffer, but since dellius reads 12 bit audio, it doesn't work well

  // // Safety: if run out of RAM, use the main delay buffer
  // if (drum_ram_buffer == NULL) {
  //   // Blink LED forever to signal Out Of Memory
  //   while (1) {
  //     REG(GPIO_OUT_REG)
  //     [3] ^= (1 << 1);
  //     delay(100);
  //   }
  // }

  // Memory pool for large variables
  // preset_volatile_pool = (uint8_t *)malloc(PRESET_POOL_SIZE);
  // if (preset_volatile_pool == NULL) {
  // }

  // Memory pool for large variables
  // preset_volatile_pool = (uint8_t *)malloc(PRESET_POOL_SIZE);
  
  // if (preset_volatile_pool == NULL) {
  //     Serial.println("    -> FATAL: preset_volatile_pool Malloc failed! Entering infinite loop.");
  //     // Fast strobe to indicate pool failure
  //     while (1) {
  //         REG(GPIO_OUT1_W1TS_REG)[0] = BIT(1);
  //         delay(100);
  //         REG(GPIO_OUT1_W1TC_REG)[0] = BIT(1);
  //         delay(100);
  //     }
  // } else {
  //     Serial.printf("    -> Success! Volatile Pool allocated at: %p\n", preset_volatile_pool);
  // }

  ///////////////END

  //delptr = delaybuffa;
  t = 0;
  xfado = 0; // initialize crossfade timer. added per peter's crossfade
  yfado = 0; // initialize crossfade timer. added per peter's crossfade

  //esp_task_wdt_init(30, false);

  REG(ESP32_SENS_SAR_DAC_CTRL1)[0] = 0x0;
  REG(ESP32_SENS_SAR_DAC_CTRL2)[0] = 0x0;

  initDIG();
  //function 2 on the 12 block
  REG(IO_MUX_GPIO12ISH_REG)
  [0] = BIT(13);  //sdi2 q MISO
  REG(IO_MUX_GPIO12ISH_REG)
  [1] = BIT(13);  //d MOSI
  REG(IO_MUX_GPIO12ISH_REG)
  [2] = BIT(13);  //clk
  REG(IO_MUX_GPIO12ISH_REG)
  [3] = BIT(13);  //cs0
  //perip clock bit 16 is spi3, 13 is timer0
  CHANGOR(DPORT_PERIP_CLK_EN_REG, BIT(16) | BIT(13))
  CHANGNOR(DPORT_PERIP_RST_EN_REG, BIT(16) | BIT(13))

  REG(TIMG0_T0CONFIG_REG)
  [0] = (1 << 18) | BIT(30) | BIT(31);

  REG(IO_MUX_GPIO5_REG)
  [0] = BIT(12);  //sdi3 cs0
  REG(IO_MUX_GPIO18_REG)
  [0] = BIT(12);  //sdi3 clk
  REG(IO_MUX_GPIO19_REG)
  [0] = BIT(12) | BIT(9);  //sdi3 q MISO
  REG(IO_MUX_GPIO23_REG)
  [0] = BIT(12);  //sdi3 d MOSI
  REG(SPI3_MOSI_DLEN_REG)
  [0] = 15;
  REG(SPI3_MISO_DLEN_REG)
  [0] = 15;
  REG(SPI3_USER_REG)
  [0] = BIT(25) | BIT(0) | BIT(27) | BIT(28) | BIT(7) | BIT(6) | BIT(5) | BIT(11) | BIT(10);
  //USR_MOSI, MISO_HIGHPART, and DOUTDIN
  REG(SPI3_PIN_REG)
  [0] = BIT(29);
  //REG(SPI3_CTRL2_REG)[0]=BIT(17);
  REG(SPI3_CLOCK_REG)
  [0] = (1 << 18) | (3 << 12) | (1 << 6) | 3;

#define SPINNER 500000
#define SPRINTER(a) \
  SPIWRITER(a); \
  spin(SPINNER);

  spin(SPINNER * 5);
  SPRINTER(0);
  SPRINTER(0);

  // SPIRTER(0b0111110110101100); //sw_reset
  SPRINTER(0x1201);  //adc_seq,9rep,1chan0
  SPRINTER(0x1800);  //gen_ctrl_reg
  SPRINTER(0x2001);  //adc_config,io0adc0
  SPRINTER(0x2802);  //dac_config,io1dac1
  SPRINTER(0x5a00);  //pd_ref_ctrl,9vref

  // SPIRTER(0b0111110110101100); //sw_reset
  SPRINTER(0x1201);  //adc_seq,9rep,1chan0
  SPRINTER(0x1800);  //gen_ctrl_reg
  SPRINTER(0x2001);  //adc_config,io0adc0
  SPRINTER(0x2802);  //dac_config,io1dac1
  SPRINTER(0x5a00);  //pd_ref_ctrl,9vref

//LEDs//YELLOW PINS and more
#define GPIO_FUNC_OUT_SEL_CFG_REG REG(0X3ff44530)
  GPIO_FUNC_OUT_SEL_CFG_REG[33] = 256;
  GPIO_FUNC_OUT_SEL_CFG_REG[12] = 256;
  GPIO_FUNC_OUT_SEL_CFG_REG[13] = 256;
  GPIO_FUNC_OUT_SEL_CFG_REG[14] = 256;
  GPIO_FUNC_OUT_SEL_CFG_REG[15] = 256;
  GPIO_FUNC_OUT_SEL_CFG_REG[16] = 256;
  GPIO_FUNC_OUT_SEL_CFG_REG[17] = 256;
  //GPIO_FUNC_OUT_SEL_CFG_REG[18]=256;
  //GPIO_FUNC_OUT_SEL_CFG_REG[19]=256;
  GPIO_FUNC_OUT_SEL_CFG_REG[21] = 256;
  GPIO_FUNC_OUT_SEL_CFG_REG[22] = 256;
  //GPIO_FUNC_OUT_SEL_CFG_REG[23]=256;
  GPIO_FUNC_OUT_SEL_CFG_REG[26] = 256;
  GPIO_FUNC_OUT_SEL_CFG_REG[27] = 256;
  REG(GPIO_ENABLE_REG)
  [0] = BIT(12) | BIT(13)
        | BIT(14) | BIT(15) | BIT(16) | BIT(17)
        | BIT(21) | BIT(22) | BIT(26) | BIT(27);  //ouit freaqs
  REG(GPIO_ENABLE_REG)
  [3] = 2;  //output enable 33
  REG(IO_MUX_GPIO32_REG)
  [0] = BIT(9) | BIT(8);  //input enable 
  REG(IO_MUX_GPIO34_REG)
  [1] = BIT(9) | BIT(8);  //input enable Flip
  REG(IO_MUX_GPIO34_REG)
  [0] = BIT(9) | BIT(8);  //input enable Skip
  REG(IO_MUX_GPIO2_REG)
  [0] = BIT(9) | BIT(8);  //input enable
}

