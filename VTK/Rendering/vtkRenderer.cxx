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
#include <stdlib.h>
#include <string.h>
#include "Renderer.hh"
#include "RenderW.hh"

vlRenderer::vlRenderer()
{
  this->ActiveCamera = NULL;

  this->Ambient[0] = 1;
  this->Ambient[1] = 1;
  this->Ambient[2] = 1;

  this->Background[0] = 0;
  this->Background[1] = 0;
  this->Background[2] = 0;

  this->DisplayPoint[0] = 0;
  this->DisplayPoint[1] = 0;
  this->DisplayPoint[2] = 0;

  this->ViewPoint[0] = 0;
  this->ViewPoint[1] = 0;
  this->ViewPoint[2] = 0;

  this->Viewport[0] = 0;
  this->Viewport[1] = 0;
  this->Viewport[2] = 1;
  this->Viewport[3] = 1;

  this->BackLight = 1;
  this->Erase = 1;
  this->StereoRender = 0;

  this->Aspect[0] = this->Aspect[1] = 1.0;
}

void vlRenderer::SetActiveCamera(vlCamera *cam)
{
  this->ActiveCamera = cam;
}

void vlRenderer::AddLights(vlLight *light)
{
  this->Lights.AddMember(light);
}

void vlRenderer::AddActors(vlActor *actor)
{
  this->Actors.AddMember(actor);
}

void vlRenderer::DoLights()
{
  vlLight *light1;

  if (!this->UpdateLights())
    {
    cerr << this->GetClassName() << " : No lights are on, creating one.\n";
    light1 = this->RenderWindow->MakeLight();
    this->AddLights(light1);
    light1->SetPosition(this->ActiveCamera->GetPosition());
    light1->SetFocalPoint(this->ActiveCamera->GetFocalPoint());
    this->UpdateLights();
    }
}

void vlRenderer::DoCameras()
{
  if (!this->UpdateCameras())
    {
    cerr << "No cameras are on, creating one.\n";
    }
}

void vlRenderer::DoActors()
{

  if (!this->UpdateActors())
    {
    cerr << "No actors are on.\n";
    }
}

void vlRenderer::SetRenderWindow(vlRenderWindow *renwin)
{
  this->RenderWindow = renwin;
}

void vlRenderer::DisplayToView()
{
  float vx,vy,vz;
  int sizex,sizey;
  int *size;
  
  /* get physical window dimensions */
  size = this->RenderWindow->GetSize();
  sizex = size[0];
  sizey = size[1];

  vx = 2.0 * (this->DisplayPoint[0] - sizex*this->Viewport[0])/ 
    (sizex*(this->Viewport[2]-this->Viewport[0])) - 1.0;
  vy = 2.0 * (this->DisplayPoint[1] - sizey*this->Viewport[1])/ 
    (sizey*(this->Viewport[3]-this->Viewport[1])) - 1.0;
  vz = this->DisplayPoint[2];

  this->SetViewPoint(vx*this->Aspect[0],vy*this->Aspect[1],vz);
}

void vlRenderer::ViewToDisplay()
{
  float dx,dy;
  int sizex,sizey;
  int *size;
  
  /* get physical window dimensions */
  size = this->RenderWindow->GetSize();
  sizex = size[0];
  sizey = size[1];

  dx = (this->ViewPoint[0]/this->Aspect[0] + 1.0) * 
    (sizex*(this->Viewport[2]-this->Viewport[0])) / 2.0 + 0.5 +
      sizex*this->Viewport[0];
  dy = (this->ViewPoint[1]/this->Aspect[1] + 1.0) * 
    (sizey*(this->Viewport[3]-this->Viewport[1])) / 2.0 + 0.5 +
      sizey*this->Viewport[1];

  this->SetDisplayPoint(dx,dy,this->ViewPoint[2]);
}

void vlRenderer::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlRenderer::GetClassName()))
    {
    this->vlObject::PrintSelf(os,indent);

    os << indent << "Actors:\n";
    this->Actors.PrintSelf(os,indent.GetNextIndent());
    os << indent << "Ambient: (" << this->Ambient[0] << ", " 
      << this->Ambient[1] << ", " << this->Ambient[2] << ")\n";
    os << indent << "Aspect: (" << this->Aspect[0] << ", " 
      << this->Aspect[1] << ")\n";
    os << indent << "Background: (" << this->Background[0] << ", " 
      << this->Background[1] << ", "  << this->Background[2] << ")\n";

    os << indent << "Back Light: " << (this->BackLight ? "On\n" : "Off\n");
    os << indent << "DisplayPoint: ("  << this->DisplayPoint[0] << ", " 
      << this->DisplayPoint[1] << ", " << this->DisplayPoint[2] << ")\n";
    os << indent << "Erase: " << (this->Erase ? "On\n" : "Off\n");
    os << indent << "Lights:\n";
    this->Lights.PrintSelf(os,indent.GetNextIndent());
    os << indent << "Stereo Render: " 
      << (this->StereoRender ? "On\n":"Off\n");

    os << indent << "ViewPoint: (" << this->ViewPoint[0] << ", " 
      << this->ViewPoint[1] << ", " << this->ViewPoint[2] << ")\n";
    os << indent << "Viewport: (" << this->Viewport[0] << ", " 
      << this->Viewport[1] << ", " << this->Viewport[2] << ", " 
	<< this->Viewport[3] << ")\n";
    }
}

