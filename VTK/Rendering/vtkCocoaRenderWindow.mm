/*=========================================================================

Program:   Visualization Toolkit
Module:    $RCSfile$

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#import "vtkCocoaRenderWindow.h"
#import "vtkIdList.h"
#import "vtkObjectFactory.h"
#import "vtkRendererCollection.h"
#import "vtkCocoaGLView.h"

#ifndef MAC_OS_X_VERSION_10_4
#define MAC_OS_X_VERSION_10_4 1040
#endif

vtkCxxRevisionMacro(vtkCocoaRenderWindow, "$Revision$");
vtkStandardNewMacro(vtkCocoaRenderWindow);


//----------------------------------------------------------------------------
vtkCocoaRenderWindow::vtkCocoaRenderWindow()
{
  this->WindowCreated = 0;
  this->ViewCreated = 0;
  this->SetContextId(0);
  this->MultiSamples = 8;
  this->SetWindowId(0);
  this->SetDisplayId(0);
  this->SetWindowName("Visualization Toolkit - Cocoa");
  this->CursorHidden = 0;
  this->ForceMakeCurrent = 0;
  this->Capabilities = 0;
  this->OnScreenInitialized = 0;
  this->OffScreenInitialized = 0;
}

//----------------------------------------------------------------------------
vtkCocoaRenderWindow::~vtkCocoaRenderWindow()
{
  if (this->CursorHidden)
    {
    this->ShowCursor();
    }
  this->Finalize();

  if (this->Capabilities)
    {
    delete[] this->Capabilities;
    this->Capabilities = 0;
    }
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::Finalize()
{
  if(this->OffScreenInitialized)
    {
    this->OffScreenInitialized = 0;
    this->DestroyOffScreenWindow();
    }
  if(this->OnScreenInitialized)
    {
    this->OnScreenInitialized = 0;
    this->DestroyWindow();
    }
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::DestroyWindow()
{
  GLuint txId;

  // finish OpenGL rendering
  if (this->GetContextId())
    {
    this->MakeCurrent();

    // now delete all textures
    glDisable(GL_TEXTURE_2D);
    for (int i = 1; i < this->TextureResourceIds->GetNumberOfIds(); i++)
      {
      txId = (GLuint) this->TextureResourceIds->GetId(i);
#ifdef GL_VERSION_1_1
      if (glIsTexture(txId))
        {
        glDeleteTextures(1, &txId);
        }
#else
      if (glIsList(txId))
        {
        glDeleteLists(txId,1);
        }
#endif
      }

    // tell each of the renderers that this render window/graphics context
    // is being removed (the RendererCollection is removed by vtkRenderWindow's
    // destructor)
    vtkCollectionSimpleIterator rsit;
    vtkRenderer *ren;
    for ( this->Renderers->InitTraversal(rsit);
          (ren = this->Renderers->GetNextRenderer(rsit));)
      {
      ren->SetRenderWindow(NULL);
      }

    [(NSOpenGLContext*)this->GetContextId() release];
    [(NSOpenGLPixelFormat*)this->GetPixelFormat() release];
    
    this->SetContextId(NULL);
    this->SetPixelFormat(NULL);
  }

  // If this class created the view, then this class must release it.
  // Note that this doesn't remove it from the window, as the window
  // has retained it.
  if (this->GetDisplayId() && this->ViewCreated)
  {
    [(NSView *)this->GetDisplayId() release];
  }
  this->SetDisplayId(NULL);
  
  // The window is already released by this point, clear it anyway
  this->SetWindowId(NULL);
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::SetWindowName( const char * _arg )
{
  vtkWindow::SetWindowName(_arg);
  if (this->GetWindowId())
    {
    NSString* winTitleStr;

#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
    winTitleStr = [NSString stringWithCString:_arg encoding:NSASCIIStringEncoding];
#else
    winTitleStr = [NSString stringWithCString:_arg];
#endif

    [(NSWindow*)this->GetWindowId() setTitle:winTitleStr];
    }
}

//----------------------------------------------------------------------------
int vtkCocoaRenderWindow::GetEventPending()
{
  return 0;
}

//----------------------------------------------------------------------------
// Initialize the rendering process.
void vtkCocoaRenderWindow::Start()
{
  this->Initialize();

  // set the current window
  this->MakeCurrent();
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::MakeCurrent()
{
  if (this->GetContextId())
    {
    [(NSOpenGLContext*)this->GetContextId() makeCurrentContext];
    }
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::UpdateContext()
{
  if (this->GetContextId())
    {
    [(NSOpenGLContext*)this->GetContextId() update];
    }
}

//----------------------------------------------------------------------------
const char* vtkCocoaRenderWindow::ReportCapabilities()
{
  this->MakeCurrent();

  const char* glVendor = (const char*) glGetString(GL_VENDOR);
  const char* glRenderer = (const char*) glGetString(GL_RENDERER);
  const char* glVersion = (const char*) glGetString(GL_VERSION);
  const char* glExtensions = (const char*) glGetString(GL_EXTENSIONS);

  ostrstream strm;
  strm << "OpenGL vendor string:  " << glVendor
       << "\nOpenGL renderer string:  " << glRenderer
       << "\nOpenGL version string:  " << glVersion
       << "\nOpenGL extensions:  " << glExtensions << endl;

  // Obtain the OpenGL context in order to keep track of the current screen.
  NSOpenGLContext* context = (NSOpenGLContext*)this->GetContextId();
  int currentScreen = [context currentVirtualScreen];

  // The NSOpenGLPixelFormat can only be queried for one particular
  // attribute at a time. Just make repeated queries to get the
  // pertinent settings.
  NSOpenGLPixelFormat* pixelFormat = (NSOpenGLPixelFormat*)this->GetPixelFormat();
  strm << "PixelFormat Descriptor:" << endl;
  GLint pfd;
  [pixelFormat getValues: &pfd forAttribute: NSOpenGLPFAColorSize forVirtualScreen: currentScreen];
  strm  << "  colorSize:  " << pfd << endl;

  [pixelFormat getValues: &pfd forAttribute: NSOpenGLPFAAlphaSize forVirtualScreen: currentScreen];
  strm  << "  alphaSize:  " << pfd << endl;

  [pixelFormat getValues: &pfd forAttribute: NSOpenGLPFAStencilSize forVirtualScreen: currentScreen];
  strm  << "  stencilSize:  " << pfd << endl;

  [pixelFormat getValues: &pfd forAttribute: NSOpenGLPFADepthSize forVirtualScreen: currentScreen];
  strm  << "  depthSize:  " << pfd << endl;

  [pixelFormat getValues: &pfd forAttribute: NSOpenGLPFAAccumSize forVirtualScreen: currentScreen];
  strm  << "  accumSize:  " << pfd << endl;

  [pixelFormat getValues: &pfd forAttribute: NSOpenGLPFADoubleBuffer forVirtualScreen: currentScreen];
  strm  << "  double buffer:  " << (pfd == YES ? "Yes" : "No") << endl;

  [pixelFormat getValues: &pfd forAttribute: NSOpenGLPFAStereo forVirtualScreen: currentScreen];
  strm  << "  stereo:  " << (pfd == YES ? "Yes" : "No") << endl;

  [pixelFormat getValues: &pfd forAttribute: NSOpenGLPFAAccelerated forVirtualScreen: currentScreen];
  strm  << "  hardware acceleration::  " << (pfd == YES ? "Yes" : "No") << endl;

  strm << ends;
  delete[] this->Capabilities;
  this->Capabilities = new char[strlen(strm.str()) + 1];
  strcpy(this->Capabilities, strm.str());
  return this->Capabilities;
}

//----------------------------------------------------------------------------
int vtkCocoaRenderWindow::SupportsOpenGL()
{
  this->MakeCurrent();
  if (!this->GetContextId() || !this->GetPixelFormat())
    {
    return 0;
    }

  NSOpenGLContext* context = (NSOpenGLContext*)this->GetContextId();
  int currentScreen = [context currentVirtualScreen];

  NSOpenGLPixelFormat* pixelFormat = (NSOpenGLPixelFormat*)this->GetPixelFormat();
  GLint pfd;
  [pixelFormat getValues: &pfd forAttribute: NSOpenGLPFACompliant forVirtualScreen: currentScreen];

  return (pfd == YES ? 1 : 0);
}

//----------------------------------------------------------------------------
int vtkCocoaRenderWindow::IsDirect()
{
  this->MakeCurrent();
  if (!this->GetContextId() || !this->GetPixelFormat())
    {
    return 0;
    }

  NSOpenGLContext* context = (NSOpenGLContext*)this->GetContextId();
  int currentScreen = [context currentVirtualScreen];

  NSOpenGLPixelFormat* pixelFormat = (NSOpenGLPixelFormat*)this->GetPixelFormat();
  GLint pfd;
  [pixelFormat getValues: &pfd forAttribute: NSOpenGLPFAFullScreen forVirtualScreen: currentScreen];

  return (pfd == YES ? 1 : 0);
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::SetSize(int* a)
{
  this->SetSize( a[0], a[1] );
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::SetSize(int x, int y)
{
  static int resizing = 0;

  if ((this->Size[0] != x) || (this->Size[1] != y))
    {
    this->Modified();
    this->Size[0] = x;
    this->Size[1] = y;
    if (this->GetWindowId() && this->Mapped)
      {
      if (!resizing)
        {
        resizing = 1;
        NSSize theSize = NSMakeSize((float)x, (float)y);
        [(NSWindow*)this->GetWindowId() setContentSize:theSize];
        resizing = 0;
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::SetForceMakeCurrent()
{
  this->ForceMakeCurrent = 1;
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::SetPosition(int* a)
{
  this->SetPosition( a[0], a[1] );
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::SetPosition(int x, int y)
{
  static int resizing = 0;

  if ((this->Position[0] != x) || (this->Position[1] != y))
    {
    this->Modified();
    this->Position[0] = x;
    this->Position[1] = y;
    if (this->GetWindowId() && this->Mapped)
      {
      if (!resizing)
        {
        resizing = 1;
        NSPoint origin = NSMakePoint((float)x, (float)y);
        [(NSWindow*)this->GetWindowId() setFrameOrigin:origin];
        resizing = 0;
        }
      }
    }
}

//----------------------------------------------------------------------------
// End the rendering process and display the image.
void vtkCocoaRenderWindow::Frame()
{
  this->MakeCurrent();
  [(NSOpenGLContext*)this->GetContextId() flushBuffer];
}

//----------------------------------------------------------------------------
// Update system if needed due to stereo rendering.
void vtkCocoaRenderWindow::StereoUpdate()
{
  // if stereo is on and it wasn't before
  if (this->StereoRender && (!this->StereoStatus))
    {
    switch (this->StereoType)
      {
      case VTK_STEREO_CRYSTAL_EYES:
        this->StereoStatus = 1;
        break;
      case VTK_STEREO_RED_BLUE:
        this->StereoStatus = 1;
        break;
      case VTK_STEREO_ANAGLYPH:
        this->StereoStatus = 1;
        break;
      case VTK_STEREO_DRESDEN:
        this->StereoStatus = 1;
        break;
      case VTK_STEREO_INTERLACED:
        this->StereoStatus = 1;
        break;
      }
    }
  else if ((!this->StereoRender) && this->StereoStatus)
    {
    switch (this->StereoType)
      {
      case VTK_STEREO_CRYSTAL_EYES:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_RED_BLUE:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_ANAGLYPH:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_DRESDEN:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_INTERLACED:
        this->StereoStatus = 0;
        break;
      }
    }
}

//----------------------------------------------------------------------------
// Specify various window parameters.
void vtkCocoaRenderWindow::WindowConfigure()
{
  // this is all handled by the desiredVisualInfo method
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::SetupPixelFormat(void*, void*, int, int, int)
{
  vtkErrorMacro(<< "vtkCocoaRenderWindow::SetupPixelFormat - IMPLEMENT\n");
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::SetupPalette(void*)
{
  vtkErrorMacro(<< "vtkCocoaRenderWindow::SetupPalette - IMPLEMENT\n");
}

//----------------------------------------------------------------------------
// Initialize the window for rendering.
void vtkCocoaRenderWindow::CreateAWindow()
{
  static int count = 1;

  // As vtk is both crossplatform and a library, we don't know if it is being
  // used in a 'regular Cocoa application' or as a 'pure vtk application'.
  // By the former I mean a regular Cocoa application that happens to have
  // a vtkCocoaGLView, by the latter I mean an application that only uses
  // vtk APIs (which happen to use Cocoa as an implementation detail).
  // Specifically, we can't know if NSApplicationMain() was ever called
  // (which is usually done in main()), nor whether the NSApplication exists.
  //
  // So here we call +sharedApplication which will create the NSApplication
  // if it does not exist.  If it does exist, this does nothing.
  // We are not actually interested in the return value.
  // This call is intentionally delayed until this CreateAWindow call
  // to prevent Cocoa-window related stuff from happening in scenarios
  // where vtkRenderWindows are created but never shown.
  (void)[NSApplication sharedApplication];

  // create an NSWindow only if neither an NSView nor an NSWindow have
  // been specified already.  This is the case for a 'pure vtk application'.
  // If you are using vtk in a 'regular Mac application' you should call
  // SetWindowId() and SetDisplayId() so that a window is not created.
  if (!this->GetWindowId() && !this->GetDisplayId())
    {
    if ((this->Size[0]+this->Size[1]) == 0)
      {
      this->Size[0] = 300;
      this->Size[1] = 300;
      }
    if ((this->Position[0]+this->Position[1]) == 0)
      {
      this->Position[0] = 50;
      this->Position[1] = 50;
      }
    NSRect ctRect = NSMakeRect((float)this->Position[0],
                               (float)this->Position[1],
                               (float)this->Size[0],
                               (float)this->Size[1]);

    NSWindow* theWindow = [[NSWindow alloc]
                           initWithContentRect:ctRect
                           styleMask:NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask
                           backing:NSBackingStoreBuffered
                           defer:NO];
    if (!theWindow)
      {
      vtkErrorMacro("Could not create window, serious error!");
      return;
      }

    [theWindow makeKeyAndOrderFront:nil];

    [theWindow setAcceptsMouseMovedEvents:YES];

    this->SetWindowId(theWindow);
    this->WindowCreated = 1;
    }

  // create a vtkCocoaGLView if one has not been specified
  if (!this->GetDisplayId())
    {
    NSRect glRect =
      NSMakeRect(0.0, 0.0, (float)this->Size[0], (float)this->Size[1]);
    vtkCocoaGLView *glView = [[vtkCocoaGLView alloc] initWithFrame:glRect];
    [(NSWindow*)this->GetWindowId() setContentView:glView];
    
    this->SetDisplayId(glView);
    this->ViewCreated = 1;
    [glView setVTKRenderWindow:this];
    }

  this->CreateGLContext();

  // Set the window title *after* CreateGLContext. We cannot do it earlier
  // because of a bug in Panther's java library (OSX 10.3.9, Java 1.4.2_09)
  //
  // Details on Apple bug:
  // http://lists.apple.com/archives/Quartz-dev/2005/Apr/msg00043.html
  // Appears to be fixed in Mac OS X 10.4, but we workaround it here anyhow
  // so that we can still work on 10.3...
  //
  // Additionally, only change the window title if it was created by vtk
  if (this->WindowCreated)
    {
    NSString * winName = [NSString stringWithFormat:@"Visualization Toolkit - Cocoa #%i", count++];
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
    this->SetWindowName([winName cStringUsingEncoding:NSASCIIStringEncoding]);
#else
    this->SetWindowName([winName cString]);
#endif
    }
  
  // the error "invalid drawable" in the console from this call can appear
  // but only early in the app's lifetime (ie sometime during launch)
  // IMPORTANT: this is necessary to update the context here in case of
  // hardware offscreen rendering
  NSOpenGLContext* context = (NSOpenGLContext*)this->GetContextId();
  [context setView:(NSView*)this->GetDisplayId()];

  [context update];

  this->MakeCurrent();

  // wipe out any existing display lists
  vtkRenderer *renderer;
  vtkCollectionSimpleIterator rsit;

  for ( this->Renderers->InitTraversal(rsit);
        (renderer = this->Renderers->GetNextRenderer(rsit));)
    {
    renderer->SetRenderWindow(0);
    renderer->SetRenderWindow(this);
    }
  this->OpenGLInit();
  this->Mapped = 1;
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::CreateGLContext()
{
  NSOpenGLPixelFormatAttribute attribs[] =
    {
      NSOpenGLPFAAccelerated,
      NSOpenGLPFADepthSize,
      (NSOpenGLPixelFormatAttribute)32,
      (this->DoubleBuffer != 0) ?
        (NSOpenGLPixelFormatAttribute)NSOpenGLPFADoubleBuffer :
        (NSOpenGLPixelFormatAttribute)nil,
      (NSOpenGLPixelFormatAttribute)nil
    };

  NSOpenGLPixelFormat* pixelFormat = [[NSOpenGLPixelFormat alloc]
                                      initWithAttributes:attribs];
  NSOpenGLContext* context = [[NSOpenGLContext alloc]
                              initWithFormat:pixelFormat
                              shareContext:nil];
  
  // This syncs the OpenGL context to the VBL to prevent tearing
  GLint one = 1;
  [context setValues:&one forParameter:NSOpenGLCPSwapInterval];

  this->SetPixelFormat((void*)pixelFormat);
  this->SetContextId((void*)context);
}

//----------------------------------------------------------------------------
// Initialize the rendering window.
void vtkCocoaRenderWindow::Initialize ()
{
  if(this->OffScreenRendering)
    {
    // destroy on screen
    if(this->OnScreenInitialized)
      {
      this->DestroyWindow();
      this->OnScreenInitialized = 0;
      }
    // create off screen
    if(!this->OffScreenInitialized)
      {
      int width=((this->Size[0]>0) ? this->Size[0] : 300);
      int height=((this->Size[1]>0) ? this->Size[1] : 300);
      if(!this->CreateHardwareOffScreenWindow(width,height))
        {
        // no other offscreen mode available, do on screen rendering
        this->CreateAWindow();
        }
      this->OffScreenInitialized = 1;
      }
    }
  else
    {
    // destroy off screen
    if(this->OffScreenInitialized)
      {
      this->DestroyOffScreenWindow();
      }
    // create on screen
    if(!this->OnScreenInitialized)
      {
      this->CreateAWindow();
      this->OnScreenInitialized = 1;
      }
    }
  if(this->OnScreenInitialized)
    {
    // the error "invalid drawable" in the console from this call can appear
    // but only early in the app's lifetime (ie sometime during launch)
    // IMPORTANT: this is necessary to update the context here in case of
    // onscreen rendering
    NSOpenGLContext* context = (NSOpenGLContext*)this->GetContextId();
    [context setView:(NSView*)this->GetDisplayId()];

    [context update];
    }
}

//-----------------------------------------------------------------------------
void vtkCocoaRenderWindow::DestroyOffScreenWindow()
{
  if(this->OffScreenUseFrameBuffer)
    {
    this->DestroyHardwareOffScreenWindow();
    }
  else
    {
    // on screen
    this->DestroyWindow();
    }
}

//----------------------------------------------------------------------------
// Get the current size of the window.
int *vtkCocoaRenderWindow::GetSize()
{
  // if we aren't mapped then just return the ivar
  if (!this->Mapped)
    {
    return this->Superclass::GetSize();
    }

  // We want to return the size of 'the window'.  But the term 'window'
  // is overloaded. It's really the NSView that vtk draws into, so we
  // return its size.
  NSRect frameRect = [(NSView*)this->GetDisplayId() frame];
  this->Size[0] = (int)NSWidth(frameRect);
  this->Size[1] = (int)NSHeight(frameRect);
  return this->Superclass::GetSize();
}

//----------------------------------------------------------------------------
// Get the current size of the screen.
int *vtkCocoaRenderWindow::GetScreenSize()
{
  NSOpenGLContext* context = (NSOpenGLContext*)this->GetContextId();
  int currentScreen = [context currentVirtualScreen];

  NSScreen* screen = [[NSScreen screens] objectAtIndex: currentScreen];
  NSRect screenRect = [screen frame];
  this->Size[0] = (int)NSWidth(screenRect);
  this->Size[1] = (int)NSHeight(screenRect);
  return this->Size;
}

//----------------------------------------------------------------------------
// Get the position in screen coordinates of the window.
int *vtkCocoaRenderWindow::GetPosition()
{
  // if we aren't mapped then just return the ivar
  if (!this->Mapped)
    {
    return this->Position;
    }

  // We want to return the position of 'the window'.  But the term 'window'
  // is overloaded. In this case, it's the position of the NSWindow itself
  // on the screen that we return. We don't much care where the NSView is
  // within the NSWindow.
  NSRect winFrameRect = [(NSWindow*)this->GetWindowId() frame];
  this->Position[0] = (int)NSMinX(winFrameRect);
  this->Position[1] = (int)NSMinY(winFrameRect);
  return this->Position;
}

//----------------------------------------------------------------------------
// Change the window to fill the entire screen.
void vtkCocoaRenderWindow::SetFullScreen(int arg)
{
  int *pos;

  if (this->FullScreen == arg)
    {
    return;
    }

  if (!this->Mapped)
    {
    this->PrefFullScreen();
    return;
    }

  // set the mode
  this->FullScreen = arg;
  if (this->FullScreen <= 0)
    {
    this->Position[0] = this->OldScreen[0];
    this->Position[1] = this->OldScreen[1];
    this->Size[0] = this->OldScreen[2];
    this->Size[1] = this->OldScreen[3];
    this->Borders = this->OldScreen[4];
    }
  else
    {
    // if window already up get its values
    if (this->GetWindowId())
      {
      pos = this->GetPosition();
      this->OldScreen[0] = pos[0];
      this->OldScreen[1] = pos[1];

      this->OldScreen[4] = this->Borders;
      this->PrefFullScreen();
      }
    }

  // remap the window
  this->WindowRemap();

  this->Modified();
}

//----------------------------------------------------------------------------
//
// Set the variable that indicates that we want a stereo capable window
// be created. This method can only be called before a window is realized.
//
void vtkCocoaRenderWindow::SetStereoCapableWindow(int capable)
{
  if (this->GetContextId() == 0)
    {
    vtkRenderWindow::SetStereoCapableWindow(capable);
    }
  else
    {
    vtkWarningMacro(<< "Requesting a StereoCapableWindow must be performed "
                    << "before the window is realized, i.e. before a render.");
    }
}

//----------------------------------------------------------------------------
// Set the preferred window size to full screen.
void vtkCocoaRenderWindow::PrefFullScreen()
{
  int *size = this->GetScreenSize();
  vtkWarningMacro(<< "Can't get full screen window of size "
                  << size[0] << 'x' << size[1] << ".");
}

//----------------------------------------------------------------------------
// Remap the window.
void vtkCocoaRenderWindow::WindowRemap()
{
  vtkWarningMacro(<< "Can't remap the window.");
  // Aquire the display and capture the screen.
  // Create the full-screen window.
  // Add the context.
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ContextId: " << this->GetContextId() << "\n";
  os << indent << "MultiSamples: " << this->MultiSamples << "\n";
}

//----------------------------------------------------------------------------
int vtkCocoaRenderWindow::GetDepthBufferSize()
{
  GLint size;

  if ( this->Mapped )
    {
    size = 0;
    glGetIntegerv( GL_DEPTH_BITS, &size );
    return (int) size;
    }
  else
    {
    vtkDebugMacro(<< "Window is not mapped yet!" );
    return 24;
    }
}

//----------------------------------------------------------------------------
// Returns the NSWindow* associated with this vtkRenderWindow.
void *vtkCocoaRenderWindow::GetWindowId()
{
  return this->WindowId;
}

//----------------------------------------------------------------------------
// Sets the NSWindow* associated with this vtkRenderWindow.
void vtkCocoaRenderWindow::SetWindowId(void *arg)
{
  this->WindowId = arg;
}

//----------------------------------------------------------------------------
// Returns the NSView* associated with this vtkRenderWindow.
void *vtkCocoaRenderWindow::GetDisplayId()
{
  return this->DisplayId;
}

//----------------------------------------------------------------------------
// Sets the NSView* associated with this vtkRenderWindow.
void vtkCocoaRenderWindow::SetDisplayId(void *arg)
{
  this->DisplayId = arg;
}

//----------------------------------------------------------------------------
// Sets the NSOpenGLContext* associated with this vtkRenderWindow.
void vtkCocoaRenderWindow::SetContextId(void *contextId)
{
  this->ContextId = contextId;
}

//----------------------------------------------------------------------------
// Returns the NSOpenGLContext* associated with this vtkRenderWindow.
void *vtkCocoaRenderWindow::GetContextId()
{
  return this->ContextId;
}

//----------------------------------------------------------------------------
// Sets the NSOpenGLPixelFormat* associated with this vtkRenderWindow.
void vtkCocoaRenderWindow::SetPixelFormat(void *pixelFormat)
{
  this->PixelFormat = pixelFormat;
}
  
//----------------------------------------------------------------------------
// Returns the NSOpenGLPixelFormat* associated with this vtkRenderWindow.
void *vtkCocoaRenderWindow::GetPixelFormat()
{
  return this->PixelFormat;
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::HideCursor()
{
  if (this->CursorHidden)
    {
    return;
    }
  this->CursorHidden = 1;

  [NSCursor hide];
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindow::ShowCursor()
{
  if (!this->CursorHidden)
    {
    return;
    }
  this->CursorHidden = 0;

  [NSCursor unhide];
}

// ---------------------------------------------------------------------------
int vtkCocoaRenderWindow::GetWindowCreated()
{
  return this->WindowCreated;
}
