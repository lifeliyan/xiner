
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <fcntl.h>		/* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>		/* for videodev2.h */

#include <linux/videodev2.h>
#include <linux/fb.h>

#define LOG_TAG "CAMERA_TEST"
#include <utils/Log.h>

#define SAVE_TO_FILE
#define PIC_SET_FILE "/sdcard/set0.yuv"

//#define UVC_CAMERA
/*
#ifdef UVC_CAMERA
#define PREVIEW_DEV  "/dev/video1"
#else
#define PREVIEW_DEV  "/dev/video2"
#endif
*/
#define PREVIEW_DEV  "/dev/video0"

#define FRAMEBUFER_DEV  "/dev/graphics/fb0"

#define IMAGE_WIDTH 640
#define IMAGE_HEIGHT 480

#define LCD_WIDTH 480
#define LCD_HEIGHT 272

#define IMAGE_SIZE_YUV420 ((IMAGE_WIDTH*IMAGE_HEIGHT*3)/2)  //460800
#define IMAGE_SIZE_YUV422 (IMAGE_WIDTH*IMAGE_HEIGHT*2)  //61440
#define IMAGE_IN_FB_SIZE_RGB888  (LCD_WIDTH*LCD_HEIGHT*3)
#define IMAGE_IN_FB_SIZE_RGB565  (LCD_WIDTH*LCD_HEIGHT*2)

#define USE_BUF_NUM 4

static unsigned int buf_index = 0;
static unsigned int max_buffers = 4;
unsigned char *fb_buf = NULL;
unsigned char *data_buf = NULL ;
static int pic_set_fd;

#define APP_ANDROID_DEBUG
#ifdef APP_ANDROID_DEBUG
#define MYLOGD(format, args...)   do{LOGD("%d:%s()", __LINE__, __FUNCTION__); LOGD(format, ##args);}while(0)
#else
#define MYLOGD(format, args...)
#endif

struct buffer
{
  void *start;
  size_t length;
}user_buffers[4];


static int open_device(char *dev_name)
{
	assert(dev_name);

	int fd = -1;

	fd = open(dev_name , O_RDWR);
	if( -1 == fd )
	{
		printf("open %s fail: %s\n", dev_name, strerror (errno));
        exit(EXIT_FAILURE);
	}
	
	printf("the fd of %s is %d ", dev_name, fd);
	return fd;
	
}
static int fimc_v4l2_querycap(int fp)
{
	struct v4l2_capability cap;
	int ret = 0;

	ret = ioctl(fp, VIDIOC_QUERYCAP, &cap);

	if (ret < 0) {
		printf("ERR(%s):VIDIOC_QUERYCAP failed\n", __FUNCTION__);
		return -1;
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		printf("ERR(%s):no capture devices\n", __FUNCTION__);
		return -1;
	}

	return ret;
}

static int cam_v4l2_enuminput(int fp)
{
        struct v4l2_input input;

        input.index = 0; 
        if(ioctl(fp, VIDIOC_ENUMINPUT, &input) != 0)
		{
			printf("ERR(%s):No matching index found\n", __FUNCTION__);
			return -1;
		}
		printf("Name of input channel[%d] is %s\n", input.index, input.name);

     	return input.index;
}

static int cam_v4l2_s_input(int fp, int index)
{
	struct v4l2_input input;
	int ret;

	input.index = index;

	ret = ioctl(fp, VIDIOC_S_INPUT, &input);
	if (ret < 0) {
		printf("ERR(%s):VIDIOC_S_INPUT failed\n", __FUNCTION__);
		return ret;
	}

	return ret;
}

static int get_pix_depth(unsigned int pix_fmt)
{
	int depth = 0;
	
	switch(pix_fmt)
	{
		case V4L2_PIX_FMT_YUV420:
			depth = 12;
			break;
		case V4L2_PIX_FMT_YUYV:
			depth = 16;
			break;
		default:
			printf("wrong pix format");
			exit(1);	
	}

	return depth;
}
static int cam_v4l2_s_format( int dev_fd, int height, int width, unsigned int fmt)
{
	int ret = -1;
	struct v4l2_format my_format;

	bzero(&my_format, sizeof(my_format));

	my_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	my_format.fmt.pix.height = height;
	my_format.fmt.pix.width = width;
	my_format.fmt.pix.pixelformat = fmt; 
	my_format.fmt.pix.sizeimage = (height * width * get_pix_depth(V4L2_PIX_FMT_YUYV))/8;
	//my_format.fmt.pix.field = V4L2_FIELD_INTERLACED; // if jpeg
	//my_format.fmt.pix.bytesperline = 640 * 2 ;  // driver will calc
	//my_format.fmt.pix.priv = 0;   // can be zero
	
	ret = ioctl(dev_fd, VIDIOC_S_FMT, &my_format);
	if(-1 == ret )
	{
		printf("set format error\n");
		exit(1) ;
	}

	return ret;
}

