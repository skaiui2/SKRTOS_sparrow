#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "vim.h"
#include "fs.h"
#include "comm.h"

struct line {
    char *data;
    int len;
};

struct buffer {
    struct line *lines;
    int line_count;
    int cursor_x;
    int cursor_y;
};

static struct line empty_line = { .data = "", .len = 0 };
static struct buffer empty_buf = {
        .lines = &empty_line,
        .line_count = 1,
        .cursor_x = 0,
        .cursor_y = 0,
};

//edit
void buf_insert_char(struct buffer *b, char c)
{
    struct line *ln = &b->lines[b->cursor_y];

    char *tmp = realloc(ln->data, ln->len + 2);
    if (!tmp) {
        return;
    }
    ln->data = tmp;
    memmove(ln->data + b->cursor_x + 1,
            ln->data + b->cursor_x,
            ln->len - b->cursor_x + 1);

    ln->data[b->cursor_x] = c;
    ln->len++;
    b->cursor_x++;
}

void buf_backspace(struct buffer *b)
{
    struct line *ln = &b->lines[b->cursor_y];

    if (b->cursor_x == 0) {
        if (b->cursor_y == 0) return;

        int prev = b->cursor_y - 1;
        struct line *pl = &b->lines[prev];

        int old_len = pl->len;
        char *tmp = realloc(pl->data, pl->len + ln->len + 1);
        if (!tmp) {
            return;
        }
        pl->data = tmp;
        memcpy(pl->data + pl->len, ln->data, ln->len + 1);
        pl->len += ln->len;

        free(ln->data);
        memmove(&b->lines[b->cursor_y],
                &b->lines[b->cursor_y + 1],
                sizeof(struct line) * (b->line_count - b->cursor_y - 1));
        b->line_count--;

        b->cursor_y--;
        b->cursor_x = old_len;
        return;
    }

    memmove(ln->data + b->cursor_x - 1,
            ln->data + b->cursor_x,
            ln->len - b->cursor_x + 1);

    ln->len--;
    b->cursor_x--;
}

void buf_newline(struct buffer *b)
{
    struct line *ln = &b->lines[b->cursor_y];

    char *right = strdup(ln->data + b->cursor_x);
    int right_len = (int)strlen(right);

    ln->data[b->cursor_x] = 0;
    ln->len = b->cursor_x;

    struct line *tmp = realloc(b->lines, sizeof(struct line) * (b->line_count + 1));
    if (!tmp) {
        return;
    }
    b->lines = tmp;

    memmove(&b->lines[b->cursor_y + 2],
            &b->lines[b->cursor_y + 1],
            sizeof(struct line) * (b->line_count - b->cursor_y - 1));

    b->lines[b->cursor_y + 1].data = right;
    b->lines[b->cursor_y + 1].len  = right_len;

    b->line_count++;

    b->cursor_y++;
    b->cursor_x = 0;
}

void buf_delete_char(struct buffer *b)
{
    struct line *ln = &b->lines[b->cursor_y];

    if (b->cursor_x >= ln->len) return;

    memmove(ln->data + b->cursor_x,
            ln->data + b->cursor_x + 1,
            ln->len - b->cursor_x);

    ln->len--;
}

static char the_load_buf[512];
int buf_load(struct buffer *b, const char *path)
{
    struct inode *ino;
    if (fs_open(path, 0, &ino) < 0) {
        b->lines = malloc(sizeof(struct line));
        b->lines[0].data = strdup("");
        b->lines[0].len  = 0;
        b->line_count    = 1;
        b->cursor_x      = 0;
        b->cursor_y      = 0;
        return 0;
    }
    memset(the_load_buf, 0, sizeof(the_load_buf));
    int r = fs_read(ino, 0, the_load_buf, sizeof(the_load_buf) - 1);
    fs_close(ino);

    if (r < 0) r = 0;
    the_load_buf[r] = 0;

    b->line_count = 0;
    b->lines      = NULL;

    char *save_ptr;
    char *p = strtok_r(the_load_buf, "\n", &save_ptr);
    while (p) {
        struct line *new_lines = realloc(b->lines, sizeof(struct line) * (b->line_count + 1));
        if (!new_lines) {
            return -1;
        }
        b->lines = new_lines;
        b->lines[b->line_count].data = strdup(p);
        b->lines[b->line_count].len  = (int)strlen(p);
        b->line_count++;
        p = strtok_r(NULL, "\n", &save_ptr);
    }

    if (b->line_count == 0) {
        b->lines = malloc(sizeof(struct line));
        b->lines[0].data = strdup("");
        b->lines[0].len  = 0;
        b->line_count    = 1;
    }

    b->cursor_x = 0;
    b->cursor_y = 0;
    return 0;
}

