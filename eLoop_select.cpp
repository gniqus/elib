#include "elib.hpp"

eLoopSelect::eLoopSelect(int setsize): eLoop(setsize) {
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
}   

void eLoopSelect::addEvent(int fd, int mask) {
    if (mask & READABLE) FD_SET(fd, &rfds);
    if (mask & WRITABLE) FD_SET(fd, &wfds);
}

void eLoopSelect::delEvent(int fd, int mask) {
    if (mask & READABLE) FD_CLR(fd, &rfds);
    if (mask & WRITABLE) FD_CLR(fd, &wfds);
}

void eLoopSelect::wait(int64_t ms) {
    fd_set trfds = rfds, twfds = wfds;
    struct timeval tv {
        tv_sec: ms / 1000,
        tv_usec: ms % 1000 * 1000,
    };

    int num = select(maxfd + 1, &trfds, &twfds, NULL, &tv);
    if (num > 0) {
        for (int i = 0; i <= maxfd; ++i) {
            int mask = 0;
            fileEvent& fe = file_events_[i];

            if (fe.mask == NONE) continue;
            if (fe.mask & READABLE && FD_ISSET(i, &trfds)) {
                mask |= READABLE;
            }
            if (fe.mask & WRITABLE && FD_ISSET(i, &twfds)) {
                mask |= WRITABLE;
            }
            rfd2mask_[i] = mask;
        }
    }
}