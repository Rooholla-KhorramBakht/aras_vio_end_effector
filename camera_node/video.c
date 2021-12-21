#include "arducam_mipicamera.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <linux/v4l2-controls.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>
#include <sys/types.h>
#include <stdint.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IP_PROTOCOL 0
char ip_address[] = "10.42.0.1";
int port = 10000;
 int transmit_socket;
struct sockaddr_in addr_con;

struct timespec last_hypervisor_update;

typedef struct{
	double id1;
    double state1;
}hypervisor_state_t;

union{
	hypervisor_state_t data;
	char buffer[sizeof(hypervisor_state_t)];
} hypervisor_packet;

#define LOG(fmt, args...) fprintf(stderr, fmt "\n", ##args)

// #define SOFTWARE_AE_AWB
 #define ENABLE_PREVIEW


static volatile int running = 1;

void intHandler(int dummy) {
    running = 0;
}

FILE *fd;
FILE *csv_fp;
char file_name[] = "recorded_data/camera_stamps";
void writeCsv(struct timespec stamp){
    static int index = 0;
    fprintf(csv_fp,"\n%lu,%lu,%lu",index, stamp.tv_sec,stamp.tv_nsec);
    index ++;
}

void openCsv(char *filename)
{
    csv_fp=fopen("data_tmp_dir/camera_stamps.csv","w+");
    fprintf(csv_fp,"index, stamp_sec, stamp_nsec");
}
void closeCsv()
{
  fclose(csv_fp);
}


int frame_count = 0;
int video_callback(BUFFER *buffer) {
    static int hypervisor_update_timeout = 0;

    if (TIME_UNKNOWN == buffer->pts) {
        // Frame data in the second half
    }
    // LOG("buffer length = %d, pts = %llu, flags = 0x%X", buffer->length, buffer->pts, buffer->flags);

    // LOG("buffer secs = %lu, nsecs = %lu",stamp.tv_sec,stamp.tv_nsec);
    if (buffer->length) {
        if (buffer->flags & MMAL_BUFFER_HEADER_FLAG_CONFIG) {
            // SPS PPS
            if (fd) {
                fwrite(buffer->data, 1, buffer->length, fd);
                fflush(fd);
            }
        }
        if (buffer->flags & MMAL_BUFFER_HEADER_FLAG_CODECSIDEINFO) {
            /// Encoder outputs inline Motion Vectors
        } else {
            // MMAL_BUFFER_HEADER_FLAG_KEYFRAME
            // MMAL_BUFFER_HEADER_FLAG_FRAME_END
            if (fd) {
                int bytes_written = fwrite(buffer->data, 1, buffer->length, fd);
                fflush(fd);
            }
            // Here may be just a part of the data, we need to check it.
            if (buffer->flags & MMAL_BUFFER_HEADER_FLAG_FRAME_END)
            {
                struct timespec stamp;
                clock_gettime(CLOCK_MONOTONIC, &stamp);
                writeCsv(stamp);
                hypervisor_packet.data.state1 = 1;

                if(stamp.tv_sec - last_hypervisor_update.tv_sec >= 1)
                {
                    clock_gettime(CLOCK_MONOTONIC, &last_hypervisor_update);
                    
                    sendto(transmit_socket, hypervisor_packet.buffer, sizeof(hypervisor_packet.buffer),
                           0,(struct sockaddr*)&addr_con, sizeof(addr_con));

                }
                frame_count++;
            }
        }
    }
    return 0;
}
static void default_status(VIDEO_ENCODER_STATE *state) {
    // Default everything to zero
    memset(state, 0, sizeof(VIDEO_ENCODER_STATE));
    state->encoding = VIDEO_ENCODING_H264;
    state->bitrate = 17000000;
    state->immutableInput = 1; // Not working
    /**********************H264 only**************************************/
    state->intraperiod = -1;                  // Not set
                                              // Specify the intra refresh period (key frame rate/GoP size).
                                              // Zero to produce an initial I-frame and then just P-frames.
    state->quantisationParameter = 0;         // Quantisation parameter. Use approximately 10-40. Default 0 (off)
    state->profile = VIDEO_PROFILE_H264_HIGH; // Specify H264 profile to use for encoding
    state->level = VIDEO_LEVEL_H264_4;        // Specify H264 level to use for encoding
    state->bInlineHeaders = 0;                // Insert inline headers (SPS, PPS) to stream
    state->inlineMotionVectors = 0;           // output motion vector estimates
    state->intra_refresh_type = -1;           // Set intra refresh type
    state->addSPSTiming = 0;                  // zero or one
    state->slices = 1;
    /**********************H264 only**************************************/
}
struct reg {
    uint16_t address;
    uint16_t value;
};

