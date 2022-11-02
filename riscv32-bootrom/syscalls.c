// See LICENSE for license details.

#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/signal.h>

#include "coremark.h"
#include "util.h"

#define SYS_write 64

#undef strcmp

// extern volatile uint64_t tohost;
// extern volatile uint64_t fromhost;

static uintptr_t syscall(uintptr_t which, uint64_t arg0, uint64_t arg1,
                         uint64_t arg2) {
  // volatile uint64_t magic_mem[8] __attribute__((aligned(64)));
  // magic_mem[0] = which;
  // magic_mem[1] = arg0;
  // magic_mem[2] = arg1;
  // magic_mem[3] = arg2;
  // __sync_synchronize();

  // tohost = (uintptr_t)magic_mem;
  // while (fromhost == 0)
  //   ;
  // fromhost = 0;

#ifndef UART_PORT
#define UART_PORT 0x54000000
#endif

  // __sync_synchronize();
  // return magic_mem[0];
  __sync_synchronize();
  if (which == SYS_write && arg0 == 1) {
    uint8_t* p = (uint8_t*)arg1;
    while (arg2--) {
      *((uint8_t*)(UART_PORT)) = *(p++);
      uint64_t delay = 0xff;
      while (delay--)
        ;
    }
  }
  __sync_synchronize();
  return 0;
}

#define NUM_COUNTERS 2
static uintptr_t counters[NUM_COUNTERS];
static char* counter_names[NUM_COUNTERS];

void setStats(int enable) {
  int i = 0;
#define READ_CTR(name)              \
  do {                              \
    while (i >= NUM_COUNTERS)       \
      ;                             \
    uintptr_t csr = read_csr(name); \
    if (!enable) {                  \
      csr -= counters[i];           \
      counter_names[i] = #name;     \
    }                               \
    counters[i++] = csr;            \
  } while (0)

  READ_CTR(mcycle);
  READ_CTR(minstret);

#undef READ_CTR
}

void __attribute__((noreturn)) tohost_exit(uintptr_t code) {
  // tohost = (code << 1) | 1;
  while (1)
    ;
}

uintptr_t __attribute__((weak))
handle_trap(uintptr_t cause, uintptr_t epc, uintptr_t regs[32]) {
  tohost_exit(1337);
}

void exit(int code) { tohost_exit(code); }

void abort() { exit(128 + SIGABRT); }

void printstr(char* s) { syscall(SYS_write, 1, (uintptr_t)s, strlen(s)); }

void __attribute__((weak)) thread_entry(int cid, int nc) {
  // multi-threaded programs override this function.
  // for the case of single-threaded programs, only let core 0 proceed.
  while (cid != 0)
    ;
}

static ee_u16 list_known_crc[] = {(ee_u16)0xd4b0, (ee_u16)0x3340,
                                  (ee_u16)0x6a79, (ee_u16)0xe714,
                                  (ee_u16)0xe3c1};
static ee_u16 matrix_known_crc[] = {(ee_u16)0xbe52, (ee_u16)0x1199,
                                    (ee_u16)0x5608, (ee_u16)0x1fd7,
                                    (ee_u16)0x0747};
static ee_u16 state_known_crc[] = {(ee_u16)0x5e47, (ee_u16)0x39bf,
                                   (ee_u16)0xe5a4, (ee_u16)0x8e3a,
                                   (ee_u16)0x8d84};

#if (SEED_METHOD == SEED_ARG)
ee_s32 get_seed_args(int i, int argc, char* argv[]);
#define get_seed(x) (ee_s16) get_seed_args(x, argc, argv)
#define get_seed_32(x) get_seed_args(x, argc, argv)
#else /* via function or volatile */
ee_s32 get_seed_32(int i);
#define get_seed(x) (ee_s16) get_seed_32(x)
#endif

extern ee_u8 static_memblk[TOTAL_DATA_SIZE];

