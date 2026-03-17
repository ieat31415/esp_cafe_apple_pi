// ==========================================
// MENU
// ==========================================
// "Apple Pi" alt Firmware for the CIAT LONBARDE CAFETERIA/CAFE QUANTUM
//
// 24 Presets: a mixed bag of effects
//
// by ieat31415
// playlist of preset demos on my YOUTUBE (youtube.com/@ieat3141592)
// ------------------------------------------

//90s cafe, warm tones, friends, extravagant laptop bezels.
//a coffee cup as big as your head.
//if arduino was bought by a printer company is this stable?

#define BYTECODES t*(t & 16384 ? 7 : 5) * (3 - (3 & t >> 9) + (3 & t >> 8)) >> (3 & -t >> (t & 4096 ? 2 : 16)) | t >> 3;

#include "synths.h"

//#define PRESETAMT 24

  // ------------------------------------------
  // PRESET MENU
  // ------------------------------------------

    //  presets[0] = coco_mod;
    //  presets[1] = echo_mod;
    //  presets[2] = formant;
    //  presets[3] = flanger;
    //  presets[4] = karplus;
    //  presets[5] = resonator;
    //  presets[6] = reverb_spring;
    //  presets[7] = reverb_granular;
    //  presets[8] = harmonizer;
    //  presets[9] = saturator;
    //  presets[10] = external_sync;
    //  presets[11] = scrambler;
    //  presets[12] = sampler;
    //  presets[13] = sampler_4x;
    //  presets[14] = granular;
    //  presets[15] = phasing;
    //  presets[16] = bytebeats_mod;
    //  presets[17] = megabytebeats;
    //  presets[18] = arcade;
    //  presets[19] = FX;
    //  presets[20] = wavetable;
    //  presets[21] = drone;
    //  presets[22] = groovebox;
    //  presets[23] = polyrhythms;

// PRESET PLAYLIST DEFINITIONS
// Define custom preset playlists below 
// The preset playlist is what will be loaded when entering the Preset Selection Mode
// Start-up default preset is the one listed first in the playlist
// IMPORTANT: Presets are zero-indexed, so pressing the button once selects the second preset

void (*playlist_classic[])() = {
    coco_mod, echo_mod
};

void (*playlist_loopers[])() = {
    coco_mod, formant, scrambler, sampler, sampler_4x, granular, phasing
};

void (*playlist_sync[])() = {
    coco_mod, external_sync, sampler, sampler_4x
};

void (*playlist_reverbs[])() = {
    echo_mod, reverb_spring, reverb_granular
};

void (*playlist_all_delays[])() = {
    coco_mod, external_sync, formant, scrambler, sampler, sampler_4x, granular, phasing, echo_mod, reverb_spring, reverb_granular, flanger
};

void (*playlist_live_FX[])() = {
   saturator, flanger
}; 

void (*playlist_bytes[])() = {
    bytebeats_mod, megabytebeats, arcade, FX
};

void (*playlist_synth_voices[])() = {
    drone, wavetable, karplus
};

void (*playlist_resonance[])() = {
    karplus, resonator
};

void (*playlist_drums[])() = {
    groovebox, polyrhythms
};

void (*playlist_ambient[])() = {
    reverb_spring, echo_mod, reverb_granular, drone, phasing
};

void (*playlist_all[])() = {
    coco_mod, echo_mod, formant, flanger, karplus, resonator, 
    reverb_spring, reverb_granular, harmonizer, saturator, external_sync, 
    scrambler, sampler, sampler_4x, granular, phasing, bytebeats_mod, 
    megabytebeats, arcade, FX, wavetable, drone, groovebox, polyrhythms
};

// ------------------------------------------
// PRESET PLAYLIST SELECTION TO LOAD
// ------------------------------------------
// Type the name of the playlist you want to load onto the Cafe: <<<<<<<<<<<<<<<<<<<<<<<<<----------
#define ACTIVE_PLAYLIST playlist_classic



