/*
 * Copyright (c) 2012-2017 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdio.h>
#include <stdlib.h>

#define CPU_FRT_INTERRUPT_PRIORITY_LEVEL 8

#define TIMER_MAX_TIMERS_COUNT 16

#define MS_VALUE (CPU_FRT_PAL_320_128_COUNT_1MS)
#define FRT_MS_NUMBER (200)

#define NUMBER_OF_TESTS 12

struct timer;

struct timer_event {
        uint32_t id;

        const struct timer *timer;
        int32_t step;

        uint32_t next_interval;
} __packed;

struct timer {
        uint32_t interval; /* Time in milliseconds */
        void (*callback)(struct timer_event *);
        void (*callbackStep)(struct timer_event *);
} __packed;

struct timer_state {
        bool valid;
        uint32_t id;
        struct timer event;
        uint32_t remaining;
} __packed;

static struct timer_state _timer_states[TIMER_MAX_TIMERS_COUNT];

static uint32_t _next_timer = 0;
static uint32_t _id_count = 0;

static void _timer_init(void);
static int32_t _timer_add(const struct timer *);
static int32_t _timer_remove(uint32_t) __unused;

/* Master */
static void _frt_compare_output_handler(void);
static void _timer_handler(struct timer_event *);
static void _timer_step_handler(struct timer_event *);

static volatile uint32_t counter [NUMBER_OF_TESTS] = {0};

static uint8_t val8;
static uint16_t val16;
static uint32_t val32;

static volatile uint32_t testId = 0;

typedef uint32_t (*testFunc) (void);

typedef struct {
  char name[20];
  testFunc func;
  volatile uint32_t* counter;
} testsuite_t;

static uint32_t testHighWRamByteWrite() {
  *((volatile uint8_t *)(0x26010100)) = 0xDE;
  *((volatile uint8_t *)(0x26010100)) = 0xDE;
  *((volatile uint8_t *)(0x26010100)) = 0xDE;
  *((volatile uint8_t *)(0x26010100)) = 0xDE;
  *((volatile uint8_t *)(0x26010100)) = 0xDE;
  *((volatile uint8_t *)(0x26010100)) = 0xDE;
  *((volatile uint8_t *)(0x26010100)) = 0xDE;
  *((volatile uint8_t *)(0x26010100)) = 0xDE;
  *((volatile uint8_t *)(0x26010100)) = 0xDE;
  *((volatile uint8_t *)(0x26010100)) = 0xDE;
  return 10;
}

static uint32_t testHighWRamWordWrite() {
  *((volatile uint16_t *)(0x26010100)) = 0xDE;
  *((volatile uint16_t *)(0x26010100)) = 0xDE;
  *((volatile uint16_t *)(0x26010100)) = 0xDE;
  *((volatile uint16_t *)(0x26010100)) = 0xDE;
  *((volatile uint16_t *)(0x26010100)) = 0xDE;
  *((volatile uint16_t *)(0x26010100)) = 0xDE;
  *((volatile uint16_t *)(0x26010100)) = 0xDE;
  *((volatile uint16_t *)(0x26010100)) = 0xDE;
  *((volatile uint16_t *)(0x26010100)) = 0xDE;
  *((volatile uint16_t *)(0x26010100)) = 0xDE;
  return 10;
}

static uint32_t testHighWRamLongWrite() {
  *((volatile uint32_t *)(0x26010100)) = 0xDE;
  *((volatile uint32_t *)(0x26010100)) = 0xDE;
  *((volatile uint32_t *)(0x26010100)) = 0xDE;
  *((volatile uint32_t *)(0x26010100)) = 0xDE;
  *((volatile uint32_t *)(0x26010100)) = 0xDE;
  *((volatile uint32_t *)(0x26010100)) = 0xDE;
  *((volatile uint32_t *)(0x26010100)) = 0xDE;
  *((volatile uint32_t *)(0x26010100)) = 0xDE;
  *((volatile uint32_t *)(0x26010100)) = 0xDE;
  *((volatile uint32_t *)(0x26010100)) = 0xDE;
  return 10;
}

