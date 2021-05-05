#ifndef __ELIB_H__
#define __ELIB_H__

#include <unordered_map>
#include <vector>
#include <queue>
#include <chrono>
#include <cstdint>
#include <sys/select.h>

#define NONE            0
#define READABLE        1
#define WRITABLE        2

#define FILE_EVENTS     1
#define TIME_EVENTS     2
#define BOTH_EVENTS     3
#define DONT_WAIT       4

#define APERIODIC       -1

#define OK              0
#define ERROR           -1

struct fileEvent {
    eLoop::fileProc *rfileProc;
    eLoop::fileProc *wfileProc;
    int mask;
    void *procParam;

    fileEvent(int mask, eLoop::fileProc *proc, void *procParam): mask(mask), procParam(procParam) {
        if (mask & READABLE) rfileProc = proc;
        if (mask & WRITABLE) wfileProc = proc;
    }

    void* operator new(size_t, void* p) {
        return p;
    }
};

struct timeEvent {
    eLoop::timeProc *proc;
    int64_t when;
    void *procParam;

    timeEvent(int64_t when, eLoop::timeProc *proc, void *procParam): when(when), proc(proc), procParam(procParam) {}
};

struct cmp {
    bool operator() (timeEvent te1, timeEvent te2) {
        return te1.when > te2.when;
    }
};

class eLoop {
private:
    typedef struct {
        bool operator() (timeEvent te1, timeEvent te2) {
            return te1.when < te2.when;
        }
    } tcmp;

    std::priority_queue<timeEvent, std::vector<timeEvent>, tcmp> time_events_;

    static inline int64_t mstime() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }
    void dispatcher(int flags);

    virtual void addEvent(int fd, int mask);
    virtual void delEvent(int fd, int mask);
    virtual void wait(int64_t ms);

public:
    int maxfd;
    std::vector<fileEvent> file_events_;
    std::unordered_map<int, int> rfd2mask_;

    typedef void fileProc(void *procParam);
    typedef void timeProc(void *procParam);

    eLoop(int setsize);
    ~eLoop();

    void run();

    void addFileEvent(int fd, int mask, fileProc *proc, void *procParam);
    void delFileEvent(int fd, int mask);

    void addTimeEvent(int64_t ms, timeProc *proc, void *procParam);
};

class eLoopSelect: public eLoop {
public:
    fd_set rfds, wfds;

    eLoopSelect(int setsize);

    void addEvent(int fd, int mask);
    void delEvent(int fd, int mask);
    void wait(int64_t ms);
};

#endif