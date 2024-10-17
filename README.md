# Setup

source esp idf (. ./export.sh from ESP-IDF)  
export ADF_PATH=...  
  
cmake -B ./build -S . -DIDF_TARGET=esp32s3  
cmake --build build --target app  

# Configure

In main/audio_main.cpp, change which audio is used by changing the defines at the top.  