#ifndef SHELL_H
#define SHELL_H

#define SHELL_MAX_LINE 128
#define SHELL_MAX_ARGS 8

void shell_main(void);

// 行读取
int shell_readline(char *buf, int max);

// 解析器
int shell_parse(char *line, char **argv, int max);

// 命令执行
void shell_exec(int argc, char **argv);

#endif
