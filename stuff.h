#include "setup.h"

// =========================================================
// SAMPLE MANAGEMENT STUFF --- NEW FIRMWARE
// =========================================================
// DRUM ENGINE MEMORY (9-Sample architecture)

// Swap Buffer
// reserve 32KB of RAM to hold the current active kit only when needed
#define DRUM_RAM_SIZE 32000 
uint8_t *drum_ram_buffer;

// pointers: Index 0=Soft, 1=Med, 2=Loud
uint8_t *current_kick[3];
uint8_t *current_snare[3];
uint8_t *current_hat[3];

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
// ASH OUTPUT MENU
// =========================================================
// ---------------------------------------------------------
// CLEAN ASH AUDIO OUTPUT (7-Bit Symmetrical, Half-Volume)
// ---------------------------------------------------------
inline void write_ash_clean(int raw_val) {
    // DC Blocker (centers post-ADC to 2048)
    static int32_t dc_tracker = 2048 << 12;
    dc_tracker += ((raw_val << 12) - dc_tracker) >> 12; 
    int32_t ac_centered = raw_val - (dc_tracker >> 12) + 2048;
    
    // TPDF Dither & Delta-Sigma Accumulation
    int32_t dither = (rand() & 15) - (rand() & 15);
    static int32_t error_accumulator = 0;
    int32_t target = ac_centered + dither + error_accumulator;
    
    // Scale for 128 peak-to-peak
    int32_t out_8bit = target >> 5;
    
    // Save discarded bits
    error_accumulator = target - (out_8bit << 5);
    
    // Output
    int32_t final_out = out_8bit; 
    
    if (final_out > 255) final_out = 255;
    if (final_out < 0) final_out = 0;
    
    REG(ESP32_RTCIO_PAD_DAC1)[0] = BIT(10) | BIT(17) | BIT(18) | ((final_out & 0xFF) << 19);
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
    
    // Asymmetrical Clipping
    if (final_out > 255) final_out = 255;
    if (final_out < 0) final_out = 0;
    
    REG(ESP32_RTCIO_PAD_DAC1)[0] = BIT(10) | BIT(17) | BIT(18) | ((final_out & 0xFF) << 19);
}
#define WARM_ASHWRITER(a) write_ash_warm(a)

// Set Default for easy copy-pasting 
// most presets use a generic "ashwriter" 
// so this is where you can define which verions of ash that points to
// just change "warm_ashwriter" to "clean_ashwriter" to swap them
#define ASHWRITER(a) WARM_ASHWRITER(a)

//ORIGINAL FIRMWARE // REPLACED ABOVE
// #define ASHWRITER(a) \
//  REG(ESP32_RTCIO_PAD_DAC1)[0]= \
//  BIT(10)|BIT(17)|BIT(18)|((a&0xFF)<<19);

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
#define DACWRITER(p) SPIWRITER(0x9000 | p)
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

