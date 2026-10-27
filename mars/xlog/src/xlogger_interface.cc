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
 * @Author: garryyan
 * @LastEditors: garryyan
 * @Date: 2019-03-05 15:10:24
 * @LastEditTime: 2019-03-05 15:10:50
 */

#include "mars/xlog/xlogger_interface.h"

#include <functional>
#include <map>

#include "mars/comm/thread/lock.h"
#include "mars/comm/thread/mutex.h"
#include "mars/comm/xlogger/xlogger_category.h"
#include "xlogger_appender.h"

using namespace mars::comm;
namespace mars {
namespace xlog {

static Mutex& GetGlobalMutex() {
    static Mutex sg_mutex;
    return sg_mutex;
}

static std::map<std::string, XloggerCategory*>& GetGlobalInstanceMap() {
    static std::map<std::string, XloggerCategory*> sg_map;
    return sg_map;
}

XloggerCategory* NewXloggerInstance(const XLogConfig& _config, TLogLevel _level) {
    if (_config.logdir_.empty() || _config.nameprefix_.empty()) {
        return nullptr;
    }

    ScopedLock lock(GetGlobalMutex());
    auto it = GetGlobalInstanceMap().find(_config.nameprefix_);
    if (it != GetGlobalInstanceMap().end()) {
        return it->second;
    }

    XloggerAppender* appender = XloggerAppender::NewInstance(_config, 0);

    using namespace std::placeholders;
    XloggerCategory* category = XloggerCategory::NewInstance(reinterpret_cast<uintptr_t>(appender),
                                                             std::bind(&XloggerAppender::Write, appender, _1, _2));
    category->SetLevel(_level);
    GetGlobalInstanceMap()[_config.nameprefix_] = category;
    return category;
}

mars::comm::XloggerCategory* GetXloggerInstance(const char* _nameprefix) {
    if (nullptr == _nameprefix) {
        return nullptr;
    }

    ScopedLock lock(GetGlobalMutex());
    auto it = GetGlobalInstanceMap().find(_nameprefix);
    if (it != GetGlobalInstanceMap().end()) {
        return it->second;
    }

    return nullptr;
}

void ReleaseXloggerInstance(const char* _nameprefix) {
    if (nullptr == _nameprefix) {
        return;
    }

    ScopedLock lock(GetGlobalMutex());
    auto it = GetGlobalInstanceMap().find(_nameprefix);
    if (it == GetGlobalInstanceMap().end()) {
        return;
    }

    XloggerCategory* category = it->second;
    XloggerAppender* appender = reinterpret_cast<XloggerAppender*>(category->GetAppender());
    XloggerAppender::DelayRelease(appender);
    XloggerCategory::DelayRelease(category);
    GetGlobalInstanceMap().erase(it);
}

void XloggerWrite(uintptr_t _instance_ptr, const XLoggerInfo* _info, const char* _log) {
    if (0 == _instance_ptr) {
        xlogger_Write(_info, _log);
    } else {
        XloggerCategory* category = reinterpret_cast<XloggerCategory*>(_instance_ptr);
        category->Write(_info, _log);
    }
}

bool IsEnabledFor(uintptr_t _instance_ptr, TLogLevel _level) {
    if (0 == _instance_ptr) {
        return xlogger_IsEnabledFor(_level);
    } else {
        XloggerCategory* category = reinterpret_cast<XloggerCategory*>(_instance_ptr);
        return category->IsEnabledFor(_level);
    }
}

TLogLevel GetLevel(uintptr_t _instance_ptr) {
    if (0 == _instance_ptr) {
        return xlogger_Level();
    } else {
        XloggerCategory* category = reinterpret_cast<XloggerCategory*>(_instance_ptr);
        TLogLevel level = category->GetLevel();
        return level;
    }
}

void SetLevel(uintptr_t _instance_ptr, TLogLevel _level) {
    if (0 == _instance_ptr) {
        xlogger_SetLevel(_level);
    } else {
        XloggerCategory* category = reinterpret_cast<XloggerCategory*>(_instance_ptr);
        return category->SetLevel(_level);
    }
}

void SetAppenderMode(uintptr_t _instance_ptr, TAppenderMode _mode) {
    if (0 == _instance_ptr) {
        appender_setmode(_mode);
    } else {
        XloggerCategory* category = reinterpret_cast<XloggerCategory*>(_instance_ptr);
        XloggerAppender* appender = reinterpret_cast<XloggerAppender*>(category->GetAppender());
        return appender->SetMode(_mode);
    }
}

void Flush(uintptr_t _instance_ptr, bool _is_sync) {
    if (0 == _instance_ptr) {
        _is_sync ? appender_flush_sync() : appender_flush();
    } else {
        XloggerCategory* category = reinterpret_cast<XloggerCategory*>(_instance_ptr);
        XloggerAppender* appender = reinterpret_cast<XloggerAppender*>(category->GetAppender());
        _is_sync ? appender->FlushSync() : appender->Flush();
    }
}

void FlushAll(bool _is_sync) {
    _is_sync ? appender_flush_sync() : appender_flush();
    ScopedLock lock(GetGlobalMutex());
    auto& xmap = GetGlobalInstanceMap();
    // loop through all categories
    for (auto it = xmap.begin(); it != xmap.end(); ++it) {
        XloggerCategory* category = it->second;
        XloggerAppender* appender = reinterpret_cast<XloggerAppender*>(category->GetAppender());
        _is_sync ? appender->FlushSync() : appender->Flush();
    }
}

void SetConsoleLogOpen(uintptr_t _instance_ptr, bool _is_open) {
    if (0 == _instance_ptr) {
        appender_set_console_log(_is_open);
    } else {
        XloggerCategory* category = reinterpret_cast<XloggerCategory*>(_instance_ptr);
        XloggerAppender* appender = reinterpret_cast<XloggerAppender*>(category->GetAppender());
        return appender->SetConsoleLog(_is_open);
    }
}

void SetMaxFileSize(uintptr_t _instance_ptr, long _max_file_size) {
    if (0 == _instance_ptr) {
        appender_set_max_file_size(_max_file_size);
    } else {
        // 目前非默认实例设置最大大小因为时序会有问题
        // 因为设置过来的时候文件已经打开过一次
        XloggerCategory* category = reinterpret_cast<XloggerCategory*>(_instance_ptr);
        XloggerAppender* appender = reinterpret_cast<XloggerAppender*>(category->GetAppender());
        return appender->SetMaxFileSize(_max_file_size);
    }
}

void SetMaxAliveTime(uintptr_t _instance_ptr, long _alive_seconds) {
    if (0 == _instance_ptr) {
        appender_set_max_alive_duration(_alive_seconds);
    } else {
        XloggerCategory* category = reinterpret_cast<XloggerCategory*>(_instance_ptr);
        XloggerAppender* appender = reinterpret_cast<XloggerAppender*>(category->GetAppender());
        return appender->SetMaxAliveDuration(_alive_seconds);
    }
}

bool GetFilePathFromTimeSpan(uintptr_t _instance_ptr,
                             int _timespan,
                             const char* _prefix,
                             std::vector<std::string>& _filepath_vec) {
    if (0 == _instance_ptr) {
        return appender_getfilepath_from_timespan(_timespan, _prefix, _filepath_vec);
    }
    XloggerCategory* category = reinterpret_cast<XloggerCategory*>(_instance_ptr);
    XloggerAppender* appender = reinterpret_cast<XloggerAppender*>(category->GetAppender());
    return appender->GetfilepathFromTimespan(_timespan, _prefix, _filepath_vec);
}

}  // namespace xlog
}  // namespace mars

