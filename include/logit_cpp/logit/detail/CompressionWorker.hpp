#pragma once
#ifndef _LOGIT_COMPRESSION_WORKER_HPP_INCLUDED
#define _LOGIT_COMPRESSION_WORKER_HPP_INCLUDED

/// \file CompressionWorker.hpp
/// \brief Background worker that compresses rotated log files.

#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <vector>
#include <cstdio>

#if defined(LOGIT_HAS_ZLIB)
#   include <zlib.h>
#endif
#if defined(LOGIT_HAS_ZSTD)
#   include <zstd.h>
#endif

namespace logit { namespace detail {

    /// \brief Compress a file using the specified compression type.
    /// \param type Compression algorithm to use.
    /// \param src Path to the source file.
    /// \param level Compression level.
    /// \param external_cmd Command template for CompressType::EXTERNAL_CMD.
    /// \return true on success.
    bool compress_file(CompressType type,
                       const std::string& src,
                       int level,
                       const std::string& external_cmd);

    /// \class CompressionWorker
    /// \brief Background worker performing asynchronous compression.
    class CompressionWorker {
    public:
        /// \brief Create worker thread.
        /// \param type Compression algorithm.
        /// \param level Compression level.
        /// \param external_cmd External command template.
        CompressionWorker(CompressType type,
                           int level,
                           std::string external_cmd);

        /// \brief Stop worker thread and finish pending tasks.
        ~CompressionWorker();

        /// \brief Enqueue a file for compression.
        /// \param path Path to file.
        void enqueue(std::string path);

        /// \brief Wait until all queued files are processed.
        void wait();

    private:
        /// \brief Worker loop processing queued files.
        void run();

        CompressType m_type;
        int m_level;
        std::string m_external_cmd;
        std::queue<std::string> m_q;
        std::thread m_thread;
        std::mutex m_mx;
        std::condition_variable m_cv;
        std::condition_variable m_cv_idle;
        bool m_stop = false;
        bool m_busy = false;
    };

    /// \brief Clamp value to inclusive range.
    /// \param v Input value.
    /// \param lo Lower bound.
    /// \param hi Upper bound.
    /// \return Clamped value.
    inline int clamp_level(int v, int lo, int hi) {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return v;
    }

    /// \brief Compress file using gzip.
    /// \param src Source path.
    /// \param dst_tmp Temporary output path.
    /// \param level Compression level.
    /// \return true on success.
    inline bool compress_file_gzip(const std::string& src,
                                   const std::string& dst_tmp,
                                   int level) {
#       if defined(LOGIT_HAS_ZLIB)
        std::ifstream in(src.c_str(), std::ios::binary);
        if (!in) return false;

        char mode[8] = "wb6";
        mode[2] = char('0' + clamp_level(level, 1, 9));
        gzFile out = gzopen(dst_tmp.c_str(), mode);
        if (!out) return false;
        gzbuffer(out, 256 * 1024);

        std::vector<char> buf(256 * 1024);
        while (in) {
            in.read(&buf[0], buf.size());
            std::streamsize got = in.gcount();
            if (got > 0) {
                int written = gzwrite(out, &buf[0], static_cast<unsigned>(got));
                if (written == 0) { gzclose(out); return false; }
            }
        }
        return gzclose(out) == Z_OK;
#       else
        (void)src; (void)dst_tmp; (void)level; return false;
#       endif
    }

    /// \brief Compress file using Zstandard.
    /// \param src Source path.
    /// \param dst_tmp Temporary output path.
    /// \param level Compression level.
    /// \return true on success.
    inline bool compress_file_zstd(const std::string& src,
                                   const std::string& dst_tmp,
                                   int level) {
#       if defined(LOGIT_HAS_ZSTD)
        std::ifstream in(src.c_str(), std::ios::binary);
        std::ofstream out(dst_tmp.c_str(), std::ios::binary | std::ios::trunc);
        if (!in || !out) return false;

        ZSTD_CCtx* cctx = ZSTD_createCCtx();
        if (!cctx) return false;
        ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, clamp_level(level, 1, 19));

        const size_t inChunk  = ZSTD_CStreamInSize();
        const size_t outChunk = ZSTD_CStreamOutSize();
        std::vector<char> inBuf(inChunk), outBuf(outChunk);

