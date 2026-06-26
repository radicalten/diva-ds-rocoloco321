// stepterm.c - single-file StepMania-like terminal player for .sm/.ssc (Mac Terminal)
// Build: clang -O2 -std=c11 stepterm.c -o stepterm
// Run:   ./stepterm /path/to/song.sm
//
// Notes / scope:
// - Terminal-only renderer (ANSI escapes), updates in-place (no scrolling), no external deps.
// - Parses .sm and .ssc, supports dance-single (4 columns) charts.
// - No audio playback; timing is based on chart BPM/offset.
// - Tap heads: '1','2','4' are hittable heads.
// - Keys: D F J K (or arrow keys).  Q quits.  +/- adjusts scroll speed.

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

typedef struct {
  double beat;
  double bpm;
  double time; // seconds at 'beat' before applying OFFSET
} BPMSeg;

typedef struct {
  double beat;
  double time;  // seconds when note should be hit, after applying OFFSET
  int col;      // 0..3
  int type;     // 1 tap-like head
  int hit;      // 0 no, 1 yes, -1 miss
} Note;

typedef struct {
  char stepstype[64];
  char difficulty[64];
  int meter;
  Note *notes;
  int nnotes, capnotes;

  int *colIdx[4];
  int nCol[4];
  int ptrCol[4];
} Chart;

typedef struct {
  char title[256];
  double offset; // seconds
  BPMSeg *bpms;
  int nbpms;

  Chart *charts;
  int ncharts, capcharts;
} Simfile;

typedef struct {
  int w, h;
  char *ch;            // w*h chars
  unsigned char *col;  // w*h color ids
} Screen;

enum {
  C_DEFAULT = 0,
  C_RED     = 1,
  C_GREEN   = 2,
  C_YELLOW  = 3,
  C_BLUE    = 4,
  C_MAG     = 5,
  C_CYAN    = 6,
  C_WHITE   = 7,
  C_DIM     = 8,   // ANSI 90m
  C_BRIGHT  = 9    // ANSI 97m
};

#define C_BORDER C_CYAN
#define C_TEXT   C_WHITE

static struct termios g_orig_termios;
static volatile sig_atomic_t g_shouldQuit = 0;

static void die(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputc('\n', stderr);
  exit(1);
}

static char* xstrdup(const char *s) {
  size_t n = strlen(s);
  char *p = (char*)malloc(n + 1);
  if (!p) die("Out of memory");
  memcpy(p, s, n + 1);
  return p;
}

static double now_sec(void) {
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
  }
  return (double)time(NULL);
}

static void term_restore(void) {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig_termios);
  const char *seq = "\x1b[0m\x1b[?25h\x1b[?1049l";
  write(STDOUT_FILENO, seq, (size_t)strlen(seq));
}

static void on_signal(int sig) {
  (void)sig;
  g_shouldQuit = 1;
}

static void term_raw(void) {
  if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO))
    die("This program requires a TTY (run in Mac Terminal).");

  if (tcgetattr(STDIN_FILENO, &g_orig_termios) != 0)
    die("tcgetattr failed: %s", strerror(errno));

  atexit(term_restore);
  signal(SIGINT, on_signal);
  signal(SIGTERM, on_signal);

  struct termios raw = g_orig_termios;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 0;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0)
    die("tcsetattr failed: %s", strerror(errno));

  const char *seq = "\x1b[?1049h\x1b[2J\x1b[H\x1b[?25l";
  write(STDOUT_FILENO, seq, (size_t)strlen(seq));
}

static void get_termsize(int *w, int *h) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
    *w = (int)ws.ws_col;
    *h = (int)ws.ws_row;
  } else {
    *w = 80; *h = 24;
  }
}

static void screen_init(Screen *s, int w, int h) {
  s->w = w; s->h = h;
  s->ch = (char*)malloc((size_t)w * (size_t)h);
  s->col = (unsigned char*)malloc((size_t)w * (size_t)h);
  if (!s->ch || !s->col) die("Out of memory");
}

static void screen_free(Screen *s) {
  free(s->ch);
  free(s->col);
  memset(s, 0, sizeof(*s));
}

static void screen_clear(Screen *s) {
  memset(s->ch, ' ', (size_t)s->w * (size_t)s->h);
  memset(s->col, 0, (size_t)s->w * (size_t)s->h);
}

static void set_cell(Screen *s, int x, int y, char ch, unsigned char col) {
  if (x < 0 || y < 0 || x >= s->w || y >= s->h) return;
  size_t i = (size_t)y * (size_t)s->w + (size_t)x;
  s->ch[i] = ch;
  s->col[i] = col;
}

