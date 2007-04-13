/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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
#include "pqProgressBar.h"
#include <QCoreApplication>
#include <QTimer>

//-----------------------------------------------------------------------------
pqProgressBar::pqProgressBar(QWidget* _p) : QProgressBar(_p)
{
  this->CleanUp = false;
}


//-----------------------------------------------------------------------------
pqProgressBar::~pqProgressBar()
{
  
}

//-----------------------------------------------------------------------------
void pqProgressBar::setProgress(const QString& message, int _value)
{
  this->setFormat(QString("%1: %p").arg(message));
  this->setValue(_value);
  //QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

//-----------------------------------------------------------------------------
void pqProgressBar::enableProgress(bool e)
{
  if(e)
    {
    this->setEnabled(e);
    this->setTextVisible(e);
    }
  else if(!this->CleanUp)
    {
    this->setValue(100);
    this->CleanUp = true;
    QTimer::singleShot(0, this, SLOT(cleanup()));
    }
}

void pqProgressBar::cleanup()
{
  this->CleanUp = false;
  this->setEnabled(false);
  this->setTextVisible(false);
  this->reset();
}


