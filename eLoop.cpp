#include "elib.hpp"
#include <algorithm>

eLoop::eLoop(int setsize): maxfd(-1) {
    file_events_.resize(setsize);
}

eLoop::~eLoop() {}

void eLoop::run() {
    while (true) {
        dispatcher(BOTH_EVENTS);
    }
}

void eLoop::addFileEvent(int fd, int mask, fileProc *proc, void *procParam) {
    addEvent(fd, mask);
    new(&file_events_[fd]) fileEvent(mask, proc, procParam);
    if (fd > maxfd) maxfd = fd;
}

void eLoop::delFileEvent(int fd, int mask) {
    file_events_[fd].mask &= ~mask;
    if (fd == maxfd) {
        while (maxfd >= 0 && file_events_[maxfd].mask == NONE) {
            --maxfd;
        }
    }
    delEvent(fd, mask);
}

void eLoop::addTimeEvent(int64_t ms, timeProc *proc, void *procParam) {
    time_events_.emplace(ms + mstime(), proc, procParam);
}

void eLoop::dispatcher(int flags) {
    const timeEvent& nextTe = time_events_.top();
    int64_t interval = std::max(int64_t(0), nextTe.when - mstime());

    wait(interval);

    for (auto it = rfd2mask_.begin(); it != rfd2mask_.end(); ++it) {
        int fd = it->first, mask = it->second;
        fileEvent& fe = file_events_[fd];

        if (fe.mask & mask & READABLE) {
            fe.rfileProc(fe.procParam);
        }
        if (fe.mask & mask & WRITABLE) {
            fe.wfileProc(fe.procParam);
        }
    }

    rfd2mask_.clear();

    const timeEvent *te = &time_events_.top();
    while (!time_events_.empty() && mstime() >= te->when) {
        int r = te->proc(te->procParam);
        if (APERIODIC == te->proc(te->procParam)) {
            time_events_.pop();
        } else {
            te->when += r;
        }
    }
}