static void draw_text(Screen *s, int x, int y, unsigned char col, const char *txt) {
  if (y < 0 || y >= s->h) return;
  for (int i = 0; txt[i] && x + i < s->w; i++) {
    set_cell(s, x + i, y, txt[i], col);
  }
}

static const char* ansi_color(unsigned char c) {
  switch (c) {
    case C_RED:    return "\x1b[31m";
    case C_GREEN:  return "\x1b[32m";
    case C_YELLOW: return "\x1b[33m";
    case C_BLUE:   return "\x1b[34m";
    case C_MAG:    return "\x1b[35m";
    case C_CYAN:   return "\x1b[36m";
    case C_WHITE:  return "\x1b[37m";
    case C_DIM:    return "\x1b[90m";
    case C_BRIGHT: return "\x1b[97m";
    default:       return "\x1b[0m";
  }
}

static void screen_render(Screen *s) {
  write(STDOUT_FILENO, "\x1b[H", 3);

  unsigned char cur = 255;
  for (int y = 0; y < s->h; y++) {
    cur = 255;
    for (int x = 0; x < s->w; x++) {
      size_t i = (size_t)y * (size_t)s->w + (size_t)x;
      unsigned char c = s->col[i];
      if (c != cur) {
        const char *seq = ansi_color(c);
        write(STDOUT_FILENO, seq, (size_t)strlen(seq));
        cur = c;
      }
      char ch = s->ch[i];
      write(STDOUT_FILENO, &ch, 1);
    }
    write(STDOUT_FILENO, "\x1b[0m", 4);
    if (y != s->h - 1) write(STDOUT_FILENO, "\r\n", 2);
  }
}

// ---------- File loading ----------
static char* read_file_all(const char *path, size_t *outSize) {
  FILE *f = fopen(path, "rb");
  if (!f) die("Cannot open: %s", path);
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (n < 0) die("ftell failed");

  char *buf = (char*)malloc((size_t)n + 1);
  if (!buf) die("Out of memory");
  size_t r = fread(buf, 1, (size_t)n, f);
  fclose(f);
  buf[r] = 0;
  if (outSize) *outSize = r;
  return buf;
}

static void str_trim_inplace(char *s) {
  if (!s) return;
  size_t n = strlen(s);
  while (n > 0 && (s[n-1] == '\r' || s[n-1] == '\n' || isspace((unsigned char)s[n-1])))
    s[--n] = 0;
  size_t i = 0;
  while (s[i] && isspace((unsigned char)s[i])) i++;
  if (i > 0) memmove(s, s + i, strlen(s + i) + 1);
}

static int str_ieq(const char *a, const char *b) {
  for (; *a && *b; a++, b++) {
    if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
  }
  return *a == 0 && *b == 0;
}

static int contains_icase(const char *hay, const char *needle) {
  if (!hay || !needle || !*needle) return 0;
  size_t nl = strlen(needle);
  for (const char *p = hay; *p; p++) {
    size_t i = 0;
    while (needle[i] && p[i] &&
           tolower((unsigned char)needle[i]) == tolower((unsigned char)p[i])) i++;
    if (i == nl) return 1;
  }
  return 0;
}

// Parse next #TAG:VALUE; from p. Returns new pointer after ';' or NULL.
static const char* parse_next_tag(const char *p, char *tagOut, size_t tagCap, char **valOut) {
  *valOut = NULL;
  tagOut[0] = 0;
  if (!p) return NULL;

  while (*p && *p != '#') p++;
  if (!*p) return NULL;
  p++;

  size_t ti = 0;
  while (*p && *p != ':' && *p != '\n' && *p != '\r') {
    if (ti + 1 < tagCap) tagOut[ti++] = (char)toupper((unsigned char)*p);
    p++;
  }
  tagOut[ti] = 0;
  while (*p && *p != ':') p++;
  if (!*p) return NULL;
  p++;

  const char *vstart = p;
  while (*p && *p != ';') p++;
  if (!*p) return NULL;
  const char *vend = p;

  size_t vn = (size_t)(vend - vstart);
  char *val = (char*)malloc(vn + 1);
  if (!val) die("Out of memory");
  memcpy(val, vstart, vn);
  val[vn] = 0;

  *valOut = val;
  return p + 1;
}

// ---------- Simfile building ----------
static void sim_init(Simfile *sf) {
  memset(sf, 0, sizeof(*sf));
  sf->offset = 0.0;
}

static void chart_init(Chart *c) {
  memset(c, 0, sizeof(*c));
  c->meter = 0;
}

