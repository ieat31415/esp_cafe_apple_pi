// ==========================================
// MENU
// ==========================================
// "Apple Pi" alt Firmware for the CIAT LONBARDE CAFETERIA/CAFE QUANTUM
//
// 31 Presets: a mixed bag of effects
//
// by ieat31415
// playlist of preset demos on my YOUTUBE (youtube.com/@ieat3141592)
// ------------------------------------------

// ==========================================
// CHANGE LOG --- VERSION 1.41421
// ==========================================
// New Cleaner Ash output (otherss available in stuff)
// Improved button response in Preset Selection Mode
// Added visual feedback in Preset Selection Mode
// Configuration for Original Cocoquantus startup mode available.
// Fixed DC Offset in Ash for Saturator Preset
// Plus: External Sync optomized to sync with a Coco_mod preset
// Plus: coco_mod expanded to set PPQN
// ------------------------------------------

// ==========================================
// CHANGE LOG --- VERSION 2.71828
// ==========================================
// New Preset: Tape Deck, an interface to save and recall loops in persistent memory, even across power cycles
// Load a tape deck slot during power on instead of noise. Set up in Boot configuration below
// New Preset: Windows from Daniel Fishkin
// New Preset: Splicer
// New Preset: Dissolve
// New Preset: Feedback reverb
// Plus: Added Crossfade from Peter's Firmware
// Plus: Added crossfades for reverbs
// Plus: Many under the hood improvements
// ------------------------------------------

// ============================================================================
// BOOT CONFIGURATION
// ============================================================================
// The original Cocoquantus booted with its delay buffer frozen and filled with noise
// By default, this firmware boots unfrozen (so the buffer is immediately cleared)
// 
// Change this to 'true' if you want the classic Cocoquantus frozen noise boot.
#define CLASSIC_NOISE_BOOT false

// TAPE SAVE SETUP--- NEW FIRMWARE
// Pre-load a tape every time cafe boots (if Classic_noise_boost is false)
#define ENABLE_BOOT_TAPE true // Set to false to clear buffer on boot (classic_noise_boot overrides this setting). Set to true to load tape slot below.
#define BOOT_TAPE_SLOT 1 // Specify which tape slot (1-8) to load at boot
#define BOOT_TAPE_FROZEN true // Set to true to load the buffer with the tape on boot
// ============================================================================

//90s cafe, warm tones, friends, extravagant laptop bezels.
//a coffee cup as big as your head.
//if arduino was bought by a printer company is this stable?

#define BYTECODES t*(t & 16384 ? 7 : 5) * (3 - (3 & t >> 9) + (3 & t >> 8)) >> (3 & -t >> (t & 4096 ? 2 : 16)) | t >> 3;

#include "synths.h"
#include <LittleFS.h> //for file system needed by tape_deck preset

  // ------------------------------------------
  // PRESET MENU
  // ------------------------------------------

    //  presets[0] = coco_mod;
    //  presets[1] = coco_og;
    //  presets[2] = echo_og;
    //  presets[3] = echo_mod;
    //  presets[4] = formant;
    //  presets[5] = flanger;
    //  presets[6] = karplus;
    //  presets[7] = resonator;
    //  presets[8] = reverb_spring;
    //  presets[9] = reverb_granular;
    //  presets[10] = reverb_feedback;
    //  presets[11] = harmonizer;
    //  presets[12] = saturator;
    //  presets[13] = external_sync;
    //  presets[14] = window;
    //  presets[15] = splicer; 
    //  presets[16] = scrambler;
    //  presets[17] = sampler;
    //  presets[18] = sampler_4x;
    //  presets[19] = granular;
    //  presets[20] = phasing;
    //  presets[21] = dissolve;
    //  presets[22] = tape_deck;
    //  presets[23] = bytebeats_mod;
    //  presets[24] = megabytebeats;
    //  presets[25] = arcade;
    //  presets[26] = FX;
    //  presets[27] = wavetable;
    //  presets[28] = drone;
    //  presets[29] = groovebox;
    //  presets[30] = polyrhythms;
    

// PRESET PLAYLIST DEFINITIONS
// Define custom preset playlists below 
// The preset playlist is what will be loaded when entering the Preset Selection Mode
// Start-up default preset is the one listed first in the playlist
// IMPORTANT: Presets are zero-indexed, so pressing the button once selects the second preset

