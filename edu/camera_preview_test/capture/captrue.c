
/* Standard Include Files */
#include <errno.h>
    
/* Verification Test Environment Include Files */
#include <sys/types.h>	
#include <sys/stat.h>	
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>    
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <asm/types.h>
#include <linux/videodev.h>
#include <sys/mman.h>
#include <string.h>
#include <malloc.h>



struct testbuffer
{
        unsigned char *start;
        size_t offset;
        unsigned int length;
};

struct testbuffer buffers;
int g_width = 640;
int g_height = 480;
int g_capture_count = 1;
int g_rotate = 0;
int g_cap_fmt = V4L2_PIX_FMT_MJPEG;

int start_capturing(int fd_v4l)
{
        unsigned int i=0;
        struct v4l2_buffer buf;
        enum v4l2_buf_type type;

        memset(&buf, 0, sizeof (buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (ioctl(fd_v4l, VIDIOC_QUERYBUF, &buf) < 0)
        {
           	printf("VIDIOC_QUERYBUF error\n");
           	return -1;
        }

        buffers.length = buf.length;
        buffers.offset = (size_t) buf.m.offset;
        buffers.start = mmap (NULL, buffers.length,PROT_READ | PROT_WRITE, MAP_SHARED, fd_v4l, buffers.offset);
        printf("buffers.length = %d,buffers.offset = %d ,buffers.start[0] = %d\n",buffers.length,buffers.offset,buffers.start[0]);
	if (ioctl (fd_v4l, VIDIOC_QBUF, &buf) < 0) 
	{
            	printf("VIDIOC_QBUF error\n");
            	return -1;
        }

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;


        if (ioctl (fd_v4l, VIDIOC_STREAMON, &type) < 0) 
	{
                printf("VIDIOC_STREAMON error\n");
                return -1;
        }
        return 0;
}


int v4l_capture_setup(void)
{
        char * v4l_device = "/dev/video0";
        struct v4l2_format fmt;
        struct v4l2_control ctrl;
        int fd_v4l = 0;
	struct v4l2_crop crop;
        if ((fd_v4l = open(v4l_device, O_RDWR, 0)) < 0)
        {
                printf("Unable to open %s\n", v4l_device);
                return 0;
        }

        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
        //fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_JPEG;
	//fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MPEG;
	//fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;
	fmt.fmt.pix.width = g_width;
        fmt.fmt.pix.height = g_height;
	//printf ("fmt.fmt.pix.sizeimage =%d\n",fmt.fmt.pix.sizeimage);
        if (ioctl(fd_v4l, VIDIOC_S_FMT, &fmt) < 0)
        {
                printf("set format failed\n");
                return 0;
        } 
	//printf ("fmt.fmt.pix.sizeimage =%d\n",fmt.fmt.pix.sizeimage);
	ctrl.id = V4L2_CID_HUE + 0;
        //ctrl.id = V4L2_CID_BRIGHTNESS + 0;
	ctrl.value = V4L2_CTRL_TYPE_INTEGER + 0;
	//ctrl.value = g_rotate;
        if (ioctl(fd_v4l, VIDIOC_S_CTRL, &ctrl) < 0)
        {
                printf("set ctrl failed\n");
                return 0;
        }

        struct v4l2_requestbuffers req;
        memset(&req, 0, sizeof (req));
        req.count = 1;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;

        if (ioctl(fd_v4l, VIDIOC_REQBUFS, &req) < 0)
        {
                printf("v4l_capture_setup: VIDIOC_REQBUFS failed\n");
                return 0;
        }
	//printf ("capture_setup_return_fd_v4l = %d \n",fd_v4l);
        return fd_v4l;
}



int v4l_capture_test(int fd_v4l, const char * file)
{
	int i;
        struct v4l2_buffer buf;
        struct v4l2_buffer temp_buf;
        struct v4l2_format fmt;
        FILE * fd_y_file = 0;
        int count = g_capture_count;//=1
   	
        if ((fd_y_file = fopen(file, "wb")) < 0)
        {
                printf("Unable to create y frame recording file\n");
                return -1;
        }
	//printf ("fopen_return_fd_y_file = %d \n",fd_y_file);
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
        if (ioctl(fd_v4l, VIDIOC_G_FMT, &fmt) < 0)
        {
                printf("get format failed\n");
                return -1;
        } 
        else
        {
                printf("Width = %d\n", fmt.fmt.pix.width);
                printf("Height = %d\n", fmt.fmt.pix.height);
        //printf("Image size = %d\n", imagesize);
		printf("Image size = %d\n", fmt.fmt.pix.sizeimage);               
		printf("pixelformat = %d\n", fmt.fmt.pix.pixelformat);
        }

        if (start_capturing(fd_v4l) < 0)
        {
                printf("start_capturing failed\n");
                return -1;
        } 
    
        memset(&buf, 0, sizeof (buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if (ioctl (fd_v4l, VIDIOC_DQBUF, &buf) < 0)printf("VIDIOC_DQBUF failed.\n");
             
	//printf ("fwrite start \n");
	//printf ("size = %d \n",fmt.fmt.pix.sizeimage);
	
	unsigned char *ptcur = buffers.start;	
//	printf ("buf.bytesused = %d \n",buf.bytesused);
	int i1;
	for(i1=0; i1<buf.bytesused; i1++)
        {
        if((buffers.start[i1] == 0x000000FF) && (buffers.start[i1+1] == 0x000000C4)) {
		printf("huffman table finded! \nbuf.bytesused = %d\nFFC4 = %d \n",buf.bytesused,i1);
		break;      
		 } 
	}
	if(i1 == buf.bytesused)printf("huffman table don't exist! \n");		
	for(i=0; i<buf.bytesused; i++)
        {
	if((buffers.start[i] == 0x000000FF) && (buffers.start[i+1] == 0x000000D8)) break;		
	ptcur++;
	}
	printf("i = %d,FF = %02x,D8 = %02x\n",i,buffers.start[i],buffers.start[i+1]);
	int imagesize =buf.bytesused - i;
//	int imagesize = fmt.fmt.pix.sizeimage;
//	printf ("buf.bytesused = %d \n",buf.bytesused);
	printf ("imagesize = %d \n",imagesize);
	if(imagesize == 0)return -1;
	fwrite(ptcur, imagesize, 1, fd_y_file); 
	printf ("fwrite end \n");

        fclose(fd_y_file);
        close(fd_v4l);
        return 0;
}

int main(int argc, char **argv)
{
        int fd_v4l,ret;
	printf("li-------------------------0\n");
        fd_v4l = v4l_capture_setup();
	printf("li-------------------1\n");
	//if(*argv[argc-1]==NULL)printf("请输入文件名\n");
       // else 
	//printf();
	if(argc == 2)
	{
		ret = v4l_capture_test(fd_v4l, argv[argc-1]);
		if(ret < 0)printf("capture error! \n");
	}
	else
	{
		printf("ly:para error\n");	
	}
	//if(ret < 0)printf("capture error! \n");
	return 0;
}


