//===--- Compression.cpp - Compression implementation ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file implements compression functions.
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/Compression.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Config/config.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/ErrorHandling.h"
#if LLVM_ENABLE_ZLIB
#include <zlib.h>
#endif
#if LLVM_ENABLE_ZSTD
#include <zstd.h>
#endif
#include <limits>

using namespace llvm;
using namespace llvm::compression;

const char *compression::getReasonIfUnsupported(compression::Format F) {
  switch (F) {
  case compression::Format::Zlib:
    if (zlib::isAvailable())
      return nullptr;
    return "LLVM was not built with LLVM_ENABLE_ZLIB or did not find zlib at "
           "build time";
  case compression::Format::Zstd:
    if (zstd::isAvailable())
      return nullptr;
    return "LLVM was not built with LLVM_ENABLE_ZSTD or did not find zstd at "
           "build time";
  }
  llvm_unreachable("");
}

void compression::compress(Params P, ArrayRef<uint8_t> Input,
                           SmallVectorImpl<uint8_t> &Output) {
  switch (P.format) {
  case compression::Format::Zlib:
    zlib::compress(Input, Output, P.level);
    break;
  case compression::Format::Zstd:
    zstd::compress(Input, Output, P.level, P.zstdEnableLdm);
    break;
  }
}

Error compression::decompress(DebugCompressionType T, ArrayRef<uint8_t> Input,
                              uint8_t *Output, size_t UncompressedSize) {
  switch (formatFor(T)) {
  case compression::Format::Zlib:
    return zlib::decompress(Input, Output, UncompressedSize);
  case compression::Format::Zstd:
    return zstd::decompress(Input, Output, UncompressedSize);
  }
  llvm_unreachable("");
}

Error compression::decompress(compression::Format F, ArrayRef<uint8_t> Input,
                              SmallVectorImpl<uint8_t> &Output,
                              size_t UncompressedSize) {
  switch (F) {
  case compression::Format::Zlib:
    return zlib::decompress(Input, Output, UncompressedSize);
  case compression::Format::Zstd:
    return zstd::decompress(Input, Output, UncompressedSize);
  }
  llvm_unreachable("");
}

Error compression::decompress(DebugCompressionType T, ArrayRef<uint8_t> Input,
                              SmallVectorImpl<uint8_t> &Output,
                              size_t UncompressedSize) {
  return decompress(formatFor(T), Input, Output, UncompressedSize);
}

#if LLVM_ENABLE_ZLIB

static StringRef convertZlibCodeToString(int Code) {
  switch (Code) {
  case Z_MEM_ERROR:
    return "zlib error: Z_MEM_ERROR";
  case Z_BUF_ERROR:
    return "zlib error: Z_BUF_ERROR";
  case Z_STREAM_ERROR:
    return "zlib error: Z_STREAM_ERROR";
  case Z_DATA_ERROR:
    return "zlib error: Z_DATA_ERROR";
  case Z_OK:
  default:
    llvm_unreachable("unknown or unexpected zlib status code");
  }
}

bool zlib::isAvailable() { return true; }

void zlib::compress(ArrayRef<uint8_t> Input,
                    SmallVectorImpl<uint8_t> &CompressedBuffer, int Level) {
  unsigned long CompressedSize = ::compressBound(Input.size());
  CompressedBuffer.resize_for_overwrite(CompressedSize);
  int Res = ::compress2((Bytef *)CompressedBuffer.data(), &CompressedSize,
                        (const Bytef *)Input.data(), Input.size(), Level);
  if (Res == Z_MEM_ERROR)
    report_bad_alloc_error("Allocation failed");
  assert(Res == Z_OK);
  // Tell MemorySanitizer that zlib output buffer is fully initialized.
  // This avoids a false report when running LLVM with uninstrumented ZLib.
  __msan_unpoison(CompressedBuffer.data(), CompressedSize);
  if (CompressedSize < CompressedBuffer.size())
    CompressedBuffer.truncate(CompressedSize);
}

Error zlib::decompress(ArrayRef<uint8_t> Input, uint8_t *Output,
                       size_t &UncompressedSize) {
  int Res = ::uncompress((Bytef *)Output, (uLongf *)&UncompressedSize,
                         (const Bytef *)Input.data(), Input.size());
  // Tell MemorySanitizer that zlib output buffer is fully initialized.
  // This avoids a false report when running LLVM with uninstrumented ZLib.
  __msan_unpoison(Output, UncompressedSize);
  return Res ? make_error<StringError>(convertZlibCodeToString(Res),
                                       inconvertibleErrorCode())
             : Error::success();
}

