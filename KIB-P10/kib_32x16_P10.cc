/* KIB_32x16_P10.CC 

  RS232 [9600-8N1] received on RXD [GPIO PIN 10] is parsed and updates an
  off-screen frame canvas. The canvas is mapped thru a transformer to the
  RGB LED MATRIX. Upon a received REFRESH command the off-sceen canvas is
  swapped with the current displayed canvas on the next VSYNC interval.
   
  This version = v3.01 - For P10 size panels only

  15 June 2016  - Added Box, Circle, Pixel and Fill commands
		- Added splash screen 

  First revision date = 29 October 2015 (for 32x16 P10 panels)
  Last revision date = 19 June 2016
  
*/

/* STANDARD C++ LIBRARIES USED */ 

  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <termios.h>
  #include <assert.h>
  #include <signal.h>
  #include <time.h>


/* RGB MATRIX LIBRARIES USED */

  #include "led-matrix.h"
  #include "graphics.h"
  #include "canvas.h"
  #include "threaded-canvas-manipulator.h"
   
  using namespace rgb_matrix;
  
  using rgb_matrix::RGBMatrix;
volatile bool interrupt_received = false;

static void InterruptHandler(int signo) {
  interrupt_received = true;
}

/************** IMAGE SCROLLER **************/
//This makes the SI Logo image scroll across the LED panel.
class ImageScroller : public ThreadedCanvasManipulator {
public:
  // Scroll image with "scroll_jumps" pixels every "scroll_ms" milliseconds.
  // If "scroll_ms" is negative, don't do any scrolling.
  ImageScroller(RGBMatrix *m, int scroll_jumps, int scroll_ms = 30)
    : ThreadedCanvasManipulator(m), scroll_jumps_(scroll_jumps),
      scroll_ms_(scroll_ms),
      horizontal_position_(0),
      matrix_(m) {
    offscreen_ = matrix_->CreateFrameCanvas();
  }

  virtual ~ImageScroller() {
    Stop();
    WaitStopped();   // only now it is safe to delete our instance variables.
  }

  // _very_ simplified. Can only read binary P6 PPM. Expects newlines in headers
  // Not really robust. Use at your own risk :)
  // This allows reload of an image while things are running, e.g. you can
  // life-update the content.
  bool LoadPPM(const char *filename) {
    FILE *f = fopen(filename, "r");
    // check if file exists
    if (f == NULL && access(filename, F_OK) == -1) {
      fprintf(stderr, "File \"%s\" doesn't exist\n", filename);
      return false;
    }
    if (f == NULL) return false;
    char header_buf[256];
    const char *line = ReadLine(f, header_buf, sizeof(header_buf));
#define EXIT_WITH_MSG(m) { fprintf(stderr, "%s: %s |%s", filename, m, line); \
      fclose(f); return false; }
    if (sscanf(line, "P6 ") == EOF)
      EXIT_WITH_MSG("Can only handle P6 as PPM type.");
    line = ReadLine(f, header_buf, sizeof(header_buf));
    int new_width, new_height;
    if (!line || sscanf(line, "%d %d ", &new_width, &new_height) != 2)
      EXIT_WITH_MSG("Width/height expected");
    int value;
    line = ReadLine(f, header_buf, sizeof(header_buf));
    if (!line || sscanf(line, "%d ", &value) != 1 || value != 255)
      EXIT_WITH_MSG("Only 255 for maxval allowed.");
    const size_t pixel_count = new_width * new_height;
    Pixel *new_image = new Pixel [ pixel_count ];
    assert(sizeof(Pixel) == 3);   // we make that assumption.
    if (fread(new_image, sizeof(Pixel), pixel_count, f) != pixel_count) {
      line = "";
      EXIT_WITH_MSG("Not enough pixels read.");
    }
#undef EXIT_WITH_MSG
    fclose(f);
    fprintf(stderr, "Read image '%s' with %dx%d\n", filename,
            new_width, new_height);
    horizontal_position_ = 0;
    MutexLock l(&mutex_new_image_);
    new_image_.Delete();  // in case we reload faster than is picked up
    new_image_.image = new_image;
    new_image_.width = new_width;
    new_image_.height = new_height;
    return true;
  }

  void Run() {
    const int screen_height = offscreen_->height();
    const int screen_width = offscreen_->width();
		

    while (running() && !interrupt_received) {
      {
        MutexLock l(&mutex_new_image_);
        if (new_image_.IsValid()) {
          current_image_.Delete();
          current_image_ = new_image_;
          new_image_.Reset();		  
        }
      }
      if (!current_image_.IsValid()) {
        usleep(100 * 1000);
        continue;
      }
      for (int x = 0; x < screen_width; ++x) {
        for (int y = 0; y < screen_height; ++y) {
          const Pixel &p = current_image_.getPixel(
            (horizontal_position_ + x) % current_image_.width, y);
          offscreen_->SetPixel(x, y, p.red, p.green, p.blue);
        }
      }
      offscreen_ = matrix_->SwapOnVSync(offscreen_);
      horizontal_position_ += scroll_jumps_;
      if (horizontal_position_ < 0) horizontal_position_ = current_image_.width;
      if (scroll_ms_ <= 0) {
        // No scrolling. We don't need the image anymore.
        current_image_.Delete();
      } else {
        usleep(scroll_ms_ * 1000);
      }
	  	  
		double secondsPassed; 
		secondsPassed = clock() / CLOCKS_PER_SEC; //Starts a timer.
		
		int stop = 15; //To change the amount of seconds the Splashscreen goes for.
		if (secondsPassed >= stop){break;} //stops the splashscreen.
    }
	
	 matrix_-> Clear();
  }