/* Function: main
        Main entry routine for the benchmark.
        This function is responsible for the following steps:

        1 - Initialize input seeds from a source that cannot be determined at
   compile time. 2 - Initialize memory block for use. 3 - Run and time the
   benchmark. 4 - Report results, testing the validity of the output if the
   seeds are known.

        Arguments:
        1 - first seed  : Any value
        2 - second seed : Must be identical to first for iterations to be
   identical 3 - third seed  : Any value, should be at least an order of
   magnitude less then the input size, but bigger then 32. 4 - Iterations  :
   Special, if set to 0, iterations will be automatically determined such that
   the benchmark will run between 10 to 100 secs

*/
// int __attribute__((weak)) main(int argc, char** argv) {
int inner_main() {
  int argc = 0;
  char** argv = 0;
  // // single-threaded programs override this function.
  // printstr("Implement main(), foo!\n");
  // return -1;
  // printstr("Implemented main()!\n");
  printstr("m\n");
  ee_u16 i, j = 0, num_algorithms = 0;
  ee_s16 known_id = -1, total_errors = 0;
  ee_u16 seedcrc = 0;
  CORE_TICKS total_time;
  core_results results[MULTITHREAD];
#if (MEM_METHOD == MEM_STACK)
  ee_u8 stack_memblock[TOTAL_DATA_SIZE * MULTITHREAD];
#endif
  /* first call any initializations needed */
  portable_init(&(results[0].port), &argc, argv);
  /* First some checks to make sure benchmark will run ok */
  if (sizeof(struct list_head_s) > 128) {
    ee_printf("list_head structure too big for comparable data!\n");
    return MAIN_RETURN_VAL;
  }
  results[0].seed1 = get_seed(1);
  results[0].seed2 = get_seed(2);
  results[0].seed3 = get_seed(3);
  results[0].iterations = get_seed_32(4);
  // #if CORE_DEBUG
  results[0].iterations = 1;
  // #endif
  results[0].execs = get_seed_32(5);
  if (results[0].execs == 0) { /* if not supplied, execute all algorithms */
    results[0].execs = ALL_ALGORITHMS_MASK;
  }
  /* put in some default values based on one seed only for easy testing */
  if ((results[0].seed1 == 0) && (results[0].seed2 == 0) &&
      (results[0].seed3 == 0)) { /* validation run */
    results[0].seed1 = 0;
    results[0].seed2 = 0;
    results[0].seed3 = 0x66;
  }
  if ((results[0].seed1 == 1) && (results[0].seed2 == 0) &&
      (results[0].seed3 == 0)) { /* perfromance run */
    results[0].seed1 = 0x3415;
    results[0].seed2 = 0x3415;
    results[0].seed3 = 0x66;
  }
#if (MEM_METHOD == MEM_STATIC)
  results[0].memblock[0] = (void*)static_memblk;
  results[0].size = TOTAL_DATA_SIZE;
  results[0].err = 0;
#if (MULTITHREAD > 1)
#error "Cannot use a static data area with multiple contexts!"
#endif
#elif (MEM_METHOD == MEM_MALLOC)
  for (i = 0; i < MULTITHREAD; i++) {
    ee_s32 malloc_override = get_seed(7);
    if (malloc_override != 0)
      results[i].size = malloc_override;
    else
      results[i].size = TOTAL_DATA_SIZE;
    results[i].memblock[0] = portable_malloc(results[i].size);
    results[i].seed1 = results[0].seed1;
    results[i].seed2 = results[0].seed2;
    results[i].seed3 = results[0].seed3;
    results[i].err = 0;
    results[i].execs = results[0].execs;
  }
#elif (MEM_METHOD == MEM_STACK)
  for (i = 0; i < MULTITHREAD; i++) {
    results[i].memblock[0] = stack_memblock + i * TOTAL_DATA_SIZE;
    results[i].size = TOTAL_DATA_SIZE;
    results[i].seed1 = results[0].seed1;
    results[i].seed2 = results[0].seed2;
    results[i].seed3 = results[0].seed3;
    results[i].err = 0;
    results[i].execs = results[0].execs;
  }
#else
#error "Please define a way to initialize a memory block."
#endif
  /* Data init */
  /* Find out how space much we have based on number of algorithms */
  for (i = 0; i < NUM_ALGORITHMS; i++) {
    if ((1 << (ee_u32)i) & results[0].execs) num_algorithms++;
  }
  for (i = 0; i < MULTITHREAD; i++)
    results[i].size = results[i].size / num_algorithms;
  /* Assign pointers */
  for (i = 0; i < NUM_ALGORITHMS; i++) {
    ee_u32 ctx;
    if ((1 << (ee_u32)i) & results[0].execs) {
      for (ctx = 0; ctx < MULTITHREAD; ctx++)
        results[ctx].memblock[i + 1] =
            (char*)(results[ctx].memblock[0]) + results[0].size * j;
      j++;
    }
  }
  /* call inits */
  // printf("pf %d!\n", 114);
  // printstr("inits\n");
  for (i = 0; i < MULTITHREAD; i++) {
    if (results[i].execs & ID_LIST) {
      results[i].list = core_list_init(results[0].size, results[i].memblock[1],
                                       results[i].seed1);
    }
    if (results[i].execs & ID_MATRIX) {
      core_init_matrix(
          results[0].size, results[i].memblock[2],
          (ee_s32)results[i].seed1 | (((ee_s32)results[i].seed2) << 16),
          &(results[i].mat));
    }
    // printstr("state\n");
    if (results[i].execs & ID_STATE) {
      core_init_state(results[0].size, results[i].seed1,
                      results[i].memblock[3]);
    }
  }
  printstr("init done\n");

  /* automatically determine number of iterations if not set */
  if (results[0].iterations == 0) {
    secs_ret secs_passed = 0;
    ee_u32 divisor;
    results[0].iterations = 1;
    while (secs_passed < (secs_ret)1) {
      results[0].iterations *= 10;
      start_time();
      iterate(&results[0]);
      stop_time();
      secs_passed = time_in_secs(get_time());
    }
    /* now we know it executes for at least 1 sec, set actual run time at about
     * 10 secs */
    divisor = (ee_u32)secs_passed;
    if (divisor ==
        0) /* some machines cast float to int as 0 since this conversion is not
              defined by ANSI, but we know at least one second passed */
      divisor = 1;
    results[0].iterations *= 1 + 10 / divisor;
  }
  /* perform actual benchmark */
  start_time();
#if (MULTITHREAD > 1)
  if (default_num_contexts > MULTITHREAD) {
    default_num_contexts = MULTITHREAD;
  }
  for (i = 0; i < default_num_contexts; i++) {
    results[i].iterations = results[0].iterations;
    results[i].execs = results[0].execs;
    core_start_parallel(&results[i]);
  }
  for (i = 0; i < default_num_contexts; i++) {
    core_stop_parallel(&results[i]);
  }
#else
  iterate(&results[0]);
#endif
  stop_time();
  total_time = get_time();
  /* get a function of the input to report */
  seedcrc = crc16(results[0].seed1, seedcrc);
  seedcrc = crc16(results[0].seed2, seedcrc);
  seedcrc = crc16(results[0].seed3, seedcrc);
  seedcrc = crc16(results[0].size, seedcrc);

  switch (seedcrc) { /* test known output for common seeds */
    case 0x8a02:     /* seed1=0, seed2=0, seed3=0x66, size 2000 per algorithm */
      known_id = 0;
      ee_printf("6k performance run parameters for coremark.\n");
      break;
    case 0x7b05: /*  seed1=0x3415, seed2=0x3415, seed3=0x66, size 2000 per
                    algorithm */
      known_id = 1;
      ee_printf("6k validation run parameters for coremark.\n");
      break;
    case 0x4eaf: /* seed1=0x8, seed2=0x8, seed3=0x8, size 400 per algorithm */
      known_id = 2;
      ee_printf("Profile generation run parameters for coremark.\n");
      break;
    case 0xe9f5: /* seed1=0, seed2=0, seed3=0x66, size 666 per algorithm */
      known_id = 3;
      ee_printf("2K performance run parameters for coremark.\n");
      break;
    case 0x18f2: /*  seed1=0x3415, seed2=0x3415, seed3=0x66, size 666 per
                    algorithm */
      known_id = 4;
      ee_printf("2K validation run parameters for coremark.\n");
      break;
    default:
      total_errors = -1;
      break;
  }
  if (known_id >= 0) {
    for (i = 0; i < default_num_contexts; i++) {
      results[i].err = 0;
      if ((results[i].execs & ID_LIST) &&
          (results[i].crclist != list_known_crc[known_id])) {
        ee_printf("[%u]ERROR! list crc 0x%04x - should be 0x%04x\n", i,
                  results[i].crclist, list_known_crc[known_id]);
        results[i].err++;
      }
      if ((results[i].execs & ID_MATRIX) &&
          (results[i].crcmatrix != matrix_known_crc[known_id])) {
        ee_printf("[%u]ERROR! matrix crc 0x%04x - should be 0x%04x\n", i,
                  results[i].crcmatrix, matrix_known_crc[known_id]);
        results[i].err++;
      }
      if ((results[i].execs & ID_STATE) &&
          (results[i].crcstate != state_known_crc[known_id])) {
        ee_printf("[%u]ERROR! state crc 0x%04x - should be 0x%04x\n", i,
                  results[i].crcstate, state_known_crc[known_id]);
        results[i].err++;
      }
      total_errors += results[i].err;
    }
  }
  total_errors += check_data_types();
  /* and report results */
  ee_printf("CoreMark Size    : %lu\n", (long unsigned)results[0].size);
  ee_printf("Total ticks      : %lu\n", (long unsigned)total_time);
#if HAS_FLOAT
  ee_printf("Total time (secs): %f\n", time_in_secs(total_time));
  if (time_in_secs(total_time) > 0)
    ee_printf("Iterations/Sec   : %f\n", default_num_contexts *
                                             results[0].iterations /
                                             time_in_secs(total_time));
#else
  ee_printf("Total time (secs): %d\n", time_in_secs(total_time));
  if (time_in_secs(total_time) > 0)
    ee_printf("Iterations/Sec   : %d\n", default_num_contexts *
                                             results[0].iterations /
                                             time_in_secs(total_time));
#endif
  if (time_in_secs(total_time) < 10) {
    ee_printf("ERROR! Must execute for at least 10 secs for a valid result!\n");
    total_errors++;
  }

  ee_printf("Iterations       : %lu\n",
            (long unsigned)default_num_contexts * results[0].iterations);
  ee_printf("Compiler version : %s\n", COMPILER_VERSION);
  ee_printf("Compiler flags   : %s\n", COMPILER_FLAGS);
#if (MULTITHREAD > 1)
  ee_printf("Parallel %s : %d\n", PARALLEL_METHOD, default_num_contexts);
#endif
  ee_printf("Memory location  : %s\n", MEM_LOCATION);
  /* output for verification */
  ee_printf("seedcrc          : 0x%04x\n", seedcrc);
  if (results[0].execs & ID_LIST)
    for (i = 0; i < default_num_contexts; i++)
      ee_printf("[%d]crclist       : 0x%04x\n", i, results[i].crclist);
  if (results[0].execs & ID_MATRIX)
    for (i = 0; i < default_num_contexts; i++)
      ee_printf("[%d]crcmatrix     : 0x%04x\n", i, results[i].crcmatrix);
  if (results[0].execs & ID_STATE)
    for (i = 0; i < default_num_contexts; i++)
      ee_printf("[%d]crcstate      : 0x%04x\n", i, results[i].crcstate);
  for (i = 0; i < default_num_contexts; i++)
    ee_printf("[%d]crcfinal      : 0x%04x\n", i, results[i].crc);
  if (total_errors == 0) {
    ee_printf(
        "Correct operation validated. See README.md for run and reporting "
        "rules.\n");
#if HAS_FLOAT
    if (known_id == 3) {
      ee_printf("CoreMark 1.0 : %f / %s %s",
                default_num_contexts * results[0].iterations /
                    time_in_secs(total_time),
                COMPILER_VERSION, COMPILER_FLAGS);
#if defined(MEM_LOCATION) && !defined(MEM_LOCATION_UNSPEC)
      ee_printf(" / %s", MEM_LOCATION);
#else
      ee_printf(" / %s", mem_name[MEM_METHOD]);
#endif

#if (MULTITHREAD > 1)
      ee_printf(" / %d:%s", default_num_contexts, PARALLEL_METHOD);
#endif
      ee_printf("\n");
    }
#endif
  }
  if (total_errors > 0) ee_printf("Errors detected\n");
  if (total_errors < 0)
    ee_printf(
        "Cannot validate operation for these seed values, please compare with "
        "results on a known platform.\n");

#if (MEM_METHOD == MEM_MALLOC)
  for (i = 0; i < MULTITHREAD; i++) portable_free(results[i].memblock[0]);
#endif
  /* And last call any target specific code for finalizing */
  portable_fini(&(results[0].port));

  return MAIN_RETURN_VAL;
}

