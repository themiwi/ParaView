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
#include "vtkSMProxyProperty.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyGroupDomain.h"
#include "vtkSMProxyManager.h"
#include "vtkSMStateLoader.h"
#include "vtkSmartPointer.h"

#include <vtkstd/algorithm>
#include <vtkstd/map>
#include <vtkstd/set>
#include <vtkstd/vector>

#include "vtkStdString.h"

vtkStandardNewMacro(vtkSMProxyProperty);
vtkCxxRevisionMacro(vtkSMProxyProperty, "$Revision$");

struct vtkSMProxyPropertyInternals
{
  typedef vtkstd::vector<vtkSmartPointer<vtkSMProxy> > VectorOfProxies;
  
  VectorOfProxies Proxies;
  VectorOfProxies PreviousProxies;
  vtkstd::vector<vtkSMProxy*> UncheckedProxies;
};

//---------------------------------------------------------------------------
vtkSMProxyProperty::vtkSMProxyProperty()
{
  this->PPInternals = new vtkSMProxyPropertyInternals;
  this->CleanCommand = 0;
  this->RepeatCommand = 0;
  this->RemoveCommand = 0;
  this->IsInternal = 0;
  this->AddConsumer = 1;
}

//---------------------------------------------------------------------------
vtkSMProxyProperty::~vtkSMProxyProperty()
{
  delete this->PPInternals;
  this->SetCleanCommand(0);
  this->SetRemoveCommand(0);
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::UpdateAllInputs()
{
  unsigned int numProxies = this->GetNumberOfProxies();
  for (unsigned int idx=0; idx < numProxies; idx++)
    {
    vtkSMProxy* proxy = this->GetProxy(idx);
    if (proxy)
      {
      proxy->UpdateSelfAndAllInputs();
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::AppendCommandToStreamWithRemoveCommand(
  vtkSMProxy* cons, vtkClientServerStream* str, vtkClientServerID objectId )
{
  if (!this->RemoveCommand || this->InformationOnly)
    {
    return;
    }

  vtkstd::set<vtkSmartPointer<vtkSMProxy> > prevProxies(
    this->PPInternals->PreviousProxies.begin(),
    this->PPInternals->PreviousProxies.end());
  vtkstd::set<vtkSmartPointer<vtkSMProxy> > curProxies(
    this->PPInternals->Proxies.begin(),
    this->PPInternals->Proxies.end());

  vtkstd::vector<vtkSmartPointer<vtkSMProxy> > proxiesToRemove;
  vtkstd::vector<vtkSmartPointer<vtkSMProxy> > proxiesToAdd;
 
  // Determine the proxies in the PreviousProxies but not in Proxies.
  // These are the proxies to remove.
  vtkstd::back_insert_iterator<
    vtkstd::vector<vtkSmartPointer<vtkSMProxy> > > ii_remove(proxiesToRemove);
  vtkstd::set_difference(prevProxies.begin(),
                         prevProxies.end(),
                         curProxies.begin(),
                         curProxies.end(),
                         ii_remove);
  
  // Determine the proxies in the Proxies but not in PreviousProxies.
  // These are the proxies to add.
  vtkstd::back_insert_iterator<
    vtkstd::vector<vtkSmartPointer<vtkSMProxy> > > ii_add(proxiesToAdd);
  vtkstd::set_difference(curProxies.begin(),
                         curProxies.end(),
                         prevProxies.begin(),
                         prevProxies.end(),
                         ii_add   );

  // Remove the proxies to remove.
  vtkstd::vector<vtkSmartPointer<vtkSMProxy> >::iterator iter1;
  for (iter1 = proxiesToRemove.begin(); iter1 != proxiesToRemove.end(); ++iter1)
    {
    vtkSMProxy* toAppend = iter1->GetPointer();
    this->AppendProxyToStream(toAppend, cons, str, objectId, 1);
    toAppend->RemoveConsumer(this, cons);
    }

  // Add the proxies to add.
  vtkstd::vector<vtkSmartPointer<vtkSMProxy> >::iterator iter;
  iter  = proxiesToAdd.begin();
  for ( ; iter != proxiesToAdd.end(); ++iter)
    {
    vtkSMProxy *toAppend = iter->GetPointer();
    // Keep track of all proxies that point to this as a
    // consumer so that we can remove this from the consumer
    // list later if necessary.
    if (this->AddConsumer)
      {
      toAppend->AddConsumer(this, cons);
      }
    this->AppendProxyToStream(toAppend, cons, str, objectId, 0);
    }
 
  // Set PreviousProxies to match the current Proxies.
  // (which is same as PreviousProxies - proxiesToRemove + proxiesToAdd).
  this->PPInternals->PreviousProxies.clear();
  this->PPInternals->PreviousProxies.insert(
    this->PPInternals->PreviousProxies.begin(),
    this->PPInternals->Proxies.begin(), this->PPInternals->Proxies.end());
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::AppendCommandToStream(
  vtkSMProxy* cons, vtkClientServerStream* str, vtkClientServerID objectId )
{
  if (!this->Command || this->InformationOnly)
    {
    return;
    }

  if (this->RemoveCommand)
    {
    this->AppendCommandToStreamWithRemoveCommand(
      cons, str, objectId);
    }
  else // no remove command.
    {
    if (this->CleanCommand)
      {
      *str << vtkClientServerStream::Invoke
        << objectId << this->CleanCommand
        << vtkClientServerStream::End;
      }

    unsigned int numProxies = this->GetNumberOfProxies();
    if (numProxies < 1)
      {
      return;
      }

    // Remove all consumers (using previous proxies)
    this->RemoveConsumerFromPreviousProxies(cons);
    // Remove all previous proxies before adding new ones.
    this->RemoveAllPreviousProxies();

    for (unsigned int idx=0; idx < numProxies; idx++)
      {
      vtkSMProxy* proxy = this->GetProxy(idx);
      // Keep track of all proxies that point to this as a
      // consumer so that we can remove this from the consumer
      // list later if necessary
      this->AddPreviousProxy(proxy);
      if (proxy && this->AddConsumer)
        {
        proxy->AddConsumer(this, cons);
        }
      this->AppendProxyToStream(proxy, cons, str, objectId, 0);
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::AppendProxyToStream(vtkSMProxy* toAppend,
  vtkSMProxy* cons, vtkClientServerStream* str, vtkClientServerID objectId,
  int remove /*=0*/)
{
  const char* command = (remove)? this->RemoveCommand : this->Command;
  if (!command)
    {
    vtkErrorMacro("Command not specified!"); //sanity check.
    return;
    }

  if (!toAppend)
    {
    vtkClientServerID nullID = { 0 };
    *str << vtkClientServerStream::Invoke << objectId << command 
      << nullID << vtkClientServerStream::End;
    return;
    }

  if (this->UpdateSelf)
    {
    *str << vtkClientServerStream::Invoke << objectId << command 
      << toAppend << vtkClientServerStream::End;
    return;
    }

  toAppend->CreateVTKObjects(1);

  unsigned int numConsIDs = cons->GetNumberOfIDs();
  unsigned int numIDs = toAppend->GetNumberOfIDs();
  // Determine now the IDs are added.
  if (numConsIDs == numIDs && !this->RepeatCommand)
    {
    // One to One Mapping between the IDs.
    for (unsigned int i = 0; i < numIDs; i++)
      {
      if (cons->GetID(i) == objectId)
        {
        // This check is essential since AppendCommandToStream is called
        // for all IDs in cons.
        *str << vtkClientServerStream::Invoke << objectId << command 
          << toAppend->GetID(i)
          << vtkClientServerStream::End;
        }
      }
    }
  else if (numConsIDs == 1 || this->RepeatCommand)
    {
    // One (or many) to Many Mapping.
    for (unsigned int i=0 ; i < numIDs; i++)
      {
      *str << vtkClientServerStream::Invoke << objectId << command
        << toAppend->GetID(i) << vtkClientServerStream::End;
      }
    }
  else if (numIDs == 1)
    {
    // Many to One Mapping.
    // No need to loop since AppendCommandToStream is called
    // for all IDs in cons.
    *str << vtkClientServerStream::Invoke << objectId << command 
      << toAppend->GetID(0)
      << vtkClientServerStream::End;
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::RemoveAllPreviousProxies()
{
  this->PPInternals->PreviousProxies.clear();
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::AddPreviousProxy(vtkSMProxy* proxy)
{
  this->PPInternals->PreviousProxies.push_back(proxy);
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::RemoveConsumerFromPreviousProxies(vtkSMProxy* cons)
{
  vtkSMProxyPropertyInternals::VectorOfProxies& prevProxies =
    this->PPInternals->PreviousProxies;

  vtkstd::vector<vtkSmartPointer<vtkSMProxy> >::iterator it =
    prevProxies.begin();
  for(; it != prevProxies.end(); it++)
    {
    if (it->GetPointer())
      {
      it->GetPointer()->RemoveConsumer(this, cons);
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::AddUncheckedProxy(vtkSMProxy* proxy)
{
  this->PPInternals->UncheckedProxies.push_back(proxy);
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::RemoveUncheckedProxy(vtkSMProxy* proxy)
{
  vtkstd::vector<vtkSMProxy* >::iterator it =
    this->PPInternals->UncheckedProxies.begin();
  for (; it != this->PPInternals->UncheckedProxies.end(); it++)
    {
    if (*it == proxy)
      {
      this->PPInternals->UncheckedProxies.erase(it);
      break;
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::SetUncheckedProxy(unsigned int idx, vtkSMProxy* proxy)
{
  if (this->PPInternals->UncheckedProxies.size() <= idx)
    {
    this->PPInternals->UncheckedProxies.resize(idx+1);
    }
  this->PPInternals->UncheckedProxies[idx] = proxy;
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::RemoveAllUncheckedProxies()
{
  this->PPInternals->UncheckedProxies.erase(
    this->PPInternals->UncheckedProxies.begin(),
    this->PPInternals->UncheckedProxies.end());
}

//---------------------------------------------------------------------------
int vtkSMProxyProperty::AddProxy(vtkSMProxy* proxy, int modify)
{
  if ( vtkSMProperty::GetCheckDomains() )
    {
    this->RemoveAllUncheckedProxies();
    this->AddUncheckedProxy(proxy);
    
    if (!this->IsInDomains())
      {
      this->RemoveAllUncheckedProxies();
      return 0;
      }
    }
  this->RemoveAllUncheckedProxies();

  this->PPInternals->Proxies.push_back(proxy);
  if (modify)
    {
    this->Modified();
    }
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::RemoveProxy(vtkSMProxy* proxy)
{
  this->RemoveProxy(proxy, 1);
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::RemoveProxy(vtkSMProxy* proxy, int modify)
{
  vtkstd::vector<vtkSmartPointer<vtkSMProxy> >::iterator iter =   
    this->PPInternals->Proxies.begin();
  for ( ; iter != this->PPInternals->Proxies.end() ; ++iter)
    {
    if (*iter == proxy)
      {
      this->PPInternals->Proxies.erase(iter);
      if (modify)
        {
        this->Modified();
        }
      break;
      }
    }
}

//---------------------------------------------------------------------------
int vtkSMProxyProperty::SetProxy(unsigned int idx, vtkSMProxy* proxy)
{
  if ( vtkSMProperty::GetCheckDomains() )
    {
    this->SetUncheckedProxy(idx, proxy);
    
    if (!this->IsInDomains())
      {
      this->RemoveAllUncheckedProxies();
      return 0;
      }
    }
  this->RemoveAllUncheckedProxies();

  this->PPInternals->Proxies[idx] = proxy;
  this->Modified();

  return 1;
}

//---------------------------------------------------------------------------
int vtkSMProxyProperty::AddProxy(vtkSMProxy* proxy)
{
  return this->AddProxy(proxy, 1);
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::RemoveAllProxies()
{
  this->PPInternals->Proxies.clear();
  this->Modified();
}

//---------------------------------------------------------------------------
unsigned int vtkSMProxyProperty::GetNumberOfProxies()
{
  return this->PPInternals->Proxies.size();
}

//---------------------------------------------------------------------------
unsigned int vtkSMProxyProperty::GetNumberOfUncheckedProxies()
{
  return this->PPInternals->UncheckedProxies.size();
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyProperty::GetProxy(unsigned int idx)
{
  return this->PPInternals->Proxies[idx].GetPointer();
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyProperty::GetUncheckedProxy(unsigned int idx)
{
  return this->PPInternals->UncheckedProxies[idx];
}

//---------------------------------------------------------------------------
int vtkSMProxyProperty::ReadXMLAttributes(vtkSMProxy* parent,
                                          vtkPVXMLElement* element)
{
  int ret = this->Superclass::ReadXMLAttributes(parent, element);
  
  const char* clean_command = element->GetAttribute("clean_command");
  if(clean_command) 
    { 
    this->SetCleanCommand(clean_command); 
    }

  int repeat_command;
  int retVal = element->GetScalarAttribute("repeat_command", &repeat_command);
  if(retVal) 
    { 
    this->SetRepeatCommand(repeat_command); 
    }

  const char* remove_command = element->GetAttribute("remove_command");
  if (remove_command)
    {
    this->SetRemoveCommand(remove_command);
    }

  int add_consumer;
  if (element->GetScalarAttribute("add_consumer", & add_consumer))
    {
    this->SetAddConsumer(add_consumer);
    }
  return ret;
}

//---------------------------------------------------------------------------
int vtkSMProxyProperty::LoadState(vtkPVXMLElement* element,
  vtkSMStateLoader* loader, int loadLastPushedValues/*=0*/)
{
  int prevImUpdate = this->ImmediateUpdate;

  // Wait until all values are set before update (if ImmediateUpdate)
  this->ImmediateUpdate = 0;
  this->Superclass::LoadState(element, loader, loadLastPushedValues);

  if (loadLastPushedValues)
    {
    element = element->FindNestedElementByName("LastPushedValues");
    if (!element)
      {
      vtkErrorMacro("Failed to locate LastPushedValues.");
      this->ImmediateUpdate = prevImUpdate;
      return 0;
      }
    }

  vtkSMProxyPropertyInternals::VectorOfProxies newValue;
  unsigned int numElems = element->GetNumberOfNestedElements();
  for (unsigned int i=0; i<numElems; i++)
    {
    vtkPVXMLElement* currentElement = element->GetNestedElement(i);
    if (currentElement->GetName() &&
        (strcmp(currentElement->GetName(), "Element") == 0 ||
         strcmp(currentElement->GetName(), "Proxy") == 0) )
      {
      int id;
      if (currentElement->GetScalarAttribute("value", &id))
        {
        if (id)
          {
          vtkSMProxy* proxy = loader->NewProxy(id);
          if (proxy)
            {
            newValue.push_back(proxy);
            proxy->Delete();
            }
          else
            {
            // It is not an error to have missing proxies in a proxy property.
            // We simply ignore such proxies.
            //vtkErrorMacro("Could not create proxy of id: " << id);
            //return 0;
            }
          }
        else
          {
          newValue.push_back(NULL);
          }
        }
      }
    }

  this->PPInternals->Proxies = newValue;
  // Do not immediately update. Leave it to the loader.
  this->Modified();
  this->ImmediateUpdate = prevImUpdate;
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::ChildSaveState(vtkPVXMLElement* propertyElement,
  int saveLastPushedValues)
{
  this->Superclass::ChildSaveState(propertyElement, saveLastPushedValues);

  unsigned int numProxies = this->GetNumberOfProxies();
  propertyElement->AddAttribute("number_of_elements", numProxies);
  for (unsigned int idx=0; idx<numProxies; idx++)
    {
    vtkSMProxy* proxy = this->GetProxy(idx);
    if (proxy)
      {
      vtkPVXMLElement* proxyElement = vtkPVXMLElement::New();
      proxyElement->SetName("Proxy");
      proxyElement->AddAttribute("value", proxy->GetSelfIDAsString());
      propertyElement->AddNestedElement(proxyElement);
      proxyElement->Delete();
      }
    }

  if (saveLastPushedValues)
    {
    numProxies = this->PPInternals->PreviousProxies.size();
    vtkPVXMLElement* element = vtkPVXMLElement::New();
    element->SetName("LastPushedValues");
    element->AddAttribute("number_of_elements", numProxies);
    for (unsigned int cc=0; cc < numProxies; cc++)
      {
      vtkSMProxy* proxy = this->PPInternals->PreviousProxies[cc];
      if (proxy)
        {
        vtkPVXMLElement* proxyElement = vtkPVXMLElement::New();
        proxyElement->SetName("Proxy");
        proxyElement->AddAttribute("value", proxy->GetSelfIDAsString());
        element->AddNestedElement(proxyElement);
        proxyElement->Delete();
        }
      }
    propertyElement->AddNestedElement(element);
    element->Delete();
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::DeepCopy(vtkSMProperty* src, 
  const char* exceptionClass, int proxyPropertyCopyFlag)
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMProxyProperty* dsrc = vtkSMProxyProperty::SafeDownCast(
    src);

  this->RemoveAllProxies();
  this->RemoveAllUncheckedProxies();
  
  if (dsrc)
    {
    int imUpdate = this->ImmediateUpdate;
    this->ImmediateUpdate = 0;
    unsigned int i;
    unsigned int numElems = dsrc->GetNumberOfProxies();

    for(i=0; i<numElems; i++)
      {
      vtkSMProxy* psrc = dsrc->GetProxy(i);
      vtkSMProxy* pdest = pxm->NewProxy(psrc->GetXMLGroup(), 
        psrc->GetXMLName());
      pdest->SetConnectionID(psrc->GetConnectionID());
      pdest->Copy(psrc, exceptionClass, proxyPropertyCopyFlag);
      this->AddProxy(pdest);
      pdest->Delete();
      }
    
    numElems = dsrc->GetNumberOfUncheckedProxies();
    for(i=0; i<numElems; i++)
      {
      vtkSMProxy* psrc = dsrc->GetUncheckedProxy(i);
      vtkSMProxy* pdest = pxm->NewProxy(psrc->GetXMLGroup(), 
        psrc->GetXMLName());
      pdest->SetConnectionID(psrc->GetConnectionID());
      pdest->Copy(psrc, exceptionClass, proxyPropertyCopyFlag);
      this->AddUncheckedProxy(pdest);
      pdest->Delete();
      }
    this->ImmediateUpdate = imUpdate;
    }
  if (this->ImmediateUpdate)
    {
    this->Modified();
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::Copy(vtkSMProperty* src)
{
  this->Superclass::Copy(src);

  this->RemoveAllProxies();
  this->RemoveAllUncheckedProxies();

  vtkSMProxyProperty* dsrc = vtkSMProxyProperty::SafeDownCast(
    src);
  if (dsrc)
    {
    int imUpdate = this->ImmediateUpdate;
    this->ImmediateUpdate = 0;
    unsigned int i;
    unsigned int numElems = dsrc->GetNumberOfProxies();
    for(i=0; i<numElems; i++)
      {
      this->AddProxy(dsrc->GetProxy(i));
      }
    numElems = dsrc->GetNumberOfUncheckedProxies();
    for(i=0; i<numElems; i++)
      {
      this->AddUncheckedProxy(dsrc->GetUncheckedProxy(i));
      }
    this->ImmediateUpdate = imUpdate;
    }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Values: ";
  for (unsigned int i=0; i<this->GetNumberOfProxies(); i++)
    {
    os << this->GetProxy(i) << " ";
    }
  os << endl;
  os << indent 
     << "CleanCommand: "
     << (this->CleanCommand ? this->CleanCommand : "(none)") 
     << endl;
}
