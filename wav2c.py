import sys
import wave
import struct

def main():
    if len(sys.argv) < 2:
        print("Usage: python wav2c.py <input.wav>")
        return

    input_filename = sys.argv[1]
    array_name = input_filename.split('.')[0].replace(' ', '_').lower() + "_sample"

    try:
        with wave.open(input_filename, 'rb') as wav:
            # Get properties
            channels = wav.getnchannels()
            width = wav.getsampwidth()
            rate = wav.getframerate()
            frames = wav.getnframes()
            
            print(f"Processing {input_filename}: {rate}Hz, {channels}ch, {frames} frames")

            # Read all frames
            raw_data = wav.readframes(frames)
            
            # Convert to mono 8-bit (0-255)
            # ESP32 DAC is 8-bit, so we compress here to save space
            output_data = []
            
            total_samples = len(raw_data) // width
            
            # Simple downsampling (skip samples) if the file is huge
            # Target ~4000-8000 samples max for memory safety
            step = 1
            if frames > 10000:
                step = frames // 10000
                print(f"File too large. Downsampling by {step}x...")

            for i in range(0, frames, step):
                # Calculate index based on width (1 byte vs 2 bytes)
                idx = i * width * channels
                
                if width == 1: # 8-bit 0-255
                    val = raw_data[idx] 
                elif width == 2: # 16-bit signed -32768 to 32767
                    # Read little endian
                    sample = struct.unpack_from('<h', raw_data, idx)[0]
                    # Convert to 0-255 range
                    val = int((sample + 32768) / 256)
                else:
                    val = 128 # Unsupported bit depth
                
                # Center around 0 for signed audio logic (-128 to 127)
                # Or keep 0-255 for raw DAC. Let's do Signed (-128 to 127)
                output_data.append(val - 128)

            # Generate C File content
            print(f"Generated {len(output_data)} bytes.")
            
            c_code = f"// Sample: {input_filename}\n"
            c_code += f"// Length: {len(output_data)}\n"
            c_code += f"const int8_t {array_name}[] = {{\n    "
            
            for i, byte in enumerate(output_data):
                c_code += f"{byte}, "
                if (i + 1) % 16 == 0:
                    c_code += "\n    "
            
            c_code += "\n};\n"
            c_code += f"const int {array_name}_len = {len(output_data)};\n"

            with open(f"{array_name}.h", "w") as f:
                f.write(c_code)
                
            print(f"Done! Saved to {array_name}.h")
            print("Copy the array into your synths.h file.")

    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main()