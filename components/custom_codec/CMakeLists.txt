# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES)
set(COMPONENT_PRIV_REQUIRES audio_hal audio_sal)

if(CONFIG_AUDIO_BOARD_CUSTOM)

list(APPEND COMPONENT_ADD_INCLUDEDIRS .)
set(COMPONENT_SRCS "new_codec.c")
endif()

register_component()