inline void write_yellow_audio(int raw_val) {
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
#define YELLOW_AUDIO(a) write_yellow_audio(a)

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

void load_drum_kit(int kit_id) {
  // STOP AUDIO
  REG(I2S_CONF_REG)
  [0] &= ~(BIT(5));

  int head = 0;

  // LOAD KICKS
  // Mapping: v0=KICK1 (Soft), v1=KICK2 (Med), v2=KICK3 (Loud)
  for (int v = 0; v < 3; v++) {
    current_kick[v] = &drum_ram_buffer[head];

    const uint8_t *source_data;
    int source_len;

    if (kit_id == 0) {
      if (v == 0) {
        source_data = KICK1_RAW;
        source_len = sizeof(KICK1_RAW);
      }
      if (v == 1) {
        source_data = KICK2_RAW;
        source_len = sizeof(KICK2_RAW);
      }
      if (v == 2) {
        source_data = KICK3_RAW;
        source_len = sizeof(KICK3_RAW);
      }
    }

    memcpy(current_kick[v], source_data, source_len);
    len_kick[v] = source_len;
    head += source_len;
  }

  // LOAD SNARES
  for (int v = 0; v < 3; v++) {
    current_snare[v] = &drum_ram_buffer[head];

    const uint8_t *source_data;
    int source_len;

    if (kit_id == 0) {
      // FIXED NAMES HERE:
      if (v == 0) {
        source_data = SNARE1_RAW;
        source_len = sizeof(SNARE1_RAW);
      }
      if (v == 1) {
        source_data = SNARE2_RAW;
        source_len = sizeof(SNARE2_RAW);
      }
      if (v == 2) {
        source_data = SNARE3_RAW;
        source_len = sizeof(SNARE3_RAW);
      }
    }

    memcpy(current_snare[v], source_data, source_len);
    len_snare[v] = source_len;
    head += source_len;
  }

  // LOAD HATS
  for (int v = 0; v < 3; v++) {
    current_hat[v] = &drum_ram_buffer[head];

    const uint8_t *source_data;
    int source_len;

    if (kit_id == 0) {
      // FIXED NAMES HERE:
      if (v == 0) {
        source_data = HAT1_RAW;
        source_len = sizeof(HAT1_RAW);
      }
      if (v == 1) {
        source_data = HAT2_RAW;
        source_len = sizeof(HAT2_RAW);
      }
      if (v == 2) {
        source_data = HAT3_RAW;
        source_len = sizeof(HAT3_RAW);
      }
    }

    memcpy(current_hat[v], source_data, source_len);
    len_hat[v] = source_len;
    head += source_len;
  }

  // RESTART AUDIO
  REG(I2S_CONF_REG)
  [0] |= (BIT(5));
}
// ---------------------------------------------------------

// =========================================================
// BUTTON --- MODIFIED FIRMWARE
// =========================================================
// Modified doubleclicker to allow for preset selection mode

// --- PRESET SELECTION VARIABLES ---
volatile bool preset_mode = false;
volatile uint32_t press_time = 0;
volatile uint32_t release_time = 0;
volatile bool is_pressed = false;
volatile int preset_counter = 0;
volatile bool audio_frozen_state = false; // <-- ADD THIS LINE

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
          preset_mode = !preset_mode; 
          
          if (preset_mode) {
            preset_counter = 0; 
          } else {
            // Exiting mode: Apply the new preset.
            //preset = preset_counter % PRESETAMT; 
            preset = preset_counter % active_preset_count;
            // set audio_frozen_state so preset changing doesn't 
            // record over transferred buffers
            audio_frozen_state = true; 
            lamp = true;
            PRESETTER(presets[preset]); 
          }
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
uint8_t *delaybuffa;
uint8_t *delaybuffb;

// ==========================================
// SHARED 16-BIT DSP BUFFER --- NEW FIRMWARE
//
// Needed for resolution on audio computations
// 60KB shared memory pool for 16-bit delay and reverb presets
// Presets check 'current_16bit_owner' and wipe the buffer if they are newly loaded.
// #define SHARED_16BIT_LEN 30000
// int16_t buffer_16bit[SHARED_16BIT_LEN];
// int current_16bit_owner = -1;
#define SHARED_16BIT_LEN 30000
// int current_16bit_owner = -1; removing to allow buffer transfer between presets

// Cast the 8-bit delaybuffa array into a 16-bit array
// 131,072 bytes of 8-bit audio equals 65,536 slots of 16-bit audio.
#define buffer_16bit ((int16_t *)delaybuffa)
// ==========================================

uint8_t *delptr;
static int t;
static int delayskp;
static int lastskp;
int adc_read;
int gyo;
int pout;  //persistent_red

// ORIGINAL FIRMWARE
// optimizes memory use for 12 bit ADC
int dellius(int ptr, int val, bool but) {
  int zut, biz, forsh;
  if (ptr & 0x10000)
    delptr = delaybuffa;
  else
    delptr = delaybuffb;
  ptr = ptr & 0xFFFF;
  ptr = ptr * 3;
  biz = ptr & 1;
  forsh = biz << 2;
  zut = delptr[(ptr >> 1) + biz] << 4;
  zut |= (delptr[(ptr >> 1) + 1 - biz] & (0xF << (forsh))) >> (forsh);
  if (!but) {
    delptr[(ptr >> 1) + biz] = (uint8_t)(val >> 4);
    delptr[(ptr >> 1) + 1 - biz] &= (uint8_t)(0xF << (4 - forsh));
    delptr[(ptr >> 1) + 1 - biz] |= (uint8_t)((val & 0xF) << forsh);
  }
  return zut;
}
// ---------------------------------------------------------

void initDEL() {
  delaybuffa = (uint8_t *)malloc((DELAYSIZE >> 2) + (DELAYSIZE >> 1)); 
  delaybuffb = (uint8_t *)malloc((DELAYSIZE >> 2) + (DELAYSIZE >> 1)); 

  // NEW FIRMWARE
  // safety check
  if (delaybuffa == NULL || delaybuffb == NULL) {
    // buffer set up fails, flash the orange lamp rapidly 
    while (1) {
      REG(GPIO_OUT1_W1TS_REG)[0] = BIT(1);
      delay(50);
      REG(GPIO_OUT1_W1TC_REG)[0] = BIT(1);
      delay(50);
    }
  }
  /////END

  /////NEW FIRMWARE
  // needed for sample management
  //drum_ram_buffer = (uint8_t*)malloc(DRUM_RAM_SIZE); //not enough spare memory for seperate drum buffer
  drum_ram_buffer = delaybuffa; //would use the shared buffer, but since dellius reads 12 bit audio, it doesn't work well

  // Safety: if run out of RAM, use the main delay buffer // DON'T THINK THIS IS NEEDED ANYMORE
  if (drum_ram_buffer == NULL) {
    // Blink LED forever to signal Out Of Memory
    while (1) {
      REG(GPIO_OUT_REG)
      [3] ^= (1 << 1);
      delay(100);
    }
  }
  ///////////////END

  delptr = delaybuffa;
  t = 0;

  delptr = delaybuffa;
  t = 0;

  //esp_task_wdt_init(30, false);

  REG(ESP32_SENS_SAR_DAC_CTRL1)
  [0] = 0x0;
  REG(ESP32_SENS_SAR_DAC_CTRL2)
  [0] = 0x0;

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