static void sim_add_chart(Simfile *sf, Chart *c) {
  if (sf->ncharts + 1 > sf->capcharts) {
    sf->capcharts = sf->capcharts ? sf->capcharts * 2 : 8;
    sf->charts = (Chart*)realloc(sf->charts, (size_t)sf->capcharts * sizeof(Chart));
    if (!sf->charts) die("Out of memory");
  }
  sf->charts[sf->ncharts++] = *c;
  chart_init(c);
}

static void chart_add_note(Chart *c, double beat, int col, int type) {
  if (c->nnotes + 1 > c->capnotes) {
    c->capnotes = c->capnotes ? c->capnotes * 2 : 1024;
    c->notes = (Note*)realloc(c->notes, (size_t)c->capnotes * sizeof(Note));
    if (!c->notes) die("Out of memory");
  }
  Note *n = &c->notes[c->nnotes++];
  n->beat = beat;
  n->time = 0.0;
  n->col = col;
  n->type = type;
  n->hit = 0;
}

static int cmp_bpms(const void *A, const void *B) {
  const BPMSeg *a = (const BPMSeg*)A, *b = (const BPMSeg*)B;
  if (a->beat < b->beat) return -1;
  if (a->beat > b->beat) return 1;
  return 0;
}

static void parse_bpms(Simfile *sf, const char *val) {
  BPMSeg *tmp = NULL;
  int n = 0, cap = 0;

  const char *p = val;
  while (*p) {
    while (*p && (isspace((unsigned char)*p) || *p == ',')) p++;
    if (!*p) break;

    char *end1 = NULL;
    double beat = strtod(p, &end1);
    if (end1 == p) break;
    p = end1;
    while (*p && *p != '=') p++;
    if (*p != '=') break;
    p++;

    char *end2 = NULL;
    double bpm = strtod(p, &end2);
    if (end2 == p) break;
    p = end2;

    if (cap < n + 1) {
      cap = cap ? cap * 2 : 8;
      tmp = (BPMSeg*)realloc(tmp, (size_t)cap * sizeof(BPMSeg));
      if (!tmp) die("Out of memory");
    }
    tmp[n++] = (BPMSeg){ .beat = beat, .bpm = bpm, .time = 0.0 };

    while (*p && *p != ',') p++;
  }

  if (n == 0) { free(tmp); return; }

  qsort(tmp, (size_t)n, sizeof(BPMSeg), cmp_bpms);

  if (tmp[0].beat > 0.0) {
    tmp = (BPMSeg*)realloc(tmp, (size_t)(n + 1) * sizeof(BPMSeg));
    if (!tmp) die("Out of memory");
    memmove(tmp + 1, tmp, (size_t)n * sizeof(BPMSeg));
    tmp[0].beat = 0.0;
    tmp[0].bpm = tmp[1].bpm;
    tmp[0].time = 0.0;
    n++;
  }

  tmp[0].time = 0.0;
  for (int i = 1; i < n; i++) {
    double dbeat = tmp[i].beat - tmp[i-1].beat;
    if (tmp[i-1].bpm <= 0.0) tmp[i-1].bpm = 120.0;
    tmp[i].time = tmp[i-1].time + dbeat * 60.0 / tmp[i-1].bpm;
  }

  free(sf->bpms);
  sf->bpms = tmp;
  sf->nbpms = n;
}

static double beat_to_time(const Simfile *sf, double beat) {
  if (!sf->bpms || sf->nbpms == 0) return beat * 60.0 / 120.0 - sf->offset;

  int seg = 0;
  for (int i = 0; i < sf->nbpms; i++) {
    if (sf->bpms[i].beat <= beat) seg = i;
    else break;
  }

  double sb = sf->bpms[seg].beat;
  double st = sf->bpms[seg].time;
  double bpm = sf->bpms[seg].bpm > 0.0 ? sf->bpms[seg].bpm : 120.0;
  double t = st + (beat - sb) * 60.0 / bpm;
  return t - sf->offset;
}