Error zlib::decompress(ArrayRef<uint8_t> Input,
                       SmallVectorImpl<uint8_t> &Output,
                       size_t UncompressedSize) {
  Output.resize_for_overwrite(UncompressedSize);
  Error E = zlib::decompress(Input, Output.data(), UncompressedSize);
  if (UncompressedSize < Output.size())
    Output.truncate(UncompressedSize);
  return E;
}

#else
bool zlib::isAvailable() { return false; }
void zlib::compress(ArrayRef<uint8_t> Input,
                    SmallVectorImpl<uint8_t> &CompressedBuffer, int Level) {
  llvm_unreachable("zlib::compress is unavailable");
}
Error zlib::decompress(ArrayRef<uint8_t> Input, uint8_t *UncompressedBuffer,
                       size_t &UncompressedSize) {
  llvm_unreachable("zlib::decompress is unavailable");
}
Error zlib::decompress(ArrayRef<uint8_t> Input,
                       SmallVectorImpl<uint8_t> &UncompressedBuffer,
                       size_t UncompressedSize) {
  llvm_unreachable("zlib::decompress is unavailable");
}
#endif

#if LLVM_ENABLE_ZLIB

bool raw_deflate::isAvailable() { return true; }

Error raw_deflate::decompress(ArrayRef<uint8_t> Input,
                              SmallVectorImpl<uint8_t> &Output,
                              size_t UncompressedSize) {
  z_stream Stream = {};
  int Res = inflateInit2(&Stream, -MAX_WBITS);
  if (Res != Z_OK)
    return createStringError(inconvertibleErrorCode(),
                             "failed to initialize raw DEFLATE decompression");
  scope_exit EndStream([&] { inflateEnd(&Stream); });

  Output.resize_for_overwrite(UncompressedSize);
  size_t InputLoaded = 0;
  size_t InputConsumed = 0;
  size_t OutputProduced = 0;
  uint8_t OverflowByte;
  while (true) {
    if (Stream.avail_in == 0 && InputLoaded != Input.size()) {
      Stream.avail_in =
          static_cast<uInt>(std::min(Input.size() - InputLoaded,
                                     size_t(std::numeric_limits<uInt>::max())));
      Stream.next_in =
          const_cast<Bytef *>(reinterpret_cast<const Bytef *>(Input.data())) +
          InputLoaded;
      InputLoaded += Stream.avail_in;
    }

    bool CheckingOverflow = OutputProduced == UncompressedSize;
    if (Stream.avail_out == 0) {
      if (CheckingOverflow) {
        Stream.next_out = &OverflowByte;
        Stream.avail_out = 1;
      } else {
        Stream.avail_out = static_cast<uInt>(
            std::min(UncompressedSize - OutputProduced,
                     size_t(std::numeric_limits<uInt>::max())));
        Stream.next_out =
            reinterpret_cast<Bytef *>(Output.data()) + OutputProduced;
      }
    }

    uInt InputBefore = Stream.avail_in;
    uInt OutputBefore = Stream.avail_out;
    Res = inflate(&Stream, Z_NO_FLUSH);
    size_t Consumed = InputBefore - Stream.avail_in;
    size_t Produced = OutputBefore - Stream.avail_out;
    InputConsumed += Consumed;
    if (CheckingOverflow && Produced != 0)
      return createStringError(
          inconvertibleErrorCode(),
          "raw DEFLATE stream exceeds the declared uncompressed size");
    OutputProduced += Produced;

    if (Res == Z_STREAM_END) {
      if (OutputProduced != UncompressedSize)
        return createStringError(
            inconvertibleErrorCode(),
            "raw DEFLATE stream does not match the declared uncompressed size");
      if (InputConsumed != Input.size())
        return createStringError(inconvertibleErrorCode(),
                                 "raw DEFLATE stream has trailing data");
      __msan_unpoison(Output.data(), Output.size());
      return Error::success();
    }
    if (Res != Z_OK)
      return createStringError(inconvertibleErrorCode(),
                               "invalid or truncated raw DEFLATE stream");
    if (Consumed == 0 && Produced == 0)
      return createStringError(inconvertibleErrorCode(),
                               "raw DEFLATE decompression made no progress");
  }
}