struct reg regs[] = {
    {0x4F00, 0x01},
    {0x3030, 0x04},
    {0x303F, 0x01},
    {0x302C, 0x00},
    {0x302F, 0x7F},
    {0x3823, 0x30},
    {0x0100, 0x00},
};

static const int regs_size = sizeof(regs) / sizeof(regs[0]);

int write_regs(CAMERA_INSTANCE camera_instance, struct reg regs[], int length){
    int status = 0;
    for(int i = 0; i < length; i++){
        if (arducam_write_sensor_reg(camera_instance, regs[i].address, regs[i].value)) {
            LOG("Failed to write register: 0x%02X, 0x%02X.", regs[i].address, regs[i].value);
            status += 1;
        }
    }
    return status;
}

int main(int argc, char **argv) {
    
    clock_gettime(CLOCK_MONOTONIC, &last_hypervisor_update);
    hypervisor_packet.data.state1 = 0;
    hypervisor_packet.data.id1 = 5;

    printf("%s", argv[0]);
    //UDP Socket
    int addrlen = sizeof(addr_con);
    addr_con.sin_family = AF_INET;
    addr_con.sin_port = htons(port);
    addr_con.sin_addr.s_addr = inet_addr(ip_address);
    transmit_socket = socket(AF_INET, SOCK_DGRAM,
                IP_PROTOCOL);
    if (transmit_socket < 0)
      printf("\nCould not open the transmit socket!!\n");
    

    signal(SIGINT, intHandler);
    openCsv(file_name);
    CAMERA_INSTANCE camera_instance;
    int count = 0;
    int width = 0, height = 0;

    LOG("Open camera...");
    int res = arducam_init_camera(&camera_instance);
    if (res) {
        LOG("init camera status = %d", res);
        return -1;
    }

    width = 1280;
    height = 720;
    LOG("Setting the resolution...");
    res = arducam_set_resolution(camera_instance, &width, &height);
    if (res) {
        LOG("set resolution status = %d", res);
        return -1;
    } else {
        LOG("Current resolution is %dx%d", width, height);
        LOG("Notice:You can use the list_format sample program to see the resolution and control supported by the camera.");
    }

#if defined(ENABLE_PREVIEW)
    LOG("Start preview...");
    PREVIEW_PARAMS preview_params = {
        .fullscreen = 0,             // 0 is use previewRect, non-zero to use full screen
        .opacity = 255,              // Opacity of window - 0 = transparent, 255 = opaque
        .window = {0, 0, 1280, 720}, // Destination rectangle for the preview window.
    };
    res = arducam_start_preview(camera_instance, &preview_params);
    if (res) {
        LOG("start preview status = %d", res);
        return -1;
    }
#endif

#if defined(SOFTWARE_AE_AWB)
    LOG("Enable Software Auto Exposure...");
    arducam_software_auto_exposure(camera_instance, 1);
    LOG("Enable Software Auto White Balance...");
    arducam_software_auto_white_balance(camera_instance, 1);
#endif
    //write_regs(camera_instance, regs, regs_size);
    arducam_set_mode(camera_instance, 13);
   if (arducam_set_control(camera_instance, V4L2_CID_EXPOSURE, (int)(3*0xFFFF/200.0))) {
    LOG("Failed to set exposure, the camera may not support this control.");
    }
   if (arducam_set_control(camera_instance, V4L2_CID_GAIN, (int)(4))) {
    LOG("Failed to set exposure, the camera may not support this control.");
    }


    fd = fopen("data_tmp_dir/video.h264", "wb");
    VIDEO_ENCODER_STATE video_state;
    default_status(&video_state);
    // start video callback
    // Set video_state to NULL, using default parameters
    LOG("Start video encoding...");
    res = arducam_set_video_callback(camera_instance, &video_state, video_callback, NULL);
    if (res) {
        LOG("Failed to start video encoding, probably due to resolution greater than 1920x1080 or video_state setting error.");
        return -1;
    }

//    struct timespec start, end;
//    clock_gettime(CLOCK_REALTIME, &start);
//    usleep(1000 * 1000 * 10);
//    clock_gettime(CLOCK_REALTIME, &end);

//    double timeElapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
//    LOG("Total frame count = %d", frame_count);
//    LOG("TimeElapsed = %f", timeElapsed);
    while(running)
        sleep(0.1);
    // stop video callback
    LOG("Stop video encoding...");
    arducam_set_video_callback(camera_instance, NULL, NULL, NULL);
    fclose(fd);
    closeCsv();

    LOG("Close camera...");
    res = arducam_close_camera(camera_instance);
    if (res) {
        LOG("close camera status = %d", res);
    }
    return 0;
}
