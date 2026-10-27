// Tencent is pleased to support the open source community by making Mars available.
// Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
// Licensed under the MIT License.

#import "MarsXLog.h"

#include <sys/time.h>

#include <string>
#include <vector>

#include "mars/comm/xlogger/xloggerbase.h"
#include "mars/xlog/appender.h"
#include "mars/xlog/xlogger_interface.h"

using mars::xlog::XLogConfig;

static TLogLevel ToNativeLevel(MarsXLogLevel level) {
    return static_cast<TLogLevel>(level);
}

static BOOL PrepareDirectory(NSString *path) {
    if (path.length == 0) {
        return NO;
    }
    NSError *error = nil;
    return [[NSFileManager defaultManager] createDirectoryAtPath:path
                                     withIntermediateDirectories:YES
                                                      attributes:nil
                                                           error:&error];
}

static XLogConfig NativeConfiguration(MarsXLogConfiguration *configuration) {
    XLogConfig config;
    config.mode_ = static_cast<mars::xlog::TAppenderMode>(configuration.mode);
    config.logdir_ = configuration.logDirectory.UTF8String ?: "";
    config.nameprefix_ = configuration.namePrefix.UTF8String ?: "";
    config.pub_key_ = configuration.publicKey.UTF8String ?: "";
    config.compress_mode_ = static_cast<mars::xlog::TCompressMode>(configuration.compressMode);
    config.compress_level_ = static_cast<int>(configuration.compressLevel);
    config.cachedir_ = configuration.cacheDirectory.UTF8String ?: "";
    config.cache_days_ = static_cast<int>(configuration.cacheDays);
    return config;
}

static void WriteLog(uintptr_t instance,
                     MarsXLogLevel level,
                     NSString *tag,
                     NSString *message,
                     NSString *file,
                     NSString *function,
                     NSInteger line) {
    TLogLevel nativeLevel = ToNativeLevel(level);
    if (!mars::xlog::IsEnabledFor(instance, nativeLevel)) {
        return;
    }

    XLoggerInfo info = XLOGGER_INFO_INITIALIZER;
    info.level = nativeLevel;
    info.tag = tag.UTF8String ?: "";
    info.filename = file.UTF8String ?: "";
    info.func_name = function.UTF8String ?: "";
    info.line = static_cast<int>(line);
    gettimeofday(&info.timeval, nullptr);
    info.pid = xlogger_pid();
    info.tid = xlogger_tid();
    info.maintid = xlogger_maintid();
    mars::xlog::XloggerWrite(instance, &info, message.UTF8String ?: "");
}

@implementation MarsXLogConfiguration

- (instancetype)initWithLogDirectory:(NSString *)logDirectory namePrefix:(NSString *)namePrefix {
    self = [super init];
    if (self) {
        _logDirectory = [logDirectory copy];
        _namePrefix = [namePrefix copy];
        _cacheDays = 0;
        _level = MarsXLogLevelInfo;
        _mode = MarsXLogModeAsync;
        _compressMode = MarsXLogCompressModeZlib;
        _compressLevel = 6;
    }
    return self;
}

- (id)copyWithZone:(NSZone *)zone {
    MarsXLogConfiguration *copy = [[[self class] allocWithZone:zone]
        initWithLogDirectory:self.logDirectory
                  namePrefix:self.namePrefix];
    copy.cacheDirectory = self.cacheDirectory;
    copy.publicKey = self.publicKey;
    copy.cacheDays = self.cacheDays;
    copy.level = self.level;
    copy.mode = self.mode;
    copy.compressMode = self.compressMode;
    copy.compressLevel = self.compressLevel;
    return copy;
}

@end

@interface MarsXLogInstance ()

@property(nonatomic, copy, readwrite) NSString *namePrefix;
@property(nonatomic, readwrite, getter=isClosed) BOOL closed;
@property(nonatomic) uintptr_t nativeInstance;

- (instancetype)initWithNativeInstance:(uintptr_t)nativeInstance namePrefix:(NSString *)namePrefix;

@end

static NSMutableDictionary<NSString *, MarsXLogInstance *> *InstanceRegistry(void) {
    static NSMutableDictionary<NSString *, MarsXLogInstance *> *instances;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instances = [[NSMutableDictionary alloc] init];
    });
    return instances;
}

@implementation MarsXLogInstance

- (instancetype)initWithNativeInstance:(uintptr_t)nativeInstance namePrefix:(NSString *)namePrefix {
    self = [super init];
    if (self) {
        _nativeInstance = nativeInstance;
        _namePrefix = [namePrefix copy];
        _closed = nativeInstance == 0;
    }
    return self;
}

