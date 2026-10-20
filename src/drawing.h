#define ADV_WIDTH 320
static const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:-/.?%~";
static const uint8_t glyphs[][7] = {
 {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
 {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
 {14,17,16,23,17,17,14},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
 {7,2,2,2,2,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
 {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
 {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
 {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
 {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
 {17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
 {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
 {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
 {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
 {14,17,17,15,1,1,14},{0,4,4,0,4,4,0},{0,0,0,31,0,0,0},
 {1,1,2,4,8,16,16},{0,0,0,0,0,6,6},{14,17,1,2,4,0,4},
 {25,26,2,4,8,11,19},{0,0,9,22,0,0,0},
};
struct canvas { uint8_t *pixels; int y, height; };
static void rect(struct canvas *c, int x, int y, int w, int h, uint16_t color) {
    int x0=x<0?0:x, x1=x+w>ADV_WIDTH?ADV_WIDTH:x+w;
    int y0=y<c->y?c->y:y, y1=y+h>c->y+c->height?c->y+c->height:y+h;
    for(int row=y0;row<y1;row++) for(int col=x0;col<x1;col++) {
        size_t p=((row-c->y)*ADV_WIDTH+col)*2;
        c->pixels[p]=color>>8; c->pixels[p+1]=color&255;
    }
}
static void text(struct canvas *c,int x,int y,const char *s,int scale,uint16_t color) {
    for(;*s;s++,x+=6*scale) {
        const char *g=strchr(chars,*s);
        if(!g || *s==' ') continue;
        const uint8_t *rows=glyphs[g-chars];
        for(int yy=0;yy<7;yy++) for(int xx=0;xx<5;xx++)
            if(rows[yy]&(1<<(4-xx)))rect(c,x+xx*scale,y+yy*scale,scale,scale,color);
    }
}