        for (;;) {
            in.read(&inBuf[0], inBuf.size());
            size_t rd = static_cast<size_t>(in.gcount());
            ZSTD_inBuffer zin  = { &inBuf[0], rd, 0 };
            const int last = in.eof() ? 1 : 0;

            while (zin.pos < zin.size || last) {
                ZSTD_outBuffer zout = { &outBuf[0], outBuf.size(), 0 };
                size_t ret = last
                    ? ZSTD_compressStream2(cctx, &zout, &zin, ZSTD_e_end)
                    : ZSTD_compressStream2(cctx, &zout, &zin, ZSTD_e_continue);
                if (ZSTD_isError(ret)) { ZSTD_freeCCtx(cctx); return false; }
                out.write(&outBuf[0], static_cast<std::streamsize>(zout.pos));
                if (!out) { ZSTD_freeCCtx(cctx); return false; }
                if (last && ret == 0 && zin.pos == zin.size) {
                    ZSTD_freeCCtx(cctx);
                    out.flush();
                    return out.good();
                }
                if (!last && zin.pos == zin.size) break;
            }
        }
#       else
        (void)src; (void)dst_tmp; (void)level; return false;
#       endif
    }

    /// \brief Compress file using external command.
    /// \param src Source path.
    /// \param cmd_tpl Command template containing {file} and {level}.
    /// \param level Compression level.
    /// \return true on success.
    inline bool compress_file_external(const std::string& src,
                                       const std::string& cmd_tpl,
                                       int level) {
        if (cmd_tpl.empty()) return false;
        std::string cmd = cmd_tpl;
        size_t pos = cmd.find("{file}");
        if (pos != std::string::npos) cmd.replace(pos, 6, "\"" + src + "\"");
        pos = cmd.find("{level}");
        if (pos != std::string::npos) cmd.replace(pos, 7, std::to_string(level));
        return std::system(cmd.c_str()) == 0;
    }

    inline bool compress_file(CompressType type,
                              const std::string& src,
                              int level,
                              const std::string& external_cmd) {
        if (type == CompressType::NONE) return true;
        if (type == CompressType::EXTERNAL_CMD) {
            return compress_file_external(src, external_cmd, level);
        }
        std::string dst = src + (type == CompressType::GZIP ? ".gz" : ".zst");
        std::string tmp = dst + ".tmp";
        bool ok = false;
        if (type == CompressType::GZIP) ok = compress_file_gzip(src, tmp, level);
        else ok = compress_file_zstd(src, tmp, level);
        if (!ok) { std::remove(tmp.c_str()); return false; }
        if (std::rename(tmp.c_str(), dst.c_str()) != 0) { std::remove(tmp.c_str()); return false; }
        std::remove(src.c_str());
        return true;
    }

    inline CompressionWorker::CompressionWorker(CompressType type,
                                                int level,
                                                std::string external_cmd)
        : m_type(type), m_level(level), m_external_cmd(std::move(external_cmd)) {
        if (m_type != CompressType::NONE) {
            m_thread = std::thread(&CompressionWorker::run, this);
        }
    }

    inline CompressionWorker::~CompressionWorker() {
        {
            std::lock_guard<std::mutex> lk(m_mx);
            m_stop = true;
            m_cv.notify_all();
        }
        if (m_thread.joinable()) m_thread.join();
    }

    inline void CompressionWorker::enqueue(std::string path) {
        std::lock_guard<std::mutex> lk(m_mx);
        m_q.push(std::move(path));
        m_cv.notify_one();
    }

    inline void CompressionWorker::wait() {
        std::unique_lock<std::mutex> lk(m_mx);
        m_cv_idle.wait(lk, [this]{ return m_q.empty() && !m_busy; });
    }

    inline void CompressionWorker::run() {
        for (;;) {
            std::string src;
            {
                std::unique_lock<std::mutex> lk(m_mx);
                m_cv.wait(lk, [this]{ return m_stop || !m_q.empty(); });
                if (m_stop && m_q.empty()) break;
                src = std::move(m_q.front());
                m_q.pop();
                m_busy = true;
            }

            compress_file(m_type, src, m_level, m_external_cmd);

            {
                std::lock_guard<std::mutex> lk(m_mx);
                m_busy = false;
                if (m_q.empty()) m_cv_idle.notify_all();
            }
        }
    }

}} // namespace logit::detail

#endif // _LOGIT_COMPRESSION_WORKER_HPP_INCLUDED