static int cam_v4l2_reqbuf(int dev_fd, int num)
{
	int ret = -1;
	struct v4l2_requestbuffers reqbuf;

	bzero(&reqbuf, sizeof(reqbuf));
	
	reqbuf.count = num;
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;// must be this

	ret = ioctl(dev_fd, VIDIOC_REQBUFS, &reqbuf);//to allocate buffer with dma in kernel space
	if( -1 == ret )
	{
		printf("VIDIOC_REQBUFS  fail: %s\n", strerror (errno));
        exit(EXIT_FAILURE);
	}

	return ret; 
	
}

static int cam_v4l2_querybuf(int dev_fd, struct v4l2_buffer *buf, int num)
{
	int ret = -1;
	assert(buf != NULL);
	buf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf->index = num;

	ret = ioctl(dev_fd, VIDIOC_QUERYBUF, buf);
	if(-1 == ret)
	{
		printf("VIDIOC_QUERYBUF error\n");
		exit(EXIT_FAILURE);
	}

	return ret;
}

static void init_cam_mmap(int dev_fd)
{
	printf("excuting...");
	struct v4l2_buffer buf;	
	int ret = -1;

	printf("VIDIOC_REQBUFS start ...,we need to do first");
	cam_v4l2_reqbuf(dev_fd, USE_BUF_NUM);
	
	printf("mmap start...");
	for(buf_index=0 ; buf_index < USE_BUF_NUM; buf_index++)
	{
		bzero(&buf, sizeof(struct v4l2_buffer));
		cam_v4l2_querybuf(dev_fd, &buf, buf_index);
		
		user_buffers[buf_index].length = buf.length;
		user_buffers[buf_index].start = mmap(NULL /* start anywhere */ ,
				       buf.length,
				       PROT_READ | PROT_WRITE /* required */ ,
				       MAP_SHARED /* recommended */ ,
				       dev_fd, buf.m.offset);
		if (MAP_FAILED == user_buffers[buf_index].start)
		{
		printf("mmap buffers  fail: %s\n", strerror (errno));
        	exit(EXIT_FAILURE);
		}
	}	


	printf("init_cam_mmap done\n");
}


static int cam_v4l2_g_fmt(int dev_fd)
{
	struct v4l2_format my_fmt;
	int ret = -1;
	my_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	ret = ioctl(dev_fd, VIDIOC_G_FMT, &my_fmt);
    if ( -1 == ret )
	{
	printf("VIDIOC_G_FMT error \n");
        	return -EINVAL;
    }else 
    {
        printf(" Width = %d\n", my_fmt.fmt.pix.width);
        printf(" Height = %d \n", my_fmt.fmt.pix.height);
        printf(" Image size = %d\n", my_fmt.fmt.pix.sizeimage);
    }	

	return ret;
}

static int init_cam_device(int dev_fd)
{
	int ret = -1;
	int input_index;

	//ret = fimc_v4l2_querycap(dev_fd);
	//assert(ret == 0);
	//获取到输入源通道
	input_index = cam_v4l2_enuminput(dev_fd);	
	assert(input_index == 0);
	
	ret = cam_v4l2_s_input(dev_fd, input_index);
	assert(ret == 0);

	printf("VIDIOC_S_FMT start... dev_fd = %d\n", dev_fd);

	ret = cam_v4l2_s_format(dev_fd, IMAGE_HEIGHT, IMAGE_WIDTH, V4L2_PIX_FMT_YUYV);
	assert(ret == 0);

	ret = cam_v4l2_g_fmt(dev_fd);
	assert(ret == 0);
		
	init_cam_mmap(dev_fd);
	
	return 1;	
	
}
static int init_fb_device(int dev_fd)
{	
	printf("After openning FB \n");
	int screensize;
	static struct fb_var_screeninfo vinfo;
	static struct fb_fix_screeninfo finfo;
	/*2.获取固定参数*/
	if(ioctl(dev_fd,FBIOGET_FSCREENINFO,&finfo))
	{
		printf("Error:failed get the framebuffer device`s fix informations!\n");
		return -1;
	}

	/*3.获取可变参数*/
	if(ioctl(dev_fd,FBIOGET_VSCREENINFO,&vinfo))
	{
		printf("Error:failed get the framebuffer device`s var informations!\n");
		return -1;
	}

	screensize =(vinfo.xres *vinfo.yres*vinfo.bits_per_pixel/8);
	//printf("screensize =%ld\n",screensize);

	/*5.映射*/
	fb_buf =(char *)mmap(  NULL,screensize,
							PROT_READ|PROT_WRITE,
							MAP_SHARED,
							dev_fd,
							0);
	if (MAP_FAILED == fb_buf) {
		printf("Error: failed to map framebuffer device to memory.\n");
		close(dev_fd);		
     	exit(EXIT_FAILURE);
	}
	memset(fb_buf,0,screensize);
	
	return 1;
}

