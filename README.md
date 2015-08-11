# pngb
PNGB: GB Graphics Converter
<b>PNGB</b> is a command line utility that converts PNG files to <a href="http://gbdk.sourceforge.net/" target="_blank">GBDK</a> data definitions and code.<br/>

<h2>What is this good for?</h2>
<b>PNGB</b> can take a PNG image, slice it in 8x8 (or 8x16 if you are using GB "big sprites") tiles and output a single C source code file containing the pixel data, the color palette, attribute array and a tilemap (that recreates the source image using the extracted tiles).
<br />
<p>
It can additionally generate "test code" that allows you to see (to some extent) the result of the conversion and how to use the data.
</p>
<p>
<b>PNGB</b> currently supports 8x8 tiles (BKG or WIN layers) and Sprites (both 8x8 and 8x16).
</p>
<p>
It also supports <b>color</b> pictures. If your image has 4 colors or less <b>PNGB</b> will generate an equivalent 15-bit palette for GBC. For images with more than 4 colors you'll have the options to let <b>PNGB</b> convert it to grayscale first and then attempt to match each color with one of the four shades of the original Gameboy.
</p>
<p>
Currently <b>PNGB</b> can only generate a single palette for the whole image, but in a future it may be able to extract multiple palettes from a single image.
</p>
<br/>

You can see more details and examples in <a href="http://damnsoft.org/software/pngb" target="_blank">http://damnsoft.org/software/pngb</a>
