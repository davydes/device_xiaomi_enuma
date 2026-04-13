/*
 * Stylus Bridge - translates KEY_PAGEUP/KEY_PAGEDOWN from
 * Xiaomi Smart Pen BLE keyboard into BTN_STYLUS/BTN_STYLUS2
 * via uinput for fusion with NVTCapacitivePen.
 *
 * Uses inotify to wait for pen — zero CPU when pen absent.
 * read() blocks when pen idle — zero CPU when no buttons pressed.
 */

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/inotify.h>
#include <linux/input.h>
#include <linux/uinput.h>

#define LOG_TAG "StylusBridge"
#include <log/log.h>

static void signal_handler(int sig) {
    ALOGE("Caught signal %d, exiting", sig);
    _exit(sig);
}

#define DEVICE_NAME "xiaomi-stylus-bridge"
#define PEN_DEVICE_NAME "Xiaomi Smart Pen"
#define INPUT_DIR "/dev/input"
#define MAX_PATH 256

static void build_path(char *dst, const char *dir, const char *name) {
    int n = 0;
    while (*dir && n < MAX_PATH - 2) dst[n++] = *dir++;
    dst[n++] = '/';
    while (*name && n < MAX_PATH - 1) dst[n++] = *name++;
    dst[n] = '\0';
}

static int try_open_pen(void) {
    DIR *dir = opendir(INPUT_DIR);
    if (!dir)
        return -1;

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strncmp(ent->d_name, "event", 5) != 0)
            continue;

        char path[MAX_PATH];
        build_path(path, INPUT_DIR, ent->d_name);

        int fd = open(path, O_RDONLY);
        if (fd < 0)
            continue;

        char name[256] = {0};
        ioctl(fd, EVIOCGNAME(sizeof(name)), name);

        if (strcmp(name, PEN_DEVICE_NAME) == 0) {
            closedir(dir);
            /* Grab exclusive access — suppress original events from reaching Android */
            if (ioctl(fd, EVIOCGRAB, 1) < 0) {
                ALOGE("EVIOCGRAB failed: %s", strerror(errno));
            }
            ALOGI("Found pen: %s (grabbed)", path);
            return fd;
        }
        close(fd);
    }
    closedir(dir);
    return -1;
}

static int wait_for_pen(void) {
    int fd = try_open_pen();
    if (fd >= 0)
        return fd;

    ALOGI("Waiting for pen (inotify)...");

    int ifd = inotify_init();
    if (ifd < 0) {
        ALOGE("inotify_init failed: %s", strerror(errno));
        return -1;
    }

    int wd = inotify_add_watch(ifd, INPUT_DIR, IN_CREATE);
    if (wd < 0) {
        ALOGE("inotify_add_watch failed: %s", strerror(errno));
        close(ifd);
        return -1;
    }

    char buf[512];
    while (1) {
        int len = read(ifd, buf, sizeof(buf));
        if (len <= 0)
            break;

        usleep(200000);

        fd = try_open_pen();
        if (fd >= 0) {
            inotify_rm_watch(ifd, wd);
            close(ifd);
            return fd;
        }
    }

    close(ifd);
    return -1;
}

static int create_uinput_device(void) {
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        ALOGE("Cannot open /dev/uinput: %s", strerror(errno));
        return -1;
    }

    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_EVBIT, EV_ABS);
    ioctl(fd, UI_SET_EVBIT, EV_SYN);
    ioctl(fd, UI_SET_KEYBIT, BTN_STYLUS);
    ioctl(fd, UI_SET_KEYBIT, BTN_STYLUS2);
    ioctl(fd, UI_SET_ABSBIT, ABS_PRESSURE);

    struct uinput_user_dev udev;
    memset(&udev, 0, sizeof(udev));
    strncpy(udev.name, DEVICE_NAME, UINPUT_MAX_NAME_SIZE - 1);
    udev.id.bustype = BUS_VIRTUAL;
    udev.id.vendor = 0x1915;
    udev.id.product = 0xEAEB;
    udev.id.version = 1;
    udev.absmin[ABS_PRESSURE] = 0;
    udev.absmax[ABS_PRESSURE] = 255;

    if (write(fd, &udev, sizeof(udev)) < 0) {
        ALOGE("write uinput_user_dev failed: %s", strerror(errno));
        close(fd);
        return -1;
    }

    if (ioctl(fd, UI_DEV_CREATE) < 0) {
        ALOGE("UI_DEV_CREATE failed: %s", strerror(errno));
        close(fd);
        return -1;
    }

    return fd;
}

static void send_event(int fd, unsigned short type, unsigned short code, int value) {
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = type;
    ev.code = code;
    ev.value = value;
    write(fd, &ev, sizeof(ev));
}

int main(void) {
    signal(SIGSEGV, signal_handler);
    signal(SIGABRT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGKILL, signal_handler);  /* won't catch, but for completeness */

    ALOGI("Starting (pid=%d)", getpid());

    while (1) {
        int evfd = wait_for_pen();
        if (evfd < 0) {
            ALOGE("Failed to find pen, retrying...");
            sleep(3);
            continue;
        }

        int uifd = create_uinput_device();
        if (uifd < 0) {
            close(evfd);
            ALOGE("uinput failed, retrying...");
            sleep(3);
            continue;
        }

        ALOGI("Bridging active");

        struct input_event ev;
        ssize_t n;
        while ((n = read(evfd, &ev, sizeof(ev))) == sizeof(ev)) {
            if (ev.type != EV_KEY)
                continue;

            int btn = -1;
            if (ev.code == KEY_PAGEUP)
                btn = BTN_STYLUS2;
            else if (ev.code == KEY_PAGEDOWN)
                btn = BTN_STYLUS;

            if (btn < 0)
                continue;

            send_event(uifd, EV_KEY, btn, ev.value);
            send_event(uifd, EV_ABS, ABS_PRESSURE, ev.value ? 255 : 0);
            send_event(uifd, EV_SYN, SYN_REPORT, 0);
        }

        ALOGI("Read loop exited: n=%zd errno=%d (%s)", n, errno, strerror(errno));
        ioctl(uifd, UI_DEV_DESTROY);
        close(uifd);
        close(evfd);
    }

    return 0;
}
