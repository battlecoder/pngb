..\..\bin\pngb -Sscpem -tr #ffffff sprite.png sprite.c
..\..\..\gbdk\bin\lcc -Wa-l -c -o sprite.o sprite.c
..\..\..\gbdk\bin\lcc -Wl-m -Wl-yp0x143=0x80 -o sprite.gb sprite.o
del sprite.o
del sprite.lst
del sprite.map