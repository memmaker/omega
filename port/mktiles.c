#include <stdio.h>
#include "defs.h"
int m[][3] = {
#include "map.inc"
};
int main(void){int i;printf("/* Kinder's WinOmega gfxMapData: (char|colour) -> [col,row] in tiles.png (32x32) */\nvar OMEGA_TILES={");
for(i=0;i<sizeof m/sizeof m[0];i++)printf("%s%d:[%d,%d]",i?",":"",m[i][0],m[i][1],m[i][2]);puts("};");return 0;}