static void parse_note_data_into_chart(Chart *c, const char *noteData) {
  int measureIndex = 0;
  char **rows = NULL;
  int nrows = 0, caprows = 0;

  char line[256];
  int li = 0;

  const char *p = noteData;
  while (1) {
    char ch = *p;
    if (ch == 0) {
      if (li > 0) {
        line[li] = 0;
        str_trim_inplace(line);
        if ((int)strlen(line) >= 4) {
          if (nrows + 1 > caprows) {
            caprows = caprows ? caprows * 2 : 64;
            rows = (char**)realloc(rows, (size_t)caprows * sizeof(char*));
            if (!rows) die("Out of memory");
          }
          rows[nrows++] = xstrdup(line);
        }
        li = 0;
      }
      if (nrows > 0) {
        for (int r = 0; r < nrows; r++) {
          double beat = (double)measureIndex * 4.0 + (double)r * (4.0 / (double)nrows);
          for (int col = 0; col < 4; col++) {
            char v = rows[r][col];
            if (v == '1' || v == '2' || v == '4') chart_add_note(c, beat, col, 1);
          }
          free(rows[r]);
        }
        nrows = 0;
        measureIndex++;
      }
      break;
    }

    if (ch == '\r') { p++; continue; }

    if (ch == '\n' || ch == ',') {
      line[li] = 0;
      str_trim_inplace(line);
      if (li > 0) {
        if ((int)strlen(line) >= 4) {
          if (nrows + 1 > caprows) {
            caprows = caprows ? caprows * 2 : 64;
            rows = (char**)realloc(rows, (size_t)caprows * sizeof(char*));
            if (!rows) die("Out of memory");
          }
          rows[nrows++] = xstrdup(line);
        }
      }
      li = 0;

      if (ch == ',') {
        if (nrows > 0) {
          for (int r = 0; r < nrows; r++) {
            double beat = (double)measureIndex * 4.0 + (double)r * (4.0 / (double)nrows);
            for (int col = 0; col < 4; col++) {
              char v = rows[r][col];
              if (v == '1' || v == '2' || v == '4') chart_add_note(c, beat, col, 1);
            }
            free(rows[r]);
          }
          nrows = 0;
          measureIndex++;
        }
      }

      p++;
      continue;
    }

    if (ch == ';') {
      if (li > 0) {
        line[li] = 0;
        str_trim_inplace(line);
        if ((int)strlen(line) >= 4) {
          if (nrows + 1 > caprows) {
            caprows = caprows ? caprows * 2 : 64;
            rows = (char**)realloc(rows, (size_t)caprows * sizeof(char*));
            if (!rows) die("Out of memory");
          }
          rows[nrows++] = xstrdup(line);
        }
        li = 0;
      }
      if (nrows > 0) {
        for (int r = 0; r < nrows; r++) {
          double beat = (double)measureIndex * 4.0 + (double)r * (4.0 / (double)nrows);
          for (int col = 0; col < 4; col++) {
            char v = rows[r][col];
            if (v == '1' || v == '2' || v == '4') chart_add_note(c, beat, col, 1);
          }
          free(rows[r]);
        }
        nrows = 0;
        measureIndex++;
      }
      break;
    }

    if (li + 1 < (int)sizeof(line)) line[li++] = ch;
    p++;
  }

  free(rows);
}

static void finalize_chart_times(const Simfile *sf, Chart *c) {
  for (int i = 0; i < c->nnotes; i++) c->notes[i].time = beat_to_time(sf, c->notes[i].beat);
}

#if defined(__APPLE__)
// macOS qsort_r comparator signature: int (*)(void *thunk, const void *, const void *)
static int cmp_note_time_idx_osx(void *ctx, const void *A, const void *B) {
  const Chart *c = (const Chart*)ctx;
  int ia = *(const int*)A, ib = *(const int*)B;
  double ta = c->notes[ia].time, tb = c->notes[ib].time;
  if (ta < tb) return -1;
  if (ta > tb) return 1;
  return 0;
}
#endif

static void chart_build_col_indices(Chart *c) {
  for (int col = 0; col < 4; col++) {
    free(c->colIdx[col]);
    c->colIdx[col] = NULL;
    c->nCol[col] = 0;
    c->ptrCol[col] = 0;
  }

  for (int i = 0; i < c->nnotes; i++) {
    int col = c->notes[i].col;
    if (col >= 0 && col < 4) c->nCol[col]++;
  }

  int pos[4] = {0,0,0,0};
  for (int col = 0; col < 4; col++) {
    if (c->nCol[col] > 0) {
      c->colIdx[col] = (int*)malloc((size_t)c->nCol[col] * sizeof(int));
      if (!c->colIdx[col]) die("Out of memory");
    }
  }

  for (int i = 0; i < c->nnotes; i++) {
    int col = c->notes[i].col;
    if (col >= 0 && col < 4) c->colIdx[col][pos[col]++] = i;
  }

#if defined(__APPLE__)
  for (int col = 0; col < 4; col++) {
    if (c->colIdx[col] && c->nCol[col] > 1) {
      qsort_r(c->colIdx[col], (size_t)c->nCol[col], sizeof(int),
              (void*)c, cmp_note_time_idx_osx);
    }
  }
#endif
}