- (MarsXLogLevel)level {
    @synchronized(self) {
        return self.closed ? MarsXLogLevelNone
                           : static_cast<MarsXLogLevel>(mars::xlog::GetLevel(self.nativeInstance));
    }
}

- (void)setLevel:(MarsXLogLevel)level {
    @synchronized(self) {
        if (!self.closed) {
            mars::xlog::SetLevel(self.nativeInstance, ToNativeLevel(level));
        }
    }
}

- (void)logWithLevel:(MarsXLogLevel)level
                 tag:(NSString *)tag
             message:(NSString *)message
                file:(NSString *)file
            function:(NSString *)function
                line:(NSInteger)line {
    @synchronized(self) {
        if (!self.closed) {
            WriteLog(self.nativeInstance, level, tag, message, file, function, line);
        }
    }
}

- (void)verbose:(NSString *)tag message:(NSString *)message {
    [self logWithLevel:MarsXLogLevelVerbose tag:tag message:message file:@"" function:@"" line:0];
}
- (void)debug:(NSString *)tag message:(NSString *)message {
    [self logWithLevel:MarsXLogLevelDebug tag:tag message:message file:@"" function:@"" line:0];
}
- (void)info:(NSString *)tag message:(NSString *)message {
    [self logWithLevel:MarsXLogLevelInfo tag:tag message:message file:@"" function:@"" line:0];
}
- (void)warning:(NSString *)tag message:(NSString *)message {
    [self logWithLevel:MarsXLogLevelWarning tag:tag message:message file:@"" function:@"" line:0];
}
- (void)error:(NSString *)tag message:(NSString *)message {
    [self logWithLevel:MarsXLogLevelError tag:tag message:message file:@"" function:@"" line:0];
}
- (void)fatal:(NSString *)tag message:(NSString *)message {
    [self logWithLevel:MarsXLogLevelFatal tag:tag message:message file:@"" function:@"" line:0];
}

- (void)flush {
    @synchronized(self) {
        if (!self.closed) mars::xlog::Flush(self.nativeInstance, false);
    }
}

- (void)flushSync {
    @synchronized(self) {
        if (!self.closed) mars::xlog::Flush(self.nativeInstance, true);
    }
}

- (void)setMode:(MarsXLogMode)mode {
    @synchronized(self) {
        if (!self.closed) {
            mars::xlog::SetAppenderMode(self.nativeInstance,
                                        static_cast<mars::xlog::TAppenderMode>(mode));
        }
    }
}

- (void)setConsoleLogEnabled:(BOOL)enabled {
    @synchronized(self) {
        if (!self.closed) mars::xlog::SetConsoleLogOpen(self.nativeInstance, enabled);
    }
}

- (void)setMaxFileSize:(int64_t)bytes {
    @synchronized(self) {
        if (!self.closed) mars::xlog::SetMaxFileSize(self.nativeInstance, static_cast<long>(bytes));
    }
}

- (void)setMaxAliveTime:(NSTimeInterval)seconds {
    @synchronized(self) {
        if (!self.closed) mars::xlog::SetMaxAliveTime(self.nativeInstance, static_cast<long>(seconds));
    }
}

- (NSArray<NSString *> *)filesFromTimeSpan:(NSInteger)days {
    @synchronized(self) {
        if (self.closed) return @[];
        std::vector<std::string> paths;
        mars::xlog::GetFilePathFromTimeSpan(self.nativeInstance,
                                            static_cast<int>(days),
                                            self.namePrefix.UTF8String,
                                            paths);
        NSMutableArray<NSString *> *result = [NSMutableArray arrayWithCapacity:paths.size()];
        for (const std::string &path : paths) {
            NSString *value = [NSString stringWithUTF8String:path.c_str()];
            if (value) [result addObject:value];
        }
        return result;
    }
}

- (void)close {
    NSString *prefix = nil;
    @synchronized(self) {
        if (self.closed) return;
        prefix = self.namePrefix;
        mars::xlog::ReleaseXloggerInstance(prefix.UTF8String);
        self.nativeInstance = 0;
        self.closed = YES;
    }
    @synchronized([MarsXLog class]) {
        [InstanceRegistry() removeObjectForKey:prefix];
    }
}

@end

@implementation MarsXLog

+ (BOOL)openWithConfiguration:(MarsXLogConfiguration *)configuration {
    if (configuration.namePrefix.length == 0 || !PrepareDirectory(configuration.logDirectory)) {
        return NO;
    }
    if (configuration.cacheDirectory.length > 0 && !PrepareDirectory(configuration.cacheDirectory)) {
        return NO;
    }
    xlogger_SetLevel(ToNativeLevel(configuration.level));
    mars::xlog::appender_open(NativeConfiguration(configuration));
    return YES;
}

