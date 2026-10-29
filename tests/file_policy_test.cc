// Copyright (C) 2026. Licensed under the MIT License.
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "mars/boost/filesystem.hpp"
#include "mars/comm/xlogger/xlogger_category.h"
#include "mars/xlog/appender.h"
#include "mars/xlog/src/log_zlib_buffer.h"
#include "mars/xlog/src/xlogger_appender.h"
#include "mars/xlog/xlogger_interface.h"

using namespace mars::xlog;
namespace fs = mars_boost::filesystem;

static void Check(bool _condition, const char* _message) {
    if (!_condition) {
        std::cerr << "FAIL: " << _message << std::endl;
        std::exit(1);
    }
}

static std::string Today() {
    const time_t now = time(nullptr);
    char date[16];
    strftime(date, sizeof(date), "%Y%m%d", localtime(&now));
    return date;
}

static void WriteFile(const fs::path& _path, const std::string& _data) {
    std::ofstream output(_path.string(), std::ios::binary);
    output.write(_data.data(), _data.size());
    Check(output.good(), "create test fixture");
}

static XLogConfig Config(const fs::path& _root, const std::string& _prefix) {
    XLogConfig config;
    config.logdir_ = (_root / _prefix).string();
    config.nameprefix_ = _prefix;
    config.mode_ = kAppenderSync;
    config.max_file_size_ = 64;
    config.max_alive_time_ = 7 * 24 * 60 * 60;
    fs::create_directories(config.logdir_);
    return config;
}

static fs::path Seed(const XLogConfig& _config) {
    const fs::path seed = fs::path(_config.logdir_) / (_config.nameprefix_ + "_" + Today() + ".xlog");
    WriteFile(seed, std::string(256, 'x'));
    return seed;
}

static void CheckSplit(const XLogConfig& _config, const fs::path& _seed) {
    Check(fs::file_size(_seed) == 256, "initial write must not append to oversized file");
    const fs::path split = fs::path(_config.logdir_) / (_config.nameprefix_ + "_" + Today() + "_1.xlog");
    Check(fs::exists(split) && fs::file_size(split) > 0, "initial write must use configured split");
}

int main(int argc, char** argv) {
    const fs::path root = fs::temp_directory_path() / fs::unique_path("xlog-policy-%%%%-%%%%");
    fs::create_directories(root);

    XLogConfig config = Config(root, "default");
    fs::path seed = Seed(config);
    appender_set_console_log(false);
    appender_open(config);
    appender_close();
    CheckSplit(config, seed);

    config = Config(root, "named");
    seed = Seed(config);
    auto* category = NewXloggerInstance(config, kLevelWarn);
    Check(category != nullptr, "create named instance");
    auto* appender = reinterpret_cast<XloggerAppender*>(category->GetAppender());
    appender->SetConsoleLog(false);
    CheckSplit(config, seed);
    config.logdir_.clear();
    config.max_file_size_ = -1;
    config.max_alive_time_ = 0;
    Check(NewXloggerInstance(config, kLevelVerbose) == category, "same prefix must reuse old instance");
    Check(category->GetLevel() == kLevelWarn, "reuse must preserve original level");
    ReleaseXloggerInstance("named");

    config = Config(root, "disabled");
    seed = Seed(config);
    config.max_file_size_ = 0;
    appender = XloggerAppender::NewInstance(config);
    Check(appender != nullptr, "zero disables size-based splitting");
    XloggerAppender::Release(appender);
    Check(fs::file_size(seed) > 256, "zero threshold must append to original file");

    config = Config(root, "invalid");
    config.logdir_ = (root / "must-not-exist").string();
    config.max_file_size_ = -1;
    Check(NewXloggerInstance(config, kLevelInfo) == nullptr, "reject negative threshold");
    config.max_file_size_ = 0;
    config.max_alive_time_ = 86399;
    Check(NewXloggerInstance(config, kLevelInfo) == nullptr, "reject retention below one day");
    Check(!fs::exists(config.logdir_), "invalid policy must not create resources");
    TFileIOAction action = kActionNone;
    appender_oneshot_flush(config, &action);
    Check(action == kActionOpenFailed, "oneshot must reject invalid policy");

    config = Config(root, "recovery");
    seed = Seed(config);
    std::vector<char> mapping(150 * 1024, 0);
    {
        LogZlibBuffer buffer(mapping.data(), mapping.size(), true, "");
        const std::string message = "recover this log\n";
        Check(buffer.Write(message.data(), message.size()), "prepare mmap log block");
        WriteFile(fs::path(config.logdir_) / "recovery.mmap3", std::string(mapping.data(), mapping.size()));
    }
    action = kActionNone;
    appender_oneshot_flush(config, &action);
    Check(action == kActionSuccess, "recover mmap using creation policy");
    CheckSplit(config, seed);

    if (argc > 1 && std::string(argv[1]) == "--expiry") {
        config = Config(root, "expiry");
        const fs::path expired = fs::path(config.logdir_) / "old.xlog";
        const fs::path recent = fs::path(config.logdir_) / "recent.xlog";
        WriteFile(expired, "old");
        WriteFile(recent, "recent");
        fs::last_write_time(expired, time(nullptr) - 8 * 24 * 60 * 60);
        fs::last_write_time(recent, time(nullptr) - 6 * 24 * 60 * 60);
        appender = XloggerAppender::NewInstance(config);
        Check(appender != nullptr, "create retention instance");
        // Changing the caller's object must not alter the captured seven-day policy.
        config.max_alive_time_ = 10 * 24 * 60 * 60;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(135);
        while (fs::exists(expired) && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        Check(!fs::exists(expired) && fs::exists(recent), "cleanup must use creation snapshot");
        XloggerAppender::Release(appender);
    }

    fs::remove_all(root);
    std::cout << "PASS: first-open policy, default/named creation, reuse, disabled splitting, validation and mmap recovery\n";
    return 0;
}
