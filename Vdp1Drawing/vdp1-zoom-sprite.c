/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdio.h>
#include <stdlib.h>

#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   240

#define ZOOM_TEX_PATH   "ZOOM.TEX"
#define ZOOM_PAL_PATH   "ZOOM.PAL"

#define SPRITE_WIDTH                (64)
#define SPRITE_HEIGHT               (64)
#define SPRITE_POINTER_SIZE         (3)
#define SPRITE_COLOR_SELECT         COLOR_RGB1555(1,  0,  0, 31)
#define SPRITE_COLOR_WAIT           COLOR_RGB1555(1, 31,  0,  0)
#define SPRITE_COLOR_HIGHLIGHT      COLOR_RGB1555(1,  0, 31,  0)

#define VDP1_CMDT_ORDER_SYSTEM_CLIP_COORDS_INDEX        0
#define VDP1_CMDT_ORDER_LOCAL_COORDS_INDEX              1
#define VDP1_CMDT_ORDER_NORMAL_INDEX                    2
#define VDP1_CMDT_ORDER_DRAW_END_INDEX                  6
#define VDP1_CMDT_ORDER_COUNT                           7

extern uint8_t root_romdisk[];

static struct {
        uint16_t *tex_base;
        uint16_t *pal_base;

        vdp1_cmdt_t *cmdt;
} _sprite;

static void *_romdisk = NULL;

static vdp1_cmdt_list_t *_cmdt_list = NULL;
static vdp1_vram_partitions_t _vdp1_vram_partitions;

static volatile uint32_t _frt_count = 0;

static void _init(void);

static void _cmdt_list_init(void);

static void _sprite_init(int id, int x, int y, int w, int h);

static uint32_t _frame_time_calculate(void);

static void _cpu_frt_ovi_handler(void);

int
main(void)
{
        _init();

        cpu_frt_count_set(0);

        while (true) {


                vdp1_sync_cmdt_list_put(_cmdt_list, 0);

                vdp1_sync_render();

                vdp1_sync();
                vdp1_sync_wait();

                dbgio_flush();

                uint32_t result;
                result = _frame_time_calculate();

                char fixed[16];
                fix16_str(result, fixed, 7);

                dbgio_printf("[H[2J%sms\n", fixed);
        }

        return 0;
}

void
user_init(void)
{
        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_240);

        vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            COLOR_RGB1555(1, 0, 0, 0));

        vdp2_sprite_priority_set(0, 6);

        /* Setup default VDP1 environment */
        vdp1_env_default_set();

        vdp2_tvmd_display_set();

        cpu_frt_init(CPU_FRT_CLOCK_DIV_32);
        cpu_frt_ovi_set(_cpu_frt_ovi_handler);

        vdp1_vram_partitions_get(&_vdp1_vram_partitions);

        vdp2_sync();
        vdp2_sync_wait();
}

static void
_init(void)
{
        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();

        romdisk_init();

        _romdisk = romdisk_mount(root_romdisk);
        assert(_romdisk != NULL);

        _cmdt_list_init();
}

