#ifndef _SPRITEMGR_H_
#define _SPRITEMGR_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "M5DisplayExt.h"

typedef struct
{
    uint16_t uid;
    TFT_eSprite* sprite;
    void* next_node;
    void* prev_node;
}
sprmgr_item_t;

class SpriteMgr
{
    public:
        SpriteMgr(M5DisplayExt* tft);
        bool load(const char* fp, int16_t width, int16_t height);
        void draw(const char* fp, int16_t x, int16_t y, int16_t width = 0, int16_t height = 0);
        TFT_eSprite* get(const char* fp);
        void unload_all(void);
        uint8_t holder_flag;
        void (*cb_needboost)(void) = NULL;

    private:
        M5DisplayExt* tft;
        sprmgr_item_t* head_node;
        sprmgr_item_t* last(void);
        void need_boost(void) { if (cb_needboost != NULL) { cb_needboost(); } };
};

#endif
