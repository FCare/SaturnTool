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

#define VDP1_CMDT_ORDER_SYSTEM_CLIP_COORDS_INDEX        0
#define VDP1_CMDT_ORDER_CLEAR_LOCAL_COORDS_INDEX        1
#define VDP1_CMDT_ORDER_CLEAR_POLYGON_INDEX             2
#define VDP1_CMDT_ORDER_LOCAL_COORDS_INDEX              3
#define VDP1_CMDT_ORDER_POLYGON_INDEX                   4
#define VDP1_CMDT_ORDER_LINE_POINTER_INDEX              (VDP1_CMDT_ORDER_POLYGON_INDEX+8)
#define VDP1_CMDT_ORDER_DRAW_END_INDEX                  (VDP1_CMDT_ORDER_LINE_POINTER_INDEX+8)
#define VDP1_CMDT_ORDER_COUNT                           (VDP1_CMDT_ORDER_DRAW_END_INDEX+1)

vdp1_cmdt_t* polygons[8];
vdp1_cmdt_t* lines[8];

static vdp1_cmdt_list_t *_cmdt_list = NULL;
static vdp1_vram_partitions_t _vdp1_vram_partitions;

static void _init(void);

static void _cmdt_list_init(void);

static void _polygon_pointer_init(void);
static void _polygon_pointer_config(void);

int
main(void)
{
        _init();

        _polygon_pointer_config();

        while (true) {

                _polygon_pointer_config();

                vdp1_sync_cmdt_list_put(_cmdt_list, 0);

                vdp1_sync_render();

                vdp1_sync();
                vdp1_sync_wait();

        }

        return 0;
}

void
user_init(void)
{
        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_240);

        vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            RGB1555(1, 0, 3, 15));

        vdp2_sprite_priority_set(0, 6);

        /* Setup default VDP1 environment */
        vdp1_env_default_set();

        vdp2_tvmd_display_set();

        vdp1_vram_partitions_get(&_vdp1_vram_partitions);

        vdp2_sync();
        vdp2_sync_wait();
}

static void
_init(void)
{
        _cmdt_list_init();
}

static void
_cmdt_list_init(void)
{
        static const int16_vec2_t system_clip_coord =
            INT16_VEC2_INITIALIZER(SCREEN_WIDTH - 1,
                                   SCREEN_HEIGHT - 1);

        static const int16_vec2_t local_coord_ul =
            INT16_VEC2_INITIALIZER(0, 0);

        static const int16_vec2_t local_coord_center =
            INT16_VEC2_INITIALIZER(SCREEN_WIDTH / 2,
                                   SCREEN_HEIGHT / 2);

        static const vdp1_cmdt_draw_mode_t polygon_draw_mode = {
                .pre_clipping_disable = true
        };

        static const int16_vec2_t polygon_points[] = {
                INT16_VEC2_INITIALIZER(               0, SCREEN_HEIGHT - 1),
                INT16_VEC2_INITIALIZER(SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1),
                INT16_VEC2_INITIALIZER(SCREEN_WIDTH - 1,                 0),
                INT16_VEC2_INITIALIZER(               0,                 0)
        };

        _cmdt_list = vdp1_cmdt_list_alloc(VDP1_CMDT_ORDER_COUNT);

        (void)memset(&_cmdt_list->cmdts[0], 0x00, sizeof(vdp1_cmdt_t) * VDP1_CMDT_ORDER_COUNT);

        _cmdt_list->count = VDP1_CMDT_ORDER_COUNT;

        vdp1_cmdt_t * const cmdts =
            &_cmdt_list->cmdts[0];

        for (int i = 0; i<8; i++) {
          polygons[i] = &cmdts[VDP1_CMDT_ORDER_POLYGON_INDEX+i];
        }
        for (int i = 0; i<8; i++) {
          lines[i] = &cmdts[VDP1_CMDT_ORDER_LINE_POINTER_INDEX+i];
        }

        _polygon_pointer_init();

        vdp1_cmdt_system_clip_coord_set(&cmdts[VDP1_CMDT_ORDER_SYSTEM_CLIP_COORDS_INDEX]);
        vdp1_cmdt_vtx_system_clip_coord_set(&cmdts[VDP1_CMDT_ORDER_SYSTEM_CLIP_COORDS_INDEX],
            system_clip_coord);

        vdp1_cmdt_local_coord_set(&cmdts[VDP1_CMDT_ORDER_CLEAR_LOCAL_COORDS_INDEX]);
        vdp1_cmdt_vtx_local_coord_set(&cmdts[VDP1_CMDT_ORDER_CLEAR_LOCAL_COORDS_INDEX],
            local_coord_ul);

        vdp1_cmdt_polygon_set(&cmdts[VDP1_CMDT_ORDER_CLEAR_POLYGON_INDEX]);
        vdp1_cmdt_draw_mode_set(&cmdts[VDP1_CMDT_ORDER_CLEAR_POLYGON_INDEX],
            polygon_draw_mode);
        vdp1_cmdt_vtx_set(&cmdts[VDP1_CMDT_ORDER_CLEAR_POLYGON_INDEX],
            polygon_points);
        vdp1_cmdt_jump_skip_next(&cmdts[VDP1_CMDT_ORDER_CLEAR_POLYGON_INDEX]);

        vdp1_cmdt_local_coord_set(&cmdts[VDP1_CMDT_ORDER_LOCAL_COORDS_INDEX]);
        vdp1_cmdt_vtx_local_coord_set(&cmdts[VDP1_CMDT_ORDER_LOCAL_COORDS_INDEX],
            local_coord_ul);

        vdp1_cmdt_end_set(&cmdts[VDP1_CMDT_ORDER_DRAW_END_INDEX]);
}