static void init_tls() {
  register void* thread_pointer asm("tp");
  extern char _tls_data;
  extern __thread char _tdata_begin, _tdata_end, _tbss_end;
  size_t tdata_size = &_tdata_end - &_tdata_begin;
  memcpy(thread_pointer, &_tls_data, tdata_size);
  size_t tbss_size = &_tbss_end - &_tdata_end;
  memset(thread_pointer + tdata_size, 0, tbss_size);
}

void _init(int cid, int nc) {
printstr("_init!\n");
// init_tls();
// thread_entry(cid, nc);

// only single-threaded programs should ever get here.
printstr("before main\n");
#if MAIN_HAS_NOARGC
  MAIN_RETURN_TYPE main(void);
  int ret = main();
#else
  MAIN_RETURN_TYPE main(int argc, char* argv[]);
  int ret = main(0, 0);
#endif
  // int ret = 0;
  // inner_main();

  char buf[NUM_COUNTERS * 32] __attribute__((aligned(64)));
  char* pbuf = buf;
  for (int i = 0; i < NUM_COUNTERS; i++)
    if (counters[i])
      pbuf += sprintf(pbuf, "%s = %d\n", counter_names[i], counters[i]);
  if (pbuf != buf) printstr(buf);

  exit(ret);
}

#undef putchar
int putchar(int ch) {
  static __thread char buf[64] __attribute__((aligned(64)));
  static __thread int buflen = 0;

  buf[buflen++] = ch;

  if (ch == '\n' || buflen == sizeof(buf)) {
    syscall(SYS_write, 1, (uintptr_t)buf, buflen);
    buflen = 0;
  }

  return 0;
}