static uint32_t testHighWRamByteRead() {
  val8 = *((volatile uint8_t *)(0x26010100));
  val8 = *((volatile uint8_t *)(0x26010100));
  val8 = *((volatile uint8_t *)(0x26010100));
  val8 = *((volatile uint8_t *)(0x26010100));
  val8 = *((volatile uint8_t *)(0x26010100));
  val8 = *((volatile uint8_t *)(0x26010100));
  val8 = *((volatile uint8_t *)(0x26010100));
  val8 = *((volatile uint8_t *)(0x26010100));
  val8 = *((volatile uint8_t *)(0x26010100));
  val8 = *((volatile uint8_t *)(0x26010100));
  return 10;
}

static uint32_t testHighWRamWordRead() {
  val16 = *((volatile uint16_t *)(0x26010100));
  val16 = *((volatile uint16_t *)(0x26010100));
  val16 = *((volatile uint16_t *)(0x26010100));
  val16 = *((volatile uint16_t *)(0x26010100));
  val16 = *((volatile uint16_t *)(0x26010100));
  val16 = *((volatile uint16_t *)(0x26010100));
  val16 = *((volatile uint16_t *)(0x26010100));
  val16 = *((volatile uint16_t *)(0x26010100));
  val16 = *((volatile uint16_t *)(0x26010100));
  val16 = *((volatile uint16_t *)(0x26010100));
  return 10;
}


static uint32_t testHighWRamLongRead() {
  val32 = *((volatile uint32_t *)(0x26010100));
  val32 = *((volatile uint32_t *)(0x26010100));
  val32 = *((volatile uint32_t *)(0x26010100));
  val32 = *((volatile uint32_t *)(0x26010100));
  val32 = *((volatile uint32_t *)(0x26010100));
  val32 = *((volatile uint32_t *)(0x26010100));
  val32 = *((volatile uint32_t *)(0x26010100));
  val32 = *((volatile uint32_t *)(0x26010100));
  val32 = *((volatile uint32_t *)(0x26010100));
  val32 = *((volatile uint32_t *)(0x26010100));
  return 10;
}


static uint32_t testLowWRamByteWrite(void){
  *((volatile uint8_t *)(0x20240100)) = 0xDE;
  *((volatile uint8_t *)(0x20240100)) = 0xDE;
  *((volatile uint8_t *)(0x20240100)) = 0xDE;
  *((volatile uint8_t *)(0x20240100)) = 0xDE;
  *((volatile uint8_t *)(0x20240100)) = 0xDE;
  *((volatile uint8_t *)(0x20240100)) = 0xDE;
  *((volatile uint8_t *)(0x20240100)) = 0xDE;
  *((volatile uint8_t *)(0x20240100)) = 0xDE;
  *((volatile uint8_t *)(0x20240100)) = 0xDE;
  *((volatile uint8_t *)(0x20240100)) = 0xDE;
  return 10;
}

static uint32_t testLowWRamWordWrite(void){
  *((volatile uint16_t *)(0x20240100)) = 0xDE;
  *((volatile uint16_t *)(0x20240100)) = 0xDE;
  *((volatile uint16_t *)(0x20240100)) = 0xDE;
  *((volatile uint16_t *)(0x20240100)) = 0xDE;
  *((volatile uint16_t *)(0x20240100)) = 0xDE;
  *((volatile uint16_t *)(0x20240100)) = 0xDE;
  *((volatile uint16_t *)(0x20240100)) = 0xDE;
  *((volatile uint16_t *)(0x20240100)) = 0xDE;
  *((volatile uint16_t *)(0x20240100)) = 0xDE;
  *((volatile uint16_t *)(0x20240100)) = 0xDE;
  return 10;
}

static uint32_t testLowWRamLongWrite(void){
  *((volatile uint32_t *)(0x20240100)) = 0xDE;
  *((volatile uint32_t *)(0x20240100)) = 0xDE;
  *((volatile uint32_t *)(0x20240100)) = 0xDE;
  *((volatile uint32_t *)(0x20240100)) = 0xDE;
  *((volatile uint32_t *)(0x20240100)) = 0xDE;
  *((volatile uint32_t *)(0x20240100)) = 0xDE;
  *((volatile uint32_t *)(0x20240100)) = 0xDE;
  *((volatile uint32_t *)(0x20240100)) = 0xDE;
  *((volatile uint32_t *)(0x20240100)) = 0xDE;
  *((volatile uint32_t *)(0x20240100)) = 0xDE;
  return 10;
}

