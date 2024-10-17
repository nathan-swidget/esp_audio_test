#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_mem.h"
#include "audio_common.h"
#include "i2s_stream.h"
#include "mp3_decoder.h"
#include "aac_decoder.h"
#include "board.h"
#include <string.h>

#define USE_M4A 1
#define USE_AAC 0
#define USE_MP3 0
#if USE_M4A
    extern const uint8_t audio_start_asm[] asm("_binary_test_m4a_start");
    extern const uint8_t audio_end_asm[] asm("_binary_test_m4a_end");
#endif // USE_M4A
#if USE_AAC
    extern const uint8_t audio_start_asm[] asm("_binary_test_aac_start");
    extern const uint8_t audio_end_asm[] asm("_binary_test_aac_end");
#endif // USE_AAC
#if USE_MP3
    extern const uint8_t audio_start_asm[] asm("_binary_test_mp3_start");
    extern const uint8_t audio_end_asm[] asm("_binary_test_mp3_end");
#endif  // USE_MP3

static int bytesConsumed = 0;
extern "C" audio_element_err_t read_audio_from_flash(audio_element_handle_t el, char *buf, int len, TickType_t wait_time, void *ctx) {
    size_t remaining = audio_end_asm - audio_start_asm - bytesConsumed;
    int read_size = remaining;
    if (read_size == 0) {
        return AEL_IO_DONE;
    } else if (len < read_size) {
        read_size = len;
    }

    memcpy(buf, audio_start_asm + bytesConsumed, read_size);
    bytesConsumed += read_size;
    return (audio_element_err_t)read_size;
}

static i2s_stream_cfg_t getDefaultI2CConfig() {
    i2s_stream_cfg_t cfg{
        .type = AUDIO_STREAM_WRITER,
        .transmit_mode = I2S_COMM_MODE_STD,
        .chan_cfg = {
            .id = I2S_NUM_0,
            .role = I2S_ROLE_MASTER,
            .dma_desc_num = 3,
            .dma_frame_num = 312,
            .auto_clear = true
        },
        .std_cfg = {
            .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
            .slot_cfg = {
                .data_bit_width = I2S_DATA_BIT_WIDTH_16BIT,
                .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
                .slot_mode = I2S_SLOT_MODE_MONO,
                .slot_mask = I2S_STD_SLOT_RIGHT,
                .ws_width = I2S_DATA_BIT_WIDTH_16BIT,
                .ws_pol = false,
                .bit_shift = true,
                #if SOC_I2S_HW_VERSION_1
                .msb_right = true,
                #else
                .left_align = true,
                .big_endian = false,
                .bit_order_lsb = false
                #endif
            },
            .gpio_cfg = { // this is loaded from custom board info
                .invert_flags = {
                    .mclk_inv = false,
                    .bclk_inv = false,
                }
            }
        },
        .use_alc = true,
        .volume = 100,
        .out_rb_size = I2S_STREAM_RINGBUFFER_SIZE,                                 
        .task_stack = I2S_STREAM_TASK_STACK,                                       
        .task_core = I2S_STREAM_TASK_CORE,                                         
        .task_prio = I2S_STREAM_TASK_PRIO,                                         
        .stack_in_ext = false,                                                     
        .multi_out_num = 0,                                                        
        .uninstall_drv = true,                                                     
        .need_expand = false,                                                      
        .buffer_len = I2S_STREAM_BUF_SIZE                                     
    };
    
    return cfg;
}


extern "C" void app_main() {
    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);
    
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    audio_pipeline_handle_t pipeline = audio_pipeline_init(&pipeline_cfg);
    

    i2s_stream_cfg_t i2s_cfg = getDefaultI2CConfig();
    audio_element_handle_t i2sWriter = i2s_stream_init(&i2s_cfg);
    i2s_stream_set_clk(i2sWriter, 16000, 16, 1);

    audio_element_handle_t decoder;
    #if USE_M4A
    aac_decoder_cfg_t aac_dec_cfg  = DEFAULT_AAC_DECODER_CONFIG();
    decoder = aac_decoder_init(&aac_dec_cfg);
    #endif  // USE_M4A
    #if USE_AAC
    aac_decoder_cfg_t aac_dec_cfg  = DEFAULT_AAC_DECODER_CONFIG();
    decoder = aac_decoder_init(&aac_dec_cfg);
    #endif  // USE_M4A
    #if USE_MP3
    mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
    decoder = mp3_decoder_init(&mp3_cfg);
    #endif  // USE_MP3

    audio_element_set_read_cb(decoder, read_audio_from_flash, NULL);

    const char *link_tag[2] = {"dec",  "i2s"};

    audio_pipeline_register(pipeline, decoder, "dec");
    audio_pipeline_register(pipeline, i2sWriter, "i2s");

    audio_pipeline_link(pipeline, &link_tag[0], 2);
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t eventHandle = audio_event_iface_init(&evt_cfg);
    audio_pipeline_set_listener(pipeline, eventHandle);
    audio_pipeline_run(pipeline);
    while(1) {
        audio_event_iface_msg_t msg;
        esp_err_t ret = audio_event_iface_listen(eventHandle, &msg, portMAX_DELAY);
        if (ret != ESP_OK) continue;
        if (msg.cmd == AEL_MSG_CMD_STOP) {
            break;
        }
        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
            audio_element_info_t music_info = {0};
            audio_element_getinfo((audio_element_handle_t)msg.source, &music_info);
            i2s_stream_set_clk(i2sWriter, music_info.sample_rates, music_info.bits, music_info.channels);
            continue;
        }

        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) i2sWriter
            && msg.cmd == AEL_MSG_CMD_REPORT_STATUS) {
            if ((int)msg.data == AEL_STATUS_STATE_FINISHED || (int)msg.data == AEL_STATUS_STATE_STOPPED) {
                break;
            }
        }
    }

    audio_pipeline_stop(pipeline);
    audio_pipeline_wait_for_stop(pipeline);
    audio_pipeline_unlink(pipeline);

    audio_pipeline_unregister(pipeline, i2sWriter);
    audio_element_deinit(i2sWriter);

    audio_pipeline_unregister(pipeline, decoder);
    audio_element_deinit(decoder);

    audio_pipeline_remove_listener(pipeline);
}