void printhex(uint64_t x) {
  char str[17];
  int i;
  for (i = 0; i < 16; i++) {
    str[15 - i] = (x & 0xF) + ((x & 0xF) < 10 ? '0' : 'a' - 10);
    x >>= 4;
  }
  str[16] = 0;

  printstr(str);
}

static inline void printnum(void (*putch)(int, void**), void** putdat,
                            unsigned long long num, unsigned base, int width,
                            int padc) {
  unsigned digs[sizeof(num) * CHAR_BIT];
  int pos = 0;

  while (1) {
    digs[pos++] = num % base;
    if (num < base) break;
    num /= base;
  }

  while (width-- > pos) putch(padc, putdat);

  while (pos-- > 0)
    putch(digs[pos] + (digs[pos] >= 10 ? 'a' - 10 : '0'), putdat);
}

static unsigned long long getuint(va_list* ap, int lflag) {
  if (lflag >= 2)
    return va_arg(*ap, unsigned long long);
  else if (lflag)
    return va_arg(*ap, unsigned long);
  else
    return va_arg(*ap, unsigned int);
}

static long long getint(va_list* ap, int lflag) {
  if (lflag >= 2)
    return va_arg(*ap, long long);
  else if (lflag)
    return va_arg(*ap, long);
  else
    return va_arg(*ap, int);
}

