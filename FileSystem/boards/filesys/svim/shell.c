#include <stdio.h>
#include <string.h>
#include "shell.h"
#include "vim.h"
#include "fs.h"
#include "comm.h"

static char linebuf[SHELL_MAX_LINE];
static char path[128];

static void normalize_path(char *path)
{
    char *src = path;
    char *dst = path;

    if (*src != '/') {
        return;
    }

    while (*src) {
        if (*src == '/' && *(src + 1) == '/') {
            src++;
            continue;
        }

        if (src[0] == '/' && src[1] == '.' && (src[2] == '/' || src[2] == '\0')) {
            src += 2;
            continue;
        }

        if (src[0] == '/' && src[1] == '.' && src[2] == '.' &&
            (src[3] == '/' || src[3] == '\0')) {

            if (dst != path) {
                dst--;
                while (dst > path && *dst != '/') dst--;
            }
            src += 3;
            continue;
        }

        *dst++ = *src++;
    }

    if (dst > path + 1 && *(dst - 1) == '/')
        dst--;

    *dst = '\0';
}

static char cwd[128] = "";

static void make_abs_path(char *out, const char *path)
{
    if (path[0] == '/') {
        path++;
    }

    if (cwd[0] == '\0') {
        snprintf(out, 128, "%s", path);
    } else {
        snprintf(out, 128, "%s/%s", cwd, path);
    }

    normalize_path(out);
}



void shell_cmd_ls(int argc, char **argv)
{
    char path[128];

    if (argc == 1) {
        strcpy(path, cwd);
    } else {
        make_abs_path(path, argv[1]);
    }

    struct dirent ents[16];
    int nread = 0;

    if (fs_readdir(path, ents, 16, &nread) != 0) {
        comm_write("ls: cannot open directory\r\n", 27);
        return;
    }

    for (int i = 0; i < nread; i++) {
        comm_write(ents[i].name, strlen(ents[i].name));
        comm_write("\r\n", 2);
    }
}

static int fs_is_dir(const char *path)
{
    struct dirent tmp[1];
    int nread = 0;
    return fs_readdir(path, tmp, 1, &nread) == 0;
}



static char *argv_buf[SHELL_MAX_ARGS];
int shell_readline(char *buf, int max)
{
    int pos = 0;
    for (;;) {
        char c = comm_getc();

        if (c == '\r' || c == '\n') {
            comm_putc('\r');
            comm_putc('\n');
            buf[pos] = 0;
            return pos;
        }

        if (c == '\b' || c == 127) {
            if (pos > 0) {
                pos--;
                comm_write("\b \b", 3);
            }
            continue;
        }

        if (pos < max - 1) {
            buf[pos++] = c;
            comm_putc(c);
        }
    }
}

void shell_main(void)
{
    while (1) {
        comm_write("> ", 2);

        int len = shell_readline(linebuf, SHELL_MAX_LINE);
        if (len <= 0) continue;

        int argc = shell_parse(linebuf, argv_buf, SHELL_MAX_ARGS);
        if (argc == 0) continue;

        shell_exec(argc, argv_buf);
    }
}

/* ---------------- ½âÎöÆ÷ ---------------- */
int shell_parse(char *line, char **argv, int max)
{
    int argc = 0;
    while (*line && argc < max) {
        while (*line == ' ') line++;
        if (!*line) break;

        argv[argc++] = line;

        while (*line && *line != ' ') line++;
        if (*line) *line++ = 0;
    }
    return argc;
}

static struct dirent ls_ents[64];

int cmd_ls(int argc, char **argv)
{
    memset(path, 0, sizeof(path));
    if (argc > 1)
        make_abs_path(path, argv[1]);
    else
        strcpy(path, cwd);

    int n = 0;
    if (fs_readdir(path, ls_ents, 64, &n) < 0) {
        comm_write("ls: cannot open ", 16);
        comm_write(path, strlen(path));
        comm_write("\r\n", 2);
        return -1;
    }

    for (int i = 0; i < n; i++) {
        comm_write(ls_ents[i].name, strlen(ls_ents[i].name));
        comm_write("  ", 2);
    }

    comm_write("\r\n", 2);
    return 0;
}

static char cat_buf[128];
int cmd_cat(int argc, char **argv)
{
    if (argc < 2) {
        comm_write("usage: cat FILE\r\n", 18);
        return -1;
    }

    memset(path, 0, sizeof(path));
    make_abs_path(path, argv[1]);

    struct inode *ino;
    if (fs_open(path, 0, &ino) < 0) {
        comm_write("cat: cannot open ", 18);
        comm_write(argv[1], strlen(argv[1]));
        comm_write("\r\n", 2);
        return -1;
    }

    uint32_t off = 0;
    int r;

    while ((r = fs_read(ino, off, cat_buf, sizeof(cat_buf))) > 0) {
        for (int i = 0; i < r; i++) {
            if (cat_buf[i] == '\n') {
                comm_write("\r\n", 2);
            } else {
                comm_putc(cat_buf[i]);
            }
        }
        off += r;
    }

    fs_close(ino);
    return 0;
}



int cmd_touch(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: touch FILE\n");
        return -1;
    }

    struct inode *ino;
    if (fs_open(argv[1], O_CREAT, &ino) < 0) {
        printf("touch: cannot create %s\n", argv[1]);
        return -1;
    }
    fs_close(ino);
    return 0;
}

int cmd_mkdir(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: mkdir DIR\n");
        return -1;
    }

    struct inode *ino;
    if (fs_mkdir(argv[1], &ino) < 0) {
        printf("mkdir: cannot create %s\n", argv[1]);
        return -1;
    }
    return 0;
}

static char abs[64];
int cmd_vim(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: edit FILE\n");
        return -1;
    }
    memset(abs, 0, sizeof(abs));
    make_abs_path(abs, argv[1]);

    vim_main(abs);
    return 0;
}


int cmd_cd(int argc, char **argv)
{
    if (argc < 2) {
        comm_write("usage: cd <dir>\r\n", 17);
        return -1;
    }
    memset(path, 0, sizeof(path));
    make_abs_path(path, argv[1]);

    if (!fs_is_dir(path)) {
        comm_write("cd: no such directory\r\n", 25);
        return -1;
    }

    strcpy(cwd, path);
    return 0;
}

int cmd_sync(int argc, char **argv)
{
    if (argc < 2) {
        comm_write("usage: sync \r\n", 17);
        return -1;
    }
    fs_sync();
    return 0;
}

struct cmd_entry {
    const char *name;
    int (*func)(int argc, char **argv);
};
static struct cmd_entry cmd_table[] = {
        {"ls",    cmd_ls},
        {"cat",   cmd_cat},
        {"touch", cmd_touch},
        {"mkdir", cmd_mkdir},
        {"vim",  cmd_vim},
        {"cd",    cmd_cd},
        {"sync",    cmd_sync},
        {NULL, NULL}
};

void shell_exec(int argc, char **argv)
{
    for (int i = 0; cmd_table[i].name; i++) {
        if (strcmp(argv[0], cmd_table[i].name) == 0) {
            cmd_table[i].func(argc, argv);
            return;
        }
    }
    printf("unknown command: %s\n", argv[0]);
}
