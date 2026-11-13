#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface VolumeCoreBridge : NSObject

+ (NSDictionary *)computeWithFootprint:(NSArray<NSNumber *> *)footprint
                                  mode:(NSString *)mode
                          manualHeight:(double)manualHeight
                                  base:(NSArray<NSNumber *> *)base
                                   top:(NSArray<NSNumber *> *)top
                                  roof:(NSArray<NSNumber *> *)roof
                                    up:(NSArray<NSNumber *> *)up;

+ (double)polygonArea:(NSArray<NSNumber *> *)points;

@end

NS_ASSUME_NONNULL_END
