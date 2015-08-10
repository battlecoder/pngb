..\..\bin\pngb -Kscpem giantenemy.png giantenemy.c
..\..\..\gbdk\bin\lcc -Wa-l -c -o giantenemy.o giantenemy.c
..\..\..\gbdk\bin\lcc -Wl-m -Wl-yp0x143=0x80 -o giantenemy.gb giantenemy.o
del giantenemy.o
del giantenemy.lst
del giantenemy.map