/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkXRenderWindowTclInteractor - a TCL event driven interface for a RenderWindow
// .SECTION Description
// vtkXRenderWindowTclInteractor is a convenience object that provides event
// bindings to common graphics functions. For example, camera and actor
// functions such as zoom-in/zoom-out, azimuth, roll, and pan. IT is one of
// the window system specific subclasses of vtkRenderWindowInteractor. Please
// see vtkRenderWindowInteractor documentation for event bindings.
//
// .SECTION see also
// vtkRenderWindowInteractor vtkXRenderWindow


#ifndef __vtkXRenderWindowTclInteractor_h
#define __vtkXRenderWindowTclInteractor_h

//===========================================================
// now we define the C++ class

#include "vtkRenderWindowInteractor.h"
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>

class VTK_EXPORT vtkXRenderWindowTclInteractor : public vtkRenderWindowInteractor
{
public:
  vtkXRenderWindowTclInteractor();
  ~vtkXRenderWindowTclInteractor();
  static vtkXRenderWindowTclInteractor *New() {
    return new vtkXRenderWindowTclInteractor;};
  const char *GetClassName() {return "vtkXRenderWindowTclInteractor";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initializes the event handlers without an XtAppContext.  This is
  // good for when you don't have a user interface, but you still
  // want to have mouse interaction.
  virtual void Initialize();

  // Description: 
  // Initializes the event handlers using an XtAppContext that you have
  // provided.  This assumes that you want to own the event loop.
  virtual void Initialize(XtAppContext app);
  vtkGetMacro( App, XtAppContext );
  
  // Description:
  // Enable/Disable interactions.  By default interactors are enabled when
  // initialized.  Initialize() must be called prior to enabling/disabling
  // interaction. These methods are used when a window/widget is being
  // shared by multiple renderers and interactors.  This allows a "modal"
  // display where one interactor is active when its data is to be displayed
  // and all other interactors associated with the widget are disabled
  // when their data is not displayed.
  virtual void Enable();
  virtual void Disable();

  // Description:
  // This will start up the X event loop and never return. If you
  // call this method it will loop processing X events until the
  // application is exited.
  virtual void Start();
  virtual void UpdateSize(int,int);

  // Description:
  // Provide implementaitons of the methods defined in
  // vtkRenderWindowInteractor. Generally the application developer should
  // not invoke these methods directly.
  virtual void StartRotate();
  virtual void EndRotate();
  virtual void StartZoom();
  virtual void EndZoom();
  virtual void StartPan();
  virtual void EndPan();
  virtual void StartSpin();
  virtual void EndSpin();
  virtual void StartDolly();
  virtual void EndDolly();
  virtual void StartUniformScale();
  virtual void EndUniformScale();

  // Description:
  // Specify the Xt widget to use for interaction. This method is
  // one of a couple steps that are required for setting up a
  // vtkRenderWindowInteractor as a widget inside of another user 
  // interface. You do not need to use this method if the render window
  // will be a stand-alone window. This is only used when you want the
  // render window to be a subwindow within a larger user interface.
  // In that case, you must tell the render window what X display id
  // to use, and then ask the render window what depth, visual and 
  // colormap it wants. Then, you must create an Xt TopLevelShell with
  // those settings. Then you can create the rest of your user interface
  // as a child of the TopLevelShell you created. Eventually, you will 
  // create a drawing area or some other widget to serve as the rendering
  // window. You must use the SetWidget method to tell this Interactor
  // about that widget. It's X and it's not terribly easy, but it looks cool.
  virtual void SetWidget(Widget);
  Widget GetWidget() {return this->top;};
  
  // Description:
  // Functions that are used internally.
  friend void vtkXRenderWindowTclInteractorCallback(Widget,XtPointer,
                                                 XEvent *,Boolean *);
  friend void vtkXRenderWindowTclInteractorTimer(XtPointer,XtIntervalId *);

protected:
  Display *DisplayId;
  Window WindowId;
  Widget top;
  Widget oldTop;
  XtAppContext App;
  int PositionBeforeStereo[2];
};

#endif
