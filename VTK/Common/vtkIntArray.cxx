/*=========================================================================

  Program:   Visualization Library
  Module:    $RCSfile$
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
//  Dynamic, self adjusting integer array
//
//
#include "IntArray.hh"

vlIntArray::Allocate(const int sz, const int ext)
{
  if ( this->Array != NULL ) delete [] this->Array;

  this->Size = ( sz > 0 ? sz : 1);
  if ( (this->Array = new int[sz]) == NULL ) return 0;
  this->Extend = ( ext > 0 ? ext : 1);
  this->MaxId = -1;

  return 1;
}

void vlIntArray::Initialize()
{
  if ( this->Array != NULL )
    {
    delete [] this->Array;
    this->Array = NULL;
    }
  this->Size = 0;
  this->MaxId = -1;
}

vlIntArray::vlIntArray(const int sz, const int ext)
{
  this->Size = ( sz > 0 ? sz : 1);
  this->Array = new int[sz];
  this->Extend = ( ext > 0 ? ext : 1);
  this->MaxId = -1;
}

vlIntArray::~vlIntArray()
{
  delete [] this->Array;
}

vlIntArray::vlIntArray(const vlIntArray& ia)
{
  int i;

  this->MaxId = ia.MaxId;
  this->Size = ia.Size;
  this->Extend = ia.Extend;

  this->Array = new int[this->Size];
  for (i=0; i<this->MaxId; i++)
    this->Array[i] = ia.Array[i];

}

vlIntArray& vlIntArray::operator=(const vlIntArray& ia)
{
  int i;

  if ( this != &ia )
    {
    delete [] this->Array;

    this->MaxId = ia.MaxId;
    this->Size = ia.Size;
    this->Extend = ia.Extend;

    this->Array = new int[this->Size];
    for (i=0; i<=this->MaxId; i++)
      this->Array[i] = ia.Array[i];
    }
  return *this;
}

//
// Copy on write if used by more than one object
//
void vlIntArray::operator+=(const vlIntArray& ia)
{
  int i, sz;

  if ( this->Size <= (sz = this->MaxId + ia.MaxId + 2) ) this->Resize(sz);

  for (i=0; i<=ia.MaxId; i++)
    {
    this->Array[this->MaxId+1+i] = ia.Array[i];
    }
  this->MaxId += ia.MaxId + 1;

}

void vlIntArray::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlIntArray::GetClassName()))
    {
    vlObject::PrintSelf(os,indent);

    os << indent << "Array: " << this->Array << "\n";
    os << indent << "Size: " << this->Size << "\n";
    os << indent << "MaxId: " << this->MaxId << "\n";
    os << indent << "Extend size: " << this->Extend << "\n";
    }
}

//
// Private function does "reallocate"
//
int *vlIntArray::Resize(const int sz)
{
  int i;
  int *newArray;
  int newSize;

  if ( sz >= this->Size ) newSize = this->Size + 
    this->Extend*(((sz-this->Size)/this->Extend)+1);
  else newSize = sz;

  if ( (newArray = new int[newSize]) == NULL )
    {
    vlErrorMacro(<< "Cannot allocate memory\n");
    return 0;
    }

  for (i=0; i<sz && i<this->Size; i++)
      newArray[i] = this->Array[i];

  this->Size = newSize;
  delete [] this->Array;
  this->Array = newArray;

  return this->Array;
}