static int cam_v4l2_qbuf(int dev_fd, int index)
{
	struct v4l2_buffer buf;
	int ret = -1;

	memset(&buf, 0 ,sizeof(struct v4l2_buffer));
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = index;

	ret = ioctl(dev_fd, VIDIOC_QBUF, &buf);
	if( -1 == ret )
	{
		printf ("VIDIOC_QBUF  fail: %s\n", strerror (errno));
        exit(EXIT_FAILURE);
	}else
	{
		printf("VIDIOC_QBUF OK \n");
	}

	return ret;
}
static int cam_v4l2_dqbuf(int dev_fd)
{
	struct v4l2_buffer dq_buf;
	int ret = -1;
		
	memset(&dq_buf, 0 ,sizeof(struct v4l2_buffer));
	dq_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	dq_buf.memory = V4L2_MEMORY_MMAP;
	
	ret = ioctl(dev_fd, VIDIOC_DQBUF, &dq_buf);
	if( -1 == ret )
	{
		printf("VIDIOC_DQBUF  fail: %s\n", strerror (errno));
        	exit(EXIT_FAILURE);
	}else
	{
		printf("VIDIOC_DQBUF OK in read frame\n");
	}

	return  dq_buf.index ;
}

static int cam_v4l2_streamon(int dev_fd, int on)
{
	int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	int ret = -1;
	switch(on)
	{
		case 0:
			ret = ioctl(dev_fd, VIDIOC_STREAMOFF, &type);
			if( -1 == ret )
			{
		printf("VIDIOC_STREAMOFF  fail: %s\n", strerror (errno));
        		exit(EXIT_FAILURE);
			}	
			break;
		case 1:
			ret = ioctl(dev_fd, VIDIOC_STREAMON, &type);
			if( -1 == ret )
			{
		printf("VIDIOC_STREAMON  fail: %s\n", strerror (errno));
        		exit(EXIT_FAILURE);
			}	
			break;
		default:
		printf("wrong switch\n");
			break;
	}

	return ret ;
}

static void start_capturing (int dev_fd)
{
	unsigned int i;
	int ret;

	for (i = 0; i < max_buffers; ++i)
	{
		cam_v4l2_qbuf(dev_fd, i);
	}

	cam_v4l2_streamon(dev_fd, 1);
}

void yuyv_to_rgb(unsigned char* src, unsigned char *data_buf) 
{
	int p=0,k=0;
	int x,y;
	int z=0;
	int h;
	unsigned short rgb;
	unsigned char *yuyv= src;
    for(h=0; h<LCD_HEIGHT; h++) 
	{
		 for (x = 0; x < LCD_WIDTH; x++) 
		 {
		     int r, g, b;
		     int y, u, v;

			  if (!z)
				  y = yuyv[0] << 8;
			  else
				  y = yuyv[2] << 8;
			  u = yuyv[1] - 128;
			  v = yuyv[3] - 128;

			  r = (y + (359 * v)) >> 8;
			  g = (y - (88 * u) - (183 * v)) >> 8;
			  b = (y + (454 * u)) >> 8;

			  r = (r > 255) ? 255 : ((r < 0) ? 0 : r);
			  g = (g > 255) ? 255 : ((g < 0) ? 0 : g);
			  b = (b > 255) ? 255 : ((b < 0) ? 0 : b);

			  r = r >> 3;
			  g = g >> 2;
			  b = b >> 3;
			  rgb = (r<<11)|(g<<5)|b;
			  
			  *((unsigned short *)data_buf+x+h*LCD_WIDTH)= rgb;
			//  *((unsigned short *)framebuffer_ptr+x+h*480)= rgb;
			  if (z++) 
			  {
				  z = 0;

				  if(p%4 == 3)
				   	  yuyv += 8;
			  	  else
					  yuyv += 4;
					
				  p++;
			  }
		 }					
		if((k%5 == 2)||(k%5 == 4))
			yuyv += IMAGE_WIDTH*2+40*2;
		else
			yuyv += 40*2;
		k++;
     }
}