//////ORIGINAL FIRMWARE
void setup() {

  // FOR DEBUGGING
  Serial.begin(115200);

  SETUPPERS
  //theCoolWifiInitiation();

  ////NEW FIRMEARE FOR SAMPLE MANAGEMENT
  load_drum_kit(0);
  //END

  lamp = false;  //set the initial recording state
  LAMPLIGHT_OVERRIDE;   // Sync the physical hardware
  //////ORIGINAL///////
  //LAMPLIGHT
  //////////////END

  ///////NEW FIRMWARE/////////////
  // Pre-charge Ash Capacitor
  // Needed so ash doesn't need to wake up to send audio
  REG(ESP32_RTCIO_PAD_DAC1)
  [0] = BIT(10) | BIT(17) | BIT(18) | (64 << 19);  // 64 to get to linearity of LM3900 past the diode drop on input
  //END


  // PRESET PLAYLIST ROUTER
  // counts the presets in the ACTIVE_PLAYLIST   
  active_preset_count = sizeof(ACTIVE_PLAYLIST) / sizeof(ACTIVE_PLAYLIST[0]);
  
  for (int i = 0; i < active_preset_count; i++) {
      presets[i] = ACTIVE_PLAYLIST[i];
  }  

  //FILLNOISE
  DOUBLECLK

  // ------------------------------------------
  // ------------------------------------------
  // THIS IS THE STARTUP PRESET 
     PRESETTER(presets[0])
  // ------------------------------------------
  // ------------------------------------------

  ///////NEW FIRMWARE/////////////
  //BOOT ANIMATION
  for (int i = 0; i < 5; i++) {
    REG(GPIO_OUT1_W1TS_REG)
    [0] = BIT(1);
    delay(50);
    REG(GPIO_OUT1_W1TC_REG)
    [0] = BIT(1);
    delay(50);
  }
  ////////////////////////END
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
    // 1. Pause the audio stream to prevent memory crashes
    REG(I2S_CONF_REG)[0] &= ~(BIT(5)); //
    
    Serial.println("Preset Mode Active: Waiting for physical button punch-in...");

    int flash_tick = 0;

    // 2. The Latching Loop
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

      // If not holding the button to exit, do the normal preset selection stutter
      if (!threshold_met) {
        // Asymmetric odd flashing pattern
        flash_tick++;
        if (flash_tick % 50 == 0) { 
          lamp = true; LAMPLIGHT_OVERRIDE; //
        } else if (flash_tick % 50 == 5) {
          lamp = false; LAMPLIGHT_OVERRIDE; //
        }
      }
      
      // Yield to FreeRTOS to prevent watchdog resets
      vTaskDelay(10); 
    }

    Serial.printf("Exiting mode. Loading preset index: %d\n", preset_counter);

    if (presets[preset] == polyrhythms) {
        load_drum_kit(0);
    }

    Serial.printf("Exiting mode. Loading preset index: %d\n", preset_counter);

    // 3. Resume Audio Engine

    // 3. Resume Audio Engine
    //lamp = false; // original
    lamp = audio_frozen_state; // <-- CHANGED: Restore the latch instead of forcing false
    LAMPLIGHT_OVERRIDE; //
    REG(I2S_INT_CLR_REG)[0] = 0xFFFFFFFF; //
    REG(I2S_CONF_REG)[0] |= (BIT(5));     //
  }
  delay(10);
}
//------------------------------------------


// // ==========================================
// // CAFE EARTH DIAGNOSTIC TOOL
// // ==========================================
// // USE THIS IF YOU WANT TO CHECK THE EARTH VALUES FOR REFERENCE DEVELOPING ANOTHER PRESET
// // COMMENT OUT THE MAIN .INO FILE AND REPLACE WITH THIS
// // TO READ THE QUANTIZED EARTH DATA READ OUT
// // IN SERIAL MONITOR

// // Fulfill the needed definitions so the .h files compile successfully
// #define PRESETAMT 3
// #define BYTECODES 0

// #include "synths.h"

// // Create a dummy function to prevent hardware button crashes
// void dummy_preset() {
//     // Does nothing, just acts as a safe placeholder for the interrupt
// }

// void setup() {
//     Serial.begin(115200);
//     delay(1000);

//     Serial.println("\n--- NATIVE I2S DIAGNOSTIC ---");

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

//     // Read the RAW 12-bit value directly from the FIFO
//     // The original macro is: (REG(I2S_FIFO_RD_REG)[0]&0x7FF)>>3
//     // We use & 0xFFF here so we can see the full 0-4095 range
//     int raw_earth = REG(I2S_FIFO_RD_REG)[0] & 0xFFF;

//     // Clear interrupts and restart the I2S Receive Engine
//     REG(I2S_INT_CLR_REG)[0]=0xFFFFFFFF;
//     REG(I2S_CONF_REG)[0] |= (BIT(5));

//     // Print
//     Serial.print("Raw Earth (12-Bit): ");
//     Serial.print(raw_earth);

//     // print the original 8-bit macro result to compare
//     int macro_earth = (raw_earth & 0x7FF) >> 3;
//     Serial.print("\t | 8-Bit Macro Result: ");
//     Serial.println(macro_earth);

//     // A 100ms delay gives a readable 10 frames per second on the Serial Monitor
//     delay(100);
// }