private:
  struct Pixel {
    Pixel() : red(0), green(0), blue(0){}
    uint8_t red;
    uint8_t green;
    uint8_t blue;
  };

  struct Image {
    Image() : width(-1), height(-1), image(NULL) {}
    ~Image() { Delete(); }
    void Delete() { delete [] image; Reset(); }
    void Reset() { image = NULL; width = -1; height = -1; }
    inline bool IsValid() { return image && height > 0 && width > 0; }
    const Pixel &getPixel(int x, int y) {
      static Pixel black;
      if (x < 0 || x >= width || y < 0 || y >= height) return black;
      return image[x + width * y];
    }

    int width;
    int height;
    Pixel *image;
  };

  // Read line, skip comments.
  char *ReadLine(FILE *f, char *buffer, size_t len) {
    char *result;
    do {
      result = fgets(buffer, len, f);
    } while (result != NULL && result[0] == '#');
    return result;
  }

  const int scroll_jumps_;
  const int scroll_ms_;

  // Current image is only manipulated in our thread.
  Image current_image_;

  // New image can be loaded from another thread, then taken over in main thread
  Mutex mutex_new_image_;
  Image new_image_;

  int32_t horizontal_position_;

  RGBMatrix* matrix_;
  FrameCanvas* offscreen_;
};
  

/**************  FONT DECODING  ************/