static void show_rgb565_img(unsigned char *data_buf, int width, int height)
{	
	__u16 x, y;
	memcpy(fb_buf, data_buf, width*height*2);
#if 0
	fb_buf += (800*14);
		
		
			for(y=0; y<ROWS; y++) {
				for(x=0; x<COLS; x++) {	//calculate 2 time
						fb_buf[x+28]   = *data_buf++ ;
					}
				fb_buf += 800;
			}
#endif
}
int process_image(unsigned char *src_address)
{
	/*TODO: put the video date into framebuffer*/
	printf("excuting...");
	yuyv_to_rgb(src_address, data_buf);
	show_rgb565_img(data_buf, LCD_WIDTH, LCD_HEIGHT);	

	return 0;		
}
int read_frame(int dev_fd)
{
	printf("excuting... \n");
	struct v4l2_buffer dq_buf;
	unsigned int index;
	int count = 5;
	int ret = 0;
	
	index = cam_v4l2_dqbuf(dev_fd);

	printf("dq_buf.buf_index is %d\n", index);
	
	assert (index < max_buffers);//max_buffers is gloable value , to mark max buffer

#ifdef SAVE_TO_FILE
	int read_byte = 0;
	int pic_num = 0;
	char buff[IMAGE_SIZE_YUV422];
	 if( ((ret = write(pic_set_fd, (unsigned char *)(user_buffers[index].start), IMAGE_SIZE_YUV422) ) < IMAGE_SIZE_YUV422) || (ret < 0) )
	 	{
	 printf("write error to file \n");
	 	}else{
	 printf("write successfully: %d bytes\n", ret);
	 	}
#else
	ret = process_image((unsigned char *)(user_buffers[index].start));//the value of buf_index is ?
#endif

	cam_v4l2_qbuf(dev_fd, index);

	return ret;
}
static void mainloop(int dev_fd)
{
	//MYLOGD("excuting ... \n");
	unsigned int count = 50;
	int maxfd = -1;
	int ret = -1;

	fd_set rd_set;
	maxfd = dev_fd;

#ifdef SAVE_TO_FILE
	printf("liyan -----------a\n");
	pic_set_fd = open(PIC_SET_FILE, O_CREAT | O_RDWR | O_TRUNC,0777);
	if( -1 == pic_set_fd )
	{
	printf("liyan -----------a\n");
		printf("open %s failed : %s\n", PIC_SET_FILE, strerror(errno));		
		exit(1);
	}else{
		printf("open pic set file Ok...\n");
		}
#endif
	while(1)
	//while(count-- > 0)
	{		  
		  FD_ZERO (&rd_set);
		  FD_SET (dev_fd, &rd_set);

		  ret = select(maxfd + 1, &rd_set, NULL, NULL, NULL);
		  if (-1 == ret)
		  {
			  if (EINTR == errno)
					continue;
			 printf("select error\n");
			  exit (EXIT_FAILURE);
		  }
		 	printf("read_frame %dth \n", (10 - count));
		 if(FD_ISSET(dev_fd, &rd_set))
		 {
			  if (read_frame(dev_fd))
			  {
			printf("cannot read any frame");
			  		exit(1);
			  }
			  
		}
	}
}
int close_device(int dev_fd)
{
	close(dev_fd);
	
	return 0;
}
int main(int argc, char *argv[])
{
	printf("IO method is mmap\n");
	int ret = -1;

	int cam_p_fd;
	int fb_fd;

	data_buf =  (unsigned char *)malloc(LCD_HEIGHT * LCD_WIDTH * 2);
	assert(data_buf);
	
	cam_p_fd = open_device( PREVIEW_DEV);
	fb_fd = open_device( FRAMEBUFER_DEV);

	init_cam_device(cam_p_fd);
	init_fb_device(fb_fd);

	start_capturing(cam_p_fd);

	mainloop(cam_p_fd);

	free(data_buf);
	ret = munmap(fb_buf, IMAGE_IN_FB_SIZE_RGB565);
	if(-1 == ret)
		printf("munmap error\n");

	close_device(cam_p_fd);
	close_device(fb_fd);
	
	return 0;
}
