/*
 * How to test v4l2loopback:
 * 1. launch this test program (even in background), it will initialize the
 *    loopback device and keep it open so it won't loose the settings.
 * 2. Feed the video device with data according to the settings specified
 *    below: size, pixelformat, etc.
 *    For instance, you can try the default settings with this command:
 *    mencoder video.avi -ovc raw -nosound -vf scale=640:480,format=yuy2 -o /dev/video1
 *    TODO: a command that limits the fps would be better :)
 *
 * Test the video in your favourite viewer, for instance:
 *   luvcview -d /dev/video1 -f yuyv
 */

#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>

#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

#include <gd.h>

const uint32_t width = 512;
const uint32_t height = 256;
const uint32_t format = v4l2_fourcc('R', 'G', 'B', '3');

gdImagePtr img;
int bg;
int speckle;
int shadow;
int text;

char *textFont = "/home/matt/.fonts/FiraMono-Bold.ttf";
int textFontSize = 20;
char *digitFont = "/home/matt/.fonts/TickingTimebombBB.ttf";
int digitFontSize = 120;
int digitSpace = 72;
int digitDotSpace = 20;

time_t lastUpdate = 0;

char *misc;
char *value;
char *units;

int quit = 0;

void terminate(int i) {
	printf("Terminating, please wait...\n");
	quit = 1;
	exit(0);
}

time_t micros() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return 1000000 * tv.tv_sec + tv.tv_usec;
}

void shadowText(int x, int y, char *txt) {
    gdImageStringFT(img, NULL, shadow, textFont, textFontSize, 0, x + 5, y + 5, txt);
    gdImageStringFT(img, NULL, text, textFont, textFontSize, 0, x, y, txt);
}

int digitWidth(char *txt) {
    int brect[8];
    gdImageStringFT(img, brect, text, digitFont, digitFontSize, 0, 9999, 9999, txt);
    return brect[2] - brect[0];
}

void shadowDigit(int x, int y, char *txt) {
    int p = x;
    int i;
    for (i = 0; i < strlen(txt); i++) {
        char d[2] = {txt[i], 0};
        int w = digitWidth(d);
        if (d[0] == '.') {
            p += digitDotSpace;
        } else {
            p += digitSpace;
        }
        gdImageStringFT(img, NULL, shadow, digitFont, digitFontSize, 0, p + 5 - w, y + 5, d);
    }
    p = x;
    for (i = 0; i < strlen(txt); i++) {
        char d[2] = {txt[i], 0};
        int w = digitWidth(d);
        if (d[0] == '.') {
            p += digitDotSpace;
        } else {
            p += digitSpace;
        }
        gdImageStringFT(img, NULL, text, digitFont, digitFontSize, 0, p - w, y, d);
    }
}

void updateImage() {

    int x, y;
    time_t now = time(NULL);

    // emulate offline
    gdImageFilledRectangle(img, 0, 0, width, height, bg);

	srand(49803785);

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            if (rand() % 100 < 10) {
                gdImageSetPixel(img, x, y, speckle);
            }
        }
    }

    if ((now - lastUpdate) >= 2) { // 2 second timeout) {
        shadowText(300, 50, "DMM Offline");
    } else {

		if (units != NULL) {
				shadowText(50, 50, units);
		}

        if (!strcmp(value, "inf")) {
            shadowDigit(20, 180, " OL.");
        } else {
            double val = strtod(value, NULL);
            char out[30];
            if (val >= 0) {
                if (abs(val) < 10) {
                    sprintf(out, " %6.4f", val);
                } else if (abs(val) < 100) {
                    sprintf(out, " %6.3f", val);
                } else if (abs(val) < 1000) {
                    sprintf(out, " %6.2f", val);
                } else if (abs(val) < 10000) {
                    sprintf(out, " %6.1f", val);
                } else {
                    sprintf(out, " %6d", (int)val);
                }
            } else {
                if (abs(val) < 10) {
                    sprintf(out, "%6.4f", val);
                } else if (abs(val) < 100) {
                    sprintf(out, "%6.3f", val);
                } else if (abs(val) < 1000) {
                    sprintf(out, "%6.2f", val);
                } else if (abs(val) < 10000) {
                    sprintf(out, "%6.1f", val);
                } else {
                    sprintf(out, "%6d", (int)val);
                }
            }
            shadowDigit(20, 180, out);
        }
    }
}

