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

- sudo examples-api-use/text-example << from the directory 'examples-api-use' use the C++ code file text-example which allows you to display a text message.
- -f fonts/9x15.bdf << choose which font size the message will be displayed as.
- --led-no-hardware-pulse << avoids using the built-in sound of the Pi. It is known to cause issues.
- --led-multiplexing=4 << a simplified pixel mappers for outdoor panels. Multiplex 4 ('ZStripe') was used for this demo.




