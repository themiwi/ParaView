/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMClientDeliveryRepresentationProxy.h"

#include "vtkAlgorithm.h"
#include "vtkClientServerStream.h"
#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVDataInformation.h"
#include "vtkSMClientDeliveryStrategyProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMProxyManager.h"

vtkStandardNewMacro(vtkSMClientDeliveryRepresentationProxy);
vtkCxxRevisionMacro(vtkSMClientDeliveryRepresentationProxy, "$Revision$");
//----------------------------------------------------------------------------
vtkSMClientDeliveryRepresentationProxy::vtkSMClientDeliveryRepresentationProxy()
{
  this->ReduceProxy = 0;
  this->StrategyProxy = 0;
  this->PostProcessorProxy = 0;

  this->ReductionType = 0;
  this->ExtractSelection = 0;

  this->SetSelectionSupported(false);
}

//----------------------------------------------------------------------------
vtkSMClientDeliveryRepresentationProxy::~vtkSMClientDeliveryRepresentationProxy()
{
  if (this->StrategyProxy)
    {
    this->StrategyProxy->Delete();
    }
  this->StrategyProxy = 0;
  this->PostProcessorProxy = 0;

  this->ExtractSelection = 0;
}

//----------------------------------------------------------------------------
bool vtkSMClientDeliveryRepresentationProxy::SetupStrategy()
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

  this->StrategyProxy = vtkSMClientDeliveryStrategyProxy::SafeDownCast(
    pxm->NewProxy("strategies", "ClientDeliveryStrategy"));
  if (!this->StrategyProxy)
    {
    vtkErrorMacro("Failed to create vtkSMClientDeliveryStrategyProxy.");
    return false;
    }

  this->StrategyProxy->SetConnectionID(this->ConnectionID);
  this->StrategyProxy->SetServers(
    vtkProcessModule::DATA_SERVER|vtkProcessModule::CLIENT);

  this->AddStrategy(this->StrategyProxy);

  this->StrategyProxy->SetEnableLOD(false);

  // Creates the strategy objects.
  this->StrategyProxy->UpdateVTKObjects();

  // Now initialize the data pipelines involving this strategy.
  if(this->ReduceProxy)
    {
    this->Connect(this->ReduceProxy, this->StrategyProxy);
    }
  else
    {
    this->Connect(this->GetInputProxy(), this->StrategyProxy, 
      "Input", this->OutputPort);
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMClientDeliveryRepresentationProxy::BeginCreateVTKObjects()
{
  if (!this->Superclass::BeginCreateVTKObjects())
    {
    return false;
    }

  this->ReduceProxy =  this->GetSubProxy("Reduce");
  if (this->ReduceProxy)
    {
    this->ReduceProxy->SetServers(this->Servers);
    }

  this->PostProcessorProxy = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("PostProcessor"));
  if (this->PostProcessorProxy)
    {
    this->PostProcessorProxy->SetServers(vtkProcessModule::CLIENT);
    }

  // Initialize selection pipeline subproxies.
  this->ExtractSelection = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("ExtractSelection"));

  this->ExtractSelection->SetServers(vtkProcessModule::DATA_SERVER);

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMClientDeliveryRepresentationProxy::EndCreateVTKObjects()
{
  // Setup selection pipeline connections.
  vtkSMSourceProxy* input = this->GetInputProxy();

  this->Connect(input, this->ExtractSelection,
    "Input", this->OutputPort);

  vtkClientServerStream stream;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  if (this->ReduceProxy)
    {
    this->Connect(input, this->ReduceProxy, "Input", this->OutputPort);

    stream
      << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetController"
      << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << this->ReduceProxy->GetID() << "SetController"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
      pm->SendStream(this->ConnectionID, 
        this->ReduceProxy->GetServers(), stream);
    }

  this->SetupStrategy();

  if (this->PostProcessorProxy)
    {
    if(this->StrategyProxy && this->StrategyProxy->GetOutput())
      {
      this->Connect(this->StrategyProxy->GetOutput(), this->PostProcessorProxy);
      }
    else if(this->ReduceProxy)
      {
      this->Connect(this->StrategyProxy->GetOutput(), this->ReduceProxy);
      }
    else
      {
      this->Connect(this->StrategyProxy->GetOutput(), this->ReduceProxy,
        "Input", this->OutputPort);
      }

    this->PostProcessorProxy->UpdateVTKObjects();
    }

  return this->Superclass::EndCreateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMClientDeliveryRepresentationProxy::SetReductionType(int type)
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Cannot set reduction type before CreateVTKObjects().");
    return;
    }

  if(this->ReductionType == type)
    {
    return;
    }

  this->ReductionType = type;

  if (!this->ReduceProxy)
    {
    return;
    }

  vtkClientServerStream stream;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  const char* classname = 0;
  switch (type)
    {
  case ADD:
    classname = "vtkAttributeDataReductionFilter";
    break;

  case POLYDATA_APPEND:
    classname = "vtkAppendPolyData";
    break;

  case UNSTRUCTURED_APPEND:
    classname = "vtkAppendFilter";
    break;

  case FIRST_NODE_ONLY:
    classname = 0;
    break;

  case RECTILINEAR_GRID_APPEND:
    classname = "vtkAppendRectilinearGrid";
    break;

  case COMPOSITE_DATASET_APPEND:
    classname = "vtkMultiBlockMergeFilter"; 
    break;

  default:
    vtkErrorMacro("Unknown reduction type: " << type);
    return;
    }

  vtkClientServerID rfid;
  if ( classname )
    {
    rfid = pm->NewStreamObject(classname, stream);
    }
  stream
    << vtkClientServerStream::Invoke
    << this->ReduceProxy->GetID() << "SetPostGatherHelper"
    << rfid
    << vtkClientServerStream::End;

  if ( classname )
    {
    pm->DeleteStreamObject(rfid, stream);
    }

  if (type == FIRST_NODE_ONLY && this->ReduceProxy)
    {
    // We re-arrange the pipeline to remove the ReduceProxy
    // from the pipeline.
    }

  pm->SendStream(this->GetConnectionID(),
    this->ReduceProxy->GetServers(), stream);
}

//-----------------------------------------------------------------------------
vtkDataObject* vtkSMClientDeliveryRepresentationProxy::GetOutput()
{
  vtkAlgorithm* dp;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (this->PostProcessorProxy)
    {
    dp = vtkAlgorithm::SafeDownCast(
      pm->GetObjectFromID(this->PostProcessorProxy->GetID())); 
    }
  else
    {
    if (pm && this->StrategyProxy && this->StrategyProxy->GetOutput())
      {
      dp = vtkAlgorithm::SafeDownCast(
        pm->GetObjectFromID(this->StrategyProxy->GetOutput()->GetID()));
      }
    else
      {
      dp = NULL;
      }
    }

  if (dp == NULL)
    {
    return NULL;
    }

  return dp->GetOutputDataObject(0);
}

//----------------------------------------------------------------------------
void vtkSMClientDeliveryRepresentationProxy::Update(vtkSMViewProxy* view)
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Objects not created yet!");
    return;
    }

  this->Superclass::Update(view);

  if (this->PostProcessorProxy)
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkAlgorithm* dp = vtkAlgorithm::SafeDownCast(
      pm->GetObjectFromID(this->PostProcessorProxy->GetID())); 
    if (!dp)
      {
      vtkErrorMacro("Failed to get algorithm for PostProcessorProxy.");
      }
    else
      {
      dp->Update();
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMClientDeliveryRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