int buf_save(struct buffer *b, const char *path)
{
    struct inode *ino;
    if (fs_open(path, O_CREAT, &ino) < 0)
        return -1;

    //把所有内容拼成一个连续 buffer
    uint32_t total = 0;
    for (int i = 0; i < b->line_count; i++)
        total += b->lines[i].len + 1;   // +1 for '\n'

    uint8_t *mem = malloc(total);
    if (!mem) {
        fs_close(ino);
        return -1;
    }

    uint32_t off = 0;
    for (int i = 0; i < b->line_count; i++) {
        memcpy(mem + off, b->lines[i].data, b->lines[i].len);
        off += b->lines[i].len;
        mem[off++] = '\n';
    }

    fs_truncate(ino, 0);

    fs_write(ino, 0, mem, total);

    free(mem);
    fs_close(ino);
    return 0;
}

void buf_free(struct buffer *b)
{
    if (!b->lines) return;

    for (int i = 0; i < b->line_count; i++) {
        free(b->lines[i].data);
    }

    free(b->lines);
    b->lines = NULL;
    b->line_count = 0;
}


//屏幕刷新
static void screen_draw(struct buffer *b, int insert_mode)
{
    char buf[64];

    comm->write("\x1b[2J", 4);  // 清屏
    comm->write("\x1b[H", 3);   // 光标到左上角

    for (int i = 0; i < b->line_count; i++) {
        comm->write(b->lines[i].data, b->lines[i].len);
        comm->write("\r\n", 2);
    }

    int n = snprintf(buf, sizeof(buf), "\x1b[%d;1H", b->line_count + 2);
    comm->write(buf, n);
    if (insert_mode) {
        comm->write("-- INSERT --", 12);
    }

    n = snprintf(buf, sizeof(buf), "\x1b[%d;%dH", b->cursor_y + 1, b->cursor_x + 1);
    comm->write(buf, n);
}

void vim_main(const char *path)
{
    struct buffer buf = {0};
    buf_load(&buf, path);

    int insert_mode = 0;

    for (;;) {
        if (buf.line_count <= 0) {
            buf.line_count = 1;
            struct line *temp = realloc(buf.lines, sizeof(struct line));
            if (!temp) {
                return;
            }
            buf.lines = temp;
            buf.lines[0].data = strdup("");
            buf.lines[0].len  = 0;
        }

        if (buf.cursor_y >= buf.line_count)
            buf.cursor_y = buf.line_count - 1;
        if (buf.cursor_y < 0)
            buf.cursor_y = 0;

        if (buf.cursor_x > buf.lines[buf.cursor_y].len)
            buf.cursor_x = buf.lines[buf.cursor_y].len;
        if (buf.cursor_x < 0)
            buf.cursor_x = 0;

        screen_draw(&buf, insert_mode);

        char c = comm->getc();

        // ESC 处理
        if ((unsigned char)c == 0x1B) {
            insert_mode = 0;
            continue;
        }

        // NORMAL 模式
        if (!insert_mode) {
            if (c == 'i') {
                insert_mode = 1;
            } else if (c == 'h') {
                if (buf.cursor_x > 0) buf.cursor_x--;
            } else if (c == 'l') {
                if (buf.cursor_x < buf.lines[buf.cursor_y].len) buf.cursor_x++;
            } else if (c == 'j') {
                if (buf.cursor_y < buf.line_count - 1) buf.cursor_y++;
            } else if (c == 'k') {
                if (buf.cursor_y > 0) buf.cursor_y--;
            } else if (c == 'x') {
                buf_delete_char(&buf);
            } else if (c == ':') {
                char tmp[64];
                int n = snprintf(tmp, sizeof(tmp), "\x1b[%d;1H:", buf.line_count + 2);
                comm->write(tmp, n);

                char cmd[16];
                int pos = 0;

                while (1) {
                    char k = comm->getc();
                    if (k == '\r' || k == '\n') {
                        cmd[pos] = 0;
                        break;
                    }
                    if (pos < 15) {
                        cmd[pos++] = k;
                        comm->putc(k);
                    }
                }

                if (strcmp(cmd, "w") == 0) {
                    buf_save(&buf, path);
                } else if (strcmp(cmd, "q") == 0) {
                    screen_draw(&empty_buf, 0);

                    comm->write("\r\n", 2);
                    buf_free(&buf);
                    return;
                }

                continue;
            }

            continue;
        }

        // INSERT 模式
        if (c == '\r' || c == '\n') {
            buf_newline(&buf);
            continue;
        }

        if (c == 127 || c == '\b') {
            buf_backspace(&buf);
            continue;
        }

        buf_insert_char(&buf, c);
    }
}
