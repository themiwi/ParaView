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
#include "vtkIndent.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkIndent* vtkIndent::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkIndent");
  if(ret)
    {
    return (vtkIndent*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkIndent;
}



#define VTK_STD_INDENT 2
#define VTK_NUMBER_OF_BLANKS 40

static const char blanks[VTK_NUMBER_OF_BLANKS+1]="                                        ";

// Determine the next indentation level. Keep indenting by two until the 
// max of forty.
vtkIndent vtkIndent::GetNextIndent()
{
  int indent = this->Indent + VTK_STD_INDENT;
  if ( indent > VTK_NUMBER_OF_BLANKS )
    {
    indent = VTK_NUMBER_OF_BLANKS;
    }
  return indent;
}
 
// Print out the indentation. Basically output a bunch of spaces.
ostream& operator<<(ostream& os, vtkIndent& ind)
{
  os << blanks + (VTK_NUMBER_OF_BLANKS-ind.Indent) ;
  return os;
}

int vtkIndent::IsA(const char *type)
{
  if ( !strcmp(this->vtkIndent::GetClassName(),type) )
    {
    return 1;
    }
  return 0;
}

vtkIndent *vtkIndent::SafeDownCast(vtkIndent *o)
{
  return (vtkIndent *)o;
}