static void vprintfmt(void (*putch)(int, void**), void** putdat,
                      const char* fmt, va_list ap) {
  register const char* p;
  const char* last_fmt;
  register int ch, err;
  unsigned long long num;
  int base, lflag, width, precision, altflag;
  char padc;

  while (1) {
    while ((ch = *(unsigned char*)fmt) != '%') {
      if (ch == '\0') return;
      fmt++;
      putch(ch, putdat);
    }
    fmt++;

    // Process a %-escape sequence
    last_fmt = fmt;
    padc = ' ';
    width = -1;
    precision = -1;
    lflag = 0;
    altflag = 0;
  reswitch:
    switch (ch = *(unsigned char*)fmt++) {
      // flag to pad on the right
      case '-':
        padc = '-';
        goto reswitch;

      // flag to pad with 0's instead of spaces
      case '0':
        padc = '0';
        goto reswitch;

      // width field
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        for (precision = 0;; ++fmt) {
          precision = precision * 10 + ch - '0';
          ch = *fmt;
          if (ch < '0' || ch > '9') break;
        }
        goto process_precision;

      case '*':
        precision = va_arg(ap, int);
        goto process_precision;

      case '.':
        if (width < 0) width = 0;
        goto reswitch;

      case '#':
        altflag = 1;
        goto reswitch;

      process_precision:
        if (width < 0) width = precision, precision = -1;
        goto reswitch;

      // long flag (doubled for long long)
      case 'l':
        lflag++;
        goto reswitch;

      // character
      case 'c':
        putch(va_arg(ap, int), putdat);
        break;

      // string
      case 's':
        if ((p = va_arg(ap, char*)) == NULL) p = "(null)";
        if (width > 0 && padc != '-')
          for (width -= strnlen(p, precision); width > 0; width--)
            putch(padc, putdat);
        for (; (ch = *p) != '\0' && (precision < 0 || --precision >= 0);
             width--) {
          putch(ch, putdat);
          p++;
        }
        for (; width > 0; width--) putch(' ', putdat);
        break;

      // (signed) decimal
      case 'd':
        num = getint(&ap, lflag);
        if ((long long)num < 0) {
          putch('-', putdat);
          num = -(long long)num;
        }
        base = 10;
        goto signed_number;

      // unsigned decimal
      case 'u':
        base = 10;
        goto unsigned_number;

      // (unsigned) octal
      case 'o':
        // should do something with padding so it's always 3 octits
        base = 8;
        goto unsigned_number;

      // pointer
      case 'p':
        static_assert(sizeof(long) == sizeof(void*));
        lflag = 1;
        putch('0', putdat);
        putch('x', putdat);
        /* fall through to 'x' */

      // (unsigned) hexadecimal
      case 'X':
      case 'x':
        base = 16;
      unsigned_number:
        num = getuint(&ap, lflag);
      signed_number:
        printnum(putch, putdat, num, base, width, padc);
        break;

      // escaped '%' character
      case '%':
        putch(ch, putdat);
        break;

      // unrecognized escape sequence - just print it literally
      default:
        putch('%', putdat);
        fmt = last_fmt;
        break;
    }
  }
}

