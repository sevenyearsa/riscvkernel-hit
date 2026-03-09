// app.c
// A tiny user-space shell for GeminiOS.

#define SYS_READ  63
#define SYS_WRITE 64
#define SYS_EXIT  93
#define SYS_SCHED_YIELD 124

#define INPUT_MAX 128

static void sys_putchar(char c) {
    asm volatile(
        "mv a7, %0\n"
        "mv a0, %1\n"
        "ecall\n"
        :
        : "r"(SYS_WRITE), "r"(c)
        : "a0", "a7"
    );
}

static long sys_read(unsigned int fd, char *buf, unsigned long count) {
    long ret;

    asm volatile(
        "mv a7, %1\n"
        "mv a0, %2\n"
        "mv a1, %3\n"
        "mv a2, %4\n"
        "ecall\n"
        "mv %0, a0\n"
        : "=r"(ret)
        : "r"(SYS_READ), "r"(fd), "r"(buf), "r"(count)
        : "a0", "a1", "a2", "a7"
    );

    return ret;
}

static void sys_yield(void) {
    asm volatile(
        "mv a7, %0\n"
        "ecall\n"
        :
        : "r"(SYS_SCHED_YIELD)
        : "a7"
    );
}

static void sys_exit(int status) {
    asm volatile(
        "mv a7, %0\n"
        "mv a0, %1\n"
        "ecall\n"
        :
        : "r"(SYS_EXIT), "r"(status)
        : "a0", "a7"
    );

    while (1) {
    }
}

static void sys_print(const char *s) {
    while (*s) {
        sys_putchar(*s++);
    }
}

static int str_eq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }

    return *a == 0 && *b == 0;
}

static int str_starts_with(const char *s, const char *prefix) {
    while (*prefix) {
        if (*s != *prefix) {
            return 0;
        }
        s++;
        prefix++;
    }

    return 1;
}

static void shell_prompt(void) {
    sys_print("geminios> ");
}

static void shell_banner(void) {
    sys_print("\n============================================\n");
    sys_print("  GeminiOS User Shell\n");
    sys_print("  Type 'help' to list built-in commands.\n");
    sys_print("============================================\n\n");
}

static int shell_read_line(char *buf, int cap) {
    int len = 0;

    while (1) {
        char ch = 0;

        if (sys_read(0, &ch, 1) <= 0) {
            continue;
        }

        if (ch == '\n') {
            sys_print("\n");
            buf[len] = 0;
            return len;
        }

        if (ch == '\b' || ch == 127) {
            if (len > 0) {
                len--;
                sys_print("\b \b");
            }
            continue;
        }

        if (ch < ' ' || ch > '~') {
            continue;
        }

        if (len + 1 < cap) {
            buf[len++] = ch;
            sys_putchar(ch);
        }
    }
}

static const char *skip_spaces(const char *s) {
    while (*s == ' ') {
        s++;
    }
    return s;
}

static void run_command(const char *line) {
    line = skip_spaces(line);

    if (*line == 0) {
        return;
    }

    if (str_eq(line, "help")) {
        sys_print("Built-ins: help, echo, clear, yield, about, exit\n");
        return;
    }

    if (str_starts_with(line, "echo")) {
        line += 4;
        line = skip_spaces(line);
        sys_print(line);
        sys_print("\n");
        return;
    }

    if (str_eq(line, "clear")) {
        sys_print("\033[2J\033[H");
        return;
    }

    if (str_eq(line, "yield")) {
        sys_print("yielding CPU...\n");
        sys_yield();
        return;
    }

    if (str_eq(line, "about")) {
        sys_print("GeminiOS shell running in user mode via execve().\n");
        return;
    }

    if (str_eq(line, "exit")) {
        sys_print("bye.\n");
        sys_exit(0);
    }

    sys_print("unknown command: ");
    sys_print(line);
    sys_print("\n");
}

void _start(void) {
    char line[INPUT_MAX];

    shell_banner();

    while (1) {
        shell_prompt();
        shell_read_line(line, INPUT_MAX);
        run_command(line);
    }
}