static uint32_t testLowWRamByteRead() {
  val8 = *((volatile uint8_t *)(0x20240100));
  val8 = *((volatile uint8_t *)(0x20240100));
  val8 = *((volatile uint8_t *)(0x20240100));
  val8 = *((volatile uint8_t *)(0x20240100));
  val8 = *((volatile uint8_t *)(0x20240100));
  val8 = *((volatile uint8_t *)(0x20240100));
  val8 = *((volatile uint8_t *)(0x20240100));
  val8 = *((volatile uint8_t *)(0x20240100));
  val8 = *((volatile uint8_t *)(0x20240100));
  val8 = *((volatile uint8_t *)(0x20240100));
  return 10;
}

static uint32_t testLowWRamWordRead() {
  val16 = *((volatile uint16_t *)(0x20240100));
  val16 = *((volatile uint16_t *)(0x20240100));
  val16 = *((volatile uint16_t *)(0x20240100));
  val16 = *((volatile uint16_t *)(0x20240100));
  val16 = *((volatile uint16_t *)(0x20240100));
  val16 = *((volatile uint16_t *)(0x20240100));
  val16 = *((volatile uint16_t *)(0x20240100));
  val16 = *((volatile uint16_t *)(0x20240100));
  val16 = *((volatile uint16_t *)(0x20240100));
  val16 = *((volatile uint16_t *)(0x20240100));
  return 10;
}


static uint32_t testLowWRamLongRead() {
  val32 = *((volatile uint32_t *)(0x20240100));
  val32 = *((volatile uint32_t *)(0x20240100));
  val32 = *((volatile uint32_t *)(0x20240100));
  val32 = *((volatile uint32_t *)(0x20240100));
  val32 = *((volatile uint32_t *)(0x20240100));
  val32 = *((volatile uint32_t *)(0x20240100));
  val32 = *((volatile uint32_t *)(0x20240100));
  val32 = *((volatile uint32_t *)(0x20240100));
  val32 = *((volatile uint32_t *)(0x20240100));
  val32 = *((volatile uint32_t *)(0x20240100));
  return 10;
}

testsuite_t tests[NUMBER_OF_TESTS] = {
  {"HighRAM W Byte", testHighWRamByteWrite, &counter[0]},
  {"HighRAM W Word", testHighWRamWordWrite, &counter[1]},
  {"HighRAM W Long", testHighWRamLongWrite, &counter[2]},
  {"HighRAM R Byte", testHighWRamByteRead,  &counter[3]},
  {"HighRAM R Word", testHighWRamWordRead,  &counter[4]},
  {"HighRAM R Long", testHighWRamLongRead,  &counter[5]},
  {"LowRAM  W Byte", testLowWRamByteWrite,  &counter[6]},
  {"LowRAM  W Word", testLowWRamWordWrite,  &counter[7]},
  {"LowRAM  W Long", testLowWRamLongWrite,  &counter[8]},
  {"LowRAM  R Byte", testLowWRamByteRead,  &counter[9]},
  {"LowRAM  R Word", testLowWRamWordRead,  &counter[10]},
  {"LowRAM  R Long", testLowWRamLongRead,  &counter[11]},
};

volatile bool testing = false;

void
main(void)
{
        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();
        dbgio_dev_font_load_wait();

        _timer_init();

        struct timer match1 __unused = {
                .interval = 1000/FRT_MS_NUMBER,
                .callback = _timer_handler,
                .callbackStep = _timer_step_handler,
        };

        _timer_add(&match1);

        cpu_cache_purge();
        cpu_cache_enable();

        testId = 0;
        testing = true;

        while(true) {
          while (testing) {
            *tests[testId].counter += tests[testId].func();
          }
          dbgio_puts("[1;1H[2J");
          int start = (testId/10)*10;
          int end = (testId/10)*10 + 10;
          if (end > NUMBER_OF_TESTS) end = NUMBER_OF_TESTS;
          for (int i=start; i< end; i++)
          dbgio_printf("\n"
                      "%s : %lu access/s\n",
                       tests[i].name,
                       *tests[i].counter);
          dbgio_flush();
          testId++;
          testId %= NUMBER_OF_TESTS;
          *tests[testId].counter = 0;
          vdp_sync();
          cpu_frt_count_set(0);
          testing = true;
        }
}