static int parse_sm_notes_fields(char *val, char **outFields, int maxFields) {
  int n = 0;
  char *p = val;
  while (n < maxFields) {
    outFields[n++] = p;
    char *c = strchr(p, ':');
    if (!c) break;
    *c = 0;
    p = c + 1;
  }
  return n;
}

static void parse_simfile_text(Simfile *sf, const char *text) {
  bool isSSC = contains_icase(text, "#NOTEDATA");

  const char *p = text;
  char tag[128];
  char *val = NULL;

  Chart cur;
  chart_init(&cur);
  bool inSSCChart = false;

  while ((p = parse_next_tag(p, tag, sizeof(tag), &val)) != NULL) {
    str_trim_inplace(val);

    if (str_ieq(tag, "TITLE")) {
      snprintf(sf->title, sizeof(sf->title), "%s", val);
    } else if (str_ieq(tag, "OFFSET")) {
      sf->offset = strtod(val, NULL);
    } else if (str_ieq(tag, "BPMS")) {
      parse_bpms(sf, val);
    }

    if (!isSSC) {
      if (str_ieq(tag, "NOTES")) {
        char *fields[8] = {0};
        int nf = parse_sm_notes_fields(val, fields, 8);
        if (nf >= 6) {
          char *stepstype = fields[0];
          char *difficulty = fields[2];
          char *meter = fields[3];
          char *notedata = fields[5];

          str_trim_inplace(stepstype);
          str_trim_inplace(difficulty);
          str_trim_inplace(meter);

          Chart c;
          chart_init(&c);
          snprintf(c.stepstype, sizeof(c.stepstype), "%s", stepstype);
          snprintf(c.difficulty, sizeof(c.difficulty), "%s", difficulty);
          c.meter = (int)strtol(meter, NULL, 10);

          if (contains_icase(c.stepstype, "dance-single")) {
            parse_note_data_into_chart(&c, notedata);
            sim_add_chart(sf, &c);
          } else {
            free(c.notes);
          }
        }
      }
    } else {
      if (str_ieq(tag, "NOTEDATA")) {
        if (inSSCChart) {
          if (cur.nnotes > 0 && contains_icase(cur.stepstype, "dance-single")) {
            sim_add_chart(sf, &cur);
          } else {
            free(cur.notes);
          }
          chart_init(&cur);
        }
        inSSCChart = true;
      } else if (inSSCChart && str_ieq(tag, "STEPSTYPE")) {
        snprintf(cur.stepstype, sizeof(cur.stepstype), "%s", val);
      } else if (inSSCChart && str_ieq(tag, "DIFFICULTY")) {
        snprintf(cur.difficulty, sizeof(cur.difficulty), "%s", val);
      } else if (inSSCChart && str_ieq(tag, "METER")) {
        cur.meter = (int)strtol(val, NULL, 10);
      } else if (inSSCChart && str_ieq(tag, "NOTES")) {
        if (contains_icase(cur.stepstype, "dance-single")) {
          parse_note_data_into_chart(&cur, val);
        }
      }
    }

    free(val);
    val = NULL;
  }

  if (isSSC && inSSCChart) {
    if (cur.nnotes > 0 && contains_icase(cur.stepstype, "dance-single")) {
      sim_add_chart(sf, &cur);
    } else {
      free(cur.notes);
    }
  }

  if (sf->title[0] == 0) snprintf(sf->title, sizeof(sf->title), "Untitled");
  if (!sf->bpms || sf->nbpms == 0) parse_bpms(sf, "0=120");

  for (int i = 0; i < sf->ncharts; i++) {
    finalize_chart_times(sf, &sf->charts[i]);
    chart_build_col_indices(&sf->charts[i]);
  }
}

static void sim_free(Simfile *sf) {
  free(sf->bpms);
  for (int i = 0; i < sf->ncharts; i++) {
    for (int col = 0; col < 4; col++) free(sf->charts[i].colIdx[col]);
    free(sf->charts[i].notes);
  }
  free(sf->charts);
  memset(sf, 0, sizeof(*sf));
}

// ---------- Gameplay ----------
typedef struct {
  int marv, perf, great, good, miss;
  int combo, maxCombo;
  int score;
  char lastJudge[32];
  double scroll; // rows/sec
} Stats;

static const char* judge_name(int j) {
  switch (j) {
    case 0: return "MARV";
    case 1: return "PERF";
    case 2: return "GREAT";
    case 3: return "GOOD";
    case 4: return "MISS";
    default: return "";
  }
}

static unsigned char judge_color(int j) {
  switch (j) {
    case 0: return C_CYAN;
    case 1: return C_GREEN;
    case 2: return C_YELLOW;
    case 3: return C_MAG;
    case 4: return C_RED;
    default: return C_TEXT;
  }
}

