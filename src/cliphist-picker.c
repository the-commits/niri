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

#define S(s) s, sizeof(s) - 1

static const char *get_env(const char *name) {
  for (char **e = environ; *e; e++) {
    const char *p = *e, *n = name;
    while (*n && *p && *p == *n) p++, n++;
    if (!*n && *p == '=') return p + 1;
  }
  return 0;
}

/* check if strings equal */
static int seq(const char *a, const char *b) {
  while (*a && *b && *a == *b) a++, b++;
  return *a == *b;
}

/* check if h starts with n */
static int sstart(const char *h, const char *n) {
  while (*n) if (*h++ != *n++) return 0;
  return 1;
}

/* read entire file into buf (max n bytes). Returns bytes read. */
static int read_file(const char *path, char *buf, int n) {
  long fd = syscall(2, path, 0, 0); /* O_RDONLY = 0 */
  if (fd < 0) return -1;
  int total = 0;
  while (total < n) {
    long r = syscall(0, fd, buf + total, n - total);
    if (r <= 0) break;
    total += r;
  }
  syscall(3, fd);
  return total;
}

/* Is `id` present as a line in the sensitive IDs data? */
static int is_sensitive(const char *data, int len, const char *id) {
  const char *p = data;
  const char *end = data + len;
  while (p < end) {
    const char *nl = p;
    while (nl < end && *nl != '\n') nl++;
    int llen = nl - p;
    if (llen > 0) {
      const char *a = p, *b = id;
      int match = 1;
      while (*b && a < nl && *a == *b) a++, b++;
      if (!*b && a == nl) return 1; /* exact match */
    }
    p = nl + 1;
  }
  return 0;
}

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

/* Run command and read first line of output into buf (n bytes). Returns length. */
static int run_sh_read(const char *cmd, char *buf, int n) {
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
  int i = 0;
  while (i < n - 1) {
    long r = syscall(0, p[0], buf + i, 1);
    if (r <= 0 || buf[i] == '\n') break;
    i++;
  }
  buf[i] = 0;
  syscall(3, p[0]);
  while (syscall(61, pid, 0, 0, 0) > 0);
  return i;
}

void _start(void) {
  const char *home = get_env("HOME");
  if (!home) syscall(60, 1);

  char tag_file[256];
  char *w = tag_file;
  w = scpy(w, home);
  w = scpy(w, "/.cache/cliphist/.sensitive-ids");
  *w = 0;

  /* Read sensitive IDs */
  char ids_buf[8192];
  int ids_len = read_file(tag_file, ids_buf, sizeof(ids_buf) - 1);
  if (ids_len < 0) ids_len = 0;
  ids_buf[ids_len] = 0;

  /* Run clipboard picker: cliphist list | sort -rn | wofi --dmenu --cache-file /dev/null */
  char selection[8192];
  int sel_len = run_sh_read(
    "cliphist list | sort -rn | wofi --dmenu --cache-file /dev/null",
    selection, sizeof(selection));

  if (sel_len <= 0) syscall(60, 0);

  /* Extract entry ID: first tab-separated field */
  char entry_id[256];
  const char *s = selection;
  int ei = 0;
  while (*s && *s != '\t' && ei < 255) entry_id[ei++] = *s++;
  entry_id[ei] = 0;

  /* Trim trailing whitespace from entry_id */
  while (ei > 0 && (entry_id[ei-1] == ' ' || entry_id[ei-1] == '\t')) ei--;
  entry_id[ei] = 0;

  int sensitive = is_sensitive(ids_buf, ids_len, entry_id);

  /* Build shell-quoted selection for piping */
  /* We need to pass selection to cliphist decode. Use printf via sh -c. */
  if (sensitive) {
    /* paste-once */
    char cmd[12000];
    w = cmd;
    w = scpyn(w, S("printf '%s' '"));
    /* Escape single quotes in selection */
    for (const char *p = selection; *p; p++) {
      if (*p == '\'') { *w++ = '\''; *w++ = '\\'; *w++ = '\''; *w++ = '\''; }
      else *w++ = *p;
    }
    w = scpyn(w, S("' | cliphist decode | wl-copy --paste-once"));
    *w = 0;
    run_sh(cmd);

    /* Delete from cliphist */
    w = cmd;
    w = scpyn(w, S("cliphist delete <<< '"));
    for (const char *p = selection; *p; p++) {
      if (*p == '\'') { *w++ = '\''; *w++ = '\\'; *w++ = '\''; *w++ = '\''; }
      else *w++ = *p;
    }
    w = scpyn(w, S("'"));
    *w = 0;
    run_sh(cmd);

    /* Remove from tag file */
    {
      char tmp[256];
      w = tmp;
      w = scpy(w, tag_file);
      w = scpy(w, ".tmp");
      *w = 0;

      /* Remove line matching entry_id from tag file */
      char rm_cmd[12000];
      w = rm_cmd;
      w = scpyn(w, S("grep -vxF '"));
      w = scpy(w, entry_id);
      w = scpyn(w, S("' '"));
      w = scpy(w, tag_file);
      w = scpyn(w, S("' > '"));
      w = scpy(w, tmp);
      w = scpyn(w, S("' && mv '"));
      w = scpy(w, tmp);
      w = scpyn(w, S("' '"));
      w = scpy(w, tag_file);
      w = scpyn(w, S("'"));
      *w = 0;
      run_sh(rm_cmd);
    }
  } else {
    /* Normal paste */
    char cmd[12000];
    w = cmd;
    w = scpyn(w, S("printf '%s' '"));
    for (const char *p = selection; *p; p++) {
      if (*p == '\'') { *w++ = '\''; *w++ = '\\'; *w++ = '\''; *w++ = '\''; }
      else *w++ = *p;
    }
    w = scpyn(w, S("' | cliphist decode | wl-copy"));
    *w = 0;
    run_sh(cmd);
  }

  /* Sleep 0.1s */
  {
    long tv[] = {0, 100000000};
    syscall(35, tv, 0);
  }

  /* Simulate paste key combo */
  if (sstr(selection, "[[ binary data")) {
    run_sh("wtype -M ctrl v -m ctrl");
  } else {
    run_sh("wtype -M ctrl -M shift v -m shift -m ctrl");
  }

  syscall(60, 0);
}
