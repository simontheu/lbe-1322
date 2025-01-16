//#include <iostream>
//using namespace std;

/* Linux */
#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>

/* Unix */
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* C */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <getopt.h>

#include <time.h>

//Leo LBE-142x defines, dont need anything external this time
#define VID_LBE		0x1dd2

#define PID_LBE_1322	0x2265


#define GPS_LOCK_BIT 		0x01
#define RES_1_BIT 		0x02
#define ANT_OK_BIT 		0x04
#define RES_3_BIT 		0x08
#define OUT1_EN_BIT 		0x10
//SetFeatureReportIDs
#define LBE_1420_EN_OUT1	0x01
#define LBE_1420_BLINK_OUT1	0x02
#define LBE_1420_SET_F1_NO_SAVE	0x03
#define LBE_1420_SET_F1		0x04

/*
 * Ugly hack to work around failing compilation on systems that don't
 * yet populate new version of hidraw.h to userspace.
 */
#ifndef HIDIOCSFEATURE
#warning Please have your distro update the userspace kernel headers
#define HIDIOCSFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x06, len)
#define HIDIOCGFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x07, len)
#endif

#define HIDIOCGRAWNAME(len)     _IOC(_IOC_READ, 'H', 0x04, len)


int processCommandLineArguments(int argc, char **argv, int *freq, int *amplitude, int *trigger_mode,int *sync_freq, int *polarity);
	