static int judge_for_delta(double dtAbs) {
  if (dtAbs <= 0.022) return 0;
  if (dtAbs <= 0.045) return 1;
  if (dtAbs <= 0.080) return 2;
  if (dtAbs <= 0.120) return 3;
  return 4;
}

static void apply_judge(Stats *st, int j) {
  snprintf(st->lastJudge, sizeof(st->lastJudge), "%s", judge_name(j));
  if (j == 0) { st->marv++; st->combo++; st->score += 5; }
  else if (j == 1) { st->perf++; st->combo++; st->score += 4; }
  else if (j == 2) { st->great++; st->combo++; st->score += 2; }
  else if (j == 3) { st->good++; st->combo++; st->score += 1; }
  else { st->miss++; st->combo = 0; }
  st->maxCombo = MAX(st->maxCombo, st->combo);
}

static int read_key_nonblock(void) {
  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(STDIN_FILENO, &rfds);
  struct timeval tv = {0, 0};
  int r = select(STDIN_FILENO + 1, &rfds, NULL, NULL, &tv);
  if (r <= 0) return -1;

  unsigned char c = 0;
  if (read(STDIN_FILENO, &c, 1) != 1) return -1;
  return (int)c;
}

static int map_key_to_col(int k, int *needsMore) {
  *needsMore = 0;
  if (k == 'd' || k == 'D') return 0;
  if (k == 'f' || k == 'F') return 1;
  if (k == 'j' || k == 'J') return 2;
  if (k == 'k' || k == 'K') return 3;
  if (k == 27) { *needsMore = 1; return -1; }
  return -1;
}

static int map_arrow_seq_to_col(void) {
  int k1 = read_key_nonblock();
  if (k1 < 0 || k1 != '[') return -1;
  int k2 = read_key_nonblock();
  if (k2 < 0) return -1;
  if (k2 == 'D') return 0; // left
  if (k2 == 'B') return 1; // down
  if (k2 == 'A') return 2; // up
  if (k2 == 'C') return 3; // right
  return -1;
}

static void try_hit(Chart *c, Stats *st, int col, double tnow) {
  if (col < 0 || col > 3) return;
  int ptr = c->ptrCol[col];
  if (ptr >= c->nCol[col]) return;

  int ni = c->colIdx[col][ptr];
  Note *n = &c->notes[ni];
  if (n->hit != 0) { c->ptrCol[col]++; return; }

  double dt = tnow - n->time;
  double adt = fabs(dt);

  if (dt < -0.150) return; // too early

  int j = judge_for_delta(adt);
  if (j == 4) {
    n->hit = -1;
    c->ptrCol[col]++;
    apply_judge(st, 4);
    return;
  }

  n->hit = 1;
  c->ptrCol[col]++;
  apply_judge(st, j);
}

static void apply_auto_miss(Chart *c, Stats *st, double tnow) {
  const double missAfter = 0.180;
  for (int col = 0; col < 4; col++) {
    while (c->ptrCol[col] < c->nCol[col]) {
      int ni = c->colIdx[col][c->ptrCol[col]];
      Note *n = &c->notes[ni];
      if (n->hit != 0) { c->ptrCol[col]++; continue; }
      if (tnow > n->time + missAfter) {
        n->hit = -1;
        c->ptrCol[col]++;
        apply_judge(st, 4);
        continue;
      }
      break;
    }
  }
}

static unsigned char col_color(int col) {
  switch (col) {
    case 0: return C_BLUE;
    case 1: return C_GREEN;
    case 2: return C_YELLOW;
    case 3: return C_MAG;
    default: return C_TEXT;
  }
}

static void draw_box(Screen *s, int x0, int y0, int x1, int y1) {
  for (int x = x0; x <= x1; x++) {
    set_cell(s, x, y0, '-', C_BORDER);
    set_cell(s, x, y1, '-', C_BORDER);
  }
  for (int y = y0; y <= y1; y++) {
    set_cell(s, x0, y, '|', C_BORDER);
    set_cell(s, x1, y, '|', C_BORDER);
  }
  set_cell(s, x0, y0, '+', C_BORDER);
  set_cell(s, x1, y0, '+', C_BORDER);
  set_cell(s, x0, y1, '+', C_BORDER);
  set_cell(s, x1, y1, '+', C_BORDER);
}

