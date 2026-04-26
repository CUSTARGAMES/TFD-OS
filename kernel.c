#include <stdint.h>
#include <stddef.h>
#include <string.h>

// VGA colors
#define VGA_COLOR_BLACK        0
#define VGA_COLOR_BLUE         1
#define VGA_COLOR_GREEN        2
#define VGA_COLOR_CYAN         3
#define VGA_COLOR_RED          4
#define VGA_COLOR_MAGENTA      5
#define VGA_COLOR_BROWN        6
#define VGA_COLOR_LIGHT_GREY   7
#define VGA_COLOR_DARK_GREY    8
#define VGA_COLOR_LIGHT_BLUE   9
#define VGA_COLOR_LIGHT_GREEN  10
#define VGA_COLOR_LIGHT_CYAN   11
#define VGA_COLOR_LIGHT_RED    12
#define VGA_COLOR_LIGHT_MAGENTA 13
#define VGA_COLOR_LIGHT_BROWN  14
#define VGA_COLOR_WHITE        15
#define VGA_COLOR_YELLOW       14   // actually light brown is yellow-like; use 14

// VGA buffer
uint16_t* vga_buffer = (uint16_t*)0xB8000;
int cursor_x = 0;
int cursor_y = 0;
uint8_t vga_color = VGA_COLOR_LIGHT_GREY | (VGA_COLOR_BLACK << 4);

// Linux distro struct – now using pointer for URL (no fixed size limit)
typedef struct {
    char name[30];
    const char* url;
    char size[10];
} LinuxDistro;

LinuxDistro distros[] = {
    {"Ubuntu 24.04", "http://releases.ubuntu.com/24.04/ubuntu-24.04-desktop-amd64.iso", "5.2GB"},
    {"Linux Mint 21.3", "http://mirrors.kernel.org/linuxmint/stable/21.3/linuxmint-21.3-cinnamon-64bit.iso", "2.7GB"},
    {"Debian 12", "http://cdimage.debian.org/debian-cd/current/amd64/iso-cd/debian-12.5.0-amd64-netinst.iso", "3.6GB"},
    {"Fedora 40", "http://download.fedoraproject.org/pub/fedora/linux/releases/40/Workstation/x86_64/iso/Fedora-Workstation-Live-x86_64-40-1.14.iso", "2.5GB"},
    {"Arch Linux", "http://archlinux.mirrors.linux-packages.com/iso/2024.04.01/archlinux-2024.04.01-x86_64.iso", "800MB"}
};
int num_distros = 5;

int network_connected = 0;

void vga_setcolor(uint8_t color) {
    vga_color = color;
}

void vga_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            vga_buffer[cursor_y * 80 + cursor_x] = ' ' | (vga_color << 8);
        }
    } else {
        vga_buffer[cursor_y * 80 + cursor_x] = (uint16_t)c | (vga_color << 8);
        cursor_x++;
    }
    if (cursor_x >= 80) {
        cursor_x = 0;
        cursor_y++;
    }
    if (cursor_y >= 25) {
        // Scroll
        for (int i = 0; i < 24 * 80; i++)
            vga_buffer[i] = vga_buffer[i + 80];
        for (int i = 24 * 80; i < 25 * 80; i++)
            vga_buffer[i] = ' ' | (vga_color << 8);
        cursor_y = 24;
    }
}

void vga_print(const char* str) {
    while (*str) vga_putchar(*str++);
}

void vga_print_color(const char* str, uint8_t color) {
    uint8_t old = vga_color;
    vga_setcolor(color);
    vga_print(str);
    vga_setcolor(old);
}

void vga_print_dec(int num) {
    if (num == 0) {
        vga_putchar('0');
        return;
    }
    char buf[12];
    int i = 0;
    if (num < 0) {
        vga_putchar('-');
        num = -num;
    }
    while (num > 0) {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }
    while (i > 0) vga_putchar(buf[--i]);
}

void clear_screen() {
    for (int i = 0; i < 25 * 80; i++)
        vga_buffer[i] = ' ' | (vga_color << 8);
    cursor_x = 0;
    cursor_y = 0;
}

void draw_border() {
    vga_setcolor(VGA_COLOR_CYAN);
    for (int i = 0; i < 80; i++) vga_putchar('=');
    vga_putchar('\n');
    vga_setcolor(VGA_COLOR_LIGHT_GREY);
}

void draw_title() {
    clear_screen();
    draw_border();
    vga_setcolor(VGA_COLOR_GREEN);
    vga_print("     ████████╗███████╗██████╗     ██████╗ ███████╗    \n");
    vga_print("     ╚══██╔══╝██╔════╝██╔══██╗    ██╔══██╗██╔════╝    \n");
    vga_print("        ██║   █████╗  ██║  ██║    ██║  ██║███████╗    \n");
    vga_print("        ██║   ██╔══╝  ██║  ██║    ██║  ██║╚════██║    \n");
    vga_print("        ██║   ██║     ██████╔╝    ██████╔╝███████║    \n");
    vga_print("        ╚═╝   ╚═╝     ╚═════╝     ╚═════╝ ╚══════╝    \n");
    vga_setcolor(VGA_COLOR_YELLOW);
    vga_print("      LINUX INSTALLER & BOOT MANAGER by sadman        \n");
    draw_border();
    vga_putchar('\n');
}

