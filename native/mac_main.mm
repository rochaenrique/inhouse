#include <Appkit/Appkit.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl.h>

int main(int argc, char **argv) 
{
	int width = 800;
	int height = 600;
	
	NSrect screenrect = [[NSScreen mainScreen] frame];
	
	NSrect windowrect = NSMakerect((screenrect.size.width - width)*0.5,
                                 (screenrect.size.height - height)*0.5,
                                 width,
                                 height);
	
	NSWindow *window = [[NSWindow alloc] 
                      initWithcontentrect:windowrect
                      styleMask: NSWindowStyleMaskTitled|
                      NSWindowStyleMaskClosable|
                      NSWindowStyleMaskminiaturizable|
                      NSWindowStyleMaskResizable
                      
                      backing: NSBackingStorebuffered
                      defer:NO];
	
	[window setBackgroundColor: NSColor.redColor];
	[window setTitle: @"Window Title"];
	[window makeKeyAndOrderFront: nil];
  
  open_gl *openGl = malloc(sizeof(open_gl));
  
	while (true) 
	{
		NSEvent *event;
		
		do 
		{
			event = [NSApp nextEventMatchingMask: NSEventMaskAny
               untilDate: nil
               inMode: NSDefaultRunLoopMode
               dequeue: YES];
			
			switch ([event type]) 
			{
				default: [NSApp sendEvent: event]; break;
			}
			
		}
		while (event != nil);
	}
}