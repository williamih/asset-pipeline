#include "SystemTrayApp.h"
#import <Cocoa/Cocoa.h>

void SystemTrayApp::LaunchHelper(unsigned short port)
{
    NSURL* URL = [[NSBundle mainBundle] URLForResource:@"Asset Pipeline Helper"
                                         withExtension:@".app"];

    NSString* str = [NSString stringWithFormat:@"%d", (int)port];
    NSDictionary* config = @{
        NSWorkspaceLaunchConfigurationArguments: @[ str ]
    };

    [[NSWorkspace sharedWorkspace] launchApplicationAtURL:URL
                                                  options:0
                                            configuration:config
                                                    error:nil];
}
