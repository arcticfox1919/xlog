// Tencent is pleased to support the open source community by making Mars available.
// Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.

// Licensed under the MIT License (the "License"); you may not use this file except in
// compliance with the License. You may obtain a copy of the License at
// http://opensource.org/licenses/MIT

// Unless required by applicable law or agreed to in writing, software distributed under the License is
// distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
// either express or implied. See the License for the specific language governing permissions and
// limitations under the License.

/**
 * created on : 2012-7-19
 * author : yanguoyue
 */

#include <android/log.h>
#include <stdio.h>
#include <string.h>

#include "mars/comm/autobuffer.h"
#include "mars/comm/xlogger/xloggerbase.h"

namespace mars {
namespace xlog {

// 这里不能加日志，会导致循环调用
void ConsoleLog(const XLoggerInfo* _info, const char* _log) {
    if (_info) {
        __android_log_write(_info->level + 2, _info->tag ? _info->tag : "", _log ? _log : "NULL==log!!!");
    } else {
        __android_log_write(ANDROID_LOG_WARN, "", _log ? _log : "NULL==log!!!");
    }
}

}  // namespace xlog
}  // namespace mars
