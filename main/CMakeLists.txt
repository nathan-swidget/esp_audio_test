set(COMPONENT_REQS 
    "custom_audio_board"
    "audio_stream"
    "audio_hal"
    "esp-adf-libs"
)
set(COMPONENT_TXTS
    "bins/test.mp3"
    "bins/test.m4a"
    "bins/test.aac"
)

idf_component_register(SRCS "audio_main.cpp"
                    INCLUDE_DIRS ""
                    REQUIRES ${COMPONENT_REQS}
                    EMBED_TXTFILES ${COMPONENT_TXTS}
)