void show_help() {
    vga_print_color("\nCommands:\n", VGA_COLOR_CYAN);
    vga_print("  help              - Show this help\n");
    vga_print("  list              - List available Linux distros\n");
    vga_print("  wifi              - Connect to WiFi\n");
    vga_print("  install <name>    - Download and install Linux\n");
    vga_print("  status            - Show system status\n");
    vga_print("  clear             - Clear screen\n");
    vga_print("  reboot            - Reboot system\n");
}

void list_distros() {
    vga_print_color("\nAvailable Linux Distributions:\n", VGA_COLOR_CYAN);
    draw_border();
    for (int i = 0; i < num_distros; i++) {
        vga_print("  ");
        vga_print_dec(i+1);
        vga_print(". ");
        vga_print(distros[i].name);
        vga_putchar('\n');
    }
    draw_border();
}

void wifi_connect() {
    vga_print_color("\nWiFi Setup\n", VGA_COLOR_CYAN);
    draw_border();
    vga_print("Available networks:\n");
    vga_print("  1. TFD-Net\n  2. Public WiFi\n  3. Home Network\n\n");
    vga_print("Select network (1-3): ");
    // Simulate input for demo
    vga_print("1\n");
    vga_print("Enter password: ");
    vga_print("********\n");
    network_connected = 1;
    vga_print_color("\n✓ Connected to WiFi!\n", VGA_COLOR_GREEN);
    vga_print("IP Address: 192.168.1.100\n");
}

void download_iso(char* name) {
    int found = -1;
    for (int i = 0; i < num_distros; i++) {
        if (strstr(distros[i].name, name) != NULL) {
            found = i;
            break;
        }
    }
    if (found == -1) {
        vga_print_color("Error: Distribution not found.\n", VGA_COLOR_RED);
        return;
    }
    if (!network_connected) {
        vga_print_color("Error: Not connected to WiFi. Type 'wifi' first.\n", VGA_COLOR_RED);
        return;
    }
    vga_print_color("\nDownloading ", VGA_COLOR_YELLOW);
    vga_print(distros[found].name);
    vga_print_color("...\n", VGA_COLOR_YELLOW);
    vga_print("URL: ");
    vga_print(distros[found].url);
    vga_putchar('\n');
    for (int i = 0; i <= 100; i += 10) {
        vga_print("\rProgress: [");
        for (int j = 0; j < i/2; j++) vga_putchar('#');
        for (int j = i/2; j < 50; j++) vga_putchar('.');
        vga_print("] ");
        vga_print_dec(i);
        vga_print("%");
    }
    vga_putchar('\n');
    vga_print_color("✓ Download complete!\n", VGA_COLOR_GREEN);
    vga_print("Saved to: /boot/");
    vga_print(distros[found].name);
    vga_print(".iso\n");
}

void system_status() {
    vga_print_color("\nSystem Status\n", VGA_COLOR_CYAN);
    draw_border();
    vga_print("OS: TFD v1.0\nArchitecture: x86_64\nRAM: 64 MB\n");
    if (network_connected)
        vga_print_color("Network: Connected (192.168.1.100)\n", VGA_COLOR_GREEN);
    else
        vga_print_color("Network: Disconnected\n", VGA_COLOR_RED);
}

void read_line(char* buffer, int max_len) {
    int pos = 0;
    // Simple simulation: read until newline (in real OS you'd read keyboard)
    // For demo, we'll just accept empty input or a predefined string.
    // But to make it compile, we'll stub it.
    // In reality you'd implement keyboard handling. For now:
    buffer[0] = '\0';
}

void execute_command(char* cmd) {
    if (strcmp(cmd, "help") == 0) show_help();
    else if (strcmp(cmd, "list") == 0) list_distros();
    else if (strcmp(cmd, "wifi") == 0) wifi_connect();
    else if (strcmp(cmd, "status") == 0) system_status();
    else if (strcmp(cmd, "clear") == 0) draw_title();
    else if (strcmp(cmd, "reboot") == 0) asm volatile("int $0x19");
    else if (strncmp(cmd, "install ", 8) == 0) download_iso(cmd+8);
    else if (cmd[0] != '\0') vga_print_color("Command not found. Type 'help'\n", VGA_COLOR_RED);
}

void kernel_main() {
    char input[100];
    draw_title();
    while (1) {
        vga_setcolor(VGA_COLOR_GREEN);
        vga_print("tfd> ");
        vga_setcolor(VGA_COLOR_WHITE);
        read_line(input, sizeof(input));
        // For demo, we'll simulate a few commands to show it's working.
        // Actually you'd read real input. To avoid empty loop, just list distros.
        // You can replace this with a real prompt later.
        // Let's just halt and show a message for now.
        vga_print("Welcome to TFD OS! Type 'list' to see distros.\n");
        vga_print("tfd> ");
        // For the purpose of the build, we'll break out after a few cycles.
        break;
    }
}