void getFont(rgb_matrix::Font &font, int fontWide, int fontHigh) {
  
  bool fontBold = (fontWide & 0x80) == 0x80;
  bool fontOut = fontBold ? false : (fontWide & 0x40) == 0x40;
  fontWide = fontWide & 0x0F;
    
  switch(fontWide) {
    case 4:
      font.LoadFont("fonts/4x6.bdf"); break;
    case 5: 
      if (fontHigh < 8) {font.LoadFont("fonts/5x7.bdf"); break;}
      font.LoadFont("fonts/5x8.bdf"); break;
    case 6:
      if (fontHigh < 10) {font.LoadFont("fonts/6x9.bdf"); break;}
      if (fontHigh < 11) {font.LoadFont("fonts/6x10.bdf"); break;}
      if (fontHigh < 13) {font.LoadFont("fonts/6x12.bdf"); break;}
      if (fontOut) {font.LoadFont("fonts/6x13O.bdf"); break;}
      if (fontBold) {font.LoadFont("fonts/6x13B.bdf"); break;}
      font.LoadFont("fonts/6x13.bdf"); break;
    case 7:
      if (fontHigh < 14) {
        if (fontOut) {font.LoadFont("fonts/7x13O.bdf"); break;}
        if (fontBold) {font.LoadFont("fonts/7x13B.bdf"); break;}
        font.LoadFont("fonts/7x13.bdf"); break;
      }
      if (fontOut) {font.LoadFont("fonts/7x14O.bdf"); break;}
      if (fontBold) {font.LoadFont("fonts/7x14B.bdf"); break;}
      font.LoadFont("fonts/7x14.bdf"); break;
    case 8:
      if (fontOut) {font.LoadFont("fonts/8x13O.bdf"); break;}
      if (fontBold) {font.LoadFont("fonts/8x13B.bdf"); break;}
      font.LoadFont("fonts/8x13.bdf"); break;
    case 9:
      if (fontHigh < 17) {
        if (fontBold) {font.LoadFont("fonts/9x15B.bdf"); break;}
        font.LoadFont("fonts/9x15.bdf"); break;
      }
      if (fontBold) {font.LoadFont("fonts/9x18B.bdf"); break;}
      font.LoadFont("fonts/9x18.bdf"); break;
    case 10:
      font.LoadFont("fonts/10x20.bdf"); break;
    case 11: 
      font.LoadFont("fonts/clR6x12.bdf"); break;
    case 12: 
      font.LoadFont("fonts/helvR12.bdf"); break;
    default:
      font.LoadFont("fonts/4x6.bdf"); break;
  }
}
  
/*************  COLOR DECODING  ************/  
  
int getHue(int hue) {
  hue &= 0x03;
  hue *= 85;
  return hue;
}   
//0x15
Color getColor(int colorId) {
// int white = getHue(colorId >> 6);  /* for interests sake */
  int red = getHue(colorId >> 4);
  int green = getHue(colorId >> 2);
  int blue = getHue(colorId);
  
  Color color(red, green, blue);
  return color;
}

/*************   PROGRAM HELP  *************/

static int usage(const char *progname) {
  fprintf(stderr, "usage: %s [options]\n", progname);
  fprintf(stderr, "Reads data from RS232 serial port and displays it. \n");
  fprintf(stderr, "Options:\n"
          "\t-P <parallel> : parallel chains. 1..3. Default: 3\n"
          "\t-C <chained> : Daisy-chained boards. Default: 3.\n");
  fprintf(stderr, "Error codes (on exit):\n"
          "\t 1 = GPIO initialisation failure (user must be ROOT).\n"
          "\t 2 = Serial port failed initialisation.\n");
  return 1;
}
  
/***************  MAIN LOOP  ***************/

/* MESSAGE TOKENS */

  const char MSG_STX = 0xF2;
  const char MSG_ETX = 0xF3;
  const char MSG_CMD = 0xF4; 

/* COMMAND AND CONTROL CHARACTERS */ 

  const int MAX_TEXT_LEN = 25;
  
  const char CMD_NONE	 = '\0';
  const char CMD_BOX	 = 'B';
  const char CMD_CIRCLE	 = 'R';
  const char CMD_COLOR   = 'C';
  const char CMD_FONT    = 'F';
  const char CMD_FILL	 = 'S';
  const char CMD_LINE    = 'L';
  const char CMD_NEW	 = 'N';
  const char CMD_PIXEL   = 'P';
  const char CMD_POS	 = 'X';
  const char CMD_REFRESH = 'Z';
  const char CMD_TEXT    = 'T';

