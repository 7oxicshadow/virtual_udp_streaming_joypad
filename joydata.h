#ifndef JOYDATA_H
#define JOYDATA_H

typedef struct virjoy_st_
{
   //buttons
   unsigned char VIRJOY_BTN_A         :1;
   unsigned char VIRJOY_BTN_B         :1;
   unsigned char VIRJOY_BTN_X         :1;
   unsigned char VIRJOY_BTN_Y         :1;
   unsigned char VIRJOY_BTN_TL        :1;
   unsigned char VIRJOY_BTN_TR        :1;
   unsigned char VIRJOY_BTN_THUMBL    :1;
   unsigned char VIRJOY_BTN_THUMBR    :1;
   
   unsigned char VIRJOY_BTN_DPAD_UP       :1;
   unsigned char VIRJOY_BTN_DPAD_DOWN     :1;
   unsigned char VIRJOY_BTN_DPAD_LEFT     :1;
   unsigned char VIRJOY_BTN_DPAD_RIGHT    :1;
   unsigned char VIRJOY_BTN_START         :1;
   unsigned char VIRJOY_BTN_SELECT        :1;
   unsigned char VIRJOY_BTN_MODE          :1;
   unsigned char VIRJOY_PADDING_1         :1;

   //analogue (signed)
   short VIRJOY_ABS_X;
   short VIRJOY_ABS_Y;
   short VIRJOY_ABS_RX;
   short VIRJOY_ABS_RY;
   short VIRJOY_ABS_LT;
   short VIRJOY_ABS_RT;
   short VIRJOY_ABS_HAT0X;
   short VIRJOY_ABS_HAT0Y;

   unsigned char VIRJOY_CHECKSUM;
   
}virjoy_st;

typedef union virjoy_un_
{
    virjoy_st virtualjoydata;
    unsigned char raw[sizeof(virjoy_st)];
}virjoy_un;

#endif