static void gameplay_loop(const Simfile *sf, Chart *c) {
  int tw, th;
  get_termsize(&tw, &th);
  if (tw < 60 || th < 22) die("Terminal too small. Resize to at least 60x22.");

  Screen scr;
  screen_init(&scr, tw, th);

  Stats st;
  memset(&st, 0, sizeof(st));
  snprintf(st.lastJudge, sizeof(st.lastJudge), "READY");
  st.scroll = 18.0;

  int headerH = 3;
  int footerH = 3;
  int playY0 = headerH;
  int playY1 = th - footerH - 1;

  int boxW = 26;
  int boxX0 = (tw - boxW) / 2;
  int boxX1 = boxX0 + boxW - 1;

  int boxY0 = playY0;
  int boxY1 = playY1;

  int receptorY = boxY1 - 2;

  int laneX[4];
  int innerW = boxW - 2;
  int center = boxX0 + 1 + innerW / 2;
  int spacing = 4;
  laneX[0] = center - spacing - 2;
  laneX[1] = center - 1;
  laneX[2] = center + 2;
  laneX[3] = center + spacing + 1;

  double lastT = 0.0;
  for (int i = 0; i < c->nnotes; i++) lastT = MAX(lastT, c->notes[i].time);
  double endT = lastT + 2.0;

  term_raw();

  double t0 = now_sec();
  while (!g_shouldQuit) {
    double t = now_sec() - t0;
    if (t > 2.2) break;

    screen_clear(&scr);
    draw_box(&scr, boxX0, boxY0, boxX1, boxY1);

    char line1[256];
    snprintf(line1, sizeof(line1), "%s", sf->title);
    draw_text(&scr, 2, 0, C_TEXT, line1);

    char line2[256];
    snprintf(line2, sizeof(line2), "dance-single  %s  %d", c->difficulty[0]?c->difficulty:"(unknown)", c->meter);
    draw_text(&scr, 2, 1, C_DIM, line2);

    const char *msg = (t < 0.8) ? "3" : (t < 1.5) ? "2" : "1";
    draw_text(&scr, center, (boxY0 + boxY1) / 2, C_BRIGHT, msg);
    draw_text(&scr, center - 7, (boxY0 + boxY1) / 2 + 2, C_DIM, "Get ready");

    screen_render(&scr);

    struct timespec ts = {0, 16 * 1000 * 1000};
    nanosleep(&ts, NULL);
  }

  double start = now_sec();

  int drawStartIdx = 0;
  while (!g_shouldQuit) {
    double tnow = now_sec() - start;

    for (;;) {
      int k = read_key_nonblock();
      if (k < 0) break;
      if (k == 'q' || k == 'Q') { g_shouldQuit = 1; break; }
      if (k == '+') st.scroll = MIN(40.0, st.scroll + 1.0);
      if (k == '-') st.scroll = MAX(6.0,  st.scroll - 1.0);

      int needsMore = 0;
      int col = map_key_to_col(k, &needsMore);
      if (needsMore) col = map_arrow_seq_to_col();
      if (col >= 0) try_hit(c, &st, col, tnow);
    }

    apply_auto_miss(c, &st, tnow);

    if (tnow > endT) break;

    screen_clear(&scr);

    char h0[512];
    snprintf(h0, sizeof(h0), "%s", sf->title);
    draw_text(&scr, 2, 0, C_TEXT, h0);

    char h1[512];
    snprintf(h1, sizeof(h1), "Chart: %s %d   Offset: %.3fs   Scroll: %.0f",
             c->difficulty[0]?c->difficulty:"(unknown)", c->meter, sf->offset, st.scroll);
    draw_text(&scr, 2, 1, C_DIM, h1);

    char h2[512];
    snprintf(h2, sizeof(h2), "Time: %6.2fs   Score: %6d   Combo: %4d  (Max %d)",
             tnow, st.score, st.combo, st.maxCombo);
    draw_text(&scr, 2, 2, C_DIM, h2);

    draw_box(&scr, boxX0, boxY0, boxX1, boxY1);

    for (int y = boxY0 + 1; y <= boxY1 - 1; y++) {
      for (int col = 0; col < 4; col++) set_cell(&scr, laneX[col], y, '.', C_DIM);
    }

    for (int col = 0; col < 4; col++) {
      set_cell(&scr, laneX[col], receptorY, '=', col_color(col));
      const char *key = (col==0)?"D":(col==1)?"F":(col==2)?"J":"K";
      draw_text(&scr, laneX[col], receptorY+1, C_DIM, key);
    }

    double lookahead = (double)(receptorY - (boxY0 + 1)) / st.scroll;
    double behind = 0.35;

    while (drawStartIdx < c->nnotes && c->notes[drawStartIdx].time < tnow - behind - 1.0) {
      drawStartIdx++;
    }

    for (int i = drawStartIdx; i < c->nnotes; i++) {
      Note *n = &c->notes[i];
      if (n->hit != 0) continue;

      double dt = n->time - tnow;
      if (dt < -behind) continue;
      if (dt > lookahead) break;

      int y = receptorY - (int)lrint(dt * st.scroll);
      if (y <= boxY0 || y >= boxY1) continue;

      int x = laneX[n->col];
      set_cell(&scr, x, y, 'O', col_color(n->col));
    }

    char f0[512];
    snprintf(f0, sizeof(f0), "Last: %-5s   MARV %d  PERF %d  GREAT %d  GOOD %d  MISS %d",
             st.lastJudge, st.marv, st.perf, st.great, st.good, st.miss);
    draw_text(&scr, 2, th - 2, C_TEXT, f0);

    draw_text(&scr, 2, th - 1, C_DIM, "Controls: D F J K (or arrows)   +/- scroll speed   Q quit");

    int lastX = 2 + 6;
    unsigned char jc = C_TEXT;
    if (strcmp(st.lastJudge, "MARV") == 0) jc = judge_color(0);
    else if (strcmp(st.lastJudge, "PERF") == 0) jc = judge_color(1);
    else if (strcmp(st.lastJudge, "GREAT") == 0) jc = judge_color(2);
    else if (strcmp(st.lastJudge, "GOOD") == 0) jc = judge_color(3);
    else if (strcmp(st.lastJudge, "MISS") == 0) jc = judge_color(4);
    draw_text(&scr, lastX, th - 2, jc, st.lastJudge);

    screen_render(&scr);

    struct timespec ts = {0, 16 * 1000 * 1000};
    nanosleep(&ts, NULL);
  }

  screen_clear(&scr);
  draw_text(&scr, 2, 1, C_TEXT, "Results");
  draw_text(&scr, 2, 2, C_DIM,  "-------");
  char r0[256];
  snprintf(r0, sizeof(r0), "Song: %s", sf->title);
  draw_text(&scr, 2, 4, C_TEXT, r0);

  char r1[256];
  snprintf(r1, sizeof(r1), "Chart: %s %d", c->difficulty[0]?c->difficulty:"(unknown)", c->meter);
  draw_text(&scr, 2, 5, C_TEXT, r1);

  char r2[256];
  snprintf(r2, sizeof(r2), "Score: %d   Max Combo: %d", st.score, st.maxCombo);
  draw_text(&scr, 2, 7, C_TEXT, r2);

  char r3[256];
  snprintf(r3, sizeof(r3), "MARV %d  PERF %d  GREAT %d  GOOD %d  MISS %d", st.marv, st.perf, st.great, st.good, st.miss);
  draw_text(&scr, 2, 8, C_TEXT, r3);

  draw_text(&scr, 2, 10, C_DIM, "Press any key to exit...");
  screen_render(&scr);

  while (read_key_nonblock() < 0) {
    struct timespec ts = {0, 30 * 1000 * 1000};
    nanosleep(&ts, NULL);
  }

  screen_free(&scr);
}

