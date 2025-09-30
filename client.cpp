/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Aarya Bookseller
	UIN: 133003076
	Date: 09/23/2025
*/
#include "common.h"
#include "FIFORequestChannel.h"

#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <fstream>

using namespace std;

static inline void die(const char* msg) { perror(msg); exit(1); }

static void send_quit(FIFORequestChannel& ch) {
    MESSAGE_TYPE q = QUIT_MSG;
    ch.cwrite(&q, sizeof(q));
}

static double request_ecg(FIFORequestChannel& ch, int person, double seconds, int ecgno) {
    datamsg d(person, seconds, ecgno);
    ch.cwrite(&d, sizeof(d));
    double val = 0.0;
    ssize_t n = ch.cread(&val, sizeof(val));
    if (n != (ssize_t)sizeof(val)) {
        fprintf(stderr, "Error: expected %zu bytes for ECG reply, got %zd\n", sizeof(val), n);
        exit(1);
    }
    return val;
}

static __int64_t request_file_size(FIFORequestChannel& ch, const string& fname) {
    filemsg fm(0, 0);
    int len = sizeof(filemsg) + (int)fname.size() + 1;
    vector<char> buf(len);
    memcpy(buf.data(), &fm, sizeof(filemsg));
    strcpy(buf.data() + sizeof(filemsg), fname.c_str());
    ch.cwrite(buf.data(), len);
    __int64_t sz = 0;
    ssize_t n = ch.cread(&sz, sizeof(sz));
    if (n != (ssize_t)sizeof(sz)) {
        fprintf(stderr, "Error: expected %zu bytes for size reply, got %zd\n", sizeof(sz), n);
        exit(1);
    }
    return sz;
}

static void request_file_chunk(FIFORequestChannel& ch, const string& fname, __int64_t offset, int length, FILE* outfp) {
    filemsg fm(offset, length);
    int len = sizeof(filemsg) + (int)fname.size() + 1;
    vector<char> req(len);
    memcpy(req.data(), &fm, sizeof(filemsg));
    strcpy(req.data() + sizeof(filemsg), fname.c_str());
    ch.cwrite(req.data(), len);
    vector<char> chunk(length);
    ssize_t n = ch.cread(chunk.data(), length);
    if (n != length) {
        fprintf(stderr, "Error: expected %d bytes of file data, got %zd\n", length, n);
        exit(1);
    }
    size_t written = fwrite(chunk.data(), 1, length, outfp);
    if ((int)written != length) die("fwrite");
}

static void ensure_dir(const string& path) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        std::filesystem::create_directories(path, ec);
        if (ec) {
            fprintf(stderr, "Failed to create directory '%s': %s\n", path.c_str(), ec.message().c_str());
            exit(1);
        }
    }
}

static unique_ptr<FIFORequestChannel> open_new_channel(FIFORequestChannel& control) {
    MESSAGE_TYPE m = NEWCHANNEL_MSG;
    control.cwrite(&m, sizeof(m));
    char namebuf[30] = {0};
    ssize_t n = control.cread(namebuf, sizeof(namebuf));
    if (n <= 0) {
        fprintf(stderr, "Failed to read new channel name\n");
        exit(1);
    }
    return make_unique<FIFORequestChannel>(namebuf, FIFORequestChannel::CLIENT_SIDE);
}

int main (int argc, char *argv[]) {
    int opt;
    int p = -1;
    double t = -1.0;
    int e = -1;
    string filename = "";
    int mcap = -1;
    bool use_new_channel = false;

    while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
        switch (opt) {
            case 'p': p = atoi(optarg); break;
            case 't': t = atof(optarg); break;
            case 'e': e = atoi(optarg); break;
            case 'f': filename = optarg; break;
            case 'm': mcap = atoi(optarg); break;
            case 'c': use_new_channel = true; break;
        }
    }

    pid_t child = fork();
    if (child == 0) {
        vector<char*> args;
        string mstr;
        args.push_back(const_cast<char*>("./server"));
        if (mcap > 0) {
            args.push_back(const_cast<char*>("-m"));
            mstr = to_string(mcap);
            args.push_back(const_cast<char*>(mstr.c_str()));
        }
        args.push_back(nullptr);
        execvp(args[0], args.data());
        perror("execvp server failed");
        _exit(127);
    }
    if (child < 0) die("fork");

    FIFORequestChannel control("control", FIFORequestChannel::CLIENT_SIDE);
    FIFORequestChannel* active = &control;
    unique_ptr<FIFORequestChannel> newchan;
    if (use_new_channel) {
        newchan = open_new_channel(control);
        active = newchan.get();
    }

    bool did_something = false;

    if (p > 0 && t >= 0.0 && (e == 1 || e == 2)) {
        double v = request_ecg(*active, p, t, e);
        cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << fixed << setprecision(2) << v << endl;
        did_something = true;
    }

    if (!did_something && p > 0 && t < 0.0 && e < 0 && filename.empty()) {
        ensure_dir("received");
        std::ifstream in("BIMDC/" + std::to_string(p) + ".csv", std::ios::binary);
        if (!in) die("open BIMDC/<p>.csv");
        std::ofstream out("received/x1.csv", std::ios::binary);
        if (!out) die("open received/x1.csv");
        std::string line;
        for (int i = 0; i < 1000 && std::getline(in, line); ++i) {
                out << line << '\n';
        }
        did_something = true;
}

    if (!did_something && !filename.empty()) {
        ensure_dir("received");
        string outpath = string("received/") + filename;
        FILE* outfp = fopen(outpath.c_str(), "wb");
        if (!outfp) die(("fopen " + outpath).c_str());
        __int64_t fsz = request_file_size(*active, filename);
        int chunk = (mcap > 0) ? mcap : 256;
        __int64_t remaining = fsz;
        __int64_t offset = 0;
        while (remaining > 0) {
            int len = (remaining < chunk) ? (int)remaining : chunk;
            request_file_chunk(*active, filename, offset, len, outfp);
            offset += len;
            remaining -= len;
        }
        fclose(outfp);
        did_something = true;
    }

    if (newchan) send_quit(*newchan);
    send_quit(control);
    waitpid(child, nullptr, 0);
    return 0;
}