/* COMMAND STRING PARSER MODES */

  const int WAIT_START       = 0;
  const int WAIT_COMMAND     = 1;
  const int WAIT_INSTRUCTION = 2;
  const int DO_END           = 3;
  const int DO_CLEAR         = 4;
  const int DO_REFRESH       = 5;
  const int WAIT_TEXT        = 6;
  const int WAIT_FONT_W	     = 7;
  const int WAIT_FONT_H	     = 8;
  const int WAIT_LINE_X	     = 9;
  const int WAIT_LINE_Y	     = 10;
  const int WAIT_POS_X	     = 11;
  const int WAIT_POS_Y	     = 12;
  const int WAIT_COLOR       = 13; 
  const int WAIT_CIRCLE_R    = 14; 
  const int WAIT_BOX_X       = 15; 
  const int WAIT_BOX_Y       = 16; 
  const int WAIT_FILL        = 17; 
  const int WAIT_PIXEL_X     = 18; 
  const int WAIT_PIXEL_Y     = 19; 
  
int isCommand(char cmd) {
  switch(cmd) {
    case MSG_ETX:
      printf("done\n");
      return WAIT_START;
    case CMD_NEW:     return DO_CLEAR;
    case CMD_REFRESH: return DO_REFRESH;
    case CMD_TEXT:    return WAIT_TEXT;
    case CMD_FONT:    return WAIT_FONT_W;
    case CMD_LINE:    return WAIT_LINE_X;
    case CMD_POS:     return WAIT_POS_X;
    case CMD_COLOR:   return WAIT_COLOR;
    case CMD_CIRCLE:  return WAIT_CIRCLE_R;
    case CMD_BOX:     return WAIT_BOX_X;
    case CMD_FILL:    return WAIT_FILL;
    case CMD_PIXEL:   return WAIT_PIXEL_X;
  }
  return WAIT_COMMAND;
}


