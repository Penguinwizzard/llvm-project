//===- llvm/unittest/Support/CompressionTest.cpp - Compression tests ------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements unit tests for the Compression functions.
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/Compression.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Config/config.h"
#include "llvm/Support/Error.h"
#include "llvm/Testing/Support/Error.h"
#include "gtest/gtest.h"

using namespace llvm;
using namespace llvm::compression;

namespace {

#if LLVM_ENABLE_ZLIB
static void testZlibCompression(StringRef Input, int Level) {
  SmallVector<uint8_t, 0> Compressed;
  SmallVector<uint8_t, 0> Uncompressed;
  zlib::compress(arrayRefFromStringRef(Input), Compressed, Level);

  // Check that uncompressed buffer is the same as original.
  Error E = zlib::decompress(Compressed, Uncompressed, Input.size());
  EXPECT_FALSE(std::move(E));
  EXPECT_EQ(Input, toStringRef(Uncompressed));

  // decompress with Z dispatches to zlib::decompress.
  E = compression::decompress(DebugCompressionType::Zlib, Compressed,
                              Uncompressed, Input.size());
  EXPECT_FALSE(std::move(E));
  EXPECT_EQ(Input, toStringRef(Uncompressed));

  if (Input.size() > 0) {
    // Decompression fails if expected length is too short.
    E = zlib::decompress(Compressed, Uncompressed, Input.size() - 1);
    EXPECT_EQ("zlib error: Z_BUF_ERROR", llvm::toString(std::move(E)));
  }
}

TEST(CompressionTest, Zlib) {
  testZlibCompression("", zlib::DefaultCompression);

  testZlibCompression("hello, world!", zlib::NoCompression);
  testZlibCompression("hello, world!", zlib::BestSizeCompression);
  testZlibCompression("hello, world!", zlib::BestSpeedCompression);
  testZlibCompression("hello, world!", zlib::DefaultCompression);

  const size_t kSize = 1024;
  char BinaryData[kSize];
  for (size_t i = 0; i < kSize; ++i)
    BinaryData[i] = i & 255;
  StringRef BinaryDataStr(BinaryData, kSize);

  testZlibCompression(BinaryDataStr, zlib::NoCompression);
  testZlibCompression(BinaryDataStr, zlib::BestSizeCompression);
  testZlibCompression(BinaryDataStr, zlib::BestSpeedCompression);
  testZlibCompression(BinaryDataStr, zlib::DefaultCompression);
}

TEST(CompressionTest, RawDeflateDecompression) {
  // Produced independently with Python's zlib.compressobj(wbits=-15).
  const uint8_t Raw[] = {0xcb, 0x48, 0xcd, 0xc9, 0xc9, 0x57, 0x28,
                         0x4a, 0x2c, 0x57, 0x48, 0x49, 0x4d, 0xcb,
                         0x49, 0x2c, 0x49, 0x05, 0x00};
  constexpr StringLiteral Expected = "hello raw deflate";
  SmallVector<uint8_t, 0> Output;
  EXPECT_THAT_ERROR(raw_deflate::decompress(Raw, Output, Expected.size()),
                    Succeeded());
  EXPECT_EQ(toStringRef(Output), Expected);
  const uint8_t Empty[] = {0x03, 0x00};
  EXPECT_THAT_ERROR(raw_deflate::decompress(Empty, Output, 0), Succeeded());
  EXPECT_TRUE(Output.empty());

  const uint8_t ZlibWrapped[] = {0x78, 0x9c, 0xcb, 0x48, 0xcd, 0xc9, 0xc9,
                                 0x57, 0x28, 0x4a, 0x2c, 0x57, 0x48, 0x49,
                                 0x4d, 0xcb, 0x49, 0x2c, 0x49, 0x05, 0x00,
                                 0x39, 0xbf, 0x06, 0x74};
  const uint8_t GzipWrapped[] = {0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
                                 0x00, 0x03, 0xcb, 0x48, 0xcd, 0xc9, 0xc9, 0x57,
                                 0x28, 0x4a, 0x2c, 0x57, 0x48, 0x49, 0x4d, 0xcb,
                                 0x49, 0x2c, 0x49, 0x05, 0x00, 0x61, 0x63, 0x50,
                                 0x05, 0x11, 0x00, 0x00, 0x00};
  EXPECT_THAT_ERROR(
      raw_deflate::decompress(ZlibWrapped, Output, Expected.size()), Failed());
  EXPECT_THAT_ERROR(
      raw_deflate::decompress(GzipWrapped, Output, Expected.size()), Failed());
  EXPECT_THAT_ERROR(raw_deflate::decompress(ArrayRef(Raw).drop_back(), Output,
                                            Expected.size()),
                    Failed());

  SmallVector<uint8_t, 0> Corrupt{ArrayRef(Raw)};
  Corrupt[0] = 0x06;
  EXPECT_THAT_ERROR(raw_deflate::decompress(Corrupt, Output, Expected.size()),
                    Failed());

  SmallVector<uint8_t, 0> WithTrailingBytes{ArrayRef(Raw)};
  WithTrailingBytes.push_back(0);
  EXPECT_THAT_ERROR(
      raw_deflate::decompress(WithTrailingBytes, Output, Expected.size()),
      FailedWithMessage("raw DEFLATE stream has trailing data"));
  EXPECT_THAT_ERROR(raw_deflate::decompress(Raw, Output, Expected.size() - 1),
                    Failed());
  EXPECT_THAT_ERROR(raw_deflate::decompress(Raw, Output, Expected.size() + 1),
                    Failed());
}
#endif

#if !LLVM_ENABLE_ZLIB
TEST(CompressionTest, RawDeflateUnavailable) {
  SmallVector<uint8_t, 0> Output;
  EXPECT_THAT_ERROR(
      raw_deflate::decompress({}, Output, 0),
      FailedWithMessage(
          "raw DEFLATE decompression is unavailable because LLVM was not "
          "built with LLVM_ENABLE_ZLIB"));
}
#endif

#if LLVM_ENABLE_ZSTD
static void testZstdCompression(StringRef Input, int Level) {
  SmallVector<uint8_t, 0> Compressed;
  SmallVector<uint8_t, 0> Uncompressed;
  zstd::compress(arrayRefFromStringRef(Input), Compressed, Level);

  // Check that uncompressed buffer is the same as original.
  Error E = zstd::decompress(Compressed, Uncompressed, Input.size());
  EXPECT_FALSE(std::move(E));
  EXPECT_EQ(Input, toStringRef(Uncompressed));

  // decompress with Zstd dispatches to zstd::decompress.
  E = compression::decompress(DebugCompressionType::Zstd, Compressed,
                              Uncompressed, Input.size());
  EXPECT_FALSE(std::move(E));
  EXPECT_EQ(Input, toStringRef(Uncompressed));

  if (Input.size() > 0) {
    // Decompression fails if expected length is too short.
    E = zstd::decompress(Compressed, Uncompressed, Input.size() - 1);
    EXPECT_EQ("Destination buffer is too small", llvm::toString(std::move(E)));
  }
}

TEST(CompressionTest, Zstd) {
  testZstdCompression("", zstd::DefaultCompression);

  testZstdCompression("hello, world!", zstd::NoCompression);
  testZstdCompression("hello, world!", zstd::BestSizeCompression);
  testZstdCompression("hello, world!", zstd::BestSpeedCompression);
  testZstdCompression("hello, world!", zstd::DefaultCompression);

  const size_t kSize = 1024;
  char BinaryData[kSize];
  for (size_t i = 0; i < kSize; ++i)
    BinaryData[i] = i & 255;
  StringRef BinaryDataStr(BinaryData, kSize);

  testZstdCompression(BinaryDataStr, zstd::NoCompression);
  testZstdCompression(BinaryDataStr, zstd::BestSizeCompression);
  testZstdCompression(BinaryDataStr, zstd::BestSpeedCompression);
  testZstdCompression(BinaryDataStr, zstd::DefaultCompression);
}
#endif
}
