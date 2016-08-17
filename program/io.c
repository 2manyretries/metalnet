
// Console Input/Output - mapped to UART.
//
// Satisfy the underlying I/O symbols on which standard library routines depend.
// Note that we adopt a specific convention here which is appropriate to
// real-time systems and fairly convenient provided all I/O users appreciate it:
// - stdout: output is real-time non-blocking but best-endevours.
//   Essentially if you overflow the innate HW FIFO, text will be lost and
//   console output will look a mess. However you can use this in real-time
//   routines including interrupt handlers.
// - stderr: output is complete but blocking.
//   The caller will be made to wait until their text can be fitted in the HW
//   HW FIFO. Should not be used when real-time operation is required.
//
// Additonally stdin is supported.
//
// All of the above sit under standard library routines like fprintf().
// Though beware on input of buffered routines that only return on line-ends.
//



#include <stdio.h>
#include <inttypes.h>

#include "io.h"

#include "socal/alt_uart.h"
#include "socal/hps.h"
#include "socal/socal.h"

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#define ISSTDOUTERR(FD) (((FD)>=STDOUT_FILENO)&&((FD)<=STDERR_FILENO))

int _close(int file) {
  return ISSTDOUTERR(file) ? 0 : -1;
}

int _fstat(int file, void *st) {
  return ISSTDOUTERR(file) ? 0 : -1;
}

int _isatty(int file) {
  return ISSTDOUTERR(file) ? 1 : -1;
}

off_t _lseek(int file, off_t ptr, int dir) {
  return ISSTDOUTERR(file) ? 0 : -1;
}

int _read(int file, void *ptr, size_t len) {
  if((!ptr)||(STDIN_FILENO!=file))
    len = -1;

  for(int i=0;i<len;++i) {
    while(!io_input_available(STDIN_FILENO)) { }
    uint8_t* pb = ptr;
    pb[i] = alt_read_word(ALT_UART0_RBR_THR_DLL_ADDR);
  }
  return len;
}

int _write(int file, char * ptr, unsigned len, int flag )
{
  if(!ISSTDOUTERR(file)) {
    return -1;
  }

  for(int i=0; i<len; i++) {
    // GJI: stderr guarantees output and may delay;
    // GJI: stdout is best-endevours output and does not wait...
    // GJI: Wait while FIFO full...
    if(STDERR_FILENO==file)
      while(ALT_UART_USR_TFNF_E_FULL == ( ALT_UART_USR_TFNF_GET(alt_read_word(ALT_UART0_USR_ADDR)))) { }

      alt_write_word(ALT_UART0_RBR_THR_DLL_ADDR, ptr[i]);
    }

    return len;
}

// ---- GJI ---->>>>
#include "alt_16550_uart.h"

/// Must be called before doing any I/O.
bool io_init() {
  ALT_16550_HANDLE_t handle;

  bool ok = true;
  ok &=( ALT_E_SUCCESS == alt_16550_init(
    ALT_16550_DEVICE_SOCFPGA_UART0,
    (void*)0, // N/A: Hard UART
    0,        // N/A: Hard UART
    &handle
  ) );
  ok &=( ALT_E_SUCCESS == alt_16550_baudrate_set( &handle, ALT_16550_BAUDRATE_115200 ) );
  ok &=( ALT_E_SUCCESS == alt_16550_enable( &handle ) );
  ok &=( ALT_E_SUCCESS == alt_16550_fifo_enable( &handle ) );
  // The r/w routine though not int-driven rely on reg settings for int mode.
  //ok &=( ALT_E_SUCCESS == alt_16550_int_enable_tx( &handle ) );
  //ok &=( ALT_E_SUCCESS == alt_16550_int_enable_rx( &handle ) );
  alt_write_word(ALT_UART0_RBR_THR_DLL_ADDR, (ok?'1':'0') );
  return ok;
}

/// Augment the standard I/O routines with a routine to peek if input is
/// available without waiting.
bool io_input_available(int fd) {
  bool maybe_avail = (STDIN_FILENO==fd);
  if(maybe_avail) {
    maybe_avail = (ALT_UART_USR_RFNE_E_NOTEMPTY == ALT_UART_USR_RFNE_GET(alt_read_word(ALT_UART0_USR_ADDR)));
  }
  return maybe_avail;
}