int main(int argc, char **argv)
{
      printf("Leo Bodnar LBE-1322 Fast Pulse Generator Config\n");
      
      int fd;
      int i, res, desc_size = 0;
      u_int8_t buf[60];
      uint32_t current_f;

      struct hidraw_devinfo info;


   /* Open the Device with non-blocking reads. In real life,
      don't use a hard coded path; use libudev instead. 
   */
      if (argc == 1)
      {
	    printf("Usage: lbe-1322 /dev/hidraw??\n\n");
            printf("        --f:  integer within the range of 30000 to 300000000 (30kHz to 300MHz)\n               the output frequency in Hz\n");
            printf("        --a:  0 to 511 amplitude\n               maps to 49mv to 1624mv\n");
            printf("        --t:  [0 or 1] 0 = trigger output, 1=sync input\n");
            printf("        --p:  [0 or 1] 0 = normal, 1= inverted\n");
            printf("        --s: integer withing ther range 100000000 to 300000000 \n               sync input frequency in Hz\n");
            printf("     --sync:  sync to incoming signal\n\n\n");
            
            return -1;
      }

      printf("Opening device %s\n", argv[1]);

      fd = open(argv[1], O_RDWR|O_NONBLOCK);

      if (fd < 0) 
      {
            perror("    Unable to open device");
            return 1;
      }

      //Device connected, setup report structs
      memset(&info, 0x0, sizeof(info));

      // Get Raw Info
      res = ioctl(fd, HIDIOCGRAWINFO, &info);
      
      if (res < 0) 
      {
            perror("HIDIOCGRAWINFO");
      } 
      else
      {
            if (info.vendor != VID_LBE || info.product != PID_LBE_1322 ) {
                printf("    Not a valid LBE-1322 Device\n\n");
                  printf("    Device Info:\n");
                  printf("        vendor: 0x%04hx\n", info.vendor);
                  printf("        product: 0x%04hx\n", info.product);
                  return -1;//Device not valid
            }
      }

      /* Get Raw Name */
      res = ioctl(fd, HIDIOCGRAWNAME(256), buf);

      if (res < 0) {
            perror("HIDIOCGRAWNAME");
      }
      else {
            printf("Connected To: %s\n\n", buf);
      }

      /* Get Feature */
      buf[0] = 0x9; /* Report Number */
      res = ioctl(fd, HIDIOCGFEATURE(256), buf);
      
      if (res < 0) {
            perror("HIDIOCGFEATURE");
      } else {
	      printf("  Status:\n");
            int f_out, v_out, polarity, trigger_mode, ext_sync_f;
             
            f_out = buf[2] & 0xff;
            f_out += buf[3] << 8;
            f_out += buf[4] << 16;
            f_out += buf[5] << 24;

            v_out = buf[6] & 0xff;
            v_out += buf[7] << 8;
            
            trigger_mode = buf[12];
            
            polarity = buf[8] & 1;

            ext_sync_f = buf[14] & 0xff;
            ext_sync_f += buf[15] << 8;
            ext_sync_f += buf[16] << 16;
            ext_sync_f += buf[17] << 24;

            printf("        Frequency output: %i\n", f_out);
            printf("        Polarity: %i\n", polarity);
            printf("        V out: %i\n", v_out);
            printf("        Trigger Mode: %i\n\n", trigger_mode);
            printf("        Sync Frequency: %i\n\n", ext_sync_f);
            
            printf("\n");
      }
      
      
      
      
      
      /* Get Raw Name */
      res = ioctl(fd, HIDIOCGRAWNAME(256), buf);

      if (res < 0) {
            perror("HIDIOCGRAWNAME");
      }
      else {

	//Get CLI values as vars
	int freq = -1;
	int amplitude = -1;
	int trigger_mode = -1;
	int sync_freq = -1;
	int polarity = -1;
	int sync_now = -1;
	
	
	processCommandLineArguments(argc, argv, &freq, &amplitude, &trigger_mode, &sync_freq, &polarity);
      	printf("  Changes:\n");
      	int changed = 0;
	if (freq != -1) {
	    //Set Frequency
	    printf ("    Setting Frequecy: %i\n", freq);
	    
	    buf[0] = 0;
	    buf[1] = 1;//feature 1
	    
	    buf[3] = (freq & 0xff);
	    buf[4] = (freq >> 8) & 0xff;
	    buf[5] = (freq >> 16) & 0xff;
	    buf[6] = (freq >> 24) & 0xff;
	    
	    /* Set Feature */
            res = ioctl(fd, HIDIOCSFEATURE(60), buf);
            if (res < 0) perror("HIDIOCSFEATURE");
            changed = 1;
	}
	
	if (amplitude != -1) {
	    buf[0] = 0;
	    buf[1] = 2;
	    
	    
	    buf[3] = (amplitude & 0xff);
	    buf[4] = (amplitude >> 8) & 0xff;
	    
	    printf ("    Amplitude:%imV\n", amplitude);
	    // Set Feature 
            res = ioctl(fd, HIDIOCSFEATURE(60), buf);
            if (res < 0) perror("HIDIOCSFEATURE");
            changed = 1;
	}
	if (trigger_mode != -1) {
	    buf[0] = 0;
	    buf[1] = 5;
	    
	    
	    buf[3] = (trigger_mode & 0xff);
	    buf[4] = 0;
	    
	    char trigger_mode_string[16];
	    if (trigger_mode == 1) {
	    	sprintf(trigger_mode_string, "Sync Input");
	    } else if (trigger_mode == 0) {
	    	sprintf(trigger_mode_string, "Output");
	    } else {
	    	printf("Invalid option: %i", trigger_mode);
	    }
	    printf ("    Trigger Mode: %s\n", trigger_mode_string);
	    // Set Feature 
            res = ioctl(fd, HIDIOCSFEATURE(60), buf);
            if (res < 0) perror("HIDIOCSFEATURE");
            changed = 1;
	}
	if (polarity != -1) {
	    buf[0] = 0;
	    buf[1] = 3;
	    
	    
	    buf[3] = (polarity & 0xff);
	    buf[4] = 0;
	    
	    char polarity_string[16];
	    if (polarity == 1) {
	    	sprintf(polarity_string, "Inverted");
	    } else if (polarity == 0) {
	    	sprintf(polarity_string, "Normal");
	    } else {
	    	printf("Invalid option: %i", polarity);
	    }
	    printf ("    Polarity: %s\n", polarity_string);
	    // Set Feature 
            res = ioctl(fd, HIDIOCSFEATURE(60), buf);
            if (res < 0) perror("HIDIOCSFEATURE");
            changed = 1;
	}
	
	if (sync_freq != -1) {
	    //Set Ext Sync Frequency
	    printf ("    Setting Ext Sync Frequecy: %i\n", sync_freq);
	    
	    buf[0] = 0;
	    buf[1] = 6;//feature 6
	    
	    buf[3] = (sync_freq & 0xff);
	    buf[4] = (sync_freq >> 8) & 0xff;
	    buf[5] = (sync_freq>> 16) & 0xff;
	    buf[6] = (sync_freq>> 24) & 0xff;
	    
	    /* Set Feature */
            res = ioctl(fd, HIDIOCSFEATURE(60), buf);
            if (res < 0) perror("HIDIOCSFEATURE");
            changed = 1;
	}
	

	if (!changed) {
	    printf("    No changes made\n");
	}
      }
      close(fd);

      return 0;
}


int processCommandLineArguments(int argc, char **argv, int *freq, int *amplitude, int *trigger_mode,int *sync_freq, int *polarity)
{
    int c;
    
    while (1)
    {
        static struct option long_options[] =
        {
                /* These options set a flag. */
                //none
                /* These options don’t set a flag.
                    We distinguish them by their indices. */
                {"f",    required_argument, 0, 'a'},
                {"a",     required_argument, 0, 'b'},
                {"t",   required_argument, 0, 'c'},
                {"p",   required_argument, 0, 'd'},
                {"s",   required_argument, 0, 'e'},
                
                {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "abc:d:f:",
                    long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
        break;

        switch (c)
        {

            case 'a'://freq
                *freq = atoi(optarg);
                break;

            case 'b'://amplitude
                *amplitude = atoi(optarg);

                break;

            case 'c'://trigger mode
                *trigger_mode = atoi(optarg);
                break;

            case 'd'://polarity
                *polarity = atoi(optarg);
                break;

            case 'e'://sync freq
                *sync_freq = atoi(optarg);
                break;
                
            case '?':
                /* getopt_long already printed an error message. */
                break;

            default:
                abort ();
        }
    }
    return 0;
}

