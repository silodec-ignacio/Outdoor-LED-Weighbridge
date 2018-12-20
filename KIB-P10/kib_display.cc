// RS232 to display interface v1.01

#include "led-matrix.h"
#include "graphics.h"
#include "transformer.h"
#include "graphics.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>


using namespace rgb_matrix;

const char PACKET_STX = 0x02;
const char PACKET_ETX = 0x03;
const int PACKET_LENGTH = 256;
const int TEXT_LENGTH = 50;

char textBuffer[TEXT_LENGTH];

// Parser class for received data
class RxParser {
public:
  // Constructor
  RxParser() {
    Reset();
    ledX = 0;
    ledY = 0;
    ledH = 0;
    ledW = 0;
    ledColour = 0;
    ledFont = 0;
    ledClear = false;
    ledBlock = false;
  }
  // check for new instructions
  bool NewCommand() {
    if (!packetReceived)
      return false;
    if (ParsedPacketOK()) {
      Reset();
      return true;
    }
    Reset();
    return false;
  }
  // Parser reset
  void Reset() {
    packetReceived = false;
    packetStart = false;
    packetEnabled = true;
    packetIndex = 0;
  }
  // takes incoming received serial data 
  void UpdatePacket(char rx) {
    if(packetEnabled) {
      switch(rx) {
        case PACKET_STX:
          packetIndex = 0;
          packetStart = true;
	  printf("\n%X - got start..\n", rx);
          break;
        case PACKET_ETX:
          if(packetStart) {
            packetEnabled = false;
            packetReceived = true;
	    printf("\n..got end - %X\n", rx);
            packetBuffer[packetIndex] = '\0';
            printf("packet = [%s]\n", packetBuffer);
          }
          break;
        default:
          if(packetStart) {
            if(packetIndex>=PACKET_LENGTH-1)
              packetStart = false;
            else
              packetBuffer[packetIndex++] = rx;
          }
      }
    }
  }

  inline bool ClearScreenOK() { return ledClear; }
  inline bool DrawBlockOK() { return ledBlock; }
  inline int PositionX() { return ledX; }
  inline int Height() { return ledH; }
  inline int Width() { return ledW; }
  inline int PositionY() { return ledY; }
  inline int TextColour() { return ledColour; }
  inline int TextFont() { return ledFont; }

private:
  int packetIndex;
  bool packetStart;
  bool packetEnabled;
  bool packetReceived;
  char packetBuffer[PACKET_LENGTH];

  bool ledClear;
  bool ledBlock;
  int ledX;
  int ledY;
  int ledH;
  int ledW;
  int ledColour;
  int ledFont;

  bool ParsedPacketOK() {
    int index = 0;
    int textIndex = 0;
    bool cmd = false;
    bool expectText = false;
    bool expectEnd = false;
    bool expectDims = false;
    const char CMD_TOKEN = 0x80;
    char opt[1];

    ledClear = false;
    ledBlock = false;

    printf("\nParsing packet...\n");
    while (packetIndex > index) {
      switch (packetBuffer[index]) {
        case CMD_TOKEN:
          if (!cmd) {
            cmd = true;
            index++;
	    printf("~");
          } else return false;
          break;
        case 'B':
          if (cmd) {
            cmd = false;
            index++;
            ledBlock = true;
            expectDims = true;
            printf("B-");
          } else {
            if (expectText) {
              textBuffer[textIndex++] = packetBuffer[index++];
            } else return false;
          }
          break;
        case 'C':
          if (cmd) {
            cmd = false;
            index++;
            opt[0] = packetBuffer[index++];
            ledColour = atoi(opt);
            printf("C:%i,",ledColour);
          } else {
            if (expectText) {
              textBuffer[textIndex++] = packetBuffer[index++];
            } else return false;
          }
          break;
        case 'F':
          if (cmd) {
            cmd = false;
            index++;
            opt[0] = packetBuffer[index++];
            ledFont = atoi(opt);
            printf("F:%i,",ledFont);
          } else {
            if (expectText) {
              textBuffer[textIndex++] = packetBuffer[index++];
            } else return false;
          }
          break;
        case 'H':
          if (expectDims) {
            index++;
            opt[0] = packetBuffer[index++];
            ledH = atoi(opt);
            ledH *= 10;
            opt[0] = packetBuffer[index++];
            ledH += atoi(opt);
            printf("H:%i",ledH);
          } else {
            if (expectText) {
              textBuffer[textIndex++] = packetBuffer[index++];
            } else return false;
          }
          break;
        case 'T':
          if (cmd) {
            cmd = false;
            index++;
            expectText = true;
            expectEnd = true;
            printf("T:");
          } else {
            if (expectText) {
              textBuffer[textIndex++] = packetBuffer[index++];
            } else return false;
          }
          break;
        case 'W':
          if (expectDims) {
            index++;
            opt[0] = packetBuffer[index++];
            ledW = atoi(opt);
            ledW *= 10;
            opt[0] = packetBuffer[index++];
            ledW += atoi(opt);
            expectEnd = true;
            printf("W:%i",ledW);
          } else {
            if (expectText) {
              textBuffer[textIndex++] = packetBuffer[index++];
            } else return false;
          }
          break;
        case 'X':
          if (cmd) {
            cmd = false;
            index++;
            opt[0] = packetBuffer[index++];
            ledX = atoi(opt);
            ledX *= 10;
            opt[0] = packetBuffer[index++];
            ledX += atoi(opt);
            printf("X:%i",ledX);
          } else {
            if (expectText) {
              textBuffer[textIndex++] = packetBuffer[index++];
            } else return false;
          }
          break;
        case 'Y':
          if (cmd) {
            cmd = false;
            index++;
            opt[0] = packetBuffer[index++];
            ledY = atoi(opt);
            ledY *= 10;
            opt[0] = packetBuffer[index++];
            ledY += atoi(opt);
            printf("Y:%i",ledY);
          } else {
            if (expectText) {
              textBuffer[textIndex++] = packetBuffer[index++];
            } else return false;
          }
          break;
        case 'Z':
          if (cmd) {
            cmd = false;
            index++;
            expectEnd = true;
            ledClear = true;
            textBuffer[0] = '\0';
            ledX = 0;
            ledY = 0;
            ledColour = 0;
            ledFont = 1;
            printf("Z:");
          } else {
            if (expectText) {
              textBuffer[textIndex++] = packetBuffer[index++];
            } else return false;
          }
          break;
        case '\0':
          if (expectEnd) {
            if (expectText) {
              textBuffer[textIndex] = '\0';
              printf("%s\nDone!\n",textBuffer);
            } else {
              printf("\nDone!");
            }
            return true;
          }
          else
            return false;
          break;
        default:
          if (expectText) {
            textBuffer[textIndex++] = packetBuffer[index++];
          } else return false;
      }
    }
    return false;
  }
};

