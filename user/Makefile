CC = riscv64-unknown-elf-gcc
CFLAGS = -nostdlib -fno-builtin -mcmodel=medany -g -Wall

all: app_bin.h

app.elf: app.c app.ld
	$(CC) $(CFLAGS) -T app.ld app.c -o app.elf

app_bin.h: app.elf
	xxd -i app.elf > app_bin.h

clean:
	rm -f app.elf app_bin.h