void (*playlist_classic[])() = {
    coco_mod, echo_mod
};

void (*playlist_old_school[])() = {
    coco_og, echo_og
};

void (*playlist_loopers[])() = {
    coco_mod, tape_deck, formant, scrambler, sampler, sampler_4x, granular, phasing, window, splicer, dissolve
};

// use sync playlist for two cafes where is in coco_mod that will be the leader, setting the main delay time, and the other is in external_sync which will stay in sync no matter its speed knob
void (*playlist_sync[])() = {
    coco_mod, external_sync, sampler, sampler_4x
};

void (*playlist_reverbs[])() = {
    echo_mod, echo_og, reverb_spring, reverb_granular, reverb_feedback
};

void (*playlist_all_delays[])() = {
    coco_mod, external_sync, formant, window, splicer, scrambler, sampler, sampler_4x, granular, phasing, echo_mod, reverb_spring, reverb_granular, reverb_feedback, flanger
};

void (*playlist_live_FX[])() = {
   saturator, flanger, harmonizer
}; 

void (*playlist_bytes[])() = {
    bytebeats_mod, megabytebeats, arcade, FX
};

void (*playlist_synth_voices[])() = {
    drone, wavetable, karplus
};

void (*playlist_resonance[])() = {
    karplus, resonator, harmonizer
};

void (*playlist_drums[])() = {
    groovebox, polyrhythms
};

void (*playlist_ambient[])() = {
    reverb_spring, echo_mod, reverb_granular, drone, phasing
};

// here for reference, not recommended
void (*playlist_all[])() = {
    coco_mod, coco_og, echo_og, echo_mod, formant, flanger, karplus, resonator, reverb_spring, reverb_granular, reverb_feedback, harmonizer, saturator, external_sync, window, splicer, scrambler, sampler, sampler_4x, granular, phasing, dissolve, tape_deck, bytebeats_mod, megabytebeats, arcade, FX, wavetable, drone, groovebox, polyrhythms
};

void (*playlist_hello_world[])() = {
    coco_mod, echo_mod, formant, scrambler, sampler, reverb_spring, granular, phasing, reverb_granular, sampler_4x, resonator, harmonizer, flanger
};

void (*playlist_mono[])() = {
    saturator, reverb_spring, reverb_granular, flanger
};

void (*playlist_new_stuff[])() = {
    coco_mod, external_sync, coco_og, echo_og, tape_deck, dissolve, splicer, window, reverb_feedback
};

void (*playlist_test[])() = {
    coco_mod, sampler,  sampler_4x, granular, phasing
};

// ------------------------------------------
// PRESET PLAYLIST SELECTION TO LOAD
// ------------------------------------------
// Type the name of the playlist you want to load onto the Cafe: <<<<<<<<<<<<<<<<<<<<<<<<<----------
#define ACTIVE_PLAYLIST playlist_all_delays