// C ABI adapter. The upstream instance lifecycle and write paths stay unchanged.
struct XLoggerWriter {
    uintptr_t target;
    std::string tag;
    XLoggerInfo info = XLOGGER_INFO_INITIALIZER;
    bool writing = false;

    XLoggerWriter(uintptr_t instance, const char* fixed_tag) : target(instance), tag(fixed_tag) {
        info.tag = tag.c_str();
        info.filename = "";
        info.func_name = "";
        info.pid = xlogger_pid();
        info.maintid = xlogger_maintid();
    }
};

XLoggerWriter* xlogger_writer_acquire_existing(const char* prefix, const char* tag) {
    uintptr_t target = 0;
    if (prefix != nullptr) {
        if (*prefix == '\0') return nullptr;
        auto* category = mars::xlog::GetXloggerInstance(prefix);
        if (category == nullptr) return nullptr;
        target = reinterpret_cast<uintptr_t>(category);
    }
    // Mars' Android build disables C++ exceptions. Allocation failure follows
    // the native runtime's OOM policy, just like its existing appender factory.
    XLoggerWriter* writer = new XLoggerWriter(target, tag == nullptr ? "" : tag);
    return writer;
}

int32_t xlogger_writer_is_enabled(const XLoggerWriter* writer, int32_t level) {
    if (writer == nullptr || level < kLevelVerbose || level >= kLevelNone) return 0;
    return mars::xlog::IsEnabledFor(writer->target, static_cast<TLogLevel>(level)) ? 1 : 0;
}

int32_t xlogger_writer_write(XLoggerWriter* writer, int32_t level, const char* message) {
    if (writer == nullptr || message == nullptr || level < kLevelVerbose || level >= kLevelNone) {
        return XLOGGER_WRITER_INVALID_ARGUMENT;
    }
    if (!mars::xlog::IsEnabledFor(writer->target, static_cast<TLogLevel>(level))) {
        return XLOGGER_WRITER_FILTERED;
    }
    // One serialized owner (the Dart isolate). Only a reentrant hook needs a
    // stack copy, so its metadata cannot overwrite the outer entry.
    if (writer->writing) {
        XLoggerInfo nested = writer->info;
        nested.level = static_cast<TLogLevel>(level);
        gettimeofday(&nested.timeval, nullptr);
        nested.tid = xlogger_tid();
        mars::xlog::XloggerWrite(writer->target, &nested, message);
    } else {
        writer->info.level = static_cast<TLogLevel>(level);
        gettimeofday(&writer->info.timeval, nullptr);
        // An isolate may resume on another OS thread; never cache this ID.
        writer->info.tid = xlogger_tid();
        writer->writing = true;
        mars::xlog::XloggerWrite(writer->target, &writer->info, message);
        writer->writing = false;
    }
    return XLOGGER_WRITER_OK;
}

void xlogger_writer_release(XLoggerWriter* writer) {
    delete writer;
}
