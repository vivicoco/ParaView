/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGenericRenderWindowInteractor.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVGenericRenderWindowInteractor.h"

#include "vtkPVRenderView.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVGenericRenderWindowInteractor);
vtkCxxRevisionMacro(vtkPVGenericRenderWindowInteractor, "1.13");

//----------------------------------------------------------------------------
vtkPVGenericRenderWindowInteractor::vtkPVGenericRenderWindowInteractor()
{
  this->PVRenderView = NULL;
  this->ReductionFactor = 1;
  this->InteractiveRenderEnabled = 0;
}

//----------------------------------------------------------------------------
vtkPVGenericRenderWindowInteractor::~vtkPVGenericRenderWindowInteractor()
{
  this->SetPVRenderView(NULL);
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::SetPVRenderView(vtkPVRenderView *view)
{
  if (this->PVRenderView != view)
    {
    // to avoid circular references
    this->PVRenderView = view;
    if (this->PVRenderView != NULL)
      {
      this->SetRenderWindow(this->PVRenderView->GetRenderWindow());
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::SetMoveEventInformationFlipY(
  int x, int y)
{
  this->SetEventInformationFlipY(x, y, this->ControlKey, this->ShiftKey,
                                 this->KeyCode, this->RepeatCount,
                                 this->KeySym);
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::Render()
{
  if ( this->PVRenderView == NULL || this->RenderWindow == NULL)
    { // The case for interactors on the satellite processes.
    return;
    }

  // This should fix the problem of the plane widget render 
  if (this->InteractiveRenderEnabled)
    {
    this->PVRenderView->Render();
    }
  else
    {
    this->PVRenderView->EventuallyRender();
    }
}

// Special methods for forwarding events to satellite processes.
// The take care of the reduction factor by comparing 
// renderer size with render window size. 


//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::SatelliteLeftPress(float wx, float wy, float wz, 
                                                     int control, int shift)
{
  int renSize[2];
  float xy[4];
  int reductionFactor;
  this->PrepareForEvent(wx, wy, wz, xy, &reductionFactor, renSize);

  this->SetEventInformation(xy[0], xy[1], control, shift);
  if (reductionFactor != this->ReductionFactor)
    {
    this->LastEventPosition[0] = this->LastEventPosition[0] 
                                   * this->ReductionFactor / reductionFactor;
    this->LastEventPosition[1] = this->LastEventPosition[1] 
                                   * this->ReductionFactor / reductionFactor;
    this->Size[0] = renSize[0];
    this->Size[1] = renSize[1];
    this->ReductionFactor = reductionFactor;
    }
  this->LeftButtonPressEvent();
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::SatelliteMiddlePress(float wx, float wy, float wz, 
                                                     int control, int shift)
{
  int renSize[2];
  float xy[4];
  int reductionFactor;
  this->PrepareForEvent(wx, wy, wz, xy, &reductionFactor, renSize);

  this->SetEventInformation(xy[0], xy[1], control, shift);
  if (reductionFactor != this->ReductionFactor)
    {
    this->LastEventPosition[0] = this->LastEventPosition[0] 
                                   * this->ReductionFactor / reductionFactor;
    this->LastEventPosition[1] = this->LastEventPosition[1] 
                                   * this->ReductionFactor / reductionFactor;
    this->Size[0] = renSize[0];
    this->Size[1] = renSize[1];
    this->ReductionFactor = reductionFactor;
    }
  this->MiddleButtonPressEvent();
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::SatelliteRightPress(float wx, float wy, float wz, 
                                                     int control, int shift)
{
  int renSize[2];
  float xy[4];
  int reductionFactor;
  this->PrepareForEvent(wx, wy, wz, xy, &reductionFactor, renSize);

  this->SetEventInformation(xy[0], xy[1], control, shift);
  if (reductionFactor != this->ReductionFactor)
    {
    this->LastEventPosition[0] = this->LastEventPosition[0] 
                                   * this->ReductionFactor / reductionFactor;
    this->LastEventPosition[1] = this->LastEventPosition[1] 
                                   * this->ReductionFactor / reductionFactor;
    this->Size[0] = renSize[0];
    this->Size[1] = renSize[1];
    this->ReductionFactor = reductionFactor;
    }
  this->RightButtonPressEvent();
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::SatelliteLeftRelease(float wx, float wy, float wz, 
                                                       int control, int shift)
{
  int renSize[2];
  float xy[4];
  int reductionFactor;
  this->PrepareForEvent(wx, wy, wz, xy, &reductionFactor, renSize);

  this->SetEventInformation(xy[0], xy[0], control, shift);
  if (reductionFactor != this->ReductionFactor)
    {
    this->LastEventPosition[0] = this->LastEventPosition[0] 
                                   * this->ReductionFactor / reductionFactor;
    this->LastEventPosition[1] = this->LastEventPosition[1] 
                                   * this->ReductionFactor / reductionFactor;
    this->Size[0] = renSize[0];
    this->Size[1] = renSize[1];
    this->ReductionFactor = reductionFactor;
    }
  this->LeftButtonReleaseEvent();
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::SatelliteMiddleRelease(float wx, float wy, float wz, 
                                                        int control, int shift)
{
  int renSize[2];
  float xy[4];
  int reductionFactor;
  this->PrepareForEvent(wx, wy, wz, xy, &reductionFactor, renSize);

  this->SetEventInformation(xy[0], xy[1], control, shift);
  if (reductionFactor != this->ReductionFactor)
    {
    this->LastEventPosition[0] = this->LastEventPosition[0] 
                                   * this->ReductionFactor / reductionFactor;
    this->LastEventPosition[1] = this->LastEventPosition[1] 
                                   * this->ReductionFactor / reductionFactor;
    this->Size[0] = renSize[0];
    this->Size[1] = renSize[1];
    this->ReductionFactor = reductionFactor;
    }
  this->MiddleButtonReleaseEvent();
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::SatelliteRightRelease(float wx, float wy, float wz, 
                                                        int control, int shift)
{
  int renSize[2];
  float xy[4];
  int reductionFactor;
  this->PrepareForEvent(wx, wy, wz, xy, &reductionFactor, renSize);

  this->SetEventInformation(xy[0], xy[1], control, shift);
  if (reductionFactor != this->ReductionFactor)
    {
    this->LastEventPosition[0] = this->LastEventPosition[0] 
                                   * this->ReductionFactor / reductionFactor;
    this->LastEventPosition[1] = this->LastEventPosition[1] 
                                   * this->ReductionFactor / reductionFactor;
    this->Size[0] = renSize[0];
    this->Size[1] = renSize[1];
    this->ReductionFactor = reductionFactor;
    }
  this->RightButtonReleaseEvent();
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::SatelliteMove(float wx, float wy, float wz) 
{
  int renSize[2];
  float xy[4];
  int reductionFactor;
  this->PrepareForEvent(wx, wy, wz, xy, &reductionFactor, renSize);

  this->SetEventInformation(xy[0], xy[1], this->ControlKey, this->ShiftKey,
                                 this->KeyCode, this->RepeatCount,
                                 this->KeySym);
  if (reductionFactor != this->ReductionFactor)
    {
    this->LastEventPosition[0] = this->LastEventPosition[0] 
                                   * this->ReductionFactor / reductionFactor;
    this->LastEventPosition[1] = this->LastEventPosition[1] 
                                   * this->ReductionFactor / reductionFactor;
    this->Size[0] = renSize[0];
    this->Size[1] = renSize[1];
    this->ReductionFactor = reductionFactor;
    }
  this->MouseMoveEvent();
}


//----------------------------------------------------------------------------
int vtkPVGenericRenderWindowInteractor::CalculateReductionFactor(int winSize,
                                                                 int renSize)
{
  if(winSize > 0 && renSize > 0)
    {
    return (int)(0.5 + ((float)winSize/(float)renSize));
    }
  else
    {
    return 1;
    }
}


//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::PrepareForEvent(float wx, float wy, float wz, float* xy,
  int *reductionFactor, int* renSize)
{
  int *winSize;

  vtkRendererCollection *rc = this->RenderWindow->GetRenderers();
  rc->InitTraversal();
  vtkRenderer *aren = rc->GetNextItem();
  aren->SetWorldPoint(wx, wy, wz, 1);
  aren->WorldToDisplay();
  aren->GetDisplayPoint(xy);

  int *rsize = aren->GetSize();
  renSize[0] = rsize[0];
  renSize[1] = rsize[1];

  winSize = this->RenderWindow->GetSize();
  *reductionFactor = this->CalculateReductionFactor(winSize[1], renSize[1]);
  xy[0] = (int)(xy[0] / (float)(*reductionFactor));
  xy[1] = (int)((float)(winSize[1]-xy[1]) / (float)(*reductionFactor));
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "PVRenderView: " << this->GetPVRenderView() << endl;
  os << indent << "InteractiveRenderEnabled: " 
     << this->InteractiveRenderEnabled << endl;
}