+ (void)close {
    NSArray<MarsXLogInstance *> *instances;
    @synchronized(self) {
        instances = InstanceRegistry().allValues;
    }
    for (MarsXLogInstance *instance in instances) [instance close];
    mars::xlog::appender_close();
}

+ (void)flush { mars::xlog::FlushAll(false); }
+ (void)flushSync { mars::xlog::FlushAll(true); }
+ (MarsXLogLevel)level { return static_cast<MarsXLogLevel>(mars::xlog::GetLevel(0)); }
+ (void)setLevel:(MarsXLogLevel)level { mars::xlog::SetLevel(0, ToNativeLevel(level)); }
+ (void)setMode:(MarsXLogMode)mode {
    mars::xlog::SetAppenderMode(0, static_cast<mars::xlog::TAppenderMode>(mode));
}
+ (void)setConsoleLogEnabled:(BOOL)enabled { mars::xlog::SetConsoleLogOpen(0, enabled); }
+ (void)setMaxFileSize:(int64_t)bytes { mars::xlog::SetMaxFileSize(0, static_cast<long>(bytes)); }
+ (void)setMaxAliveTime:(NSTimeInterval)seconds {
    mars::xlog::SetMaxAliveTime(0, static_cast<long>(seconds));
}

+ (NSArray<NSString *> *)filesFromTimeSpan:(NSInteger)days prefix:(NSString *)prefix {
    std::vector<std::string> paths;
    mars::xlog::GetFilePathFromTimeSpan(0,
                                        static_cast<int>(days),
                                        prefix.UTF8String,
                                        paths);
    NSMutableArray<NSString *> *result = [NSMutableArray arrayWithCapacity:paths.size()];
    for (const std::string &path : paths) {
        NSString *value = [NSString stringWithUTF8String:path.c_str()];
        if (value) [result addObject:value];
    }
    return result;
}

+ (void)logWithLevel:(MarsXLogLevel)level
                  tag:(NSString *)tag
              message:(NSString *)message
                 file:(NSString *)file
             function:(NSString *)function
                 line:(NSInteger)line {
    WriteLog(0, level, tag, message, file, function, line);
}

+ (void)verbose:(NSString *)tag message:(NSString *)message {
    [self logWithLevel:MarsXLogLevelVerbose tag:tag message:message file:@"" function:@"" line:0];
}
+ (void)debug:(NSString *)tag message:(NSString *)message {
    [self logWithLevel:MarsXLogLevelDebug tag:tag message:message file:@"" function:@"" line:0];
}
+ (void)info:(NSString *)tag message:(NSString *)message {
    [self logWithLevel:MarsXLogLevelInfo tag:tag message:message file:@"" function:@"" line:0];
}
+ (void)warning:(NSString *)tag message:(NSString *)message {
    [self logWithLevel:MarsXLogLevelWarning tag:tag message:message file:@"" function:@"" line:0];
}
+ (void)error:(NSString *)tag message:(NSString *)message {
    [self logWithLevel:MarsXLogLevelError tag:tag message:message file:@"" function:@"" line:0];
}
+ (void)fatal:(NSString *)tag message:(NSString *)message {
    [self logWithLevel:MarsXLogLevelFatal tag:tag message:message file:@"" function:@"" line:0];
}

+ (MarsXLogInstance *)openInstanceWithConfiguration:(MarsXLogConfiguration *)configuration {
    if (configuration.namePrefix.length == 0 || !PrepareDirectory(configuration.logDirectory)) return nil;
    if (configuration.cacheDirectory.length > 0 && !PrepareDirectory(configuration.cacheDirectory)) return nil;

    @synchronized(self) {
        MarsXLogInstance *existing = InstanceRegistry()[configuration.namePrefix];
        if (existing) return existing;
        mars::comm::XloggerCategory *category =
            mars::xlog::NewXloggerInstance(NativeConfiguration(configuration), ToNativeLevel(configuration.level));
        if (!category) return nil;
        MarsXLogInstance *instance = [[MarsXLogInstance alloc]
            initWithNativeInstance:reinterpret_cast<uintptr_t>(category)
                      namePrefix:configuration.namePrefix];
        InstanceRegistry()[configuration.namePrefix] = instance;
        return instance;
    }
}

+ (MarsXLogInstance *)instanceForNamePrefix:(NSString *)namePrefix {
    @synchronized(self) {
        return InstanceRegistry()[namePrefix];
    }
}

+ (NSArray<NSString *> *)instanceNamePrefixes {
    @synchronized(self) {
        return InstanceRegistry().allKeys;
    }
}

+ (void)closeInstanceWithNamePrefix:(NSString *)namePrefix {
    [[self instanceForNamePrefix:namePrefix] close];
}

@end