//////ORIGINAL FIRMWARE
void setup() {

  // FOR DEBUGGING
  Serial.begin(115200);
  delay(1000); // Give the serial monitor a moment to connect
  Serial.printf("\n--- BOOT START ---\n");
  Serial.printf("Initial Free Heap: %d bytes\n", ESP.getFreeHeap());

// --- EXPLICIT FORMAT & MOUNT ---
  if (!LittleFS.begin(false)) {
    Serial.println("Mount failed. Formatting LittleFS...");
    LittleFS.format(); // Explicitly structure the raw flash
    
    if (!LittleFS.begin(false)) {
      Serial.println("LittleFS Mount Failed After Format");
    } else {
      Serial.println("LittleFS Formatted and Mounted Successfully");
    }
  } else {
    Serial.println("LittleFS Mounted Successfully");
  }

  Serial.println("[1] Running SETUPPERS (Hardware Init)...");

  SETUPPERS
  Serial.printf("[1] SETUPPERS Complete. Free Heap: %d bytes\n", ESP.getFreeHeap()); // FOR DEBUGGING

  //theCoolWifiInitiation();

  // --------------------------------------------------------
  // BOOT STATE
  // --------------------------------------------------------
  if (CLASSIC_NOISE_BOOT) {
    // Noise frozen on startup
    Serial.println("    -> Booting with Classic Frozen Noise.");
    audio_frozen_state = true;
    lamp = true;
    FILLNOISE
  } 
  else if (ENABLE_BOOT_TAPE) {
    // Tape frozen on startup
    String boot_filename = "/tape" + String(BOOT_TAPE_SLOT) + ".raw";
    Serial.printf("    -> Checking for Boot Tape (%s) in Flash...\n", boot_filename.c_str());
    
    File boot_file = LittleFS.open(boot_filename, FILE_READ);
    if (boot_file) {
        boot_file.read((uint8_t*)delaybuffb, 98304);
        boot_file.read((uint8_t*)delaybuffa, 98304);
        boot_file.close();
        Serial.println("    -> Boot Tape restored to RAM");
        audio_frozen_state = BOOT_TAPE_FROZEN;
        lamp = BOOT_TAPE_FROZEN;
    } else {
        // Clear buffer on startup
        Serial.println("    -> Clearing buffer, no tape found.");
        audio_frozen_state = false;
        lamp = false;
        for (int i = 0; i < DELAYSIZE; i++) dellius(i, 0, false);
        load_drum_kit(0);
    }
  } 
  else {
    // Clear buffer on startup
    Serial.println("    -> Clearing buffer.");
    audio_frozen_state = false;
    lamp = false;
    
    // Wipe the uninitialized RAM with silence
    for (int i = 0; i < DELAYSIZE; i++) {
        dellius(i, 0, false);
    }
    
    //load_drum_kit(0); // load drum samples into RAM
  }
  // --------------------------------------------------------

  // Pre-charge Ash Capacitor
  // Needed so ash doesn't need to wake up to send audio
  REG(ESP32_RTCIO_PAD_DAC1)
  [0] = BIT(10) | BIT(17) | BIT(18) | (64 << 19);  // 64 to get to linearity of LM3900 past the diode drop on input
  //END

  // FOR DEBUGGING
  Serial.println("[2] Routing Preset Playlist...");
  active_preset_count = sizeof(ACTIVE_PLAYLIST) / sizeof(ACTIVE_PLAYLIST[0]);
  Serial.printf("    Active Preset Count: %d\n", active_preset_count);


  // PRESET PLAYLIST ROUTER
  // counts the presets in the ACTIVE_PLAYLIST   
  active_preset_count = sizeof(ACTIVE_PLAYLIST) / sizeof(ACTIVE_PLAYLIST[0]);
  
  for (int i = 0; i < active_preset_count; i++) {
      presets[i] = ACTIVE_PLAYLIST[i];
  }  
  Serial.printf("[2] Routing Complete. Free Heap: %d bytes\n", ESP.getFreeHeap()); // FOR DEBUGGING

  DOUBLECLK

  Serial.println("[3] Starting Startup PRESETTER (Preset 0)..."); // FOR DEBUGGING

  // ------------------------------------------
  // ------------------------------------------
  // THIS IS THE STARTUP PRESET 
     PRESETTER(presets[0])
  // ------------------------------------------
  // ------------------------------------------

  // FOR DEBUGGING
  Serial.printf("[3] PRESETTER Complete. Free Heap: %d bytes\n", ESP.getFreeHeap());
  Serial.println("[4] Running Boot Animation...");

  //BOOT ANIMATION
  for (int i = 0; i < 5; i++) {
    REG(GPIO_OUT1_W1TS_REG)
    [0] = BIT(1);
    delay(50);
    REG(GPIO_OUT1_W1TC_REG)
    [0] = BIT(1);
    delay(50);
  }

LAMPLIGHT_OVERRIDE;   // Sync the physical hardware following boot animation

Serial.println("--- BOOT COMPLETE: Entering Main Loop ---\n"); // FOR DEBUGGING
}

////////////


// ==========================================
// PRESET SELECTION MODE --- NEW FIRMWARE
// ==========================================
//------------------------------------------
// long press button
// lamp will flash
// press button number of times as the preset index

