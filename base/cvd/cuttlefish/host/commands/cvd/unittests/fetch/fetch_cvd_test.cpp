//
// Copyright (C) 2024 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <fstream>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "android-base/file.h"
#include "common/libs/utils/files.h"
#include "common/libs/utils/result.h"
#include "fmt/core.h"

namespace cuttlefish {

std::string CreateTempFileWithText(const std::string& filepath,
                                   const std::string& text) {
  std::ofstream file(filepath);
  file << text;
  file.close();
  return filepath;
}

// Test fixture for common setup and teardown
class FetchCvdTests : public ::testing::Test {
 protected:
  void SetUp() override {
    // The output file for the fake cas client.
    cas_output_filepath_ = std::string(temp_dir_.path) + "/cas_output";

    // Create a fake cas client that outputs the command line when invoked.
    std::string script = fmt::format(
        R"(#!/bin/bash
if [[ "$1" == "-help" ]]; then
  echo "Usage of casdownloader:" >&2
fi
rm -rf {CAS_OUTPUT_FILEPATH}
echo $@ > {CAS_OUTPUT_FILEPATH}
)",
        fmt::arg("CAS_OUTPUT_FILEPATH", cas_output_filepath_));
    cas_downloader_path_ = CreateTempFileWithText(
        std::string(temp_dir_.path) + "/casdownloader", script);
    MakeFileExecutable(cas_downloader_path_);

    // Create a test CAS config to inject the fake cas downloader.
    std::string text =
        fmt::format(R"({{
    "downloader-path": "{DOWNLOADER_PATH}"
}})",
                    fmt::arg("DOWNLOADER_PATH", cas_downloader_path_));
    cas_config_filepath_ = CreateTempFileWithText(
        std::string(temp_dir_.path) + "/cas_config.json", text);
  }

  TemporaryDir temp_dir_;
  std::string cas_config_filepath_;
  std::string cas_downloader_path_;
  std::string cas_output_filepath_;
};

using testing::IsTrue;

inline constexpr char kFetchCvd[] = "fetch-cvd";
inline constexpr char kTargetDirectory[] = "--target_directory=/tmp/fetch_test";
inline constexpr char kDefaultBuild[] =
    "--default_build=aosp-main/aosp_cf_x86_64_phone-trunk_staging-userdebug";

Result<void> FetchCvdMain(int argc, char** argv);

TEST_F(FetchCvdTests, DownloaderInvokedIfCasConfigSet) {
  std::string fetch_cvd = std::string(kFetchCvd);
  std::string target_directory = std::string(kTargetDirectory);
  std::string default_build = std::string(kDefaultBuild);
  std::string cas_config_filepath =
      "--cas_config_filepath=" + cas_config_filepath_;
  char* argv[] = {fetch_cvd.data(), target_directory.data(),
                  default_build.data(), cas_config_filepath.data()};
  int argc = sizeof(argv) / sizeof(char*);

  Result<void> fetch_cvd_main = FetchCvdMain(argc, argv);

  EXPECT_THAT(FileExists(cas_output_filepath_), IsTrue());
}

TEST_F(FetchCvdTests, DownloaderInvokedIfDownloaderPathSet) {
  std::string fetch_cvd = std::string(kFetchCvd);
  std::string target_directory = std::string(kTargetDirectory);
  std::string default_build = std::string(kDefaultBuild);
  std::string cas_downloader_path =
      "--cas_downloader_path=" + cas_downloader_path_;
  char* argv[] = {fetch_cvd.data(), target_directory.data(),
                  default_build.data(), cas_downloader_path.data()};
  int argc = sizeof(argv) / sizeof(char*);

  Result<void> fetch_cvd_main = FetchCvdMain(argc, argv);

  EXPECT_THAT(FileExists(cas_output_filepath_), IsTrue());
}

}  // namespace cuttlefish
