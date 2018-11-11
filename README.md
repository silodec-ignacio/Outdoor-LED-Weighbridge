# Outdoor LED Weighbridge

This is a documentation detailing how to set an Outdoor LED panel for the SI Lodec Weighbridge.

Set up your raspberry pi and connect it to your LED panel. 

You'll need to clone the GitHub repository made by hzeller onto your Raspberry Pi:

`git clone https://github.com/hzeller/rpi-rgb-led-matrix.git`

from there navigate to the 'rpi-rgb-led-matrix' repository:

`cd rpi-rgb-led-matrix`

To display a simple text message type:

`sudo examples-api-use/text-example -f fonts/9x15.bdf --led-no-hardware-pulse --led-multiplexing=4`

A message will show up "Enter lines. Full screen or empty line clears screen. Supports UTF-8. CTRL-D for exit."
At this point type your message and press Enter.
Your message should display on your LED panel.

To break down each part of the command:

- sudo examples-api-use/text-example << from the directory 'examples-api-use' use the C++ code file "text-example" which allows you to display a text message.
- -f fonts/9x15.bdf << choose which font size the message will be displayed as.
- --led-no-hardware-pulse << avoids using the built-in sound of the Pi. It is known to cause issues.
- --led-multiplexing=4 << a simplified pixel mappers for outdoor panels. Multiplex 4 ('ZStripe') was used for this demo.

To add different options to how the message is displayed for you, here is a list of commands:

```
Options:
        -D <demo-nr>              : Always needs to be set
        -t <seconds>              : Run for these number of seconds, then exit.
        --led-gpio-mapping=<name> : Name of GPIO mapping used. Default "regular"
        --led-rows=<rows>         : Panel rows. Typically 8, 16, 32 or 64. (Default: 32).
        --led-cols=<cols>         : Panel columns. Typically 32 or 64. (Default: 32).
        --led-chain=<chained>     : Number of daisy-chained panels. (Default: 1).
        --led-parallel=<parallel> : Parallel chains. range=1..3 (Default: 1).
        --led-multiplexing=<0..6> : Mux type: 0=direct; 1=Stripe; 2=Checkered; 3=Spiral; 4=ZStripe; 5=ZnMirrorZStripe; 6=coreman (Default: 0)
        --led-pixel-mapper        : Semicolon-separated list of pixel-mappers to arrange pixels.
                                    Optional params after a colon e.g. "U-mapper;Rotate:90"
                                    Available: "Rotate", "U-mapper". Default: ""
        --led-pwm-bits=<1..11>    : PWM bits (Default: 11).
        --led-brightness=<percent>: Brightness in percent (Default: 100).
        --led-scan-mode=<0..1>    : 0 = progressive; 1 = interlaced (Default: 0).
        --led-row-addr-type=<0..2>: 0 = default; 1 = AB-addressed panels; 2 = direct row select(Default: 0).
        --led-show-refresh        : Show refresh rate.
        --led-inverse             : Switch if your matrix has inverse colors on.
        --led-rgb-sequence        : Switch if your matrix has led colors swapped (Default: "RGB")
        --led-pwm-lsb-nanoseconds : PWM Nanoseconds for LSB (Default: 130)
        --led-no-hardware-pulse   : Don't use hardware pin-pulse generation.
        --led-slowdown-gpio=<0..2>: Slowdown GPIO. Needed for faster Pis/slower panels (Default: 1).
        --led-daemon              : Make the process run in the background as daemon.
        --led-no-drop-privs       : Don't drop privileges from 'root' after initializing the hardware.
Demos, choosen with -D
        0  - some rotating square
        1  - forward scrolling an image (-m <scroll-ms>)
        2  - backward scrolling an image (-m <scroll-ms>)
        3  - test image: a square
        4  - Pulsing color
        5  - Grayscale Block
        6  - Abelian sandpile model (-m <time-step-ms>)
        7  - Conway's game of life (-m <time-step-ms>)
        8  - Langton's ant (-m <time-step-ms>)
        9  - Volume bars (-m <time-step-ms>)
        10 - Evolution of color (-m <time-step-ms>)
        11 - Brightness pulse generator
```