void loop() {

  // LONG PRESS INDICATOR
  // If the button is held down, watch the hardware timer.
  if (is_pressed && !preset_mode) {
    REG(TIMG0_T0UPDATE_REG)[0] = BIT(1);
    uint32_t current_time = REG(TIMG0_T0LO_REG)[0];
    
    // Handle timer overflow
    uint32_t hold_time = current_time - press_time;
    if (current_time < press_time) hold_time = (0xFFFFFFFF - press_time) + current_time;

    // Once held past 0.8 seconds (2,000,000 ticks), flash lamp rapidly
    if (hold_time > 2000000) {
      // 250,000 ticks = 100ms
      lamp = ((current_time % 250000) > 125000); 
      LAMPLIGHT_OVERRIDE; //
    }
  }

  // --- PRESET SELECTION MODE ---
  if (preset_mode) {
    
    Serial.println("Preset Mode Active: Waiting for physical button punch-in...");

    // variables for visual feedback
    int blink_state = 0; // 0=Pause, 1=Tens ON, 2=Tens OFF, 3=Ones ON, 4=Ones OFF
    int blink_count = 0;
    int tick_timer = 0;

    int flash_tick = 0;

    // The Latching Loop
    while (preset_mode) {
      bool threshold_met = false;
      
      // --- LONG PRESS INDICATOR ---
      if (is_pressed) {
        REG(TIMG0_T0UPDATE_REG)[0] = BIT(1);
        uint32_t current_time = REG(TIMG0_T0LO_REG)[0];
        
        uint32_t hold_time = current_time - press_time;
        if (current_time < press_time) hold_time = (0xFFFFFFFF - press_time) + current_time;

        if (hold_time > 2000000) {
          // Override the slow stutter with the rapid strobe to say "Let Go!"
          lamp = ((current_time % 250000) > 125000); 
          LAMPLIGHT_OVERRIDE; //
          threshold_met = true;
        }
      }


      // PRESET BLINKER FOR VISUAL FEEDBACK
      if (!threshold_met && !is_pressed) {
          
          // Calculate the actual human-readable preset number (1 to 24)
          int display_num = (preset_counter % active_preset_count);
          int tens = display_num / 10;
          int ones = display_num % 10;

          tick_timer++; // Increments roughly every 10ms due to vTaskDelay

          if (blink_state == 0) { 
              // State 0: Long pause (1 second) before repeating the pattern
              lamp = false;
              if (tick_timer > 100) { tick_timer = 0; blink_state = 1; blink_count = 0; }
          }
          else if (blink_state == 1) { 
              // State 1: Tens ON (Long Blink - 400ms)
              if (tens == 0) { blink_state = 3; tick_timer = 0; blink_count = 0; } // Skip to ones
              else {
                  lamp = true;
                  if (tick_timer > 40) { tick_timer = 0; blink_state = 2; }
              }
          }
          else if (blink_state == 2) { 
              // State 2: Tens OFF (Gap - 200ms)
              lamp = false;
              if (tick_timer > 20) { 
                  tick_timer = 0; 
                  blink_count++;
                  if (blink_count < tens) blink_state = 1; // Loop back for next Ten
                  else { blink_state = 3; blink_count = 0; tick_timer = -30; } // Extra 300ms gap before Ones
              }
          }
          else if (blink_state == 3) { 
              // State 3: Ones ON (Short Blink - 150ms)
              if (ones == 0) { blink_state = 0; tick_timer = 0; } // Skip back to start
              else {
                  lamp = true;
                  if (tick_timer > 15) { tick_timer = 0; blink_state = 4; }
              }
          }
          else if (blink_state == 4) { 
              // State 4: Ones OFF (Gap - 200ms)
              lamp = false;
              if (tick_timer > 20) {
                  tick_timer = 0;
                  blink_count++;
                  if (blink_count < ones) blink_state = 3; // Loop back for next One
                  else blink_state = 0; 
              }
          }
          
          LAMPLIGHT_OVERRIDE;
          
      } else if (is_pressed && !threshold_met) {
          // If actively tapping, reset
          blink_state = 0;
          tick_timer = 0;
      }
      
      // Process Autoload instantly while in the menu
      if (tape_load_flag) {
          Serial.printf("\n[TAPE DECK] --- AUTOLOAD INITIATED ---\n");
          REG(I2S_CONF_REG)[0] &= ~(BIT(5)); // Pause audio stream
          
          String filename = "/tape" + String(tape_index) + ".raw";
          File file = LittleFS.open(filename, FILE_READ);
          if(file) {
              Serial.printf("[TAPE DECK] Pulling 196KB file into RAM buffers...\n");
              file.read((uint8_t*)delaybuffb, 98304); 
              file.read((uint8_t*)delaybuffa, 98304); 
              file.close();
          }
          tape_load_flag = false;
          
          REG(I2S_INT_CLR_REG)[0] = 0xFFFFFFFF;
          REG(I2S_CONF_REG)[0] |= (BIT(5)); // Resume audio stream
      }

      // Yield to FreeRTOS to prevent watchdog resets
      vTaskDelay(10); 
    }

    Serial.printf("Exiting mode. Loading preset index: %d\n", preset_counter);

    if (presets[preset] == polyrhythms) {
        load_drum_kit(0);
    }

    // EXIT PRESET SELECTION MODE

    Serial.println("[EXITING] 1. Temporarily disabling button (to prevent bounce)...");
    detachInterrupt(32); // Disconnect the black button

    // Unplug the clock
    Serial.println("[EXITING] 2. Detaching interrupt...");
    detachInterrupt(2);                
    
    // Pause the water main
    Serial.println("[EXITING] 3. Starting 10ms crossfade...");
    // flush the incoming audio so the 64-sample RX FIFO doesn't overflow
    int start_vol = pout; 
    int steps = 500; 
    
    for (int i = 0; i <= steps; i++) {
        // read and discard incoming audio samples
        // prevents  overflow AND natively paces the loop to exactly 48kHz (20.8us per step)!
        volatile uint32_t sinkhole = REG(I2S_FIFO_RD_REG)[0]; 
        
        int current_vol = start_vol + ((2048 - start_vol) * i) / steps;
        DACWRITER(current_vol);
        ASHWRITER(current_vol);
    }

    Serial.println("[EXITING] 4. Pausing I2S reading audio...");
    REG(I2S_CONF_REG)[0] &= ~(BIT(5));

    Serial.println("[EXITING] 5. Crossfade complete. Loading drum samples if polyrhythms preset is selected...");
    if (presets[preset] == polyrhythms) {
        load_drum_kit(0);
    }

    // Resume Audio Engine
    Serial.println("[EXITING] 6. Restoring in loop mode...");
    lamp = audio_frozen_state; 
    LAMPLIGHT_OVERRIDE; 

    Serial.println("[EXITING] 7. Hardware Reset...");
    REG(I2S_CONF_REG)[0] |= BIT(30);  // Set I2S_RX_FIFO_RESET
    REG(I2S_CONF_REG)[0] &= ~BIT(30); // Clear I2S_RX_FIFO_RESET

    Serial.println("[EXITING] 8. Clearing and resuming I2S pipeline...");
    REG(I2S_INT_CLR_REG)[0] = 0xFFFFFFFF; // Clear any clock ticks that queued up on the GPIO pin during the delay
    REG(I2S_CONF_REG)[0] |= (BIT(5)); 

    Serial.println("[EXITING] 8. Resuming I2S pipeline...");
    REG(I2S_INT_CLR_REG)[0] = 0xFFFFFFFF; 
    REG(I2S_CONF_REG)[0] |= (BIT(5));

    Serial.println("[EXITING] 9. Hardware Spin-up Loop (1 millisecond)...");
    // Hold the CPU safely in place while the I2S hardware packs the FIFO with fresh audio!
    for (volatile int i = 0; i < 100000; i++) {
        __asm__ __volatile__ ("nop");
    }

    // Load the new preset while everything is paused
    Serial.println("[EXITING] 10. Attaching clock to new preset...");
    PRESETTER(presets[preset]);

    Serial.println("[EXITING] 11. Re-attaching button..."); 
    CLICKETTE(doubleclicker);

   //Serial.println("[EXITING] SUCCESS: Sequence complete. Loaded preset: %d\n", preset_counter);
   Serial.printf("[EXITING] SUCCESS: Sequence complete. Loaded preset: %d\n", preset_counter);

  }

// --- TAPE DECK SETUP ---
  if (tape_save_flag) {
    Serial.printf("\n[TAPE DECK] --- SAVE INITIATED ---\n");
    Serial.printf("[TAPE DECK] Pausing audio engine...\n");
    REG(I2S_CONF_REG)[0] &= ~(BIT(5)); // Pause audio stream
    
    String filename = "/tape" + String(tape_index) + ".raw";
    Serial.printf("[TAPE DECK] Opening %s for writing...\n", filename.c_str());
    
    File file = LittleFS.open(filename, FILE_WRITE);
    if(file) {
        Serial.printf("[TAPE DECK] Burning 196KB RAM buffers to Flash...\n");
        file.write((const uint8_t*)delaybuffb, 98304); // Write first half
        file.write((const uint8_t*)delaybuffa, 98304); // Write second half
        file.close();
        Serial.printf("[TAPE DECK] Save successful!\n");
    } else {
        Serial.printf("[TAPE DECK] ERROR: Failed to open %s for writing\n", filename.c_str());
    }
    
    tape_save_flag = false;
    Serial.printf("[TAPE DECK] Resuming audio engine...\n");
    REG(I2S_INT_CLR_REG)[0] = 0xFFFFFFFF;
    REG(I2S_CONF_REG)[0] |= (BIT(5)); // Resume audio stream
  }

  if (tape_load_flag) {
    Serial.printf("\n[TAPE DECK] --- LOAD INITIATED ---\n");
    Serial.printf("[TAPE DECK] Pausing audio engine...\n");
    REG(I2S_CONF_REG)[0] &= ~(BIT(5)); // Pause audio stream
    
    String filename = "/tape" + String(tape_index) + ".raw";
    Serial.printf("[TAPE DECK] Locating %s in Flash...\n", filename.c_str());
    
    File file = LittleFS.open(filename, FILE_READ);
    if(file) {
        Serial.printf("[TAPE DECK] Pulling 196KB file into RAM buffers...\n");
        file.read((uint8_t*)delaybuffb, 98304); // Load first half
        file.read((uint8_t*)delaybuffa, 98304); // Load second half
        file.close();
        Serial.printf("[TAPE DECK] Load successful!\n");
    } else {
        Serial.printf("[TAPE DECK] ERROR: %s is empty or missing. RAM untouched.\n", filename.c_str());
    }
    
    tape_load_flag = false;
    Serial.printf("[TAPE DECK] Resuming audio engine...\n");
    REG(I2S_INT_CLR_REG)[0] = 0xFFFFFFFF;
    REG(I2S_CONF_REG)[0] |= (BIT(5)); // Resume audio stream
  }

  delay(10);
}