int printf(const char* fmt, ...) {
  return 0;
}
int printf2(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  vprintfmt((void*)putchar, 0, fmt, ap);

  va_end(ap);
  return 0;  // incorrect return value, but who cares, anyway?
}

int puts(const char* s) {
  printf(s);
  printf("\n");
  return 0;  // incorrect return value, but who cares, anyway?
}

int sprintf(char* str, const char* fmt, ...) {
  va_list ap;
  char* str0 = str;
  va_start(ap, fmt);

  void sprintf_putch(int ch, void** data) {
    char** pstr = (char**)data;
    **pstr = ch;
    (*pstr)++;
  }

  vprintfmt(sprintf_putch, (void**)&str, fmt, ap);
  *str = 0;

  va_end(ap);
  return str - str0;
}

void* memcpy(void* dest, const void* src, size_t len) {
  if ((((uintptr_t)dest | (uintptr_t)src | len) & (sizeof(uintptr_t) - 1)) ==
      0) {
    const uintptr_t* s = src;
    uintptr_t* d = dest;
    while (d < (uintptr_t*)(dest + len)) *d++ = *s++;
  } else {
    const char* s = src;
    char* d = dest;
    while (d < (char*)(dest + len)) *d++ = *s++;
  }
  return dest;
}

void* memset(void* dest, int byte, size_t len) {
  if ((((uintptr_t)dest | len) & (sizeof(uintptr_t) - 1)) == 0) {
    uintptr_t word = byte & 0xFF;
    word |= word << 8;
    word |= word << 16;
    word |= word << 16 << 16;

    uintptr_t* d = dest;
    while (d < (uintptr_t*)(dest + len)) *d++ = word;
  } else {
    char* d = dest;
    while (d < (char*)(dest + len)) *d++ = byte;
  }
  return dest;
}

size_t strlen(const char* s) {
  const char* p = s;
  while (*p) p++;
  return p - s;
}

size_t strnlen(const char* s, size_t n) {
  const char* p = s;
  while (n-- && *p) p++;
  return p - s;
}

int strcmp(const char* s1, const char* s2) {
  unsigned char c1, c2;

  do {
    c1 = *s1++;
    c2 = *s2++;
  } while (c1 != 0 && c1 == c2);

  return c1 - c2;
}

char* strcpy(char* dest, const char* src) {
  char* d = dest;
  while ((*d++ = *src++))
    ;
  return dest;
}

long atol(const char* str) {
  long res = 0;
  int sign = 0;

  while (*str == ' ') str++;

  if (*str == '-' || *str == '+') {
    sign = *str == '-';
    str++;
  }

  while (*str) {
    res *= 10;
    res += *str++ - '0';
  }

  return sign ? -res : res;
}
