#include <stdio.h>
#include <gd.h>
#include <tenma.h>

#include <linux/videodev2.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>


const int width = 400;
const int height = 240;
const uint32_t format = v4l2_fourcc('R', 'G', 'B', '3');

int main(int argc, char **argv) {

    struct v4l2_capability vid_caps;
    struct v4l2_format vid_format;

    int opt;

    char *video_device = "/dev/video103";
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

    fdwr = open(video_device, O_RDWR);
    if (fdwr < 0) {
        printf("Unable to open %s\n", video_device);
        exit(10);
    }

    int ret_code = 0;
    ret_code = ioctl(fdwr, VIDIOC_QUERYCAP, &vid_caps);

    if (ret_code < 0) {
        printf("Error querying caps: %s\n", strerror(errno));
        exit(10);
    }

    memset(&vid_format, 0, sizeof(vid_format));

    vid_format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    vid_format.fmt.pix.width = width * 2;
    vid_format.fmt.pix.height = height * 2;
    vid_format.fmt.pix.sizeimage = ((width * 2) * (height * 2)) * 3;
    vid_format.fmt.pix.pixelformat = format;
    vid_format.fmt.pix.field = V4L2_FIELD_NONE;
    vid_format.fmt.pix.bytesperline = width * 2 * 3;
    vid_format.fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;

    ret_code = ioctl(fdwr, VIDIOC_S_FMT, &vid_format);

    uint8_t *buffer = malloc(vid_format.fmt.pix.sizeimage);

    if (!buffer) {
        printf("Unable to allocate %d bytes.\n", vid_format.fmt.pix.sizeimage);
        exit(10);
    }

	printf("Buffer size: %d bytes. Expected: %d bytes\n", vid_format.fmt.pix.sizeimage, width * 2 * height * 2 * 3);

	int opened = 0;
	int fails = 0;

	while (1) {
		if (opened == 0) {
			if (dso_728705_open() < 0) {
    		    fprintf(stderr, "Unable to open DSO\n");
				sleep(1);
				continue;
    		}
			opened = 1;
			fails = 0;
		} 


	    gdImagePtr img = dso_728705_getFrame();

	    if (!img) {
			fails++;
			printf("Frame failure\n");
			sleep(1);
			if (fails > 10) {
				dso_728705_close();
				opened = 0;
			}
			continue;
    	}

		fails = 0;

		gdImagePtr img2 = gdImageCreate(gdImageSX(img) * 2, gdImageSY(img) * 2);
    
    	gdImageColorAllocate(img2, 0, 0, 0);
    	gdImageColorAllocate(img2, 0x2b, 0x2b, 0x2b);
    	gdImageColorAllocate(img2, 0, 0, 0); // Not used?
    	gdImageColorAllocate(img2, 0x60, 0x60, 0x60);
    	gdImageColorAllocate(img2, 0, 0, 0); // Not used?
    	gdImageColorAllocate(img2, 0, 0, 0); // Not used?
    	gdImageColorAllocate(img2, 20, 130, 218);
    	gdImageColorAllocate(img2, 0, 0, 0); // Not used?
    	gdImageColorAllocate(img2, 0, 0, 0); // Not used?
    	gdImageColorAllocate(img2, 0, 0, 0); // Not used?
    	gdImageColorAllocate(img2, 0, 0, 0); // Not used?
    	gdImageColorAllocate(img2, 0x40, 0x40, 0x40);
    	gdImageColorAllocate(img2, 0, 200, 80);
    	gdImageColorAllocate(img2, 200, 170, 0);
    	gdImageColorAllocate(img2, 200, 0, 100);
    	gdImageColorAllocate(img2, 255, 255, 255);

    	int y = 0;
    	int x = 0;

		for (y = 0; y < height; y++) {
        	for (x = 0; x < width; x++) {
				gdImageSetPixel(img2, x * 2, y * 2, gdImagePalettePixel(img, x, y));
				gdImageSetPixel(img2, x * 2 + 1, y * 2, gdImagePalettePixel(img, x, y));
				gdImageSetPixel(img2, x * 2, y * 2 + 1, gdImagePalettePixel(img, x, y));
				gdImageSetPixel(img2, x * 2 + 1, y * 2 + 1, gdImagePalettePixel(img, x, y));
			}
		}
	
	    int p = 0;

    	for (y = 0; y < height * 2; y++) {
        	for (x = 0; x < width * 2; x++) {
            	uint32_t pix = gdImagePalettePixel(img2, x, y);
            	buffer[p++] = gdImageRed(img2, pix);
            	buffer[p++] = gdImageGreen(img2, pix);
            	buffer[p++] = gdImageBlue(img2, pix);
        	}
    	}
        write(fdwr, buffer, vid_format.fmt.pix.sizeimage);
		
	}

    dso_728705_close();
	return 0;
}
