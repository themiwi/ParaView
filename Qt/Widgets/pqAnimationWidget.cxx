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

#include "pqAnimationWidget.h"

#include <QResizeEvent>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QScrollBar>

#include "pqAnimationModel.h"
#include "pqAnimationTrack.h"

pqAnimationWidget::pqAnimationWidget(QWidget* p) 
  : QAbstractScrollArea(p) 
{
  this->setAutoFillBackground(true);
  this->View = new QGraphicsView(this->viewport());
  this->viewport()->setBackgroundRole(QPalette::Window);
  this->View->setBackgroundRole(QPalette::Window);
  this->View->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  this->View->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  this->View->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  this->View->setFrameShape(QFrame::NoFrame);
  this->Model = new pqAnimationModel(this->View);
  this->View->setScene(this->Model);

  this->CreateDeleteHeader = new QHeaderView(Qt::Vertical, this);
  this->CreateDeleteHeader->viewport()->setBackgroundRole(QPalette::Window);

  this->CreateDeleteHeader->setClickable(true);
  this->CreateDeleteHeader->setSizePolicy(QSizePolicy::Minimum,
                              QSizePolicy::MinimumExpanding);
  this->CreateDeleteHeader->setResizeMode(QHeaderView::Fixed);
  this->CreateDeleteHeader->setModel(&this->CreateDeleteModel);

  this->Header = new QHeaderView(Qt::Vertical, this);
  this->Header->viewport()->setBackgroundRole(QPalette::Window);
  this->Header->setObjectName("TrackHeader");
  this->Header->setSizePolicy(QSizePolicy::Preferred,
                              QSizePolicy::MinimumExpanding);
  this->View->setSizePolicy(QSizePolicy::Preferred,
                              QSizePolicy::MinimumExpanding);
  this->Header->setResizeMode(QHeaderView::Fixed);
  this->Header->setMinimumSectionSize(0);
  this->Header->setModel(this->Model->header());
  this->Model->setRowHeight(this->Header->defaultSectionSize());
  
  this->CreateDeleteWidget = new QWidget(this);

  QObject::connect(this->Header->model(),
                   SIGNAL(rowsInserted(QModelIndex,int,int)),
                   this, SLOT(updateSizes()));
  QObject::connect(this->Header->model(),
                   SIGNAL(headerDataChanged(Qt::Orientation, int, int)),
                   this, SLOT(updateSizes()));
  QObject::connect(this->Header->model(),
                   SIGNAL(rowsRemoved(QModelIndex,int,int)),
                   this, SLOT(updateSizes()));
  QObject::connect(this->Header,
                   SIGNAL(sectionDoubleClicked(int)),
                   this, SLOT(headerDblClicked(int)));
  QObject::connect(this->Model,
                   SIGNAL(trackSelected(pqAnimationTrack*)),
                   this, SIGNAL(trackSelected(pqAnimationTrack*)));
  QObject::connect(this->CreateDeleteHeader,
                   SIGNAL(sectionClicked(int)),
                   this, SLOT(headerDeleteClicked(int)));
}

pqAnimationWidget::~pqAnimationWidget()
{
}

pqAnimationModel* pqAnimationWidget::animationModel() const
{
  return this->Model;
}

QHeaderView* pqAnimationWidget::createDeleteHeader() const
{
  return this->CreateDeleteHeader;
}

QWidget* pqAnimationWidget::createDeleteWidget() const
{
  return this->CreateDeleteWidget;
}

void pqAnimationWidget::updateSizes()
{
  this->CreateDeleteModel.clear();
  this->CreateDeleteModel.insertRow(0);
  this->CreateDeleteModel.setHeaderData(0, Qt::Vertical, QVariant(), Qt::DisplayRole);

  int num = this->Model->count();

  for(int i=0; i<num; i++)
    {
    this->CreateDeleteModel.insertRow(i+1);
    if(this->Model->track(i)->isDeletable())
      {
      this->CreateDeleteModel.setHeaderData(i+1, Qt::Vertical,
        QPixmap(":/QtWidgets/Icons/pqDelete16.png"), Qt::DecorationRole);
      }
    this->CreateDeleteModel.setHeaderData(i+1, Qt::Vertical, QVariant(), Qt::DisplayRole);
    }
  this->CreateDeleteModel.insertRow(this->Header->count());
  this->CreateDeleteModel.setHeaderData(this->Header->count(), Qt::Vertical,
    QPixmap(":/QtWidgets/Icons/pqPlus16.png"), Qt::DecorationRole);
  
  this->updateGeometries();
}

  
void pqAnimationWidget::headerDblClicked(int which)
{
  if(which > 0)
    {
    emit this->trackSelected(this->Model->track(which-1));
    }
}
  
