#include "platform/TouchIdAuth.h"

#import <LocalAuthentication/LocalAuthentication.h>

bool touchIdAvailable()
{
    LAContext *context = [[LAContext alloc] init];
    NSError *error = nil;
    return [context canEvaluatePolicy:LAPolicyDeviceOwnerAuthenticationWithBiometrics error:&error];
}

bool touchIdAuthenticate(const char *reason)
{
    LAContext *context = [[LAContext alloc] init];
    NSError *error = nil;

    if (![context canEvaluatePolicy:LAPolicyDeviceOwnerAuthenticationWithBiometrics error:&error]) {
        return false;
    }

    dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
    __block bool success = false;

    NSString *nsReason = [NSString stringWithUTF8String:reason ? reason : "Desbloquear Mani"];
    [context evaluatePolicy:LAPolicyDeviceOwnerAuthenticationWithBiometrics
            localizedReason:nsReason
                      reply:^(BOOL ok, NSError * /*authError*/) {
                          success = ok;
                          dispatch_semaphore_signal(semaphore);
                      }];

    dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
    return success;
}
