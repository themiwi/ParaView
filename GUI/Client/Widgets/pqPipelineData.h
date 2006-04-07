/*=========================================================================

   Program:   ParaQ
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#ifndef _pqPipelineData_h
#define _pqPipelineData_h

#include "pqWidgetsExport.h"
#include <QObject>

class pqMultiView;
class pqNameCount;
class pqPipelineDataInternal;
class pqPipelineObject;
class pqPipelineServer;
class pqPipelineWindow;
class pqServer;
class QVTKWidget;
class vtkObject;
class vtkCommand;
class vtkEventQtSlotConnect;
class vtkPVXMLElement;
class vtkSMProxy;
class vtkSMRenderModuleProxy;
class vtkSMSourceProxy;
class vtkSMCompoundProxy;
class vtkSMDisplayProxy;


/// interface for querying pipline state, also provides signals for pipeline changes
class PQWIDGETS_EXPORT pqPipelineData : public QObject
{
  Q_OBJECT

public:
  pqPipelineData(QObject* parent=0);
  virtual ~pqPipelineData();

  static pqPipelineData *instance();

  pqNameCount *getNameCount() const {return this->Names;}

  void saveState(vtkPVXMLElement *root, pqMultiView *multiView=0);
  void loadState(vtkPVXMLElement *root, pqMultiView *multiView=0);

  void clearPipeline();

  void addServer(pqServer *server);
  void removeServer(pqServer *server);

  int getServerCount() const;
  pqPipelineServer *getServer(int index) const;
  pqPipelineServer *getServerFor(pqServer *server);

  void addWindow(QVTKWidget *window, pqServer *server);
  void removeWindow(QVTKWidget *window);

  void addViewMapping(QVTKWidget *widget, vtkSMRenderModuleProxy *module);
  vtkSMRenderModuleProxy *removeViewMapping(QVTKWidget *widget);
  vtkSMRenderModuleProxy *getRenderModule(QVTKWidget *widget) const;
  void clearViewMapping();

  // TODO: perhaps not tie source proxies to windows

  vtkSMSourceProxy *createSource(const char *proxyName, QVTKWidget *window);
  vtkSMSourceProxy *createFilter(const char *proxyName, QVTKWidget *window);
  vtkSMCompoundProxy *createCompoundProxy(const char *proxyName, QVTKWidget *window);

  void deleteProxy(vtkSMProxy *proxy);

  // TODO: should take a window to create a display proxy in, but source proxy is already tied to window
  //! create a display proxy for a source proxy
  vtkSMDisplayProxy* createDisplay(vtkSMSourceProxy* source, vtkSMProxy* rep = NULL);
  //! set the visibility on/off for a display
  void setVisibility(vtkSMDisplayProxy* proxy, bool on);

  void addInput(vtkSMProxy *proxy, vtkSMProxy *input);
  void removeInput(vtkSMProxy *proxy, vtkSMProxy *input);

  QVTKWidget *getWindowFor(vtkSMProxy *proxy) const;
  pqPipelineObject *getObjectFor(vtkSMProxy *proxy) const;
  pqPipelineWindow *getObjectFor(QVTKWidget *window) const;

signals:
  void clearingPipeline();

  void serverAdded(pqPipelineServer *server);
  void removingServer(pqPipelineServer *server);

  void windowAdded(pqPipelineWindow *window);
  void removingWindow(pqPipelineWindow *window);

  void sourceCreated(pqPipelineObject *source);
  void filterCreated(pqPipelineObject *filter);
  void removingObject(pqPipelineObject *object);

  void connectionCreated(pqPipelineObject *source, pqPipelineObject *sink);
  void removingConnection(pqPipelineObject *source, pqPipelineObject *sink);

  /// signal for new proxy created
  void newPipelineObject(vtkSMProxy* proxy);
  /// signal for new input connection to proxy
  void newPipelineObjectInput(vtkSMProxy* proxy, vtkSMProxy* input);

  /// new current proxy
  void currentProxyChanged(vtkSMProxy* newproxy);

public slots:
  /// set the current proxy
  void setCurrentProxy(vtkSMProxy* proxy);

public:
  /// get the current proxy
  vtkSMProxy* currentProxy() const;

private slots:
  void proxyRegistered(vtkObject* object, unsigned long e, void* clientData,
      void* callData, vtkCommand* command);
  void inputChanged(vtkObject* object, unsigned long e, void* clientData,
      void* callData, vtkCommand* command);

protected:
  pqPipelineDataInternal *Internal;  ///< Stores the pipeline objects.
  pqNameCount *Names;                ///< Keeps track of the proxy names.
  vtkEventQtSlotConnect *VTKConnect; ///< Used to listen to proxy events.
  vtkSMProxy* CurrentProxy;

  static pqPipelineData *Instance;   ///< Pointer to the pipeline instance.
};

#endif // _pqPipelineData_h

