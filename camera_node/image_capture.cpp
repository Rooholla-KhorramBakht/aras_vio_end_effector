#include "arducam_mipicamera.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <linux/v4l2-controls.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>

#define CSV_PATH "/dev/shm/camera_stamps.csv"
#define VID_PATH "/dev/shm/vid.avi"
#define DS_RATIO 2
#define RECORD_DURATION  90

#define LOG(fmt, args...) fprintf(stderr, fmt "\n", ##args)
#define N_FRAMES  (50/DS_RATIO)*RECORD_DURATION

using namespace cv;
using namespace std;

VideoWriter outputVideo; 

uint8_t image_buffer[1280*720];
cv::Mat *image; 
volatile bool new_frame_flag = false;
FILE *fd;
FILE *csv_fp;
int frame_count = 0;

void writeCsv(struct timeval stamp){
    static int index = 0;
    fprintf(csv_fp,"\n%lu,%llu,%llu",index, (unsigned long long)stamp.tv_sec,(unsigned long long)stamp.tv_usec);
    index ++;
//    printf("\n%lu,%llu,%llu",index, (unsigned long long)stamp.tv_sec,(unsigned long long)stamp.tv_usec);
}

int raw_callback(BUFFER *buffer) {
    // This buffer will be automatically released after the callback function ends.
    // If you do a time-consuming operation here, it will affect other parts,
    // such as preview, video encoding. That's why we're not saving anything here.
    if (buffer->length) {
        if (TIME_UNKNOWN == buffer->pts) {
            // Frame data in the second half
        }
        // LOG("counter = %d, buffer length = %d, pts = %llu, flags = 0x%X",frame_count, buffer->length, buffer->pts, buffer->flags);
        frame_count++;
        // The camera takes images at a rate of 50 Hz but we record them every n frames.
        if (frame_count%DS_RATIO ==0)
        {
            struct timeval stamp;
            //clock_gettime(CLOCK_MONOTONIC, &stamp);
            gettimeofday(&stamp,NULL);
            writeCsv(stamp);
            new_frame_flag = true;
            //Put the frame into a seperate buffer to save for later.
            memcpy(image_buffer, (uint8_t *) buffer->data, buffer->length);
        }
    }
    return 0;
}

void open_video_file(char *path)
{
    int ex = VideoWriter::fourcc('H', '2', '6', '5');
    outputVideo.open(path, ex, 50/DS_RATIO, Size((int)1280, (int)720), false);
    if (!outputVideo.isOpened())
    {
        cout  << "Could not open the output video for write: " << endl;
    }
}

void close_video_name()
{
    outputVideo.release();
}

void save_frame()
{
    outputVideo.write(*image);
    new_frame_flag = false;
}

void save_raw(int n)
{
    char file_name[128];
    sprintf(file_name, "/dev/shm/%d.raw", n);
    // LOG("file = %s",file_name);
    FILE *file = fopen(file_name, "wb");
    fwrite(image_buffer, 1280*720, 1, file);
    fclose(file);
    new_frame_flag = false;
}

void save_jpeg(int n)
{
    char file_name[128];
    sprintf(file_name, "/dev/shm/%d.h265", n);
    std::string name = std::string(file_name);
    // LOG("file = %s",file_name);
    cv::imwrite(name, *image);
    new_frame_flag = false;
}

void openCsv(char *filename)
{
    csv_fp=fopen(filename,"w+");
    fprintf(csv_fp,"index, stamp_sec, stamp_usec");
}

void closeCsv()
{
  fclose(csv_fp);
}

int main(int argc, char **argv) {
    CAMERA_INSTANCE camera_instance;
    int count = 0;
    int width = 0, height = 0;
    image = new cv::Mat(cv::Size(1280,720), CV_8UC1, image_buffer);

    openCsv(CSV_PATH);
    
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

    // start raw callback
    LOG("Start raw data callback...");
    arducam_set_mode(camera_instance, 13);

   if (arducam_set_control(camera_instance, V4L2_CID_EXPOSURE, (int)(3*0xFFFF/200.0))) {
    LOG("Failed to set exposure, the camera may not support this control.");
    }
   if (arducam_set_control(camera_instance, V4L2_CID_GAIN, (int)(4))) {
    LOG("Failed to set exposure, the camera may not support this control.");
    }

    res = arducam_set_raw_callback(camera_instance, raw_callback, NULL);
    if (res) {
        LOG("Failed to start raw data callback.");
        return -1;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);
    open_video_file(VID_PATH);
    for(int i = 0; i < N_FRAMES; i ++)
    {
        LOG("N = %d.", i);

        while(new_frame_flag == false)
            usleep(500);
        save_raw(i);
        // save_jpeg(i);
        // save_frame();
    }
    clock_gettime(CLOCK_REALTIME, &end);
    close_video_name();
    closeCsv();
    double timeElapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
    LOG("Total frame count = %d", N_FRAMES);
    LOG("TimeElapsed = %f", timeElapsed);
    // stop raw data callback
    LOG("Stop raw data callback...");
    arducam_set_raw_callback(camera_instance, NULL, NULL);

    LOG("Close camera...");
    res = arducam_close_camera(camera_instance);
    if (res) {
        LOG("close camera status = %d", res);
    }
    return 0;
}