static void
_polygon_pointer_init(void)
{
        static const vdp1_cmdt_draw_mode_t draw_mode = {
                .pre_clipping_disable = true
        };

        for (int i=0; i<8; i++) {
          vdp1_cmdt_polygon_set(polygons[i]);
          vdp1_cmdt_draw_mode_set(polygons[i], draw_mode);
        }
        for (int i=0; i<8; i++) {
          vdp1_cmdt_line_set(lines[i]);
          vdp1_cmdt_draw_mode_set(lines[i], draw_mode);
        }
}

static void
_polygon_pointer_config(void)
{
    int16_vec2_t points[4];

    points[0].x = 10;
    points[1].x = 9;
    points[2].x = 9;
    points[3].x = 10;
    points[0].y = 10;
    points[1].y = 14;
    points[2].y = 14;
    points[3].y = 10;

    vdp1_cmdt_vtx_set(polygons[0], points);
    vdp1_cmdt_color_set(polygons[0], RGB1555(1,0x1F,0x1F,0x1F));

    points[0].x = 10 + SCREEN_WIDTH/4;
    points[1].x = 9 + SCREEN_WIDTH/4;
    points[0].y = 10;
    points[1].y = 14;
    vdp1_cmdt_vtx_set(lines[0], points);
    vdp1_cmdt_color_set(lines[0], RGB1555(1,0x1F,0x1F,0x1F));

    points[0].x = 9;
    points[1].x = 10;
    points[2].x = 10;
    points[3].x = 9;
    points[0].y = 10 + (SCREEN_HEIGHT/4);
    points[1].y = 14 + (SCREEN_HEIGHT/4);
    points[2].y = 14 + (SCREEN_HEIGHT/4);
    points[3].y = 10 + (SCREEN_HEIGHT/4);

    vdp1_cmdt_vtx_set(polygons[1], points);
    vdp1_cmdt_color_set(polygons[1], RGB1555(1,0x1F,0x1F,0x1F));

    points[0].x = 9 + SCREEN_WIDTH/4;
    points[1].x = 10 + SCREEN_WIDTH/4;
    points[2].x = 10 + SCREEN_WIDTH/4;
    points[3].x = 9 + SCREEN_WIDTH/4;
    points[0].y = 10 + (SCREEN_HEIGHT/4);
    points[1].y = 14 + (SCREEN_HEIGHT/4);
    points[2].y = 14 + (SCREEN_HEIGHT/4);
    points[3].y = 10 + (SCREEN_HEIGHT/4);

    vdp1_cmdt_vtx_set(lines[1], points);
    vdp1_cmdt_color_set(lines[1], RGB1555(1,0x1F,0x1F,0x1F));

    points[0].x = 9;
    points[1].x = 10;
    points[2].x = 10;
    points[3].x = 9;
    points[0].y = 14 + ((2*SCREEN_HEIGHT)/4);
    points[1].y = 10 + ((2*SCREEN_HEIGHT)/4);
    points[2].y = 10 + ((2*SCREEN_HEIGHT)/4);
    points[3].y = 14 + ((2*SCREEN_HEIGHT)/4);

    vdp1_cmdt_vtx_set(polygons[2], points);
    vdp1_cmdt_color_set(polygons[2], RGB1555(1,0x1F,0x1F,0x1F));

    points[0].x = 9 + SCREEN_WIDTH/4;
    points[1].x = 10 + SCREEN_WIDTH/4;
    points[2].x = 10 + SCREEN_WIDTH/4;
    points[3].x = 9 + SCREEN_WIDTH/4;
    points[0].y = 14 + ((2*SCREEN_HEIGHT)/4);
    points[1].y = 10 + ((2*SCREEN_HEIGHT)/4);
    points[2].y = 10 + ((2*SCREEN_HEIGHT)/4);
    points[3].y = 14 + ((2*SCREEN_HEIGHT)/4);

    vdp1_cmdt_vtx_set(lines[2], points);
    vdp1_cmdt_color_set(lines[2], RGB1555(1,0x1F,0x1F,0x1F));

    points[0].x = 10;
    points[1].x = 9;
    points[2].x = 9;
    points[3].x = 10;
    points[0].y = 14 + ((3*SCREEN_HEIGHT)/4);
    points[1].y = 10 + ((3*SCREEN_HEIGHT)/4);
    points[2].y = 10 + ((3*SCREEN_HEIGHT)/4);
    points[3].y = 14 + ((3*SCREEN_HEIGHT)/4);

    vdp1_cmdt_vtx_set(polygons[3], points);
    vdp1_cmdt_color_set(polygons[3], RGB1555(1,0x1F,0x1F,0x1F));

    points[0].x = 10 + SCREEN_WIDTH/4;
    points[1].x = 9 + SCREEN_WIDTH/4;
    points[2].x = 9 + SCREEN_WIDTH/4;
    points[3].x = 10 + SCREEN_WIDTH/4;
    points[0].y = 14 + ((3*SCREEN_HEIGHT)/4);
    points[1].y = 10 + ((3*SCREEN_HEIGHT)/4);
    points[2].y = 10 + ((3*SCREEN_HEIGHT)/4);
    points[3].y = 14 + ((3*SCREEN_HEIGHT)/4);

    vdp1_cmdt_vtx_set(lines[3], points);
    vdp1_cmdt_color_set(lines[3], RGB1555(1,0x1F,0x1F,0x1F));

    points[0].x = 10 + SCREEN_WIDTH/2;
    points[1].x = 14 + SCREEN_WIDTH/2;
    points[2].x = 14 + SCREEN_WIDTH/2;
    points[3].x = 10 + SCREEN_WIDTH/2;
    points[0].y = 10;
    points[1].y = 9;
    points[2].y = 9;
    points[3].y = 10;

    vdp1_cmdt_vtx_set(polygons[4], points);
    vdp1_cmdt_color_set(polygons[4], RGB1555(1,0x1F,0x1F,0x1F));

    points[0].x = 10 + SCREEN_WIDTH/4 + SCREEN_WIDTH/2;
    points[1].x = 14 + SCREEN_WIDTH/4 + SCREEN_WIDTH/2;
    points[2].x = 14 + SCREEN_WIDTH/4 + SCREEN_WIDTH/2;
    points[3].x = 10 + SCREEN_WIDTH/4 + SCREEN_WIDTH/2;
    points[0].y = 10;
    points[1].y = 9;
    points[2].y = 9;
    points[3].y = 10;

    vdp1_cmdt_vtx_set(lines[4], points);
    vdp1_cmdt_color_set(lines[4], RGB1555(1,0x1F,0x1F,0x1F));

    points[0].x = 10 + SCREEN_WIDTH/2;
    points[1].x = 14 + SCREEN_WIDTH/2;
    points[2].x = 14 + SCREEN_WIDTH/2;
    points[3].x = 10 + SCREEN_WIDTH/2;
    points[0].y = 9 + (SCREEN_HEIGHT/4);
    points[1].y = 10 + (SCREEN_HEIGHT/4);
    points[2].y = 10 + (SCREEN_HEIGHT/4);
    points[3].y = 9 + (SCREEN_HEIGHT/4);

    vdp1_cmdt_vtx_set(polygons[5], points);
    vdp1_cmdt_color_set(polygons[5], RGB1555(1,0x1F,0x1F,0x1F));

    points[0].x = 10 + SCREEN_WIDTH/4 + SCREEN_WIDTH/2;
    points[1].x = 14 + SCREEN_WIDTH/4 + SCREEN_WIDTH/2;
    points[2].x = 14 + SCREEN_WIDTH/4 + SCREEN_WIDTH/2;
    points[3].x = 10 + SCREEN_WIDTH/4 + SCREEN_WIDTH/2;
    points[0].y = 9 + (SCREEN_HEIGHT/4);
    points[1].y = 10 + (SCREEN_HEIGHT/4);
    points[2].y = 10 + (SCREEN_HEIGHT/4);
    points[3].y = 9 + (SCREEN_HEIGHT/4);

    vdp1_cmdt_vtx_set(lines[5], points);
    vdp1_cmdt_color_set(lines[5], RGB1555(1,0x1F,0x1F,0x1F));

    points[0].x = 14 + SCREEN_WIDTH/2;
    points[1].x = 10 + SCREEN_WIDTH/2;
    points[2].x = 10 + SCREEN_WIDTH/2;
    points[3].x = 14 + SCREEN_WIDTH/2;
    points[0].y = 9 + ((2*SCREEN_HEIGHT)/4);
    points[1].y = 10 + ((2*SCREEN_HEIGHT)/4);
    points[2].y = 10 + ((2*SCREEN_HEIGHT)/4);
    points[3].y = 9 + ((2*SCREEN_HEIGHT)/4);

    vdp1_cmdt_vtx_set(polygons[6], points);
    vdp1_cmdt_color_set(polygons[6], RGB1555(1,0x1F,0x1F,0x1F));

    points[0].x = 14 + SCREEN_WIDTH/4 + SCREEN_WIDTH/2;
    points[1].x = 10 + SCREEN_WIDTH/4 + SCREEN_WIDTH/2;
    points[2].x = 10 + SCREEN_WIDTH/4 + SCREEN_WIDTH/2;
    points[3].x = 14 + SCREEN_WIDTH/4 + SCREEN_WIDTH/2;
    points[0].y = 9 + ((2*SCREEN_HEIGHT)/4);
    points[1].y = 10 + ((2*SCREEN_HEIGHT)/4);
    points[2].y = 10 + ((2*SCREEN_HEIGHT)/4);
    points[3].y = 9 + ((2*SCREEN_HEIGHT)/4);

    vdp1_cmdt_vtx_set(lines[6], points);
    vdp1_cmdt_color_set(lines[6], RGB1555(1,0x1F,0x1F,0x1F));

    points[0].x = 14 + SCREEN_WIDTH/2;
    points[1].x = 10 + SCREEN_WIDTH/2;
    points[2].x = 10 + SCREEN_WIDTH/2;
    points[3].x = 14 + SCREEN_WIDTH/2;
    points[0].y = 10 + ((3*SCREEN_HEIGHT)/4);
    points[1].y = 9 + ((3*SCREEN_HEIGHT)/4);
    points[2].y = 9 + ((3*SCREEN_HEIGHT)/4);
    points[3].y = 10 + ((3*SCREEN_HEIGHT)/4);

    vdp1_cmdt_vtx_set(polygons[7], points);
    vdp1_cmdt_color_set(polygons[7], RGB1555(1,0x1F,0x1F,0x1F));

    points[0].x = 14 + SCREEN_WIDTH/4 + SCREEN_WIDTH/2;
    points[1].x = 10 + SCREEN_WIDTH/4 + SCREEN_WIDTH/2;
    points[2].x = 10 + SCREEN_WIDTH/4 + SCREEN_WIDTH/2;
    points[3].x = 14 + SCREEN_WIDTH/4 + SCREEN_WIDTH/2;
    points[0].y = 10 + ((3*SCREEN_HEIGHT)/4);
    points[1].y = 9 + ((3*SCREEN_HEIGHT)/4);
    points[2].y = 9 + ((3*SCREEN_HEIGHT)/4);
    points[3].y = 10 + ((3*SCREEN_HEIGHT)/4);

    vdp1_cmdt_vtx_set(lines[7], points);
    vdp1_cmdt_color_set(lines[7], RGB1555(1,0x1F,0x1F,0x1F));
}

static void
_vblank_out_handler(void *work __unused)
{
}

static void
_cpu_frt_ovi_handler(void)
{
}