static int usage(const char *progname) {
  fprintf(stderr, "usage: %s [options]\n", progname);
  fprintf(stderr, "Reads data from RS232 serial port and displays it. \n");
  fprintf(stderr, "Options:\n"
          "\t-P <parallel> : parallel chains. 1..3. Default: 3\n"
          "\t-C <chained> : Daisy-chained boards. Default: 3.\n");
  return 1;
}

int main(int argc, char* argv[]) {
  int rows = 8;
  int chain = 6;
  int parallel = 3;

  int opt;
  while ((opt = getopt(argc, argv, "C:P:")) != -1) {
    switch (opt) {
    case 'P': parallel = atoi(optarg); break;
    case 'C': chain = atoi(optarg) * 2; break;
    default: return usage(argv[0]);
    }
  }

  const int x_orig = 0;
  const int y_orig = -1;

  int fd = -1;
  fd = open("/dev/ttyAMA0", O_RDONLY | O_NOCTTY | O_NDELAY );
  if (fd == -1)
    return 2;

  struct termios options;
  tcgetattr(fd, &options);
  options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
  options.c_iflag = IGNPAR;
  options.c_oflag = 0;
  options.c_lflag = 0;
  tcflush(fd, TCIFLUSH);
  tcsetattr(fd,TCSANOW, &options);

  GPIO io;
  if (!io.Init())
    return 1;

  RGBMatrix *matrix = new RGBMatrix(&io, rows, chain, parallel);

  // Our magic pixel re-mapping transform
  // LinkedTransformer *transformer = new LinkedTransformer();
  // matrix->SetTransformer(transformer);
  // transformer->AddTransformer(new Snake8x2Transformer());

  Canvas *canvas = matrix;

  rgb_matrix::Font font;

  RxParser rxd;  // access to the parser

  while (1) {
    int x;
    int y;
    int clrRed = 0;
    int clrGreen = 0;
    int clrBlue = 0;

    int bytesRead = 0;
    char buffer[1];
    do {
      bytesRead = read(fd, buffer, 1);
      if (bytesRead > 0) {
        rxd.UpdatePacket(buffer[0]);
        printf("%X,", buffer[0]);
      }
      if (buffer[0] == 0xFF) break;
    } while (bytesRead == sizeof(buffer));

    if (rxd.NewCommand()) {
      if(rxd.ClearScreenOK()) {
        canvas->Clear();
        printf("\nClearing screen\n");
      } else {
        switch (rxd.TextColour()) {
          case 0:
            clrRed = 255;
            break;
          case 1:
            clrGreen = 255;
            break;
          case 2:
            clrBlue = 255;
            break;
          case 3:
            clrRed = 255;
            clrGreen = 255;
            break;
          case 4:
            clrRed = 255;
            clrBlue = 255;
            break;
          case 5:
            clrGreen = 255;
            clrBlue = 255;
            break;
          case 6:
            clrRed = 127;
            clrGreen = 255;
            break;
          case 7:
            clrRed = 255;
            clrBlue = 127;
            break;
          case 8:
            clrRed = 0;
            clrBlue = 0;
            clrGreen = 0;
            break;
          case 9:
            clrRed = 255;
            clrGreen = 255;
            clrBlue = 255;
            break;
        }
        Color color(clrRed, clrGreen, clrBlue);
        if (rxd.DrawBlockOK()) {
          x = x_orig + rxd.PositionX();
          y = y_orig + rxd.PositionY();
          int x1 = x + rxd.Width();
          int y1 = y + rxd.Height();
	  for (int ypos = y; ypos < y1; ypos++) {
            rgb_matrix::DrawLine(canvas, x, ypos, x1, ypos, color);
          }
        } else {
          switch (rxd.TextFont()) {
            case 1:
              font.LoadFont("fonts/6x9.bdf");
              break;
            case 2:
              font.LoadFont("fonts/clR6x12.bdf");
              break;
            case 3:
              font.LoadFont("fonts/9x18B.bdf");
              break;
          }

          x = x_orig + rxd.PositionX();
          y = y_orig + rxd.PositionY() + font.baseline();

	  printf("\nText = %s\n\n", textBuffer);
          rgb_matrix::DrawText(canvas, font, x, y, color, textBuffer);
        }
      }
    }
  }
}
