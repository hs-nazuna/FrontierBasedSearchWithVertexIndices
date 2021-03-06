/*
 * TdZdd: a Top-down/Breadth-first Decision Diagram Manipulation Framework
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2014 ERATO MINATO Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/time.h>
#ifdef WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <sys/resource.h>
#endif

namespace tdzdd {

struct ResourceUsage {
    double etime;
    double utime;
    double stime;
    long maxrss;

    ResourceUsage() {
        update();
    }

    ResourceUsage(double etime, double utime, double stime, long maxrss) :
            etime(etime), utime(utime), stime(stime), maxrss(maxrss) {
    }

    ResourceUsage& update() {
        struct timeval t;
        gettimeofday(&t, 0);
        etime = double(t.tv_sec) + double(t.tv_usec) / 1000000;

#ifdef WIN32
        HANDLE h = GetCurrentProcess();
        FILETIME ft_creat, ft_exit, ft_kernel, ft_user;
        if (GetProcessTimes(h, &ft_creat, &ft_exit, &ft_kernel, &ft_user)) {
            ULARGE_INTEGER ul_kernel, ul_user;
            ul_kernel.LowPart = ft_kernel.dwLowDateTime;
            ul_kernel.HighPart = ft_kernel.dwHighDateTime;
            ul_user.LowPart = ft_user.dwLowDateTime;
            ul_user.HighPart = ft_user.dwHighDateTime;
            stime = ul_kernel.QuadPart * 1e-7;
            utime = ul_user.QuadPart * 1e-7;
        }

        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(h, &pmc, sizeof(pmc))) {
            maxrss = pmc.WorkingSetSize / 1024;
        }
#else
        struct rusage s;
        getrusage(RUSAGE_SELF, &s);
        utime = s.ru_utime.tv_sec + s.ru_utime.tv_usec * 1e-6;
        stime = s.ru_stime.tv_sec + s.ru_stime.tv_usec * 1e-6;
        maxrss = s.ru_maxrss;
//        if (maxrss == 0) maxrss = readMemoryStatus("VmHWM:");
#endif
        return *this;
    }

    ResourceUsage operator+(ResourceUsage const& u) const {
        return ResourceUsage(etime + u.etime, utime + u.utime, stime + u.stime,
                std::max(maxrss, u.maxrss));
    }

    ResourceUsage& operator+=(ResourceUsage const& u) {
        etime += u.etime;
        utime += u.utime;
        stime += u.stime;
        if (maxrss < u.maxrss) maxrss = u.maxrss;
        return *this;
    }

    ResourceUsage operator-(ResourceUsage const& u) const {
        return ResourceUsage(etime - u.etime, utime - u.utime, stime - u.stime,
                std::max(maxrss, u.maxrss));
    }

    ResourceUsage& operator-=(ResourceUsage const& u) {
        etime -= u.etime;
        utime -= u.utime;
        stime -= u.stime;
        if (maxrss < u.maxrss) maxrss = u.maxrss;
        return *this;
    }

    std::string elapsedTime() const {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << etime << "s";
        return ss.str();
    }

    std::string userTime() const {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << utime << "s";
        return ss.str();
    }

    std::string memory() const {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(0) << maxrss / 1024.0 << "MB";
        return ss.str();
    }

    friend std::ostream& operator<<(std::ostream& os, ResourceUsage const& u) {
        std::ios_base::fmtflags backup = os.flags(std::ios::fixed);
        os.setf(std::ios::fixed);

        os << std::setprecision(2) << u.etime << "s elapsed, ";
        os << std::setprecision(2) << u.utime << "s user, ";
        os << std::setprecision(0) << u.maxrss / 1024.0 << "MB";

        os.flags(backup);
        return os;
    }

private:
//    long readMemoryStatus(std::string key) {
//        std::ifstream ifs("/proc/self/status");
//        std::string buf;
//
//        while (ifs.good()) {
//            getline(ifs, buf);
//            if (buf.compare(0, key.length(), key) == 0) {
//                std::istringstream iss(buf.substr(key.length()));
//                double size;
//                std::string unit;
//                iss >> size >> unit;
//                switch (tolower(unit[0])) {
//                case 'b':
//                    size *= 1.0 / 1024.0;
//                    break;
//                case 'm':
//                    size *= 1024.0;
//                    break;
//                case 'g':
//                    size *= 1024.0 * 1024.0;
//                    break;
//                case 't':
//                    size *= 1024.0 * 1024.0 * 1024.0;
//                    break;
//                }
//                return long(size);
//            }
//        }
//
//        return 0;
//    }
};

class ElapsedTimeCounter {
    double totalTime;
    double startTime;

public:
    ElapsedTimeCounter() :
            totalTime(0), startTime(0) {
    }

    ElapsedTimeCounter& reset() {
        totalTime = 0;
        return *this;
    }

    ElapsedTimeCounter& start() {
        timeval t;
        gettimeofday(&t, 0);
        startTime = t.tv_sec + t.tv_usec * 1e-6;
        return *this;
    }

    ElapsedTimeCounter& stop() {
        timeval t;
        gettimeofday(&t, 0);
        totalTime += t.tv_sec + t.tv_usec * 1e-6 - startTime;
        return *this;
    }

    operator double() const {
        return totalTime;
    }

    friend std::ostream& operator<<(std::ostream& os,
            ElapsedTimeCounter const& o) {
        std::ios_base::fmtflags backup = os.flags(std::ios::fixed);
        os.setf(std::ios::fixed);
        os << std::setprecision(2) << o.totalTime << "s";
        os.flags(backup);
        return os;
    }
};

} // namespace tdzdd
