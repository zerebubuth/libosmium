#ifndef OSMIUM_IO_GZIP_COMPRESSION_HPP
#define OSMIUM_IO_GZIP_COMPRESSION_HPP

/*

This file is part of Osmium (http://osmcode.org/libosmium).

Copyright 2013,2014 Jochen Topf <jochen@topf.org> and others (see README).

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

*/

#define OSMIUM_LINK_WITH_LIBS_ZLIB -lz

#include <stdexcept>
#include <string>

#include <errno.h>
#include <zlib.h>

#include <osmium/io/compression.hpp>
#include <osmium/io/file_compression.hpp>
#include <osmium/util/cast.hpp>
#include <osmium/util/compatibility.hpp>

namespace osmium {

    /**
     * Exception thrown when there are problems gzip compressing or
     * uncompressing files.
     */
    struct gzip_error : public std::runtime_error {

        int gzip_error_code;
        int system_errno;

        gzip_error(const std::string& what, int error_code) :
            std::runtime_error(what),
            gzip_error_code(error_code),
            system_errno(error_code == Z_ERRNO ? errno : 0) {
        }

    }; // struct gzip_error

    namespace io {

        class GzipCompressor : public Compressor {

            gzFile m_gzfile;

            OSMIUM_NORETURN inline void throw_gzip_error(const char* msg, int zlib_error=0) {
                std::string error("gzip compress error: ");
                error += msg;
                error += ": ";
                int errnum = zlib_error;
                if (zlib_error) {
                    error += std::to_string(zlib_error);
                } else {
                    error += ::gzerror(m_gzfile, &errnum);
                }
                throw osmium::gzip_error(error, errnum);
            }

        public:

            explicit GzipCompressor(int fd) :
                Compressor(),
                m_gzfile(::gzdopen(fd, "w")) {
                if (!m_gzfile) {
                    throw_gzip_error("initialization failed");
                }
            }

            ~GzipCompressor() override final {
                close();
            }

            void write(const std::string& data) override final {
                if (!data.empty()) {
                    int nwrite = ::gzwrite(m_gzfile, data.data(), static_cast_with_assert<unsigned int>(data.size()));
                    if (nwrite == 0) {
                        throw_gzip_error("write failed");
                    }
                }
            }

            void close() override final {
                if (m_gzfile) {
                    int result = ::gzclose(m_gzfile);
                    m_gzfile = nullptr;
                    if (result != Z_OK) {
                        throw_gzip_error("close failed", result);
                    }
                }
            }

        }; // class GzipCompressor

        class GzipDecompressor : public Decompressor {

            gzFile m_gzfile;

            OSMIUM_NORETURN inline void throw_gzip_error(const char* msg, int zlib_error=0) {
                std::string error("gzip decompress error: ");
                error += msg;
                error += ": ";
                int errnum = zlib_error;
                if (zlib_error) {
                    error += std::to_string(zlib_error);
                } else {
                    error += ::gzerror(m_gzfile, &errnum);
                }
                throw osmium::gzip_error(error, errnum);
            }

        public:

            explicit GzipDecompressor(int fd) :
                Decompressor(),
                m_gzfile(::gzdopen(fd, "r")) {
                if (!m_gzfile) {
                    throw_gzip_error("initialization failed");
                }
            }

            ~GzipDecompressor() override final {
                close();
            }

            std::string read() override final {
                std::string buffer(osmium::io::Decompressor::input_buffer_size, '\0');
                int nread = ::gzread(m_gzfile, const_cast<char*>(buffer.data()), static_cast_with_assert<unsigned int>(buffer.size()));
                if (nread < 0) {
                    throw_gzip_error("read failed");
                }
                buffer.resize(static_cast<std::string::size_type>(nread));
                return buffer;
            }

            void close() override final {
                if (m_gzfile) {
                    int result = ::gzclose(m_gzfile);
                    m_gzfile = nullptr;
                    if (result != Z_OK) {
                        throw_gzip_error("close failed", result);
                    }
                }
            }

        }; // class GzipDecompressor

        namespace {

            const bool registered_gzip_compression = osmium::io::CompressionFactory::instance().register_compression(osmium::io::file_compression::gzip,
                [](int fd) { return new osmium::io::GzipCompressor(fd); },
                [](int fd) { return new osmium::io::GzipDecompressor(fd); }
            );

        } // anonymous namespace

    } // namespace io

} // namespace osmium

#endif // OSMIUM_IO_GZIP_COMPRESSION_HPP