static void
_cmdt_list_init(void)
{
        static const int16_vec2_t system_clip_coord =
            INT16_VEC2_INITIALIZER(SCREEN_WIDTH - 1,
                                   SCREEN_HEIGHT - 1);

        static const int16_vec2_t local_coord_center =
            INT16_VEC2_INITIALIZER(SCREEN_WIDTH / 2,
                                   SCREEN_HEIGHT / 2);

        _cmdt_list = vdp1_cmdt_list_alloc(VDP1_CMDT_ORDER_COUNT);

        (void)memset(&_cmdt_list->cmdts[0], 0x00, sizeof(vdp1_cmdt_t) * VDP1_CMDT_ORDER_COUNT);

        _cmdt_list->count = VDP1_CMDT_ORDER_COUNT;

        vdp1_cmdt_t * const cmdts =
            &_cmdt_list->cmdts[0];

        _sprite_init(VDP1_CMDT_ORDER_NORMAL_INDEX, -SCREEN_WIDTH/4, -SCREEN_HEIGHT/4, SPRITE_WIDTH, SPRITE_WIDTH );
        _sprite_init(VDP1_CMDT_ORDER_NORMAL_INDEX+1, -SCREEN_WIDTH/4, SCREEN_HEIGHT/4, SPRITE_WIDTH, SPRITE_WIDTH );
        _sprite_init(VDP1_CMDT_ORDER_NORMAL_INDEX+2, SCREEN_WIDTH/4, SCREEN_HEIGHT/4, SPRITE_WIDTH, SPRITE_WIDTH );
        _sprite_init(VDP1_CMDT_ORDER_NORMAL_INDEX+3, SCREEN_WIDTH/4, -SCREEN_HEIGHT/4, SPRITE_WIDTH, SPRITE_WIDTH );

        vdp1_cmdt_system_clip_coord_set(&cmdts[VDP1_CMDT_ORDER_SYSTEM_CLIP_COORDS_INDEX]);
        vdp1_cmdt_param_vertex_set(&cmdts[VDP1_CMDT_ORDER_SYSTEM_CLIP_COORDS_INDEX],
            CMDT_VTX_SYSTEM_CLIP,
            &system_clip_coord);

        vdp1_cmdt_local_coord_set(&cmdts[VDP1_CMDT_ORDER_LOCAL_COORDS_INDEX]);
        vdp1_cmdt_param_vertex_set(&cmdts[VDP1_CMDT_ORDER_LOCAL_COORDS_INDEX],
            CMDT_VTX_LOCAL_COORD,
            &local_coord_center);

        vdp1_cmdt_end_set(&cmdts[VDP1_CMDT_ORDER_DRAW_END_INDEX]);
}

static uint32_t
_frame_time_calculate(void)
{
        uint16_t frt;
        frt = cpu_frt_count_get();

        cpu_frt_count_set(0);

        uint32_t delta_fix;
        delta_fix = frt << 4;

        uint32_t divisor_fix;
        divisor_fix = CPU_FRT_NTSC_320_32_COUNT_1MS << 4;

        cpu_divu_32_32_set(delta_fix << 4, divisor_fix);

        /* Remember to wait ~40 cycles */
        for (uint32_t i = 0; i < 8; i++) {
                cpu_instr_nop();
                cpu_instr_nop();
                cpu_instr_nop();
                cpu_instr_nop();
                cpu_instr_nop();
        }

        uint32_t result;
        result = cpu_divu_quotient_get();
        result <<= 12;

        return result;
}

static void
_sprite_init(int id, int x, int y, int w, int h)
{
        const vdp1_cmdt_draw_mode_t draw_mode = {
                .bits.trans_pixel_disable = true,
                .bits.pre_clipping_disable = true,
                .bits.end_code_disable = true
        };

        const vdp1_cmdt_color_bank_t color_bank = {
                .type_0.data.dc = 0x0100
        };
        int16_vec2_t _position;
        _position.x = x-w/2;
        _position.y = y-h/2;
        _sprite.cmdt = &_cmdt_list->cmdts[id];

        _sprite.tex_base = _vdp1_vram_partitions.texture_base;
        _sprite.pal_base = (void *)VDP2_CRAM_MODE_1_OFFSET(1, 0, 0x0000);

        vdp1_cmdt_normal_sprite_set(_sprite.cmdt);
        vdp1_cmdt_param_char_base_set(_sprite.cmdt, (uint32_t)_sprite.tex_base);
        vdp1_cmdt_param_vertex_set(_sprite.cmdt, CMDT_VTX_NORMAL_SPRITE, &_position);
        vdp1_cmdt_param_size_set(_sprite.cmdt, w, h);



        vdp1_cmdt_param_draw_mode_set(_sprite.cmdt, draw_mode);
        vdp1_cmdt_param_color_mode4_set(_sprite.cmdt, color_bank);


        void *fh[2];
        void *p;
        size_t len;

        fh[0] = romdisk_open(_romdisk, ZOOM_TEX_PATH);
        assert(fh[0] != NULL);
        p = romdisk_direct(fh[0]);
        len = romdisk_total(fh[0]);
        scu_dma_transfer(0, _sprite.tex_base, p, len);

        fh[1] = romdisk_open(_romdisk, ZOOM_PAL_PATH);
        assert(fh[1] != NULL);
        p = romdisk_direct(fh[1]);
        len = romdisk_total(fh[1]);
        scu_dma_transfer(0, _sprite.pal_base, p, len);

        romdisk_close(fh[0]);
        romdisk_close(fh[1]);
}

static void
_cpu_frt_ovi_handler(void)
{
}