//------------------------------------------

// // ==========================================
// // CAFE EARTH DIAGNOSTIC TOOL
// // ==========================================
// // USE THIS IF YOU WANT TO CHECK THE EXACT EARTH VALUES TO CALIBRATE PRESETS.
// // COMMENT OUT THE ORIGINAL SETUP() AND LOOP() FUNCTIONS IN YOUR MAIN .INO FILE 
// // AND UNCOMMENT THIS SECTION TO RUN.

// // 1. Fulfill the OS definitions so synths.h compiles without errors
// #define PRESETAMT 3
// #define BYTECODES 0
// #define BOOT_TAPE_SLOT 1
// #define CROSSFADE 256
// #define CROSSBITE 8
// #define BUTTON_PRESSED (!(REG(GPIO_IN1_REG)[0] & 0x1))

// #include "synths.h"

// // 2. Create a dummy function to prevent hardware button crashes
// void dummy_preset() {
//     // Does nothing, just acts as a safe placeholder for the interrupt
// }

// void setup() {
//     Serial.begin(115200);
//     delay(1000);

//     Serial.println("\n--- NATIVE I2S EARTH DIAGNOSTIC ---");

//     // Safely fill the preset array so the doubleclicker() interrupt has a safe target
//     presets[0] = dummy_preset;
//     presets[1] = dummy_preset;
//     presets[2] = dummy_preset;

//     // Turn on the hardware and I2S ADC routing
//     SETUPPERS
// }

// void loop() {
//     // Pause the I2S Receive Engine to safely access the queue
//     REG(I2S_CONF_REG)[0] &= ~(BIT(5));

//     // Configure the ADC pattern table to ensure we are targeting the right pins
//     REG(APB_SARADC_SAR1_PATT_TAB1_REG)[0] = (0x0C<<24) | (0x6C<<16);
    
//     // Pull the interleaved data from the FIFO
//     uint32_t fifo_data = REG(I2S_FIFO_RD_REG)[0];
//     int channel = (fifo_data >> 12) & 0xF;
//     int raw_val = fifo_data & 0xFFF;

//     // Clear interrupts and restart the I2S Receive Engine
//     REG(I2S_INT_CLR_REG)[0]=0xFFFFFFFF;
//     REG(I2S_CONF_REG)[0] |= (BIT(5));

//     // Only print when we hit Channel 0 (Earth)
//     if (channel == 0) {
//         Serial.print("Raw Earth (12-Bit): ");
//         Serial.println(raw_val);
//     }

//     // A 100ms delay gives a readable 10 frames per second on the Serial Monitor
//     delay(100);
// }

