# Setup

source esp idf (. ./export.sh from ESP-IDF)  
export ADF_PATH=...  
  
cmake -B ./build -S . -DIDF_TARGET=esp32s3  
cmake --build build --target app  

# Configure

In main/audio_main.cpp, change which audio is used by changing the defines at the top.  

# Additional Info

MP3 and AAC playback works, but M4A playback fails. This is using a custom board built on the ESP-S3-PICO-1.  

ESP-IDF: release/v5.3 (707d097b01756687cca18be855a2675d150247ae)
ESP-ADF: v2.7 (9cf556de500019bb79f3bb84c821fda37668c052)