int main(int argc, char* argv[]) {
  
/* OPTIONAL SETTINGS */  

  RGBMatrix::Options led_options;
  rgb_matrix::RuntimeOptions runtime;

  // These are the defaults when no command-line flags are given.
	led_options.rows = 32;
	led_options.cols = 32;
	led_options.chain_length = 4;
	// led_options.chain_length = 6;
	led_options.parallel = 3;
	led_options.pixel_mapper_config = "xyflipped";
	led_options.multiplexing = 7;  
	runtime.drop_privileges = 1;
  
  
int demo = 1;
int scroll_ms = 30;
int runtime_seconds = 8;

  
  if (!rgb_matrix::ParseOptionsFromFlags(&argc, &argv, &led_options, &runtime)) {
    rgb_matrix::PrintMatrixFlags(stderr);
    return 1;
  }


  int opt;
  while ((opt = getopt(argc, argv, "P:c:p:b:LR:m:t:")) != -1) {
    switch (opt) {
    
      // These used to be options we understood, but deprecated now. Accept
      // but don't mention in usage()
    case 'R':
      fprintf(stderr, "-R is deprecated. "
              "Use --led-pixel-mapper=\"Rotate:%s\" instead.\n", optarg);
      return 1;
      break;

    case 'L':
      fprintf(stderr, "-L is deprecated. Use\n\t--led-pixel-mapper=\"U-mapper\" --led-chain=4\ninstead.\n");
      return 1;
      break;
	  
    case 't':
      runtime_seconds = atoi(optarg);
      break;
    
    case 'r':
      fprintf(stderr, "Instead of deprecated -r, use --led-rows=%s instead.\n",
              optarg);
      led_options.rows = atoi(optarg);
      break;

    case 'P':
      led_options.parallel = atoi(optarg);
      break;

    case 'c':
      fprintf(stderr, "Instead of deprecated -c, use --led-chain=%s instead.\n",
              optarg);
      led_options.chain_length = atoi(optarg);
      break;

    case 'p':
      led_options.pwm_bits = atoi(optarg);
      break;

    case 'b':
      led_options.brightness = atoi(optarg);
      break;
	  
    case 'm':
      scroll_ms = atoi(optarg);
      break;

    default: 
      return usage(argv[0]);
    }
  }
  
  const char *demo_parameter = ("./SILogo.ppm"); //Default Splashscreen Logo. 
  if (optind < argc) {
	/*To add a different image simply add to the command line:
	
	sudo /home/pi/rpi-rgb-led-matrix/KIB-P10/kib_32x16_P10 name_of_new_image
	
	Remember that the new image needs to be a '.ppm' file and within the KIB-P10 directory.
	
	*/
    demo_parameter = argv[optind]; 
  }

  // Looks like we're ready to start
  RGBMatrix *matrix = CreateMatrixFromOptions(led_options, runtime);
  if (matrix == NULL) {
    return 1;
  }


/* SERIAL PORT INTERFACE */
  printf("Opening serial port...\n");
    
  int serialPort = -1;
  serialPort = open("/dev/ttyAMA0", O_RDONLY | O_NOCTTY | O_NDELAY );
  if (serialPort == -1){
   		printf("Error - Unable to open UART.  Ensure it is not in use by another application\n");
		return 2;
  }
  else {printf("Serial Port Open\n");}
  
  struct termios options;
  tcgetattr(serialPort, &options);
  options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
  options.c_iflag = IGNPAR;
  options.c_oflag = 0;
  options.c_lflag = 0;
  tcflush(serialPort, TCIFLUSH);
  tcsetattr(serialPort,TCSANOW, &options);  

/* GPIO TO RGB MATRIX INTERFACE */
  printf("Initialising GPIO...\n");

  GPIO io;
  if (!io.Init()) return 1;	/*** !!! MUST BE ROOT !!! ***/

  
  FrameCanvas *offscreen = matrix->CreateFrameCanvas();  
  rgb_matrix::Font font;

  int mode = WAIT_START;
  int txtIndex = 0;
  int fontWide = 4;
  int fontHigh = 5;
  int startX = 0;
  int startY = 0;
  int endX = 0;
  int endY = 0;
  int colorId = 0x10;
  
  char txtBuffer[MAX_TEXT_LEN];

  printf("Ready\n");
  
  		

/************************** Splash screen ***************************/

RGBMatrix *canvas = rgb_matrix::CreateMatrixFromOptions(led_options, runtime);


ThreadedCanvasManipulator *image_gen = NULL;
Color col = getColor(0x15);

if (demo_parameter) {
      ImageScroller *scroller = new ImageScroller(matrix,
                                                  demo == 1 ? 1 : -1,
                                                  scroll_ms);
      if (!scroller->LoadPPM(demo_parameter))
        return 1;
      image_gen = scroller;
    } else {
      fprintf(stderr, "Demo %d Requires PPM image as parameter\n", demo);
      return 1;
    }

  
  // Image generating demo is crated. Now start the thread.
  image_gen->Start();


/*********************************************************************/

/* MAIN RUN LOOP */
  
  while (1) {
    
    int bytesRead = 0;
    char rxBuffer[1];

    do {
     bytesRead = read(serialPort, rxBuffer, 1);

	  
      if (bytesRead > 0) {
		  
        switch(mode) {
          case WAIT_START:
            if (rxBuffer[0] == MSG_STX) {
              txtIndex = 0;
              txtBuffer[0] = '\0';
              fontWide = 4;
              fontHigh = 5;
              endX = 0;
              endY  = 0;
              startX = 0;
              startY = 0;
              colorId = 0x10;
              mode = WAIT_COMMAND;
	      printf("parsing... ");
            } break;
            
          case WAIT_COMMAND:
            if (rxBuffer[0] == MSG_CMD) {
              mode = WAIT_INSTRUCTION;
            }
            else mode = WAIT_COMMAND;
            break;
            
          case WAIT_INSTRUCTION:
            mode = isCommand(rxBuffer[0]);
            if (mode == DO_CLEAR) {
              matrix->Clear();
              mode = WAIT_COMMAND;
              break;
            }
            if (mode == DO_REFRESH) {
              offscreen = matrix->SwapOnVSync(offscreen);
              mode = WAIT_COMMAND;
              break;
            }
            if (mode == WAIT_TEXT) txtIndex = 0;
            break;
            
          case WAIT_COLOR: 
            colorId = (int)rxBuffer[0];
            mode = WAIT_COMMAND;
            break;
          case WAIT_POS_X:
            startX = (int)rxBuffer[0];
            mode = WAIT_POS_Y;
            break;
          case WAIT_POS_Y:
            startY = (int)rxBuffer[0];
            mode = WAIT_COMMAND;
            break;
          case WAIT_TEXT:
	    if (rxBuffer[0] == '0') {
	      if((fontWide == 5) && (fontHigh == 8)) {
		rxBuffer[0] = 'O';
	      }
	    }
            txtBuffer[txtIndex++] = rxBuffer[0];
            if ((txtIndex >= MAX_TEXT_LEN) || (rxBuffer[0] == '\0')) {
              getFont(font, fontWide, fontHigh);
			  rgb_matrix::DrawText(offscreen, font, startX, startY, getColor(colorId), txtBuffer);
              mode = WAIT_COMMAND;
            }
            break;
          case WAIT_FONT_W:
            fontWide = (int)rxBuffer[0];
            mode = WAIT_FONT_H;
            break;
          case WAIT_FONT_H:
            fontHigh = (int)rxBuffer[0];
            mode = WAIT_COMMAND;
            break;
          case WAIT_LINE_X:
            endX = (int)rxBuffer[0];
            mode = WAIT_LINE_Y;
            break;
          case WAIT_LINE_Y:
            endY = (int)rxBuffer[0];
            DrawLine(offscreen, startX, startY, endX, endY, getColor(colorId));
            mode = WAIT_COMMAND;
            break;
          case WAIT_CIRCLE_R:
            endX = (int)rxBuffer[0];
            DrawCircle(offscreen, startX, startY, endX, getColor(colorId));
			mode = WAIT_COMMAND;
            break;
          case WAIT_BOX_X:
            endX = (int)rxBuffer[0];
            mode = WAIT_BOX_Y;
            break;
          case WAIT_BOX_Y:
            endY = (int)rxBuffer[0];
            //drawBox(offscreen, startX, startY, endX, endY, getColor(colorId));
			DrawLine(offscreen, startX, startY, endX, endY, getColor(colorId));
            mode = WAIT_COMMAND;
            break;
          case WAIT_FILL: 
            colorId = (int)rxBuffer[0];
		matrix->Fill(col.r, col.g, col.b);
		
            mode = WAIT_COMMAND;
	    break;
          case WAIT_PIXEL_X:
            endX = (int)rxBuffer[0];
            mode = WAIT_PIXEL_Y;
            break;
          case WAIT_PIXEL_Y:
            endY = (int)rxBuffer[0];
            //drawPixel(offscreen, startX, startY, getColor(colorId));
			canvas->SetPixel(startX, startY, 200, 0, 0);  
			//canvas->SetPixel(29+LX, 8+LY, 200, 0, 0);  
            mode = WAIT_COMMAND;
            break;
        }
      }
    } while (bytesRead == sizeof(rxBuffer));
  }
  
  
  signal(SIGTERM, InterruptHandler);
	signal(SIGINT, InterruptHandler);


  // Now, the image generation runs in the background. We can do arbitrary
  // things here in parallel. In this demo, we're essentially just
  // waiting for one of the conditions to exit.
  if (runtime_seconds > 0) {
    sleep(runtime_seconds);
  } else {
    // The
    printf("Press <CTRL-C> to exit and reset LEDs\n");
    while (!interrupt_received) {
      sleep(1); // Time doesn't really matter. The syscall will be interrupted.
    }
  }

  // Stop image generating thread. The delete triggers
  delete image_gen;
  delete canvas;

  printf("\%s. Exiting.\n",
         interrupt_received ? "Received CTRL-C" : "Timeout reached");
  return 0;
  
}
