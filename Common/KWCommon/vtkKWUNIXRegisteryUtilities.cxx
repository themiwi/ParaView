/*=========================================================================

  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWUNIXRegisteryUtilities.h"

#include "vtkObjectFactory.h"
#include "vtkString.h"
#include "vtkArrayMap.txx"
#include "vtkArrayMapIterator.txx"

#ifdef VTK_USE_ANSI_STDLIB
#define VTK_IOS_NOCREATE 
#else
#define VTK_IOS_NOCREATE | ios::nocreate
#endif

#define BUFFER_SIZE 8192

vtkStandardNewMacro( vtkKWUNIXRegisteryUtilities );
vtkCxxRevisionMacro(vtkKWUNIXRegisteryUtilities, "$Revision$");

//----------------------------------------------------------------------------
vtkKWUNIXRegisteryUtilities::vtkKWUNIXRegisteryUtilities()
{
  this->EntriesMap = 0;
  this->SubKey  = 0;
}

//----------------------------------------------------------------------------
vtkKWUNIXRegisteryUtilities::~vtkKWUNIXRegisteryUtilities()
{
  if ( this->EntriesMap )
    {
    this->EntriesMap->Delete();
    }
}


//----------------------------------------------------------------------------
int vtkKWUNIXRegisteryUtilities::OpenInternal(const char *toplevel,
                                              const char *subkey, 
                                              int readonly)
{  
  int res = 0;
  int cc;
  ostrstream str;
  if ( !getenv("HOME") )
    {
    return 0;
    }
  str << getenv("HOME") << "/." << toplevel << "rc" << ends;
  if ( readonly == vtkKWRegisteryUtilities::READWRITE )
    {
    ofstream ofs( str.str(), ios::out|ios::app );
    if ( ofs.fail() )
      {
      return 0;
      }
    ofs.close();
    }

  ifstream *ifs = new ifstream(str.str(), ios::in VTK_IOS_NOCREATE);
  if ( !ifs )
    {
    return 0;
    }
  if ( ifs->fail())
    {
    delete ifs;
    return 0;
    }
  if ( !this->EntriesMap )
    {
    this->EntriesMap = vtkKWUNIXRegisteryUtilities::StringStringMap::New();
    }

  res = 1;
  char buffer[BUFFER_SIZE];
  while( !ifs->fail() )
    {
    int found = 0;
    ifs->getline(buffer, BUFFER_SIZE);
    if ( ifs->fail() || ifs->eof() )
      {
      break;
      }
    char *line = this->Strip(buffer);
    if ( *line == '#'  || *line == 0 )
      {
      // Comment
      continue;
      }   
    for ( cc = 0; cc< static_cast<int>(strlen(line)); cc++ )
      {
      if ( line[cc] == '=' )
        {
        char *key = new char[ cc+1 ];
        strncpy( key, line, cc );
        key[cc] = 0;
        char *value = line + cc + 1;
        char *nkey = this->Strip(key);
        char *nvalue = this->Strip(value);
        this->EntriesMap->SetItem( nkey, nvalue );
        this->Empty = 0;
        delete [] key;
        found = 1;      
        break;
        }
      }
    }
  ifs->close();
  this->SetSubKey( subkey );
  delete ifs;
  return res;
}

//----------------------------------------------------------------------------
int vtkKWUNIXRegisteryUtilities::CloseInternal()
{
  int res = 0;
  if ( !this->Changed )
    {
    this->EntriesMap->Delete();
    this->EntriesMap = 0;
    this->Empty = 1;
    this->SetSubKey(0);
    return 1;
    }

  ostrstream str;
  if ( !getenv("HOME") )
    {
    return 0;
    }
  str << getenv("HOME") << "/." << this->GetTopLevel() << "rc" << ends;
  ofstream *ofs = new ofstream(str.str(), ios::out);
  if ( !ofs )
    {
    return 0;
    }
  if ( ofs->fail())
    {
    delete ofs;
    return 0;
    }
  *ofs << "# This file is automatically generated by the application" << endl
       << "# If you change any lines or add new lines, note that all" << endl
       << "# coments and empty lines will be deleted. Every line has" << endl
       << "# to be in format: " << endl
       << "# key = value" << endl
       << "#" << endl;

  if ( this->EntriesMap )
    {
    vtkKWUNIXRegisteryUtilities::StringStringMap::IteratorType *it
      = this->EntriesMap->NewIterator();
    while ( !it->IsDoneWithTraversal() )
      {
      const char *key = 0;
      const char *value = 0;
      it->GetKey(key);
      it->GetData(value);
      *ofs << key << " = " << value << endl;
      it->GoToNextItem();
      }
    it->Delete();
    }
  this->EntriesMap->Delete();
  this->EntriesMap = 0;
  ofs->close();
  delete ofs;
  res = 1;
  this->SetSubKey(0);
  this->Empty = 1;
  return res;
}

//----------------------------------------------------------------------------
int vtkKWUNIXRegisteryUtilities::ReadValueInternal(const char *skey,
                                                   char *value)

{
  int res = 0;
  char *key = this->CreateKey( skey );
  if ( !key )
    {
    return 0;
    }
  const char* val = 0;
  if ( this->EntriesMap->GetItem(key, val) == VTK_OK )
    {
    strcpy(value, val);
    res = 1;
    }
  delete [] key;
  return res;
}

//----------------------------------------------------------------------------
int vtkKWUNIXRegisteryUtilities::DeleteKeyInternal(const char* vtkNotUsed(key))
{
  int res = 0;
  return res;
}

//----------------------------------------------------------------------------
int vtkKWUNIXRegisteryUtilities::DeleteValueInternal(const char *skey)
{
  char *key = this->CreateKey( skey );
  if ( !key )
    {
    return 0;
    }
  this->EntriesMap->RemoveItem(key);
  delete [] key;
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWUNIXRegisteryUtilities::SetValueInternal(const char *skey, 
                                                  const char *value)
{
  char *key = this->CreateKey( skey );
  if ( !key )
    {
    return 0;
    }
  this->EntriesMap->SetItem(key, value);
  delete [] key;
  return 1;
}

//----------------------------------------------------------------------------
char *vtkKWUNIXRegisteryUtilities::CreateKey( const char *key )
{
  char *newkey;
  if ( !this->SubKey || !key )
    {
    return 0;
    }
  int len = strlen(this->SubKey) + strlen(key) + 1;
  newkey = new char[ len+1 ] ;
  sprintf(newkey, "%s\\%s", this->SubKey, key);
  return newkey;
}

//----------------------------------------------------------------------------
void vtkKWUNIXRegisteryUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}



