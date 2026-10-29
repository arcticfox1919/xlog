// Tencent is pleased to support the open source community by making Mars available.
// Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.

// Licensed under the MIT License (the "License"); you may not use this file except in
// compliance with the License. You may obtain a copy of the License at
// http://opensource.org/licenses/MIT

// Unless required by applicable law or agreed to in writing, software distributed under the License is
// distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
// either express or implied. See the License for the specific language governing permissions and
// limitations under the License.

/*
 * appender.h
 *
 *  Created on: 2013-3-7
 *      Author: yerungui
 */

#ifndef APPENDER_H_
#define APPENDER_H_

#include <stdint.h>

#include <string>
#include <vector>

namespace mars {
namespace xlog {

enum TAppenderMode {
    kAppenderAsync,
    kAppenderSync,
};

enum TCompressMode {
    kZlib,
    kZstd,
};

enum TFileIOAction {
    kActionNone = 0,
    kActionSuccess = 1,
    kActionUnnecessary = 2,
    kActionOpenFailed = 3,
    kActionReadFailed = 4,
    kActionWriteFailed = 5,
    kActionCloseFailed = 6,
    kActionRemoveFailed = 7,
};

struct XLogConfig {
    TAppenderMode mode_ = kAppenderAsync;
    // Different instances must use separate log and cache directories: expiry
    // cleanup scans the entire directory, not just this instance's name prefix.
    std::string logdir_;
    std::string nameprefix_;
    std::string pub_key_;
    TCompressMode compress_mode_ = kZlib;
    int compress_level_ = 6;
    std::string cachedir_;
    int cache_days_ = 0;
    // Size-based splitting threshold in bytes, not a hard cap. 0 disables it.
    int64_t max_file_size_ = 0;
    // Expiry threshold since last modification, in seconds; minimum one day.
    // Cleanup is delayed after opening; this is not an exact deletion deadline.
    int64_t max_alive_time_ = 10LL * 24 * 60 * 60;

    bool IsFilePolicyValid() const {
        return max_file_size_ >= 0 && max_alive_time_ >= 24 * 60 * 60;
    }
};

#ifdef __APPLE__
enum TConsoleFun {
    kConsolePrintf,
    kConsoleNSLog,
    kConsoleOSLog,
};
#endif

void appender_open(const XLogConfig& _config);

void appender_flush();
void appender_flush_sync();
void appender_close();
void appender_setmode(TAppenderMode _mode);
bool appender_getfilepath_from_timespan(int _timespan, const char* _prefix, std::vector<std::string>& _filepath_vec);
bool appender_make_logfile_name(int _timespan, const char* _prefix, std::vector<std::string>& _filepath_vec);
bool appender_get_current_log_path(char* _log_path, unsigned int _len);
bool appender_get_current_log_cache_path(char* _logPath, unsigned int _len);
void appender_set_console_log(bool _is_open);

#ifdef __APPLE__
void appender_set_console_fun(TConsoleFun _fun);
#endif
void appender_oneshot_flush(const XLogConfig& _config, TFileIOAction* _result);

}  // namespace xlog
}  // namespace mars

#endif /* APPENDER_H_ */
