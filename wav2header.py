import wave
import struct
import os

# --- CONFIGURATION ---
INPUT_FILES = ['kick.wav', 'snare.wav', 'hat.wav']
OUTPUT_HEADER = 'drums.h'

def wav_to_c_array(filename):
    name = os.path.splitext(filename)[0].upper() + "_RAW"
    
    try:
        with wave.open(filename, 'rb') as w:
            # Check specs
            if w.getnchannels() != 1:
                print(f"Warning: {filename} is Stereo. It will be read as Mono.")
            
            # Read all frames
            frames = w.readframes(w.getnframes())
            sample_width = w.getsampwidth()
            
            data = []
            
            # Convert 16-bit PCM to 8-bit Unsigned (0-255)
            # 16-bit is signed (-32768 to 32767). We scale to 0-255.
            if sample_width == 2:
                raw_data = struct.unpack(f"{w.getnframes()}h", frames)
                for sample in raw_data:
                    # Normalize to 0-255 range
                    scaled = int((sample + 32768) / 256)
                    # Clip safety
                    scaled = max(0, min(255, scaled))
                    data.append(scaled)
            
            # If already 8-bit (0-255 unsigned usually)
            elif sample_width == 1:
                raw_data = struct.unpack(f"{w.getnframes()}B", frames)
                data = list(raw_data)
            
            print(f"Processed {filename}: {len(data)} samples ({len(data)/22050:.3f}s @ 22kHz)")
            return name, data

    except FileNotFoundError:
        print(f"Error: {filename} not found.")
        return None, None

# --- MAIN EXECUTION ---
c_output = []
c_output.append(f"// Generated Drum Samples for ESP32")
c_output.append(f"// Format: 8-Bit Unsigned Audio Data\n")
c_output.append(f"#ifndef DRUMS_H")
c_output.append(f"#define DRUMS_H\n")

total_bytes = 0

for wav in INPUT_FILES:
    name, data = wav_to_c_array(wav)
    if data:
        c_output.append(f"const uint8_t {name}[{len(data)}] = {{")
        
        # Format lines for readability
        line = "  "
        for i, byte in enumerate(data):
            line += f"{byte},"
            if (i + 1) % 16 == 0:
                c_output.append(line)
                line = "  "
        if line.strip():
            c_output.append(line)
            
        c_output.append(f"}};\n")
        c_output.append(f"const uint32_t {name}_LEN = {len(data)};\n")
        total_bytes += len(data)

c_output.append(f"#endif")

# Write to file
with open(OUTPUT_HEADER, 'w') as f:
    f.write('\n'.join(c_output))

print(f"\nSuccess! Created {OUTPUT_HEADER}")
print(f"Total Memory Usage: {total_bytes / 1024:.2f} KB")
if total_bytes > 150000:
    print("WARNING: Total size > 150KB. You might crash the ESP32 RAM.")
else:
    print("Memory is within safe limits.")