// ---------- Main / selection ----------
static int choose_chart_interactive(const Simfile *sf) {
  if (sf->ncharts <= 0) return -1;
  if (sf->ncharts == 1) return 0;

  fprintf(stderr, "Found %d dance-single chart(s):\n", sf->ncharts);
  for (int i = 0; i < sf->ncharts; i++) {
    const Chart *c = &sf->charts[i];
    fprintf(stderr, "  [%d] %-12s  meter %2d  notes %5d\n",
            i, c->difficulty[0]?c->difficulty:"(unknown)", c->meter, c->nnotes);
  }
  fprintf(stderr, "Choose chart index: ");
  fflush(stderr);

  int idx = 0;
  if (scanf("%d", &idx) != 1) return 0;
  if (idx < 0) idx = 0;
  if (idx >= sf->ncharts) idx = sf->ncharts - 1;
  return idx;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <simfile.sm|simfile.ssc> [chartIndex]\n", argv[0]);
    return 2;
  }

  size_t sz = 0;
  char *text = read_file_all(argv[1], &sz);

  Simfile sf;
  sim_init(&sf);
  parse_simfile_text(&sf, text);
  free(text);

  if (sf.ncharts <= 0) {
    sim_free(&sf);
    die("No supported charts found (needs dance-single).");
  }

  int idx = -1;
  if (argc >= 3) {
    idx = atoi(argv[2]);
    idx = MIN(MAX(idx, 0), sf.ncharts - 1);
  } else {
    idx = choose_chart_interactive(&sf);
  }

  gameplay_loop(&sf, &sf.charts[idx]);

  sim_free(&sf);
  return 0;
}