void loadFrame(uint8_t *buffer) {
    int y = 0;
    int x = 0;

    int p = 0;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            uint32_t pix = gdImageTrueColorPixel(img, x, y);
            buffer[p++] = (pix >> 16);
            buffer[p++] = (pix >> 8);
            buffer[p++] = (pix >> 0);
        }
    }
}

int main(int argc, char**argv)
{
	struct v4l2_capability vid_caps;
	struct v4l2_format vid_format;

	signal(SIGQUIT, terminate);
	signal(SIGINT, terminate);

	int opt;

  	char *video_device = "/dev/video104";
	int fdwr = 0;

	while ((opt = getopt(argc, argv, "d:")) != -1) {
		switch (opt) {
			case 'd':
				video_device = strdup(optarg);
				break;
			default:
				printf("Usage: %s [-d device]\n",
					argv[0]);
				exit(10);
		}
	}

	int ret_code = 0;

	fdwr = open(video_device, O_RDWR);
	if (fdwr < 0) {
		printf("Unable to open %s\n", video_device);
		exit(10);
	}	

	ret_code = ioctl(fdwr, VIDIOC_QUERYCAP, &vid_caps);

	if (ret_code < 0) {
		printf("Error querying caps: %s\n", strerror(errno));
		exit(10);
	}

	memset(&vid_format, 0, sizeof(vid_format));

	vid_format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	vid_format.fmt.pix.width = width;
	vid_format.fmt.pix.height = height;
    vid_format.fmt.pix.sizeimage = 512 * 256 * 3;
	vid_format.fmt.pix.pixelformat = format;
	vid_format.fmt.pix.field = V4L2_FIELD_NONE;
    vid_format.fmt.pix.bytesperline = 512 * 3;
	vid_format.fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;

	ret_code = ioctl(fdwr, VIDIOC_S_FMT, &vid_format);


    uint8_t *buffer = malloc(vid_format.fmt.pix.sizeimage);

    if (!buffer) {
        printf("Unable to allocate %d bytes.\n", vid_format.fmt.pix.sizeimage);
        exit(10);
    } 

    img = gdImageCreateTrueColor(width, height);
    bg = gdImageColorAllocate(img, 180, 200, 200);
//    bg = gdImageColorAllocate(img, 0, 0, 0);
    speckle = gdImageColorAllocate(img, 140, 190, 190);
    shadow = gdImageColorAllocate(img, 150, 160, 160);
    text = gdImageColorAllocate(img, 50, 50, 50);

    updateImage();

    time_t ts = micros();


    FILE *frd = popen("sigrok-cli --driver=uni-t-ut61e:conn=1a86.e008 --continuous 2>&1", "r");

    int c = 0;

    char line[100];
    int cpos = 0;

    while (quit == 0) {

        if (feof(frd)) {
            pclose(frd);
            usleep(10);
            frd = popen("sigrok-cli --driver=uni-t-ut61e:conn=1a86.e008 --continuous 2>&1", "r");
        } else {
            int fd = fileno(frd);
            fd_set rfds;
            struct timeval tv;
            int retval;

            FD_ZERO(&rfds);
            FD_SET(fd, &rfds);

            tv.tv_sec = 0;
            tv.tv_usec = 10;

            retval = select(fd+1, &rfds, NULL, NULL, &tv);

            if (retval) {
                char ch = fgetc(frd);
                if (ch == '\n') {
                    lastUpdate = time(NULL);

                    misc = strtok(line, " ");
                    value = strtok(NULL, " ");
                    units = strtok(NULL, "\n");

                    cpos = 0;
                    line[0] = 0;
                } else {
                    line[cpos++] = ch;
                    line[cpos] = 0;
                }
            }
        }

        if (micros() - ts >= 33300) {
            ts = micros();
            loadFrame(buffer);
            write(fdwr, buffer, vid_format.fmt.pix.sizeimage);
            updateImage();
        }

    }

	pclose(frd);

	close(fdwr);

    pause();

	return 0;
}
