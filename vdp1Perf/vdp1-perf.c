/*
 * Copyright (c) 2012-2017 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdio.h>
#include <stdlib.h>

#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   224

#define STATE_IDLE              (-1)
#define STATE_TYPE_SET          (0)
#define STATE_DRAW_MODE_SET     (1)
#define STATE_VERTEX_SELECT     (2)
#define STATE_VERTEX_RESET      (3)

#define PRIMITIVE_TYPE_POLYLINE   (0)
#define PRIMITIVE_TYPE_POLYGON    (1)
#define PRIMITIVE_TYPE_COUNT      (2)

#define PRIMITIVE_DRAW_MODE_NORMAL                (0)
#define PRIMITIVE_DRAW_MODE_MESH                  (1)
#define PRIMITIVE_DRAW_MODE_SHADOW                (2)
#define PRIMITIVE_DRAW_MODE_HALF_LUMINANCE        (3)
#define PRIMITIVE_DRAW_MODE_HALF_TRANSPARENT      (4)
#define PRIMITIVE_DRAW_MODE_GOURAUD_SHADING       (5)
#define PRIMITIVE_DRAW_MODE_GOURAUD_HALF_LUM      (6)
#define PRIMITIVE_DRAW_MODE_GOURAUD_HALF_TRANS    (7)
#define PRIMITIVE_DRAW_MODE_COUNT                 (8)

#define PRIMITIVE_WIDTH           64
#define PRIMITIVE_HEIGHT          64
#define PRIMITIVE_HALF_WIDTH      (PRIMITIVE_WIDTH / 2)
#define PRIMITIVE_HALF_HEIGHT     (PRIMITIVE_HEIGHT / 2)

#define PRIMITIVE_COLOR           COLOR_RGB1555(1, 31, 0, 31)

#define MS_VALUE (CPU_FRT_PAL_320_8_COUNT_1MS)

#define NB_CMD (1<<8)

#define ORDER_SYSTEM_CLIP_COORDS_INDEX  0
#define ORDER_LOCAL_COORDS_INDEX        1
#define ORDER_POLYGON_INDEX             2
#define ORDER_DRAW_END_INDEX            (NB_CMD+ORDER_POLYGON_INDEX)
#define ORDER_COUNT                     (ORDER_DRAW_END_INDEX+1)

static vdp1_cmdt_list_t *_cmdt_list = NULL;
static vdp1_vram_partitions_t _vdp1_vram_partitions;

static struct {
        int8_t type;
        int8_t draw_mode;
        color_rgb1555_t color;
        int16_vec2_t points[4];
} _primitive;

static vdp1_cmdt_draw_mode_t _primitive_draw_modes[] = {
        {
                .raw = 0x0000
        },
        {
                .raw = 0x0000,
                .bits.mesh_enable = true
        },
        {
                .raw = 0x0000,
                .bits.cc_mode = 1
        },
        {
                .raw = 0x0000,
                .bits.cc_mode = 2
        },
        {
                .raw = 0x0000,
                .bits.cc_mode = 3
        },
        {
                .raw = 0x0000,
                .bits.cc_mode = 4
        },
        {
                .raw = 0x0000,
                .bits.cc_mode = 6
        },
        {
                .raw = 0x0000,
                .bits.cc_mode = 7
        }
};

// static char *_primitive_type_strings[] = {
//         "POLYLINE",
//         "POLYGON"
// };
//
// static char *_primitive_draw_mode_strings[] = {
//         "NORMAL",
//         "MESH",
//         "SHADOW",
//         "HALF-LUMINANCE",
//         "REPLACE/HALF-TRANSPARENT",
//         "GOURAUD",
//         "GOURAUD+HALF-LUMINANCE",
//         "GOURAUD+HALF-TRANSPARENT"
// };

static void _cmdt_list_init(void);
static void _primitive_init(void);

static volatile uint16_t _frt_ovf_count = 0;

static void
_frt_ovi_handler(void)
{
        _frt_ovf_count++;
}

static void
_timing_start(void)
{
        cpu_frt_count_set(0);
        _frt_ovf_count = 0;
}

static fix16_t
_timing_get(void)
{
        const uint32_t ticks_rem = cpu_frt_count_get();

        const uint32_t overflow_count =
            ((65536 / MS_VALUE) * _frt_ovf_count);

        const uint32_t total_ticks =
            (fix16_int32_from(overflow_count) + (fix16_int32_from(ticks_rem) / MS_VALUE));

        return total_ticks;
}

void
main(void)
{
        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();

        _cmdt_list_init();
        _primitive_init();
        cpu_frt_init(CPU_FRT_CLOCK_DIV_8);
        cpu_frt_ovi_set(_frt_ovi_handler);
        while(true) {
          _timing_start();
          vdp1_sync_cmdt_list_put(_cmdt_list, 0);
          fix16_t frt_count = _timing_get();
          vdp1_sync_render();
          vdp1_sync();
          while(vdp1_cmdt_current_get() != ORDER_DRAW_END_INDEX) {}
          frt_count = _timing_get() - frt_count;
          vdp1_sync_wait();
          dbgio_puts("[1;1H[2J");
          dbgio_printf("\n""0x%x %x Done\n", NB_CMD, frt_count);
          dbgio_flush();
          vdp2_sync();
          vdp2_sync_wait();
        }
}

void
user_init(void)
{
        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);
        vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            COLOR_RGB1555(1, 0, 3, 15));
        vdp2_sprite_priority_set(0, 6);

        vdp1_env_t env;
        vdp1_env_default_init(&env);

        env.erase_color = COLOR_RGB1555(1, 0, 3, 15);

        vdp1_env_set(&env);

        cpu_intc_mask_set(0);

        vdp2_tvmd_display_set();

        vdp1_vram_partitions_get(&_vdp1_vram_partitions);

}

static void
_cmdt_list_init(void)
{
        static const int16_vec2_t system_clip_coord =
            INT16_VEC2_INITIALIZER(SCREEN_WIDTH - 1,
                                      SCREEN_HEIGHT - 1);

        _cmdt_list = vdp1_cmdt_list_alloc(ORDER_COUNT);

        (void)memset(&_cmdt_list->cmdts[0], 0x00,
            sizeof(vdp1_cmdt_t) * ORDER_COUNT);

        _cmdt_list->count = ORDER_COUNT;

        vdp1_cmdt_t *cmdts;
        cmdts = &_cmdt_list->cmdts[0];

        vdp1_cmdt_system_clip_coord_set(&cmdts[ORDER_SYSTEM_CLIP_COORDS_INDEX]);
        vdp1_cmdt_param_vertex_set(&cmdts[ORDER_SYSTEM_CLIP_COORDS_INDEX],
            CMDT_VTX_SYSTEM_CLIP,
            &system_clip_coord);

        vdp1_cmdt_end_set(&cmdts[ORDER_DRAW_END_INDEX]);
}

static void
_primitive_init(void)
{
        static const int16_vec2_t local_coord_center =
            INT16_VEC2_INITIALIZER((SCREEN_WIDTH / 2) - PRIMITIVE_HALF_WIDTH - 1,
                                      (SCREEN_HEIGHT / 2) - PRIMITIVE_HALF_HEIGHT - 1);

        vdp1_cmdt_t *cmdt_local_coords;
        cmdt_local_coords = &_cmdt_list->cmdts[ORDER_LOCAL_COORDS_INDEX];

        vdp1_cmdt_local_coord_set(cmdt_local_coords);
        vdp1_cmdt_param_vertex_set(cmdt_local_coords, CMDT_VTX_LOCAL_COORD,
            &local_coord_center);


        vdp1_gouraud_table_t *gouraud_base;
        gouraud_base = _vdp1_vram_partitions.gouraud_base;

        gouraud_base->colors[0] = COLOR_RGB1555(1, 31,  0,  0);
        gouraud_base->colors[1] = COLOR_RGB1555(1,  0, 31,  0);
        gouraud_base->colors[2] = COLOR_RGB1555(1,  0,  0, 31);
        gouraud_base->colors[3] = COLOR_RGB1555(1, 31, 31, 31);

        for (int i = 0; i<NB_CMD; i++){
          vdp1_cmdt_t *cmdt_polygon;
          cmdt_polygon = &_cmdt_list->cmdts[ORDER_POLYGON_INDEX+i];

          _primitive.type = PRIMITIVE_TYPE_POLYLINE;
          _primitive.draw_mode = PRIMITIVE_DRAW_MODE_NORMAL;

          _primitive.color = COLOR_RGB1555(1, (31+i)%32, (0+i)%32, (31+i)%32);

          _primitive.points[0].x = (i - NB_CMD/2);
          _primitive.points[0].y = PRIMITIVE_HEIGHT - 1 + (i - NB_CMD/2);

          _primitive.points[1].x = PRIMITIVE_WIDTH - 1 + (i - NB_CMD/2);
          _primitive.points[1].y = PRIMITIVE_HEIGHT - 1 + (i - NB_CMD/2);

          _primitive.points[2].x = PRIMITIVE_WIDTH - 1 + (i - NB_CMD/2);
          _primitive.points[2].y = (i - NB_CMD/2);

          _primitive.points[3].x = (i - NB_CMD/2);
          _primitive.points[3].y = (i - NB_CMD/2);

          vdp1_cmdt_param_color_set(cmdt_polygon, _primitive.color);
          vdp1_cmdt_param_draw_mode_set(cmdt_polygon, _primitive_draw_modes[_primitive.draw_mode]);
          vdp1_cmdt_param_vertices_set(cmdt_polygon, &_primitive.points[0]);
          vdp1_cmdt_param_gouraud_base_set(cmdt_polygon, (uint32_t)gouraud_base);
          vdp1_cmdt_polygon_set(cmdt_polygon);
        }
}