void pqAnimationWidget::updateGeometries()
{
  int width1 = 0;
  int width2 = 0;
  int height1 = 0;
  int height2 = 0;
  if(!this->CreateDeleteHeader->isHidden())
    {
    int tmp = qMax(this->CreateDeleteHeader->minimumWidth(),
                 this->CreateDeleteHeader->sizeHint().width());
    width1 = qMin(tmp, this->CreateDeleteHeader->maximumWidth());
    height1 = this->CreateDeleteHeader->defaultSectionSize() *
              this->CreateDeleteHeader->count();
    }
  if(!this->Header->isHidden())
    {
    int tmp = qMax(this->Header->minimumWidth(),
                 this->Header->sizeHint().width());
    width2 = qMin(tmp, this->Header->maximumWidth());
    height2 = this->Header->defaultSectionSize() *
              this->Header->count();
    }

  this->setViewportMargins(width1 + width2, 0, 0, 0);

  QRect vg = this->contentsRect();
  this->CreateDeleteHeader->setGeometry(vg.left(), vg.top(), width1, height1);
  this->Header->setGeometry(vg.left() + width1, vg.top(), width2, height2);

  this->updateScrollBars();
}

void pqAnimationWidget::scrollContentsBy(int dx, int dy)
{
  if(dy)
    {
    this->CreateDeleteHeader->setOffset(this->verticalScrollBar()->value());
    this->Header->setOffset(this->verticalScrollBar()->value());
    }
  this->updateWidgetPosition();
  QAbstractScrollArea::scrollContentsBy(dx, dy);
}

void pqAnimationWidget::updateScrollBars()
{
  int h = this->View->sizeHint().height();
  int extraw = 0;
  int viewh = h;
  if(this->CreateDeleteHeader->isVisible())
    {
    h = qMax(h, this->CreateDeleteHeader->length());
    }
  if(this->Header->isVisible())
    {
    h = qMax(h, this->Header->length());
    extraw = this->Header->width();
    viewh = h;
    }
  
  QSize vsize = this->viewport()->size();
  this->View->resize(vsize.width(), viewh);
  this->CreateDeleteWidget->resize(vsize.width()+extraw,
                                   this->Header->defaultSectionSize());

  this->updateWidgetPosition();

  this->verticalScrollBar()->setPageStep(vsize.height());
  this->verticalScrollBar()->setRange(0, h-vsize.height());
}

void pqAnimationWidget::updateWidgetPosition()
{
  int s = this->verticalScrollBar()->value();
  this->View->move(0, -s);
  if(this->CreateDeleteHeader->isVisible())
    {
    int xpos = this->CreateDeleteHeader->frameGeometry().right()+1;
    int ypos = this->CreateDeleteHeader->frameGeometry().bottom()-
               this->CreateDeleteHeader->defaultSectionSize() -
               this->CreateDeleteHeader->offset();
    this->CreateDeleteWidget->raise();
    this->CreateDeleteWidget->move(xpos, ypos);
    }
  else
    {
    this->CreateDeleteWidget->lower();
    }
}

bool pqAnimationWidget::event(QEvent* e)
{
  if(e->type() == QEvent::FontChange)
    {
    this->Model->setRowHeight(this->Header->defaultSectionSize());
    }
  if(e->type() == QEvent::Show)
    {
    this->updateGeometries();
    }
  return QAbstractScrollArea::event(e);
}

void pqAnimationWidget::resizeEvent(QResizeEvent* e)
{
  this->updateScrollBars();
  QAbstractScrollArea::resizeEvent(e);
}
  
void pqAnimationWidget::headerDeleteClicked(int which)
{
  if(which > 0)
    {
    if(which == this->CreateDeleteHeader->count() - 1)
      {
      emit this->createTrackClicked();
      }
    else
      {
      pqAnimationTrack* t = this->Model->track(which-1);
      if(t && t->isDeletable())
        {
        emit this->deleteTrackClicked(t);
        }
      }
    }
}

