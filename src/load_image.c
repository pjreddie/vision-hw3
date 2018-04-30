// You probably don't want to edit this file
#include <stdio.h>
#include <stdlib.h>

#include "image.h"

image make_empty_image(int w, int h, int c)
{
    image out;
    out.data = 0;
    out.h = h;
    out.w = w;
    out.c = c;
    return out;
}

image make_image(int w, int h, int c)
{
    image out = make_empty_image(w,h,c);
    out.data = calloc(h*w*c, sizeof(float));
    return out;
}

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void save_image_stb(image im, const char *name, int png)
{
    char buff[256];
    unsigned char *data = calloc(im.w*im.h*im.c, sizeof(char));
    int i,k;
    for(k = 0; k < im.c; ++k){
        for(i = 0; i < im.w*im.h; ++i){
            data[i*im.c+k] = (unsigned char) roundf((255*im.data[i + k*im.w*im.h]));
        }
    }
    int success = 0;
    if(png){
        sprintf(buff, "%s.png", name);
        success = stbi_write_png(buff, im.w, im.h, im.c, data, im.w*im.c);
    } else {
        sprintf(buff, "%s.jpg", name);
        success = stbi_write_jpg(buff, im.w, im.h, im.c, data, 100);
    }
    free(data);
    if(!success) fprintf(stderr, "Failed to write image %s\n", buff);
}

void save_png(image im, const char *name)
{
    save_image_stb(im, name, 1);
}

void save_image(image im, const char *name)
{
    save_image_stb(im, name, 0);
}

// 
// Load an image using stb
// channels = [0..4]
// channels > 0 forces the image to have that many channels
//
image load_image_stb(char *filename, int channels)
{
    int w, h, c;
    unsigned char *data = stbi_load(filename, &w, &h, &c, channels);
    if (!data) {
        fprintf(stderr, "Cannot load image \"%s\"\nSTB Reason: %s\n",
            filename, stbi_failure_reason());
        exit(0);
    }
    if (channels) c = channels;
    int i,j,k;
    image im = make_image(w, h, c);
    for(k = 0; k < c; ++k){
        for(j = 0; j < h; ++j){
            for(i = 0; i < w; ++i){
                int dst_index = i + w*j + w*h*k;
                int src_index = k + c*i + c*w*j;
                im.data[dst_index] = (float)data[src_index]/255.;
            }
        }
    }
    //We don't like alpha channels, #YOLO
    if(im.c == 4) im.c = 3;
    free(data);
    return im;
}

image load_image(char *filename)
{
    image out = load_image_stb(filename, 0);
    return out;
}

void free_image(image im)
{
    free(im.data);
}

#ifdef OPENCV

void rgbgr_image(image im)
{
    int i;
    for(i = 0; i < im.w*im.h; ++i){
        float swap = im.data[i];
        im.data[i] = im.data[i+im.w*im.h*2];
        im.data[i+im.w*im.h*2] = swap;
    }
}

void ipl_into_image(IplImage* src, image im)
{
    unsigned char *data = (unsigned char *)src->imageData;
    int h = src->height;
    int w = src->width;
    int c = src->nChannels;
    int step = src->widthStep;
    int i, j, k;

    for(i = 0; i < h; ++i){
        for(k= 0; k < c; ++k){
            for(j = 0; j < w; ++j){
                im.data[k*w*h + i*w + j] = data[i*step + j*c + k]/255.;
            }
        }
    }
}

image ipl_to_image(IplImage* src)
{
    int h = src->height;
    int w = src->width;
    int c = src->nChannels;
    image out = make_image(w, h, c);
    ipl_into_image(src, out);
    return out;
}

image get_image_from_stream(CvCapture *cap)
{
    IplImage* src = cvQueryFrame(cap);
    if (!src) return make_empty_image(0,0,0);
    image im = ipl_to_image(src);
    rgbgr_image(im);
    return im;
}

int show_image(image im, const char *name, int ms)
{
    image p = copy_image(im);
    IplImage *disp = cvCreateImage(cvSize(p.w,p.h), IPL_DEPTH_8U, p.c);
    clamp_image(p);

    int x,y,k;
    if(p.c == 3) rgbgr_image(p);

    char buff[256];
    sprintf(buff, "%s", name);

    int step = disp->widthStep;
    cvNamedWindow(buff, CV_WINDOW_NORMAL); 
    for(y = 0; y < p.h; ++y){
        for(x = 0; x < p.w; ++x){
            for(k= 0; k < p.c; ++k){
                disp->imageData[y*step + x*p.c + k] = (unsigned char)(get_pixel(p,x,y,k)*255);
            }
        }
    }
    cvShowImage(buff, disp);
    free_image(p);
    cvReleaseImage(&disp);
    return cvWaitKey(ms);
}

#endif
