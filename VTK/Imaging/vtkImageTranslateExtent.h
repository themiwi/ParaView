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
// .NAME vtkImageTranslateExtent - Changes extent, nothing else.
// .SECTION Description
// vtkImageTranslateExtent  shift the whole extent, but does not
// change the data.

#ifndef __vtkImageTranslateExtent_h
#define __vtkImageTranslateExtent_h

#include "vtkImageAlgorithm.h"

class VTK_IMAGING_EXPORT vtkImageTranslateExtent : public vtkImageAlgorithm
{
public:
  static vtkImageTranslateExtent *New();
  vtkTypeRevisionMacro(vtkImageTranslateExtent,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Delta to change "WholeExtent". -1 changes 0->10 to -1->9.
  vtkSetVector3Macro(Translation, int);
  vtkGetVector3Macro(Translation, int);

protected:
  vtkImageTranslateExtent();
  ~vtkImageTranslateExtent() {};

  int Translation[3];
  
  void RequestUpdateExtent (vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  void RequestInformation (vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  
  void RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  vtkImageTranslateExtent(const vtkImageTranslateExtent&);  // Not implemented.
  void operator=(const vtkImageTranslateExtent&);  // Not implemented.
};

#endif