void
user_init(void)
{
        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);

        vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            COLOR_RGB1555(1, 0, 3, 3));

        cpu_intc_mask_set(0);

        vdp2_tvmd_display_set();
}

static void
_frt_compare_output_handler(void)
{
        uint32_t frt_count;
        frt_count = cpu_frt_count_get();

        int32_t count_diff __unused;
        count_diff = frt_count - (FRT_MS_NUMBER * MS_VALUE);

        if (count_diff >= 0) {
                cpu_frt_count_set(count_diff);
        }

        uint32_t i;
        for (i = 0; i < TIMER_MAX_TIMERS_COUNT; i++) {
                struct timer_state *timer_state;
                struct timer_event event;
                timer_state = &_timer_states[i];

                event.id = timer_state->id;
                event.timer = &timer_state->event;
                event.next_interval = timer_state->event.interval;
                event.step = count_diff;

                /* Invalid timer */
                if (!timer_state->valid) {
                        continue;
                }

                timer_state->remaining--;

                timer_state->event.callbackStep (&event);

                if (timer_state->remaining != 0) {
                        continue;
                }



                timer_state->event.callback (&event);

                if (event.next_interval > 0) {
                        /* Choose the next non-zero interval */
                        timer_state->remaining = event.next_interval;

                        continue;
                }

                /* Invalidate the timer */
                timer_state->valid = false;
        }
}

static void
_timer_init(void)
{
        uint32_t timer;
        for (timer = 0; timer < TIMER_MAX_TIMERS_COUNT; timer++) {
                _timer_states[timer].valid = false;
        }

        cpu_frt_init(CPU_FRT_CLOCK_DIV_128);
        cpu_frt_oca_set((FRT_MS_NUMBER * MS_VALUE), _frt_compare_output_handler);
        /* Match every 9.525μs */
        cpu_frt_count_set(0);
        cpu_frt_interrupt_priority_set(CPU_FRT_INTERRUPT_PRIORITY_LEVEL);
}

static int32_t
_timer_add(const struct timer *timer)
{
        /* Disable interrupts */
        uint32_t i_mask;
        i_mask = cpu_intc_mask_get();

        cpu_intc_mask_set(15);

        if (timer->callback == NULL) {
                return -1;
        }

        if (timer->callbackStep == NULL) {
                return -1;
        }

        if (timer->interval == 0) {
                return -1;
        }

        struct timer_state *timer_state;
        timer_state = &_timer_states[_next_timer & 0x1F];

        if (timer_state->valid) {
                /* Look for a free timer */
                uint32_t timer;
                for (timer = 0; timer < TIMER_MAX_TIMERS_COUNT; timer++) {
                        timer_state = &_timer_states[timer];

                        if (!timer_state->valid) {
                                break;
                        }
                }

                _next_timer = timer;
        } else {
                _next_timer++;
        }

        if (_next_timer > TIMER_MAX_TIMERS_COUNT) {
                return -1;
        }

        timer_state->id = _id_count;
        memcpy(&timer_state->event, timer, sizeof(struct timer));
        timer_state->remaining = timer->interval;

        timer_state->valid = true;

        _id_count++;

        /* Enable interrupts */
        cpu_intc_mask_set(i_mask);

        return 0;
}

static int32_t
_timer_remove(uint32_t id)
{
        int32_t ret;

        /* Disable interrupts */
        uint32_t i_mask;
        i_mask = cpu_intc_mask_get();

        cpu_intc_mask_set(15);

        struct timer_state *timer_state;

        uint32_t timer;
        for (timer = 0; timer < TIMER_MAX_TIMERS_COUNT; timer++) {
                timer_state = &_timer_states[timer];

                if (timer_state->valid && (id == timer_state->id)) {
                        break;
                }
        }

        if (timer == TIMER_MAX_TIMERS_COUNT) {
                ret = -1;

                goto exit;
        }

        /* Point to next free timer */
        _next_timer = timer;

        timer_state->valid = false;

        ret = 0;

exit:
        cpu_intc_mask_set(i_mask);

        return ret;
}

static void
_timer_handler(struct timer_event *event __unused)
{
  testing = false;
}

static void _timer_step_handler(struct timer_event *event __unused)
{

}
