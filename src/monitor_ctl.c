long syscall(long number, ...);
extern char **environ;

static int slen(const char *s) {
  int n = 0;
  while (*s++) n++;
  return n;
}

static char *scpy(char *d, const char *s) {
  while (*s) *d++ = *s++;
  return d;
}

static char *scpyn(char *d, const char *s, int n) {
  while (n--) *d++ = *s++;
  return d;
}

static const char *sstr(const char *h, const char *n) {
  if (!*n) return h;
  for (; *h; h++) {
    const char *a = h, *b = n;
    while (*a && *b && *a == *b) a++, b++;
    if (!*b) return h;
  }
  return 0;
}

static int s2i(const char *s) {
  int n = 0;
  while (*s >= '0' && *s <= '9') n = n * 10 + (*s++ - '0');
  return n;
}

#define S(s) s, sizeof(s) - 1

/* Run command, wait for it, return nothing */
static long run_sh(const char *cmd) {
  long pid = syscall(57);
  if (pid == 0) {
    syscall(59, "/bin/sh",
            (char *[]){(char *)"sh", (char *)"-c", (char *)cmd, 0}, environ);
    syscall(60, 127);
  }
  while (syscall(61, pid, 0, 0, 0) > 0)
    ;
  return pid;
}

/* Run command and capture full output into buf (max n-1 bytes). Returns bytes read. */
static int run_sh_capture(const char *cmd, char *buf, int n) {
  int p[2];
  syscall(22, p, 0);
  long pid = syscall(57);
  if (pid == 0) {
    syscall(3, p[0]);
    syscall(33, p[1], 1);
    syscall(3, p[1]);
    syscall(59, "/bin/sh",
            (char *[]){(char *)"sh", (char *)"-c", (char *)cmd, 0}, environ);
    syscall(60, 127);
  }
  syscall(3, p[1]);
  int total = 0;
  while (total < n - 1) {
    long r = syscall(0, p[0], buf + total, n - 1 - total);
    if (r <= 0) break;
    total += r;
  }
  buf[total] = 0;
  syscall(3, p[0]);
  while (syscall(61, pid, 0, 0, 0) > 0);
  return total;
}

/* Find I2C bus dynamically for Fujitsu B24W-7 */
static int find_bus(void) {
  char buf[4096];
  int n = run_sh_capture("ddcutil detect --brief 2>/dev/null", buf, sizeof(buf));
  if (n <= 0) return -1;

  int bus_num = -1;
  char *p = buf;
  while (p < buf + n) {
    /* Look for "I2C bus:" */
    const char *i2c = sstr(p, "I2C bus:");
    if (i2c && i2c < buf + n) {
      const char *dev = sstr(i2c, "/dev/i2c-");
      if (dev) {
        dev += 9; /* skip "/dev/i2c-" */
        bus_num = s2i(dev);
      }
    }
    /* Look for "B24W-7" */
    if (sstr(p, "B24W-7")) {
      if (bus_num != -1) return bus_num;
    }
    /* Advance to next line */
    while (p < buf + n && *p != '\n') p++;
    if (p < buf + n) p++;
  }
  return -1;
}

static void write_str(int fd, const char *s) {
  syscall(1, fd, s, slen(s));
}

void _start(void) {
  /* Read args from /proc/self/cmdline - actually we can't easily get argc/argv
     without libc. Instead, read from /proc/self/cmdline. */
  char cmdline[256];
  long fd = syscall(2, "/proc/self/cmdline", 0, 0);
  if (fd < 0) syscall(60, 1);
  int n = syscall(0, fd, cmdline, sizeof(cmdline) - 1);
  syscall(3, fd);
  if (n <= 0) syscall(60, 1);

  /* Parse argv[1] from cmdline (null-separated) */
  char *arg = cmdline;
  /* Skip argv[0] */
  while (arg < cmdline + n && *arg) arg++;
  if (arg >= cmdline + n) {
    write_str(2, "Usage: monitor_ctl [dp|dvi]\n");
    syscall(60, 1);
  }
  arg++; /* skip null */

  /* Determine input value */
  const char *val;
  if (sstr(arg, "dp") == arg && (arg[2] == 0 || arg[2] == ' ')) {
    val = "0x0f";
  } else if (sstr(arg, "dvi") == arg && (arg[3] == 0 || arg[3] == ' ')) {
    val = "0x03";
  } else {
    write_str(2, "Usage: monitor_ctl [dp|dvi]\n");
    syscall(60, 1);
  }

  int bus = find_bus();
  if (bus == -1) {
    write_str(2, "No B24W-7 monitor found. Check cables.\n");
    syscall(60, 1);
  }

  /* Build and run ddcutil command */
  char cmd[1024];
  char *w = cmd;
  w = scpyn(w, S("ddcutil setvcp 60 "));
  w = scpy(w, val);
  w = scpyn(w, S(" --bus "));
  /* Convert bus number to string */
  char bus_str[16];
  char *bp = bus_str + 15;
  *bp = 0;
  int tmp = bus;
  if (tmp == 0) *--bp = '0';
  else while (tmp) { *--bp = '0' + (tmp % 10); tmp /= 10; }
  w = scpy(w, bp);
  w = scpyn(w, S(" --noverify --force-slave-address --sleep-multiplier 20 --maxtries 0,15,0"));
  *w = 0;

  /* Write status to stderr for debugging */
  syscall(1, 2, cmd, slen(cmd));
  syscall(1, 2, "\n", 1);

  run_sh(cmd);
  syscall(60, 0);
}
