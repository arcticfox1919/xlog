// Tencent is pleased to support the open source community by making Mars available.
// Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
// Licensed under the MIT License.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, MarsXLogLevel) {
    MarsXLogLevelVerbose = 0,
    MarsXLogLevelDebug = 1,
    MarsXLogLevelInfo = 2,
    MarsXLogLevelWarning = 3,
    MarsXLogLevelError = 4,
    MarsXLogLevelFatal = 5,
    MarsXLogLevelNone = 6,
};

typedef NS_ENUM(NSInteger, MarsXLogMode) {
    MarsXLogModeAsync = 0,
    MarsXLogModeSync = 1,
};

typedef NS_ENUM(NSInteger, MarsXLogCompressMode) {
    MarsXLogCompressModeZlib = 0,
    MarsXLogCompressModeZstd = 1,
};

@interface MarsXLogConfiguration : NSObject <NSCopying>

// Each instance must have its own log and cache directories. Expiry cleanup
// scans the entire directory and does not filter by namePrefix.
@property(nonatomic, copy) NSString *logDirectory;
@property(nonatomic, copy) NSString *namePrefix;
@property(nonatomic, copy, nullable) NSString *cacheDirectory;
@property(nonatomic, copy, nullable) NSString *publicKey;
@property(nonatomic) NSInteger cacheDays;
@property(nonatomic) MarsXLogLevel level;
@property(nonatomic) MarsXLogMode mode;
@property(nonatomic) MarsXLogCompressMode compressMode;
@property(nonatomic) NSInteger compressLevel;
// File splitting threshold in bytes (0 disables size-based splitting).
@property(nonatomic) int64_t maxFileSizeBytes;
// Expiry threshold in whole seconds, minimum 86400; defaults to 10 days.
@property(nonatomic) int64_t maxAliveTimeSeconds;

- (instancetype)initWithLogDirectory:(NSString *)logDirectory
                          namePrefix:(NSString *)namePrefix NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;

@end

@interface MarsXLogInstance : NSObject

@property(nonatomic, copy, readonly) NSString *namePrefix;
@property(nonatomic, readonly, getter=isClosed) BOOL closed;
@property(nonatomic) MarsXLogLevel level;

- (void)logWithLevel:(MarsXLogLevel)level
                 tag:(NSString *)tag
             message:(NSString *)message
                file:(NSString *)file
            function:(NSString *)function
                line:(NSInteger)line;

- (void)verbose:(NSString *)tag message:(NSString *)message;
- (void)debug:(NSString *)tag message:(NSString *)message;
- (void)info:(NSString *)tag message:(NSString *)message;
- (void)warning:(NSString *)tag message:(NSString *)message;
- (void)error:(NSString *)tag message:(NSString *)message;
- (void)fatal:(NSString *)tag message:(NSString *)message;

- (void)flush;
- (void)flushSync;
- (void)setMode:(MarsXLogMode)mode;
- (void)setConsoleLogEnabled:(BOOL)enabled;
- (NSArray<NSString *> *)filesFromTimeSpan:(NSInteger)days;
- (void)close;

- (instancetype)init NS_UNAVAILABLE;

@end

@interface MarsXLog : NSObject

+ (BOOL)openWithConfiguration:(MarsXLogConfiguration *)configuration;
+ (void)close;
+ (void)flush;
+ (void)flushSync;

+ (MarsXLogLevel)level;
+ (void)setLevel:(MarsXLogLevel)level;
+ (void)setMode:(MarsXLogMode)mode;
+ (void)setConsoleLogEnabled:(BOOL)enabled;
+ (NSArray<NSString *> *)filesFromTimeSpan:(NSInteger)days prefix:(NSString *)prefix;

+ (void)logWithLevel:(MarsXLogLevel)level
                  tag:(NSString *)tag
              message:(NSString *)message
                 file:(NSString *)file
             function:(NSString *)function
                 line:(NSInteger)line;

+ (void)verbose:(NSString *)tag message:(NSString *)message;
+ (void)debug:(NSString *)tag message:(NSString *)message;
+ (void)info:(NSString *)tag message:(NSString *)message;
+ (void)warning:(NSString *)tag message:(NSString *)message;
+ (void)error:(NSString *)tag message:(NSString *)message;
+ (void)fatal:(NSString *)tag message:(NSString *)message;

// An existing namePrefix returns the same instance without applying new settings.
+ (nullable MarsXLogInstance *)openInstanceWithConfiguration:(MarsXLogConfiguration *)configuration;
+ (nullable MarsXLogInstance *)instanceForNamePrefix:(NSString *)namePrefix;
+ (NSArray<NSString *> *)instanceNamePrefixes;
+ (void)closeInstanceWithNamePrefix:(NSString *)namePrefix;

@end

NS_ASSUME_NONNULL_END