#else

bool raw_deflate::isAvailable() { return false; }

Error raw_deflate::decompress(ArrayRef<uint8_t> Input,
                              SmallVectorImpl<uint8_t> &Output,
                              size_t UncompressedSize) {
  return createStringError(
      inconvertibleErrorCode(),
      "raw DEFLATE decompression is unavailable because LLVM was not built "
      "with LLVM_ENABLE_ZLIB");
}

#endif

#if LLVM_ENABLE_ZSTD

bool zstd::isAvailable() { return true; }

#include <zstd.h> // Ensure ZSTD library is included

int zstd::getMinCompressionLevel() { return ZSTD_minCLevel(); }

int zstd::getMaxCompressionLevel() { return ZSTD_maxCLevel(); }

void zstd::compress(ArrayRef<uint8_t> Input,
                    SmallVectorImpl<uint8_t> &CompressedBuffer, int Level,
                    bool EnableLdm) {
  ZSTD_CCtx *Cctx = ZSTD_createCCtx();
  if (!Cctx)
    report_bad_alloc_error("Failed to create ZSTD_CCtx");

  if (ZSTD_isError(ZSTD_CCtx_setParameter(
          Cctx, ZSTD_c_enableLongDistanceMatching, EnableLdm ? 1 : 0))) {
    ZSTD_freeCCtx(Cctx);
    report_bad_alloc_error("Failed to set ZSTD_c_enableLongDistanceMatching");
  }

  if (ZSTD_isError(
          ZSTD_CCtx_setParameter(Cctx, ZSTD_c_compressionLevel, Level))) {
    ZSTD_freeCCtx(Cctx);
    report_bad_alloc_error("Failed to set ZSTD_c_compressionLevel");
  }

  unsigned long CompressedBufferSize = ZSTD_compressBound(Input.size());
  CompressedBuffer.resize_for_overwrite(CompressedBufferSize);

  size_t const CompressedSize =
      ZSTD_compress2(Cctx, CompressedBuffer.data(), CompressedBufferSize,
                     Input.data(), Input.size());

  ZSTD_freeCCtx(Cctx);

  if (ZSTD_isError(CompressedSize))
    report_bad_alloc_error("Compression failed");

  __msan_unpoison(CompressedBuffer.data(), CompressedSize);
  if (CompressedSize < CompressedBuffer.size())
    CompressedBuffer.truncate(CompressedSize);
}

Error zstd::decompress(ArrayRef<uint8_t> Input, uint8_t *Output,
                       size_t &UncompressedSize) {
  const size_t Res = ::ZSTD_decompress(
      Output, UncompressedSize, (const uint8_t *)Input.data(), Input.size());
  UncompressedSize = Res;
  if (ZSTD_isError(Res))
    return make_error<StringError>(ZSTD_getErrorName(Res),
                                   inconvertibleErrorCode());
  // Tell MemorySanitizer that zstd output buffer is fully initialized.
  // This avoids a false report when running LLVM with uninstrumented ZLib.
  __msan_unpoison(Output, UncompressedSize);
  return Error::success();
}

Error zstd::decompress(ArrayRef<uint8_t> Input,
                       SmallVectorImpl<uint8_t> &Output,
                       size_t UncompressedSize) {
  Output.resize_for_overwrite(UncompressedSize);
  Error E = zstd::decompress(Input, Output.data(), UncompressedSize);
  if (UncompressedSize < Output.size())
    Output.truncate(UncompressedSize);
  return E;
}

#else
bool zstd::isAvailable() { return false; }
int zstd::getMinCompressionLevel() { llvm_unreachable("zstd is unavailable"); }
int zstd::getMaxCompressionLevel() { llvm_unreachable("zstd is unavailable"); }
void zstd::compress(ArrayRef<uint8_t> Input,
                    SmallVectorImpl<uint8_t> &CompressedBuffer, int Level,
                    bool EnableLdm) {
  llvm_unreachable("zstd::compress is unavailable");
}
Error zstd::decompress(ArrayRef<uint8_t> Input, uint8_t *Output,
                       size_t &UncompressedSize) {
  llvm_unreachable("zstd::decompress is unavailable");
}
Error zstd::decompress(ArrayRef<uint8_t> Input,
                       SmallVectorImpl<uint8_t> &Output,
                       size_t UncompressedSize) {
  llvm_unreachable("zstd::decompress is unavailable");
}
#endif
