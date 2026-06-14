//
// Created by gjt on 10/17/24.
//

#include "mars/comm/xlogger/xlogger.h"

namespace mars {
namespace xlog {
void ConsoleLog(const XLoggerInfo* _info, const char* _log) {
    if (NULL == _info || NULL == _log)
        return;
    static const char* levelStrings[] = {
        "V",
        "D",  // debug
        "I",  // info
        "W",  // warn
        "E",  // error
        "F"   // fatal
    };
    char log[16 * 1024] = {0};
    snprintf(log,
             sizeof(log),
             "[%s][%s] %s\n",
             levelStrings[_info->level],
             NULL == _info->tag ? "" : _info->tag,
             _log);
    printf("%s", log);
}
}  // namespace xlog
}  // namespace mars
