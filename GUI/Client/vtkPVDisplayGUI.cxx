/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDisplayGUI.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDisplayGUI.h"

#include "vtkPVColorMap.h"
#include "vtkCellData.h"
#include "vtkCollection.h"
#include "vtkDataSetAttributes.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkSMPartDisplay.h"
#include "vtkSMLODPartDisplay.h"
#include "vtkCollection.h"
#include "vtkColorTransferFunction.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkSMLODPartDisplay.h"
#include "vtkImageData.h"
#include "vtkKWBoundsDisplay.h"
#include "vtkKWChangeColorButton.h"
#include "vtkKWCheckButton.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledEntry.h"
#include "vtkKWMenuButton.h"
#include "vtkKWNotebook.h"
#include "vtkKWOptionMenu.h"
#include "vtkKWPushButton.h"
#include "vtkKWScale.h"
#include "vtkKWThumbWheel.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWView.h"
#include "vtkKWWidget.h"
#include "vtkKWMenu.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPVApplication.h"
#include "vtkPVConfig.h"
#include "vtkPVNumberOfOutputsInformation.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSource.h"
#include "vtkPVVolumeAppearanceEditor.h"
#include "vtkPVWindow.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProp3D.h"
#include "vtkProperty.h"
#include "vtkProperty2D.h"
#include "vtkRectilinearGrid.h"
#include "vtkRenderer.h"
#include "vtkStructuredGrid.h"
#include "vtkTexture.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVRenderView.h"
#include "vtkPVRenderModule.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVRenderModuleUI.h"
#include "vtkVolumeProperty.h"
#include "vtkDataSet.h"
#include "vtkPVOptions.h"
#include "vtkStdString.h"

#define VTK_PV_OUTLINE_LABEL "Outline"
#define VTK_PV_SURFACE_LABEL "Surface"
#define VTK_PV_WIREFRAME_LABEL "Wireframe of Surface"
#define VTK_PV_POINTS_LABEL "Points of Surface"
#define VTK_PV_VOLUME_LABEL "Volume Render"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVDisplayGUI);
vtkCxxRevisionMacro(vtkPVDisplayGUI, "1.24");

int vtkPVDisplayGUICommand(ClientData cd, Tcl_Interp *interp,
                     int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVDisplayGUI::vtkPVDisplayGUI()
{
  static int instanceCount = 0;

  this->CommandFunction = vtkPVDisplayGUICommand;

  this->PVSource = 0;
  
  this->EditColorMapButtonVisible = 1;
  this->MapScalarsCheckVisible = 0;
  this->ColorButtonVisible = 1;
  this->ScalarBarCheckVisible = 1;
  this->InterpolateColorsCheckVisible = 1;

  // Create a unique id for creating tcl names.
  ++instanceCount;
  this->InstanceCount = instanceCount;
    
  this->ColorFrame = vtkKWLabeledFrame::New();
  this->VolumeAppearanceFrame = vtkKWLabeledFrame::New();
  this->DisplayStyleFrame = vtkKWLabeledFrame::New();
  this->ViewFrame = vtkKWLabeledFrame::New();
  
  this->ColorMenuLabel = vtkKWLabel::New();
  this->ColorMenu = vtkKWOptionMenu::New();

  this->MapScalarsCheck = vtkKWCheckButton::New();
  this->InterpolateColorsCheck = vtkKWCheckButton::New();
  this->EditColorMapButtonFrame = vtkKWWidget::New();
  this->EditColorMapButton = vtkKWPushButton::New();
  this->DataColorRangeButton = vtkKWPushButton::New();
  
  this->ColorButton = vtkKWChangeColorButton::New();

  this->VolumeScalarsMenuLabel = vtkKWLabel::New();
  this->VolumeScalarsMenu = vtkKWOptionMenu::New();
  
  this->EditVolumeAppearanceButton = vtkKWPushButton::New();

  this->RepresentationMenuLabel = vtkKWLabel::New();
  this->RepresentationMenu = vtkKWOptionMenu::New();
  
  this->InterpolationMenuLabel = vtkKWLabel::New();
  this->InterpolationMenu = vtkKWOptionMenu::New();
  
  this->PointSizeLabel = vtkKWLabel::New();
  this->PointSizeThumbWheel = vtkKWThumbWheel::New();
  this->LineWidthLabel = vtkKWLabel::New();
  this->LineWidthThumbWheel = vtkKWThumbWheel::New();
  
  this->ScalarBarCheck = vtkKWCheckButton::New();
  this->CubeAxesCheck = vtkKWCheckButton::New();
  this->PointLabelCheck = vtkKWCheckButton::New();
  this->VisibilityCheck = vtkKWCheckButton::New();

  this->ResetCameraButton = vtkKWPushButton::New();

  this->ActorControlFrame = vtkKWLabeledFrame::New();
  this->TranslateLabel = vtkKWLabel::New();
  this->ScaleLabel = vtkKWLabel::New();
  this->OrientationLabel = vtkKWLabel::New();
  this->OriginLabel = vtkKWLabel::New();

  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->TranslateThumbWheel[cc] = vtkKWThumbWheel::New();
    this->ScaleThumbWheel[cc] = vtkKWThumbWheel::New();
    this->OrientationScale[cc] = vtkKWScale::New();
    this->OriginThumbWheel[cc] = vtkKWThumbWheel::New();
    }

  this->OpacityLabel = vtkKWLabel::New();
  this->OpacityScale = vtkKWScale::New();
  
  this->ActorColor[0] = this->ActorColor[1] = this->ActorColor[2] = 1.0;

  this->ColorSetByUser = 0;
  this->ArraySetByUser = 0;
    
  this->VolumeRenderMode = 0;
  
  this->VolumeAppearanceEditor = NULL;

  this->ShouldReinitialize = 0;
}

//----------------------------------------------------------------------------
vtkPVDisplayGUI::~vtkPVDisplayGUI()
{  
  if ( this->VolumeAppearanceEditor )
    {
    this->VolumeAppearanceEditor->UnRegister(this);
    this->VolumeAppearanceEditor = NULL;
    }
  
  this->SetPVSource(NULL);
    
  this->ColorMenuLabel->Delete();
  this->ColorMenuLabel = NULL;
  
  this->ColorMenu->Delete();
  this->ColorMenu = NULL;

  this->EditColorMapButtonFrame->Delete();
  this->EditColorMapButtonFrame = NULL;
  this->EditColorMapButton->Delete();
  this->EditColorMapButton = NULL;
  this->DataColorRangeButton->Delete();
  this->DataColorRangeButton = NULL;

  this->MapScalarsCheck->Delete();
  this->MapScalarsCheck = NULL;  
    
  this->InterpolateColorsCheck->Delete();
  this->InterpolateColorsCheck = NULL;  
    
  this->ColorButton->Delete();
  this->ColorButton = NULL;
  
  this->VolumeScalarsMenuLabel->Delete();
  this->VolumeScalarsMenuLabel = NULL;

  this->VolumeScalarsMenu->Delete();
  this->VolumeScalarsMenu = NULL;

  this->EditVolumeAppearanceButton->Delete();
  this->EditVolumeAppearanceButton = NULL;
  
  this->RepresentationMenuLabel->Delete();
  this->RepresentationMenuLabel = NULL;  
  this->RepresentationMenu->Delete();
  this->RepresentationMenu = NULL;
  
  this->InterpolationMenuLabel->Delete();
  this->InterpolationMenuLabel = NULL;
  this->InterpolationMenu->Delete();
  this->InterpolationMenu = NULL;
  
  this->PointSizeLabel->Delete();
  this->PointSizeLabel = NULL;
  this->PointSizeThumbWheel->Delete();
  this->PointSizeThumbWheel = NULL;
  this->LineWidthLabel->Delete();
  this->LineWidthLabel = NULL;
  this->LineWidthThumbWheel->Delete();
  this->LineWidthThumbWheel = NULL;

  this->ActorControlFrame->Delete();
  this->TranslateLabel->Delete();
  this->ScaleLabel->Delete();
  this->OrientationLabel->Delete();
  this->OriginLabel->Delete();

  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->TranslateThumbWheel[cc]->Delete();
    this->ScaleThumbWheel[cc]->Delete();
    this->OrientationScale[cc]->Delete();
    this->OriginThumbWheel[cc]->Delete();
    }

  this->OpacityLabel->Delete();
  this->OpacityScale->Delete();
 
  this->ScalarBarCheck->Delete();
  this->ScalarBarCheck = NULL;  

  this->CubeAxesCheck->Delete();
  this->CubeAxesCheck = NULL;

  this->PointLabelCheck->Delete();
  this->PointLabelCheck = NULL;

  this->VisibilityCheck->Delete();
  this->VisibilityCheck = NULL;
  
  this->ColorFrame->Delete();
  this->ColorFrame = NULL;
  this->VolumeAppearanceFrame->Delete();
  this->VolumeAppearanceFrame = NULL;
  this->DisplayStyleFrame->Delete();
  this->DisplayStyleFrame = NULL;
  this->ViewFrame->Delete();
  this->ViewFrame = NULL;
  
  this->ResetCameraButton->Delete();
  this->ResetCameraButton = NULL;
}


//----------------------------------------------------------------------------
// Legacy for old scripts.
void vtkPVDisplayGUI::SetVisibility(int v)
{
  this->PVSource->SetVisibility(v);
}
void vtkPVDisplayGUI::SetCubeAxesVisibility(int v)
{
  this->PVSource->SetCubeAxesVisibility(v);
}
void vtkPVDisplayGUI::SetPointLabelVisibility(int v)
{
  this->PVSource->SetPointLabelVisibility(v);
}
void vtkPVDisplayGUI::SetScalarBarVisibility(int v)
{
  if (this->PVSource && this->PVSource->GetPVColorMap())
    {
    this->PVSource->GetPVColorMap()->SetScalarBarVisibility(v);
    }
}
vtkPVColorMap* vtkPVDisplayGUI::GetPVColorMap()
{
  if (this->PVSource)
    {
    return this->PVSource->GetPVColorMap();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::Close()
{
  this->VolumeAppearanceEditor->Close();
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetVolumeAppearanceEditor(vtkPVVolumeAppearanceEditor *appearanceEditor)
{
  if ( this->VolumeAppearanceEditor == appearanceEditor )
    {
    return;
    }
  
  if ( this->VolumeAppearanceEditor )
    {
    this->VolumeAppearanceEditor->UnRegister(this);
    this->VolumeAppearanceEditor = NULL;
    }
  
  if ( appearanceEditor )
    {
    this->VolumeAppearanceEditor = appearanceEditor;
    this->VolumeAppearanceEditor->Register(this);
    }
}


//----------------------------------------------------------------------------
void vtkPVDisplayGUI::DeleteCallback()
{
  if (this->PVSource)
    {
    this->PVSource->SetCubeAxesVisibility(0);
    }
  if (this->PVSource)
    {
    this->PVSource->SetPointLabelVisibility(0);
    }
}



//----------------------------------------------------------------------------
// WE DO NOT REFERENCE COUNT HERE BECAUSE OF CIRCULAR REFERENCES.
// THE SOURCE OWNS THE DATA.
void vtkPVDisplayGUI::SetPVSource(vtkPVSource *source)
{
  if (this->PVSource == source)
    {
    return;
    }
  this->Modified();

  this->PVSource = source;

  this->SetTraceReferenceObject(source);
  this->SetTraceReferenceCommand("GetPVOutput");
}






// ============= Use to be in vtkPVActorComposite ===================

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::Create(vtkKWApplication* app, const char* options)
{
  if (this->GetApplication())
    {
    vtkErrorMacro("Widget already created.");
    return;
    }
  this->Superclass::Create(app, options);

  // We are going to 'grid' most of it, so let's define some const

  int col_1_padx = 2;
  int button_pady = 1;
  int col_0_weight = 0;
  int col_1_weight = 1;
  float col_0_factor = 1.5;
  float col_1_factor = 1.0;

  // View frame

  this->ViewFrame->SetParent(this->GetFrame());
  this->ViewFrame->ShowHideFrameOn();
  this->ViewFrame->Create(this->GetApplication(), 0);
  this->ViewFrame->SetLabel("View");
 
  this->VisibilityCheck->SetParent(this->ViewFrame->GetFrame());
  this->VisibilityCheck->Create(this->GetApplication(), "-text Data");
  this->GetApplication()->Script(
    "%s configure -command {%s VisibilityCheckCallback}",
    this->VisibilityCheck->GetWidgetName(),
    this->GetTclName());
  this->VisibilityCheck->SetState(1);
  this->VisibilityCheck->SetBalloonHelpString(
    "Toggle the visibility of this dataset's geometry.");

  this->ResetCameraButton->SetParent(this->ViewFrame->GetFrame());
  this->ResetCameraButton->Create(this->GetApplication(), "");
  this->ResetCameraButton->SetLabel("Set View to Data");
  this->ResetCameraButton->SetCommand(this, "CenterCamera");
  this->ResetCameraButton->SetBalloonHelpString(
    "Change the camera location to best fit the dataset in the view window.");

  this->ScalarBarCheck->SetParent(this->ViewFrame->GetFrame());
  this->ScalarBarCheck->Create(this->GetApplication(), "-text {Scalar bar}");
  this->ScalarBarCheck->SetBalloonHelpString(
    "Toggle the visibility of the scalar bar for this data.");
  this->GetApplication()->Script(
    "%s configure -command {%s ScalarBarCheckCallback}",
    this->ScalarBarCheck->GetWidgetName(),
    this->GetTclName());

  this->CubeAxesCheck->SetParent(this->ViewFrame->GetFrame());
  this->CubeAxesCheck->Create(this->GetApplication(), "-text {Cube Axes}");
  this->CubeAxesCheck->SetCommand(this, "CubeAxesCheckCallback");
  this->CubeAxesCheck->SetBalloonHelpString(
    "Toggle the visibility of X,Y,Z scales for this dataset.");

  this->PointLabelCheck->SetParent(this->ViewFrame->GetFrame());
  this->PointLabelCheck->Create(this->GetApplication(), "-text {Label Point Ids}");
  this->PointLabelCheck->SetCommand(this, "PointLabelCheckCallback");
  this->PointLabelCheck->SetBalloonHelpString(
    "Toggle the visibility of point id labels for this dataset.");
  
  this->Script("grid %s %s -sticky wns",
               this->VisibilityCheck->GetWidgetName(),
               this->ResetCameraButton->GetWidgetName());

  this->Script("grid %s -sticky news -padx %d -pady %d",
               this->ResetCameraButton->GetWidgetName(),
               col_1_padx, button_pady);

  this->Script("grid %s -sticky wns",
               this->ScalarBarCheck->GetWidgetName());
  
  this->Script("grid %s -sticky wns",
               this->CubeAxesCheck->GetWidgetName());

  if ((this->GetPVApplication()->GetProcessModule()->GetNumberOfPartitions() == 1) &&
      (!this->GetPVApplication()->GetOptions()->GetClientMode()))
    {
    this->Script("grid %s -sticky wns",
                 this->PointLabelCheck->GetWidgetName());
    }

  // Color
  this->ColorFrame->SetParent(this->GetFrame());
  this->ColorFrame->ShowHideFrameOn();
  this->ColorFrame->Create(this->GetApplication(), 0);
  this->ColorFrame->SetLabel("Color");

  this->ColorMenuLabel->SetParent(this->ColorFrame->GetFrame());
  this->ColorMenuLabel->Create(this->GetApplication(), "");
  this->ColorMenuLabel->SetLabel("Color by:");
  this->ColorMenuLabel->SetBalloonHelpString(
    "Select method for coloring dataset geometry.");
  
  this->ColorMenu->SetParent(this->ColorFrame->GetFrame());
  this->ColorMenu->Create(this->GetApplication(), "");   
  this->ColorMenu->SetBalloonHelpString(
    "Select method for coloring dataset geometry.");

  this->ColorButton->SetParent(this->ColorFrame->GetFrame());
  this->ColorButton->SetText("Actor Color");
  this->ColorButton->Create(this->GetApplication(), "");
  this->ColorButton->SetCommand(this, "ChangeActorColor");
  this->ColorButton->SetBalloonHelpString(
    "Edit the constant color for the geometry.");

  this->MapScalarsCheck->SetParent(this->ColorFrame->GetFrame());
  this->MapScalarsCheck->Create(this->GetApplication(), "-text {Map Scalars}");
  this->MapScalarsCheck->SetState(0);
  this->MapScalarsCheck->SetBalloonHelpString(
    "Pass attriubte through color map or use unsigned char values as color.");
  this->GetApplication()->Script(
    "%s configure -command {%s MapScalarsCheckCallback}",
    this->MapScalarsCheck->GetWidgetName(),
    this->GetTclName());
    
  this->InterpolateColorsCheck->SetParent(this->ColorFrame->GetFrame());
  this->InterpolateColorsCheck->Create(this->GetApplication(), "-text {Interpolate Colors}");
  this->InterpolateColorsCheck->SetState(0);
  this->InterpolateColorsCheck->SetBalloonHelpString(
    "Interpolate colors after mapping.");
  this->GetApplication()->Script(
    "%s configure -command {%s InterpolateColorsCheckCallback}",
    this->InterpolateColorsCheck->GetWidgetName(),
    this->GetTclName());
    
  // Group these two buttons in the place of one.
  this->EditColorMapButtonFrame->SetParent(this->ColorFrame->GetFrame());
  this->EditColorMapButtonFrame->Create(this->GetApplication(), "frame", "");
  // --
  this->EditColorMapButton->SetParent(this->EditColorMapButtonFrame);
  this->EditColorMapButton->Create(this->GetApplication(), "");
  this->EditColorMapButton->SetLabel("Edit Color Map");
  this->EditColorMapButton->SetCommand(this,"EditColorMapCallback");
  this->EditColorMapButton->SetBalloonHelpString(
    "Edit the table used to map data attributes to pseudo colors.");
  // --
  this->DataColorRangeButton->SetParent(this->EditColorMapButtonFrame);
  this->DataColorRangeButton->Create(this->GetApplication(), "");
  this->DataColorRangeButton->SetLabel("Reset Range");
  this->DataColorRangeButton->SetCommand(this,"DataColorRangeCallback");
  this->DataColorRangeButton->SetBalloonHelpString(
    "Sets the range of the color map to this module's scalar range.");
  // --
  this->Script("pack %s %s -side left -fill x -expand t -pady 2", 
               this->EditColorMapButton->GetWidgetName(),
               this->DataColorRangeButton->GetWidgetName());

  this->Script("grid %s %s -sticky wns",
               this->ColorMenuLabel->GetWidgetName(),
               this->ColorMenu->GetWidgetName());
  this->Script("grid %s -sticky news -padx %d -pady %d",
               this->ColorMenu->GetWidgetName(),
               col_1_padx, button_pady);

  this->Script("grid %s %s -sticky wns",
               this->MapScalarsCheck->GetWidgetName(),
               this->ColorButton->GetWidgetName());
  this->Script("grid %s -column 1 -sticky news -padx %d -pady %d",
               this->ColorButton->GetWidgetName(),
               col_1_padx, button_pady);
  this->ColorButtonVisible = 0;
  this->InterpolateColorsCheckVisible = 0;

  this->Script("grid %s %s -sticky wns",
               this->InterpolateColorsCheck->GetWidgetName(),
               this->EditColorMapButtonFrame->GetWidgetName());
  this->Script("grid %s -sticky news -padx %d -pady %d",
               this->EditColorMapButtonFrame->GetWidgetName(),
               col_1_padx, button_pady);

  // Volume Appearance
  this->SetVolumeAppearanceEditor(this->GetPVApplication()->GetMainWindow()->
                                  GetVolumeAppearanceEditor());

  this->VolumeAppearanceFrame->SetParent(this->GetFrame());
  this->VolumeAppearanceFrame->ShowHideFrameOn();
  this->VolumeAppearanceFrame->Create(this->GetApplication(), 0);
  this->VolumeAppearanceFrame->SetLabel("Volume Appearance");

  this->VolumeScalarsMenuLabel->
    SetParent(this->VolumeAppearanceFrame->GetFrame());
  this->VolumeScalarsMenuLabel->Create(this->GetApplication(), "");
  this->VolumeScalarsMenuLabel->SetLabel("View Scalars:");
  this->VolumeScalarsMenuLabel->SetBalloonHelpString(
    "Select scalars to view with volume rendering.");
  
  this->VolumeScalarsMenu->SetParent(this->VolumeAppearanceFrame->GetFrame());
  this->VolumeScalarsMenu->Create(this->GetApplication(), "");   
  this->VolumeScalarsMenu->SetBalloonHelpString(
    "Select scalars to view with volume rendering.");

  this->EditVolumeAppearanceButton->
    SetParent(this->VolumeAppearanceFrame->GetFrame());
  this->EditVolumeAppearanceButton->Create(this->GetApplication(), "");
  this->EditVolumeAppearanceButton->SetLabel("Edit Volume Appearance...");
  this->EditVolumeAppearanceButton->
    SetCommand(this,"EditVolumeAppearanceCallback");
  this->EditVolumeAppearanceButton->SetBalloonHelpString(
    "Edit the color and opacity functions for the volume.");

  
  this->Script("grid %s %s -sticky wns",
               this->VolumeScalarsMenuLabel->GetWidgetName(),
               this->VolumeScalarsMenu->GetWidgetName());

  this->Script("grid %s -sticky news -padx %d -pady %d",
               this->VolumeScalarsMenu->GetWidgetName(),
               col_1_padx, button_pady);

  this->Script("grid %s -column 1 -sticky news -padx %d -pady %d",
               this->EditVolumeAppearanceButton->GetWidgetName(),
               col_1_padx, button_pady);


  // Display style
  this->DisplayStyleFrame->SetParent(this->GetFrame());
  this->DisplayStyleFrame->ShowHideFrameOn();
  this->DisplayStyleFrame->Create(this->GetApplication(), 0);
  this->DisplayStyleFrame->SetLabel("Display Style");
  
  this->RepresentationMenuLabel->SetParent(
    this->DisplayStyleFrame->GetFrame());
  this->RepresentationMenuLabel->Create(this->GetApplication(), "");
  this->RepresentationMenuLabel->SetLabel("Representation:");

  this->RepresentationMenu->SetParent(this->DisplayStyleFrame->GetFrame());
  this->RepresentationMenu->Create(this->GetApplication(), "");
  this->RepresentationMenu->AddEntryWithCommand(VTK_PV_OUTLINE_LABEL, this,
                                                "DrawOutline");
  this->RepresentationMenu->AddEntryWithCommand(VTK_PV_SURFACE_LABEL, this,
                                                "DrawSurface");
  this->RepresentationMenu->AddEntryWithCommand(VTK_PV_WIREFRAME_LABEL, this,
                                                "DrawWireframe");
  this->RepresentationMenu->AddEntryWithCommand(VTK_PV_POINTS_LABEL, this,
                                                "DrawPoints");

  this->RepresentationMenu->SetBalloonHelpString(
    "Choose what geometry should be used to represent the dataset.");

  this->InterpolationMenuLabel->SetParent(this->DisplayStyleFrame->GetFrame());
  this->InterpolationMenuLabel->Create(this->GetApplication(), "");
  this->InterpolationMenuLabel->SetLabel("Interpolation:");

  this->InterpolationMenu->SetParent(this->DisplayStyleFrame->GetFrame());
  this->InterpolationMenu->Create(this->GetApplication(), "");
  this->InterpolationMenu->AddEntryWithCommand("Flat", this,
                                               "SetInterpolationToFlat");
  this->InterpolationMenu->AddEntryWithCommand("Gouraud", this,
                                               "SetInterpolationToGouraud");
  this->InterpolationMenu->SetValue("Gouraud");
  this->InterpolationMenu->SetBalloonHelpString(
    "Choose the method used to shade the geometry and interpolate point attributes.");

  this->PointSizeLabel->SetParent(this->DisplayStyleFrame->GetFrame());
  this->PointSizeLabel->Create(this->GetApplication(), "");
  this->PointSizeLabel->SetLabel("Point size:");
  this->PointSizeLabel->SetBalloonHelpString(
    "If your dataset contains points/verticies, "
    "this scale adjusts the diameter of the rendered points.");

  this->PointSizeThumbWheel->SetParent(this->DisplayStyleFrame->GetFrame());
  this->PointSizeThumbWheel->PopupModeOn();
  this->PointSizeThumbWheel->SetValue(1.0);
  this->PointSizeThumbWheel->SetResolution(1.0);
  this->PointSizeThumbWheel->SetMinimumValue(1.0);
  this->PointSizeThumbWheel->ClampMinimumValueOn();
  this->PointSizeThumbWheel->Create(this->GetApplication(), "");
  this->PointSizeThumbWheel->DisplayEntryOn();
  this->PointSizeThumbWheel->DisplayEntryAndLabelOnTopOff();
  this->PointSizeThumbWheel->SetBalloonHelpString("Set the point size.");
  this->PointSizeThumbWheel->GetEntry()->SetWidth(5);
  this->PointSizeThumbWheel->SetCommand(this, "ChangePointSize");
  this->PointSizeThumbWheel->SetEndCommand(this, "ChangePointSizeEndCallback");
  this->PointSizeThumbWheel->SetEntryCommand(this, "ChangePointSizeEndCallback");
  this->PointSizeThumbWheel->SetBalloonHelpString(
    "If your dataset contains points/verticies, "
    "this scale adjusts the diameter of the rendered points.");

  this->LineWidthLabel->SetParent(this->DisplayStyleFrame->GetFrame());
  this->LineWidthLabel->Create(this->GetApplication(), "");
  this->LineWidthLabel->SetLabel("Line width:");
  this->LineWidthLabel->SetBalloonHelpString(
    "If your dataset containes lines/edges, "
    "this scale adjusts the width of the rendered lines.");
  
  this->LineWidthThumbWheel->SetParent(this->DisplayStyleFrame->GetFrame());
  this->LineWidthThumbWheel->PopupModeOn();
  this->LineWidthThumbWheel->SetValue(1.0);
  this->LineWidthThumbWheel->SetResolution(1.0);
  this->LineWidthThumbWheel->SetMinimumValue(1.0);
  this->LineWidthThumbWheel->ClampMinimumValueOn();
  this->LineWidthThumbWheel->Create(this->GetApplication(), "");
  this->LineWidthThumbWheel->DisplayEntryOn();
  this->LineWidthThumbWheel->DisplayEntryAndLabelOnTopOff();
  this->LineWidthThumbWheel->SetBalloonHelpString("Set the line width.");
  this->LineWidthThumbWheel->GetEntry()->SetWidth(5);
  this->LineWidthThumbWheel->SetCommand(this, "ChangeLineWidth");
  this->LineWidthThumbWheel->SetEndCommand(this, "ChangeLineWidthEndCallback");
  this->LineWidthThumbWheel->SetEntryCommand(this, "ChangeLineWidthEndCallback");
  this->LineWidthThumbWheel->SetBalloonHelpString(
    "If your dataset containes lines/edges, "
    "this scale adjusts the width of the rendered lines.");

  this->Script("grid %s %s -sticky wns",
               this->RepresentationMenuLabel->GetWidgetName(),
               this->RepresentationMenu->GetWidgetName());

  this->Script("grid %s -sticky news -padx %d -pady %d",
               this->RepresentationMenu->GetWidgetName(), 
               col_1_padx, button_pady);

  this->Script("grid %s %s -sticky wns",
               this->InterpolationMenuLabel->GetWidgetName(),
               this->InterpolationMenu->GetWidgetName());

  this->Script("grid %s -sticky news -padx %d -pady %d",
               this->InterpolationMenu->GetWidgetName(),
               col_1_padx, button_pady);
  
  this->Script("grid %s %s -sticky wns",
               this->PointSizeLabel->GetWidgetName(),
               this->PointSizeThumbWheel->GetWidgetName());

  this->Script("grid %s -sticky news -padx %d -pady %d",
               this->PointSizeThumbWheel->GetWidgetName(), 
               col_1_padx, button_pady);

  this->Script("grid %s %s -sticky wns",
               this->LineWidthLabel->GetWidgetName(),
               this->LineWidthThumbWheel->GetWidgetName());

  this->Script("grid %s -sticky news -padx %d -pady %d",
               this->LineWidthThumbWheel->GetWidgetName(),
               col_1_padx, button_pady);

  // Now synchronize all those grids to have them aligned

  const char *widgets[4];
  widgets[0] = this->ViewFrame->GetFrame()->GetWidgetName();
  widgets[1] = this->ColorFrame->GetFrame()->GetWidgetName();
  widgets[2] = this->VolumeAppearanceFrame->GetFrame()->GetWidgetName();
  widgets[3] = this->DisplayStyleFrame->GetFrame()->GetWidgetName();

  int weights[2];
  weights[0] = col_0_weight;
  weights[1] = col_1_weight;

  float factors[2];
  factors[0] = col_0_factor;
  factors[1] = col_1_factor;

  vtkKWTkUtilities::SynchroniseGridsColumnMinimumSize(
    this->GetPVApplication()->GetMainInterp(), 4, widgets, factors, weights);
  
  // Actor Control

  this->ActorControlFrame->SetParent(this->GetFrame());
  this->ActorControlFrame->ShowHideFrameOn();
  this->ActorControlFrame->Create(this->GetApplication(), 0);
  this->ActorControlFrame->SetLabel("Actor Control");

  this->TranslateLabel->SetParent(this->ActorControlFrame->GetFrame());
  this->TranslateLabel->Create(this->GetApplication(), 0);
  this->TranslateLabel->SetLabel("Translate:");
  this->TranslateLabel->SetBalloonHelpString(
    "Translate the geometry relative to the dataset location.");

  this->ScaleLabel->SetParent(this->ActorControlFrame->GetFrame());
  this->ScaleLabel->Create(this->GetApplication(), 0);
  this->ScaleLabel->SetLabel("Scale:");
  this->ScaleLabel->SetBalloonHelpString(
    "Scale the geometry relative to the size of the dataset.");

  this->OrientationLabel->SetParent(this->ActorControlFrame->GetFrame());
  this->OrientationLabel->Create(this->GetApplication(), 0);
  this->OrientationLabel->SetLabel("Orientation:");
  this->OrientationLabel->SetBalloonHelpString(
    "Orient the geometry relative to the dataset origin.");

  this->OriginLabel->SetParent(this->ActorControlFrame->GetFrame());
  this->OriginLabel->Create(this->GetApplication(), 0);
  this->OriginLabel->SetLabel("Origin:");
  this->OriginLabel->SetBalloonHelpString(
    "Set the origin point about which rotations take place.");

  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->TranslateThumbWheel[cc]->SetParent(this->ActorControlFrame->GetFrame());
    this->TranslateThumbWheel[cc]->PopupModeOn();
    this->TranslateThumbWheel[cc]->SetValue(0.0);
    this->TranslateThumbWheel[cc]->Create(this->GetApplication(), 0);
    this->TranslateThumbWheel[cc]->DisplayEntryOn();
    this->TranslateThumbWheel[cc]->DisplayEntryAndLabelOnTopOff();
    this->TranslateThumbWheel[cc]->ExpandEntryOn();
    this->TranslateThumbWheel[cc]->GetEntry()->SetWidth(5);
    this->TranslateThumbWheel[cc]->SetCommand(this, "ActorTranslateCallback");
    this->TranslateThumbWheel[cc]->SetEndCommand(this, 
                                                 "ActorTranslateEndCallback");
    this->TranslateThumbWheel[cc]->SetEntryCommand(this,
                                                   "ActorTranslateEndCallback");
    this->TranslateThumbWheel[cc]->SetBalloonHelpString(
      "Translate the geometry relative to the dataset location.");

    this->ScaleThumbWheel[cc]->SetParent(this->ActorControlFrame->GetFrame());
    this->ScaleThumbWheel[cc]->PopupModeOn();
    this->ScaleThumbWheel[cc]->SetValue(1.0);
    this->ScaleThumbWheel[cc]->SetMinimumValue(0.0);
    this->ScaleThumbWheel[cc]->ClampMinimumValueOn();
    this->ScaleThumbWheel[cc]->SetResolution(0.05);
    this->ScaleThumbWheel[cc]->Create(this->GetApplication(), 0);
    this->ScaleThumbWheel[cc]->DisplayEntryOn();
    this->ScaleThumbWheel[cc]->DisplayEntryAndLabelOnTopOff();
    this->ScaleThumbWheel[cc]->ExpandEntryOn();
    this->ScaleThumbWheel[cc]->GetEntry()->SetWidth(5);
    this->ScaleThumbWheel[cc]->SetCommand(this, "ActorScaleCallback");
    this->ScaleThumbWheel[cc]->SetEndCommand(this, "ActorScaleEndCallback");
    this->ScaleThumbWheel[cc]->SetEntryCommand(this, "ActorScaleEndCallback");
    this->ScaleThumbWheel[cc]->SetBalloonHelpString(
      "Scale the geometry relative to the size of the dataset.");

    this->OrientationScale[cc]->SetParent(this->ActorControlFrame->GetFrame());
    this->OrientationScale[cc]->PopupScaleOn();
    this->OrientationScale[cc]->Create(this->GetApplication(), 0);
    this->OrientationScale[cc]->SetRange(0, 360);
    this->OrientationScale[cc]->SetResolution(1);
    this->OrientationScale[cc]->SetValue(0);
    this->OrientationScale[cc]->DisplayEntry();
    this->OrientationScale[cc]->DisplayEntryAndLabelOnTopOff();
    this->OrientationScale[cc]->ExpandEntryOn();
    this->OrientationScale[cc]->GetEntry()->SetWidth(5);
    this->OrientationScale[cc]->SetCommand(this, "ActorOrientationCallback");
    this->OrientationScale[cc]->SetEndCommand(this, 
                                              "ActorOrientationEndCallback");
    this->OrientationScale[cc]->SetEntryCommand(this, 
                                                "ActorOrientationEndCallback");
    this->OrientationScale[cc]->SetBalloonHelpString(
      "Orient the geometry relative to the dataset origin.");

    this->OriginThumbWheel[cc]->SetParent(this->ActorControlFrame->GetFrame());
    this->OriginThumbWheel[cc]->PopupModeOn();
    this->OriginThumbWheel[cc]->SetValue(0.0);
    this->OriginThumbWheel[cc]->Create(this->GetApplication(), 0);
    this->OriginThumbWheel[cc]->DisplayEntryOn();
    this->OriginThumbWheel[cc]->DisplayEntryAndLabelOnTopOff();
    this->OriginThumbWheel[cc]->ExpandEntryOn();
    this->OriginThumbWheel[cc]->GetEntry()->SetWidth(5);
    this->OriginThumbWheel[cc]->SetCommand(this, "ActorOriginCallback");
    this->OriginThumbWheel[cc]->SetEndCommand(this, "ActorOriginEndCallback");
    this->OriginThumbWheel[cc]->SetEntryCommand(this,"ActorOriginEndCallback");
    this->OriginThumbWheel[cc]->SetBalloonHelpString(
      "Orient the geometry relative to the dataset origin.");
    }

  this->OpacityLabel->SetParent(this->ActorControlFrame->GetFrame());
  this->OpacityLabel->Create(this->GetApplication(), 0);
  this->OpacityLabel->SetLabel("Opacity:");
  this->OpacityLabel->SetBalloonHelpString(
    "Set the opacity of the dataset's geometry.  "
    "Artifacts may appear in translucent geomtry "
    "because primatives are not sorted.");

  this->OpacityScale->SetParent(this->ActorControlFrame->GetFrame());
  this->OpacityScale->PopupScaleOn();
  this->OpacityScale->Create(this->GetApplication(), 0);
  this->OpacityScale->SetRange(0, 1);
  this->OpacityScale->SetResolution(0.1);
  this->OpacityScale->SetValue(1);
  this->OpacityScale->DisplayEntry();
  this->OpacityScale->DisplayEntryAndLabelOnTopOff();
  this->OpacityScale->ExpandEntryOn();
  this->OpacityScale->GetEntry()->SetWidth(5);
  this->OpacityScale->SetCommand(this, "OpacityChangedCallback");
  this->OpacityScale->SetEndCommand(this, "OpacityChangedEndCallback");
  this->OpacityScale->SetEntryCommand(this, "OpacityChangedEndCallback");
  this->OpacityScale->SetBalloonHelpString(
    "Set the opacity of the dataset's geometry.  "
    "Artifacts may appear in translucent geomtry "
    "because primatives are not sorted.");

  this->Script("grid %s %s %s %s -sticky news -pady %d",
               this->TranslateLabel->GetWidgetName(),
               this->TranslateThumbWheel[0]->GetWidgetName(),
               this->TranslateThumbWheel[1]->GetWidgetName(),
               this->TranslateThumbWheel[2]->GetWidgetName(),
               button_pady);

  this->Script("grid %s -sticky nws",
               this->TranslateLabel->GetWidgetName());

  this->Script("grid %s %s %s %s -sticky news -pady %d",
               this->ScaleLabel->GetWidgetName(),
               this->ScaleThumbWheel[0]->GetWidgetName(),
               this->ScaleThumbWheel[1]->GetWidgetName(),
               this->ScaleThumbWheel[2]->GetWidgetName(),
               button_pady);

  this->Script("grid %s -sticky nws",
               this->ScaleLabel->GetWidgetName());

  this->Script("grid %s %s %s %s -sticky news -pady %d",
               this->OrientationLabel->GetWidgetName(),
               this->OrientationScale[0]->GetWidgetName(),
               this->OrientationScale[1]->GetWidgetName(),
               this->OrientationScale[2]->GetWidgetName(),
               button_pady);

  this->Script("grid %s -sticky nws",
               this->OrientationLabel->GetWidgetName());

  this->Script("grid %s %s %s %s -sticky news -pady %d",
               this->OriginLabel->GetWidgetName(),
               this->OriginThumbWheel[0]->GetWidgetName(),
               this->OriginThumbWheel[1]->GetWidgetName(),
               this->OriginThumbWheel[2]->GetWidgetName(),
               button_pady);

  this->Script("grid %s -sticky nws",
               this->OriginLabel->GetWidgetName());

  this->Script("grid %s %s -sticky news -pady %d",
               this->OpacityLabel->GetWidgetName(),
               this->OpacityScale->GetWidgetName(),
               button_pady);

  this->Script("grid %s -sticky nws",
               this->OpacityLabel->GetWidgetName());

  this->Script("grid columnconfigure %s 0 -weight 0", 
               this->ActorControlFrame->GetFrame()->GetWidgetName());

  this->Script("grid columnconfigure %s 1 -weight 2", 
               this->ActorControlFrame->GetFrame()->GetWidgetName());

  this->Script("grid columnconfigure %s 2 -weight 2", 
               this->ActorControlFrame->GetFrame()->GetWidgetName());

  this->Script("grid columnconfigure %s 3 -weight 2", 
               this->ActorControlFrame->GetFrame()->GetWidgetName());

  // Pack

  this->Script("pack %s %s %s %s -fill x -expand t -pady 2", 
               this->ViewFrame->GetWidgetName(),
               this->ColorFrame->GetWidgetName(),
               this->DisplayStyleFrame->GetWidgetName(),
               this->ActorControlFrame->GetWidgetName());

  // Information page

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::EditColorMapCallback()
{
  if (this->PVSource == 0 || this->PVSource->GetPVColorMap() == 0)
    {
    // We could get the color map from the window,
    // but it must already be set for this button to be visible.
    vtkErrorMacro("Expecting a color map.");
    return;
    }
  this->Script("pack forget [pack slaves %s]",
          this->GetPVRenderView()->GetPropertiesParent()->GetWidgetName());
          
  this->Script("pack %s -side top -fill both -expand t",
          this->PVSource->GetPVColorMap()->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::DataColorRangeCallback()
{
  this->AddTraceEntry("$kw(%s) DataColorRangeCallback", this->GetTclName());
  if (this->PVSource)
    {
    vtkPVColorMap* colorMap = this->PVSource->GetPVColorMap();
    if (colorMap)
      {
      colorMap->ResetScalarRangeInternal(this->PVSource);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::EditVolumeAppearanceCallback()
{
  if (this->VolumeAppearanceEditor == NULL)
    {
    vtkErrorMacro("Expecting a volume appearance editor");
    return;
    }
  
  this->AddTraceEntry("$kw(%s) ShowVolumeAppearanceEditor",
                      this->GetTclName());

  this->ShowVolumeAppearanceEditor();
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ShowVolumeAppearanceEditor()
{
  if (this->VolumeAppearanceEditor == NULL)
    {
    vtkErrorMacro("Expecting a volume appearance editor");
    return;
    }
  
  this->Script("pack forget [pack slaves %s]",
          this->GetPVRenderView()->GetPropertiesParent()->GetWidgetName());
  this->Script("pack %s -side top -fill both -expand t",
          this->VolumeAppearanceEditor->GetWidgetName());
  
  vtkPVSource* source = this->GetPVSource();

  if (!source)
    {
    return;
    }

  vtkStdString command(this->VolumeScalarsMenu->GetValue());

  vtkStdString::size_type firstspace = command.find_first_of(' ');
  vtkStdString::size_type lastspace = command.find_last_of(' ');
  vtkStdString name = command.substr(firstspace+1, lastspace-firstspace-1);

  if ( command.c_str() && strlen(command.c_str()) > 6 )
    {
    vtkPVDataInformation* dataInfo = source->GetDataInformation();
    vtkPVArrayInformation *arrayInfo;
    int colorField = this->PVSource->GetPartDisplay()->GetColorField();
    if (colorField == vtkDataSet::POINT_DATA_FIELD)
      {
      vtkPVDataSetAttributesInformation *attrInfo
        = dataInfo->GetPointDataInformation();
      arrayInfo = attrInfo->GetArrayInformation(name.c_str());
      }
    else
      {
      vtkPVDataSetAttributesInformation *attrInfo
        = dataInfo->GetCellDataInformation();
      arrayInfo = attrInfo->GetArrayInformation(name.c_str());
      }
    this->VolumeAppearanceEditor->SetPVSourceAndArrayInfo( source, arrayInfo );
    }
  else
    {
    this->VolumeAppearanceEditor->SetPVSourceAndArrayInfo( NULL, NULL );
    }                                              
}


//----------------------------------------------------------------------------
void vtkPVDisplayGUI::Update()
{
  if (this->PVSource == 0 || this->PVSource->GetPartDisplay() == 0)
    {
    this->SetEnabled(0);
    this->UpdateEnableState();
    return;
    }
  this->SetEnabled(1);
  this->UpdateEnableState();

  // This call makes sure the information is up to date.
  // If not, it gathers information and updates the properties (internal).
  this->GetPVSource()->GetDataInformation();
  this->UpdateInternal();
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdateInternal()
{
  vtkPVSource* source = this->GetPVSource();
  vtkSMPartDisplay* pDisp = source->GetPartDisplay();
  
  // First reset all the values of the widgets.
  // Active states, and menu items will ge generated later.  
  // This could be done with a mechanism similar to reset
  // of parameters page.  
  // Parameters pages could just use/share the prototype UI instead of cloning.
  
  //law int fixmeEventually; // Use proper SM properties with reset.
  
  // Visibility check
  this->VisibilityCheck->SetState(this->PVSource->GetVisibility());
  // Cube axis visibility
  this->UpdateCubeAxesVisibilityCheck();
  // Point label visibility
  this->UpdatePointLabelVisibilityCheck();
  // Colors
  this->UpdateColorGUI();
    
  // Representation menu.
  switch (pDisp->GetRepresentation())
    {
    case VTK_OUTLINE:
      this->RepresentationMenu->SetValue(VTK_PV_OUTLINE_LABEL);
      break;
    case VTK_SURFACE:
      this->RepresentationMenu->SetValue(VTK_PV_SURFACE_LABEL);
      break;
    case VTK_WIREFRAME:
      this->RepresentationMenu->SetValue(VTK_PV_WIREFRAME_LABEL);
      break;
    case VTK_POINTS:
      this->RepresentationMenu->SetValue(VTK_PV_POINTS_LABEL);
      break;
    case VTK_VOLUME:
      this->RepresentationMenu->SetValue(VTK_PV_VOLUME_LABEL);
      break;
    default:
      vtkErrorMacro("Unknown representation.");
    }

  // Interpolation menu.
  switch (pDisp->GetInterpolation())
    {
    case VTK_FLAT:
      this->InterpolationMenu->SetValue("Flat");
      break;
    case VTK_GOURAUD:
      this->InterpolationMenu->SetValue("Gouraud");
      break;
    default:
      vtkErrorMacro("Unknown representation.");
    }

  this->PointSizeThumbWheel->SetValue(this->PVSource->GetPartDisplay()->GetPointSize());
  this->LineWidthThumbWheel->SetValue(this->PVSource->GetPartDisplay()->GetLineWidth());

  // Update actor control resolutions
  this->UpdateActorControl();
  this->UpdateActorControlResolutions();

  this->OpacityScale->SetValue(this->PVSource->GetPartDisplay()->GetOpacity());

  this->UpdateVolumeGUI();
}


//----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdateVisibilityCheck()
{
  int v = 0;
  if (this->PVSource)
    {
    v = this->PVSource->GetVisibility();
    }
  if (this->VisibilityCheck->GetApplication())
    {
    this->VisibilityCheck->SetState(v);
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdateCubeAxesVisibilityCheck()
{
  if (this->PVSource && this->VisibilityCheck->GetApplication())
    {
    this->CubeAxesCheck->SetState(this->PVSource->GetCubeAxesVisibility());
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdatePointLabelVisibilityCheck()
{
  if (this->PVSource && this->VisibilityCheck->GetApplication())
    {
    this->PointLabelCheck->SetState(this->PVSource->GetPointLabelVisibility());
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdateColorGUI()
{
  this->UpdateColorMenu();       // Computed value used in later methods.
  this->UpdateMapScalarsCheck(); // Computed value used in later methods.
  this->UpdateColorButton();
  this->UpdateEditColorMapButton();
  this->UpdateInterpolateColorsCheck();
  this->UpdateScalarBarVisibilityCheck();
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdateScalarBarVisibilityCheck()
{
  // Set enabled.
  if (this->PVSource->GetPVColorMap() == 0)
    {
    this->ScalarBarCheckVisible = 0;
    }
  else if (this->MapScalarsCheckVisible && 
           this->PVSource->GetPartDisplay()->GetDirectColorFlag())
    {
    this->ScalarBarCheckVisible = 0;
    }
  else
    {
    this->ScalarBarCheckVisible = 1;
    }

  // Set check on or off.
  if (this->ScalarBarCheckVisible)
    {
    this->ScalarBarCheck->SetState(
          this->PVSource->GetPVColorMap()->GetScalarBarVisibility());
    }
  else
    {
    this->ScalarBarCheck->SetState(0);
    }

  this->UpdateEnableState();
}


//----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdateColorMenu()
{  
  // Variables that hold the current color state.
  char tmp[350], cmd[1024], current[350]; 
  int currentColorByFound = 0;
  vtkPVDataInformation *dataInfo;
  vtkPVDataSetAttributesInformation *attrInfo;
  vtkPVArrayInformation *arrayInfo;
  vtkPVColorMap* colorMap = this->PVSource->GetPVColorMap();
  int colorField = -1;

  if (colorMap)
    {
    colorField = this->PVSource->GetPartDisplay()->GetColorField();
    }
  dataInfo = this->PVSource->GetDataInformation();
    
  // See if the current selection still exists.
  // If not, set a new default.
  if (colorMap)
    {
    if (colorField == vtkDataSet::POINT_DATA_FIELD)
      {
      attrInfo = dataInfo->GetPointDataInformation();
      }
    else
      {
      attrInfo = dataInfo->GetCellDataInformation();
      }
    arrayInfo = attrInfo->GetArrayInformation(colorMap->GetArrayName());
    if (arrayInfo == 0)
      {
      this->PVSource->SetDefaultColorParameters();
      colorMap = this->PVSource->GetPVColorMap();
      if (colorMap)
        {
        colorField = this->PVSource->GetPartDisplay()->GetColorField();
        }
      else
        {
        colorField = -1;
        }
      }
    }
      
  // Populate menus
  this->ColorMenu->DeleteAllEntries();
  this->ColorMenu->AddEntryWithCommand("Property",
                                       this, "ColorByProperty");
  
  attrInfo = dataInfo->GetPointDataInformation();
  int i;
  int numArrays;
  int numComps;
  int firstField = 1;
  
  // First look at point data.
  numArrays = attrInfo->GetNumberOfArrays();
  for (i = 0; i < numArrays; i++)
    {
    arrayInfo = attrInfo->GetArrayInformation(i);
    numComps = arrayInfo->GetNumberOfComponents();
    sprintf(cmd, "ColorByPointField {%s} %d", 
            arrayInfo->GetName(), numComps);
    if (numComps > 1)
      {
      sprintf(tmp, "Point %s (%d)", arrayInfo->GetName(), numComps);
      }
    else
      {
      sprintf(tmp, "Point %s", arrayInfo->GetName());
      firstField = 0;
      }
    this->ColorMenu->AddEntryWithCommand(tmp, this, cmd);
    // Is this the same selection as the source has?
    if (colorField == vtkDataSet::POINT_DATA_FIELD && 
        strcmp(arrayInfo->GetName(), colorMap->GetArrayName()) == 0)
      {
      currentColorByFound = 1;
      strcpy(current, tmp);
      }
    }

  // Now look at cell attributes.
  attrInfo = dataInfo->GetCellDataInformation();
  numArrays = attrInfo->GetNumberOfArrays();
  for (i = 0; i < numArrays; i++)
    {
    arrayInfo = attrInfo->GetArrayInformation(i);
    sprintf(cmd, "ColorByCellField {%s} %d", 
            arrayInfo->GetName(), arrayInfo->GetNumberOfComponents());
    if (arrayInfo->GetNumberOfComponents() > 1)
      {
      sprintf(tmp, "Cell %s (%d)", arrayInfo->GetName(),
              arrayInfo->GetNumberOfComponents());
      }
    else
      {
      sprintf(tmp, "Cell %s", arrayInfo->GetName());
      }
    this->ColorMenu->AddEntryWithCommand(tmp, this, cmd);
    if (colorField == vtkDataSet::CELL_DATA_FIELD && 
        strcmp(arrayInfo->GetName(), colorMap->GetArrayName()) == 0)
      {
      currentColorByFound = 1;
      strcpy(current, tmp);
      }
    }

  if (colorMap == 0)
    {
    this->ColorMenu->SetValue("Property");
    return;
    }

  // If the current array we are coloring by has disappeared,
  // then default back to the property.
  if (currentColorByFound)
    {
    this->ColorMenu->SetValue(current);
    }
  else
    { // Choose another color.
    this->ColorMenu->SetValue("Property");
    vtkErrorMacro("Could not find previous color setting.");
    }    
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdateColorButton()
{
  float rgb[3];
  this->PVSource->GetPartDisplay()->GetColor(rgb);
  this->ColorButton->SetColor(rgb[0], rgb[1], rgb[2]);
  
  // We could look at the color menu's value too.
  this->ColorButtonVisible = 1;
  if (this->PVSource && this->PVSource->GetPVColorMap())
    {
    this->ColorButtonVisible = 0;
    }
  this->UpdateEnableState();
}  

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdateEditColorMapButton()
{
  if (this->PVSource->GetPVColorMap() == 0)
    {
    this->EditColorMapButtonVisible = 0;
    }
  else if (this->MapScalarsCheckVisible && 
           this->PVSource->GetPartDisplay()->GetDirectColorFlag())
    {
    this->EditColorMapButtonVisible = 0;
    }
  else
    {
    this->EditColorMapButtonVisible = 1;
    }
  this->UpdateEnableState();
}  

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdateInterpolateColorsCheck()
{
  if (this->PVSource->GetPVColorMap() == 0 ||
      (this->PVSource->GetPartDisplay()->GetDirectColorFlag() && 
       this->MapScalarsCheckVisible) ||
      this->PVSource->GetPartDisplay()->GetColorField() 
                                            == vtkDataSet::CELL_DATA_FIELD)
    {
    this->InterpolateColorsCheckVisible = 0;
    this->InterpolateColorsCheck->SetState(0);
    }
  else
    {
    this->InterpolateColorsCheckVisible = 1;
    this->InterpolateColorsCheck->SetState(
              this->PVSource->GetPartDisplay()->GetInterpolateColorsFlag());
    }
  this->UpdateEnableState();
}

//-----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdateVolumeGUI()
{
  vtkSMPartDisplay *pDisp = this->PVSource->GetPartDisplay();
  char tmp[350], volCmd[1024], defCmd[350];
  int i, numArrays, numComps;
  vtkPVDataInformation *dataInfo = this->PVSource->GetDataInformation();
  vtkPVDataSetAttributesInformation *attrInfo;
  vtkPVArrayInformation *arrayInfo;
  int defPoint = 0;
  vtkPVArrayInformation *defArray, *inputArray, *volRenArray;
  int dataType = dataInfo->GetDataSetType();

  this->VolumeRenderMode = pDisp->GetVolumeRenderMode();
  
  // Default is the scalars to use when current color is not found.
  // This is sort of a mess, and should be handled by a color selection widget.
  defCmd[0] = '\0'; 
  defArray = NULL;
  inputArray = NULL;
  volRenArray = NULL;
  
  const char *currentVolumeField = pDisp->GetVolumeRenderField();
  this->VolumeScalarsMenu->DeleteAllEntries();
  
  attrInfo = dataInfo->GetPointDataInformation();
  numArrays = attrInfo->GetNumberOfArrays();
  int firstField = 1;
  for (i = 0; i < numArrays; i++)
    {
    arrayInfo = attrInfo->GetArrayInformation(i);
    numComps = arrayInfo->GetNumberOfComponents();
    sprintf(volCmd, "VolumeRenderPointField {%s} %d",
            arrayInfo->GetName(), numComps);
    if (numComps > 1)
      {
      sprintf(tmp, "Point %s (%d)", arrayInfo->GetName(), numComps);
      }
    else
      {
      sprintf(tmp, "Point %s", arrayInfo->GetName());
      }
    this->VolumeScalarsMenu->AddEntryWithCommand(tmp, this, volCmd);
    if (   firstField
        || (   currentVolumeField
            && (pDisp->GetColorField() == vtkDataSet::POINT_DATA_FIELD)
            && (strcmp(arrayInfo->GetName(), currentVolumeField) == 0) ) )
      {
      this->VolumeScalarsMenu->SetValue( tmp );
      volRenArray = arrayInfo;
      firstField = 0;
      }
    if (attrInfo->IsArrayAnAttribute(i) == vtkDataSetAttributes::SCALARS)
      {
      strcpy(defCmd, tmp);
      defPoint = 1;
      defArray = arrayInfo;
      if (!currentVolumeField)
        {
        volRenArray = arrayInfo;
        this->VolumeScalarsMenu->SetValue( tmp );
        }
      }
    }
  
  attrInfo = dataInfo->GetCellDataInformation();
  numArrays = attrInfo->GetNumberOfArrays();
  for (i = 0; i < numArrays; i++)
    {
    arrayInfo = attrInfo->GetArrayInformation(i);
    numComps = arrayInfo->GetNumberOfComponents();
    sprintf(volCmd, "VolumeRenderCellField {%s} %d",
            arrayInfo->GetName(), numComps);
    if (numComps > 1)
      {
      sprintf(tmp, "Cell %s (%d)", arrayInfo->GetName(), numComps);
      }
    else
      {
      sprintf(tmp, "Cell %s", arrayInfo->GetName());
      }
    this->VolumeScalarsMenu->AddEntryWithCommand(tmp, this, volCmd);
    if (   firstField
        || (   currentVolumeField
            && (pDisp->GetColorField() == vtkDataSet::CELL_DATA_FIELD)
            && (strcmp(arrayInfo->GetName(), currentVolumeField) == 0) ) )
      {
      this->VolumeScalarsMenu->SetValue( tmp );
      volRenArray = arrayInfo;
      firstField = 0;
      }
    if (attrInfo->IsArrayAnAttribute(i) == vtkDataSetAttributes::SCALARS)
      {
      strcpy(defCmd, tmp);
      defPoint = 1;
      defArray = arrayInfo;
      if (!currentVolumeField)
        {
        volRenArray = arrayInfo;
        this->VolumeScalarsMenu->SetValue( tmp );
        }
      }
    }

  // Determine if this is unstructured grid data and add the 
  // volume rendering option
  if ( this->RepresentationMenu->HasEntry( VTK_PV_VOLUME_LABEL ) )
    {
      this->RepresentationMenu->DeleteEntry( VTK_PV_VOLUME_LABEL );
    }
  
  if (dataType == VTK_UNSTRUCTURED_GRID && volRenArray)
    {
    this->RepresentationMenu->AddEntryWithCommand(VTK_PV_VOLUME_LABEL, this,
                                                  "DrawVolume");
      
    // Update the transfer functions    
    pDisp->InitializeTransferFunctions(volRenArray, dataInfo);
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetActorColor(double r, double g, double b)
{
  this->ActorColor[0] = r;
  this->ActorColor[1] = g;
  this->ActorColor[2] = b;
  this->PVSource->GetPartDisplay()->SetColor(r, g, b);
}  

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ChangeActorColor(double r, double g, double b)
{
  this->AddTraceEntry("$kw(%s) ChangeActorColor %f %f %f",
                      this->GetTclName(), r, g, b);

  this->SetActorColor(r, g, b);
  this->ColorButton->SetColor(r, g, b);

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
  
  if (strcmp(this->ColorMenu->GetValue(), "Property") == 0)
    {
    this->ColorSetByUser = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ColorByProperty()
{
  this->ColorSetByUser = 1;
  this->AddTraceEntry("$kw(%s) ColorByProperty", this->GetTclName());
  this->ColorMenu->SetValue("Property");
  this->ColorByPropertyInternal();
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ColorByPropertyInternal()
{
  this->PVSource->GetPartDisplay()->SetScalarVisibility(0);
  double *color = this->ColorButton->GetColor();
  this->SetActorColor(color[0], color[1], color[2]);

  this->PVSource->SetPVColorMap(0);

  this->UpdateColorGUI();
  if (this->GetPVRenderView())
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}


//----------------------------------------------------------------------------
// Select which point field to use for volume rendering
//
void vtkPVDisplayGUI::VolumeRenderPointField(const char *name, int numComps)
{
  if (name == NULL)
    {
    return;
    }

  this->AddTraceEntry("$kw(%s) VolumeRenderPointField {%s} %d", 
                      this->GetTclName(), name, numComps);

  this->ArraySetByUser = 1;

  char *str;
  str = new char [strlen(name) + 16];
  if (numComps == 1)
    {
    sprintf(str, "Point %s", name);
    }
  else
    {
    sprintf(str, "Point %s (%d)", name, numComps);
    }
  this->VolumeScalarsMenu->SetValue(str);
  delete[] str;
  
  // Update the transfer functions  
  vtkPVDataInformation* dataInfo = this->GetPVSource()->GetDataInformation();
  vtkPVDataSetAttributesInformation *attrInfo = dataInfo->GetPointDataInformation();
  vtkPVArrayInformation *arrayInfo = attrInfo->GetArrayInformation(name);
  
  this->PVSource->GetPartDisplay()->ResetTransferFunctions(arrayInfo, dataInfo);
  
  this->VolumeRenderPointFieldInternal(name);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::VolumeRenderPointFieldInternal(const char *name)
{
  this->PVSource->GetPartDisplay()->VolumeRenderPointField( name );
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }

}

//----------------------------------------------------------------------------
// Select which cell field to use for volume rendering
//
void vtkPVDisplayGUI::VolumeRenderCellField(const char *name, int numComps)
{
  if (name == NULL)
    {
    return;
    }

  this->AddTraceEntry("$kw(%s) VolumeRenderCellField {%s} %d", 
                      this->GetTclName(), name, numComps);

  this->ArraySetByUser = 1;

  char *str;
  str = new char [strlen(name) + 16];
  if (numComps == 1)
    {
    sprintf(str, "Cell %s", name);
    }
  else
    {
    sprintf(str, "Cell %s (%d)", name, numComps);
    }
  this->VolumeScalarsMenu->SetValue(str);
  delete[] str;
  
  // Update the transfer functions  
  vtkPVDataInformation* dataInfo = this->GetPVSource()->GetDataInformation();
  vtkPVDataSetAttributesInformation *attrInfo = dataInfo->GetCellDataInformation();
  vtkPVArrayInformation *arrayInfo = attrInfo->GetArrayInformation(name);
  
  this->PVSource->GetPartDisplay()->ResetTransferFunctions(arrayInfo, dataInfo);
  
  this->VolumeRenderCellFieldInternal(name);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::VolumeRenderCellFieldInternal(const char *name)
{
  this->PVSource->GetPartDisplay()->VolumeRenderCellField( name );
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ColorByPointField(const char *name, int numComps)
{
  if (name == NULL)
    {
    return;
    }

  this->AddTraceEntry("$kw(%s) ColorByPointField {%s} %d", 
                      this->GetTclName(), name, numComps);

  this->ArraySetByUser = 1;
  
  char *str;
  str = new char [strlen(name) + 16];
  if (numComps == 1)
    {
    sprintf(str, "Point %s", name);
    }
  else
    {
    sprintf(str, "Point %s (%d)", name, numComps);
    }
  this->ColorMenu->SetValue(str);
  delete [] str;

  this->ColorByPointFieldInternal(name, numComps);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ColorByPointFieldInternal(const char *name, int numComps)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVColorMap* colorMap = pvApp->GetMainWindow()->GetPVColorMap(name, numComps);

  if (colorMap == 0)
    {
    vtkErrorMacro("Could not get the color map.");
    return;
    }
  this->PVSource->SetPVColorMap(colorMap);

  this->PVSource->GetPartDisplay()->ColorByArray(
            colorMap->GetProxyByName("LookupTable"), 
            vtkDataSet::POINT_DATA_FIELD);
  this->UpdateColorGUI();

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }

}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ColorByCellField(const char *name, int numComps)
{
  if (name == NULL)
    {
    return;
    }

  this->AddTraceEntry("$kw(%s) ColorByCellField {%s} %d", 
                      this->GetTclName(), name, numComps);
  
  this->ArraySetByUser = 1;
  
  // Set the menu value.
  char *str;
  str = new char [strlen(name) + 16];
  if (numComps == 1)
    {
    sprintf(str, "Cell %s", name);
    }
  else
    {
    sprintf(str, "Cell %s (%d)", name, numComps);
    }
  this->ColorMenu->SetValue(str);
  delete [] str;

  this->ColorByCellFieldInternal(name, numComps);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ColorByCellFieldInternal(const char *name, int numComps)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVColorMap* colorMap = pvApp->GetMainWindow()->GetPVColorMap(name, numComps);

  if (colorMap == NULL)
    {
    vtkErrorMacro("Could not get the color map.");
    return;
    }
  this->PVSource->SetPVColorMap(colorMap);

  this->PVSource->GetPartDisplay()->ColorByArray(
                      colorMap->GetProxyByName("LookupTable"), 
                      vtkDataSet::CELL_DATA_FIELD);

  // These three shoiuld be combined into a single method.
  this->UpdateColorGUI();

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}


//----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdateMapScalarsCheck()
{
  vtkPVColorMap* colorMap = this->PVSource->GetPVColorMap();

  this->MapScalarsCheckVisible = 0;  
  this->MapScalarsCheck->SetState(0);  
  if (colorMap)
    {
    this->MapScalarsCheck->SetState(1);
    // See if the array satisfies conditions necessary for direct coloring.  
    vtkPVDataInformation* dataInfo = this->PVSource->GetDataInformation();
    vtkPVDataSetAttributesInformation* attrInfo;
    if (this->PVSource->GetPartDisplay()->GetColorField() == vtkDataSet::POINT_DATA_FIELD)
      {
      attrInfo = dataInfo->GetPointDataInformation();
      }
    else
      {
      attrInfo = dataInfo->GetCellDataInformation();
      }
    vtkPVArrayInformation* arrayInfo = attrInfo->GetArrayInformation(colorMap->GetArrayName());      
    // First set of conditions.
    if (arrayInfo && arrayInfo->GetDataType() == VTK_UNSIGNED_CHAR)
      {
      // Number of component restriction.
      if (arrayInfo->GetNumberOfComponents() == 3)
        { // I would like to have two as an option also ...
        // One component causes more trouble than it is worth.
        this->MapScalarsCheckVisible = 1;
        this->MapScalarsCheck->SetState( ! this->PVSource->GetPartDisplay()->GetDirectColorFlag());
        }
      else
        { // Keep VTK from directly coloring single component arrays.
        this->PVSource->GetPartDisplay()->SetDirectColorFlag(0);
        }
      }
    }
    this->UpdateEnableState();    
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetRepresentation(const char* repr)
{
  if (!repr)
    {
    return;
    }
  if (!strcmp(repr, VTK_PV_WIREFRAME_LABEL) )
    {
    this->DrawWireframe();
    }
  else if (!strcmp(repr, VTK_PV_SURFACE_LABEL) )
    {
    this->DrawSurface();
    }
  else if (!strcmp(repr, VTK_PV_POINTS_LABEL) )
    {
    this->DrawPoints();
    }
  else if (!strcmp(repr, VTK_PV_OUTLINE_LABEL) )
    {
    this->DrawOutline();
    }
  else if (!strcmp(repr, VTK_PV_VOLUME_LABEL) )
    {
    this->DrawVolume();
    }
  else
    {
    vtkErrorMacro("Don't know the representation: " << repr);
    this->DrawSurface();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::DrawWireframe()
{
  if (this->GetPVSource()->GetInitialized())
    {
    this->AddTraceEntry("$kw(%s) DrawWireframe", this->GetTclName());
    }
  this->RepresentationMenu->SetValue(VTK_PV_WIREFRAME_LABEL);
  this->VolumeRenderModeOff();
  
  this->PVSource->GetPartDisplay()->SetRepresentation(VTK_WIREFRAME);

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::DrawPoints()
{
  if (this->GetPVSource()->GetInitialized())
    {
    this->AddTraceEntry("$kw(%s) DrawPoints", this->GetTclName());
    }
  this->RepresentationMenu->SetValue(VTK_PV_POINTS_LABEL);
  this->VolumeRenderModeOff();
  
  this->PVSource->GetPartDisplay()->SetRepresentation(VTK_POINTS);
  
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::DrawVolume()
{
  if (this->PVSource->GetDataInformation()->GetNumberOfCells() > 500000)
    {
    vtkWarningMacro("Sorry.  Unstructured grids with more than 500,000 "
                    "cells cannot currently be rendered with ParaView.  "
                    "Consider thresholding cells you are not interested in.");
    return;
    }
  if (this->GetPVSource()->GetInitialized())
    {
    this->AddTraceEntry("$kw(%s) DrawVolume", this->GetTclName());
    }
  this->RepresentationMenu->SetValue(VTK_PV_VOLUME_LABEL);
  this->VolumeRenderModeOn();
  
  this->PVSource->GetPartDisplay()->SetRepresentation(VTK_VOLUME);

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::DrawSurface()
{
  if (this->GetPVSource()->GetInitialized())
    {
    this->AddTraceEntry("$kw(%s) DrawSurface", this->GetTclName());
    }
  this->RepresentationMenu->SetValue(VTK_PV_SURFACE_LABEL);
  this->VolumeRenderModeOff();
  
  // fixme
  // It would be better to loop over part displays from the render module.
  this->PVSource->GetPartDisplay()->SetRepresentation(VTK_SURFACE);

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::DrawOutline()
{
  if (this->GetPVSource()->GetInitialized())
    {
    this->AddTraceEntry("$kw(%s) DrawOutline", this->GetTclName());
    }
  this->RepresentationMenu->SetValue(VTK_PV_OUTLINE_LABEL);
  this->VolumeRenderModeOff();
  
  this->PVSource->GetPartDisplay()->SetRepresentation(VTK_OUTLINE);

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}


//----------------------------------------------------------------------------
// Change visibility / enabled state of widgets for actor properties
//
void vtkPVDisplayGUI::VolumeRenderModeOff()
{
  this->Script("pack forget %s", 
               this->VolumeAppearanceFrame->GetWidgetName() );
  this->Script("pack forget %s", 
               this->ColorFrame->GetWidgetName() );

  this->Script("pack %s -after %s -fill x -expand t -pady 2", 
               this->ColorFrame->GetWidgetName(),
               this->ViewFrame->GetWidgetName() );

  // Make the color selection the same as the scalars we were just volume
  // rendering.
  if (this->VolumeRenderMode)
    {
    const char *colorSelection = this->VolumeScalarsMenu->GetValue();
    // The command is more specific about what is the variable name and
    // what is the number of vector entries.
    int menuIdx = this->VolumeScalarsMenu->GetMenu()->GetIndex(colorSelection);
    vtkStdString command(this->VolumeScalarsMenu->GetMenu()->GetItemCommand(menuIdx));

    // The form of the command is of the form
    // vtkTemp??? VolumeRender???Field {Field Name} NumComponents
    // The field name is between the first and last braces, and
    // the number of components is at the end of the string.
    vtkStdString::size_type firstbrace = command.find_first_of('{');
    vtkStdString::size_type lastbrace = command.find_last_of('}');
    vtkStdString name = command.substr(firstbrace+1, lastbrace-firstbrace-1);
    vtkStdString numCompsStr = command.substr(command.find_last_of(' '));
    int numComps;
    sscanf(numCompsStr.c_str(), "%d", &numComps);

    if (strncmp(colorSelection, "Point", 5) == 0)
      {
      this->ColorByPointField(name.c_str(), numComps);
      }
    else
      {
      this->ColorByCellField(name.c_str(), numComps);
      }
    }
  
  this->VolumeRenderMode = 0;
  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
// Change visibility / enabled state of widgets for volume properties
//
void vtkPVDisplayGUI::VolumeRenderModeOn()
{
  this->Script("pack forget %s", 
               this->VolumeAppearanceFrame->GetWidgetName() );
  this->Script("pack forget %s", 
               this->ColorFrame->GetWidgetName() );
  
  this->Script("pack %s -after %s -fill x -expand t -pady 2", 
               this->VolumeAppearanceFrame->GetWidgetName(),
               this->ViewFrame->GetWidgetName() );

  // Make scalar selection be the same for colors.
  if (!this->VolumeRenderMode)
    {
    const char *colorSelection = this->ColorMenu->GetValue();
    if (strcmp(colorSelection, "Property") != 0)
      {
      // The command is more specific about what is the variable name and
      // what is the number of vector entries.
      int menuIdx = this->ColorMenu->GetMenu()->GetIndex(colorSelection);
      vtkStdString command(this->ColorMenu->GetMenu()->GetItemCommand(menuIdx));

      // The form of the command is of the form
      // vtkTemp??? ColorBy???Field {Field Name} NumComponents
      // The field name is between the first and last braces, and
      // the number of components is at the end of the string.
      vtkStdString::size_type firstbrace = command.find_first_of('{');
      vtkStdString::size_type lastbrace = command.find_last_of('}');
      vtkStdString name = command.substr(firstbrace+1, lastbrace-firstbrace-1);
      vtkStdString numCompsStr = command.substr(command.find_last_of(' '));
      int numComps;
      sscanf(numCompsStr.c_str(), "%d", &numComps);

      if (strncmp(colorSelection, "Point", 5) == 0)
        {
        this->VolumeRenderPointField(name.c_str(), numComps);
        }
      else
        {
        this->VolumeRenderCellField(name.c_str(), numComps);
        }
      }
    }
  
  this->VolumeRenderMode = 1;
  this->UpdateEnableState();
}

  
//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetInterpolation(const char* repr)
{
  if (!repr)
    {
    return;
    }
  if (!strcmp(repr, "Flat") )
    {
    this->SetInterpolationToFlat();
    }
  else if (!strcmp(repr, "Gouraud") )
    {
    this->SetInterpolationToGouraud();
    }
  else
    {
    vtkErrorMacro("Don't know the interpolation: " << repr);
    this->DrawSurface();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetInterpolationToFlat()
{
  this->AddTraceEntry("$kw(%s) SetInterpolationToFlat", 
                      this->GetTclName());
  this->InterpolationMenu->SetValue("Flat");
  this->PVSource->GetPartDisplay()->SetInterpolation(VTK_FLAT);

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}


//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetInterpolationToGouraud()
{
  this->AddTraceEntry("$kw(%s) SetInterpolationToGouraud", 
                      this->GetTclName());
  this->InterpolationMenu->SetValue("Gouraud");

  this->PVSource->GetPartDisplay()->SetInterpolation(VTK_GOURAUD);
  
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
// Set up the default UI values.  Data information is valid by this time.
void vtkPVDisplayGUI::Initialize()
{
  if (this->PVSource == 0)
    {
    return;
    }
  double bounds[6];

  vtkDebugMacro( << "Initialize --------")
  
  this->GetPVSource()->GetDataInformation()->GetBounds(bounds);

  // Choose the representation based on the data.
  // Polydata is always surface.
  // Structured data is surface when 2d, outline when 3d.
  int dataSetType = this->GetPVSource()->GetDataInformation()->GetDataSetType();
  if (dataSetType == VTK_POLY_DATA)
    {
    this->SetRepresentation(VTK_PV_SURFACE_LABEL);
    }
  else if (dataSetType == VTK_STRUCTURED_GRID || 
           dataSetType == VTK_RECTILINEAR_GRID ||
           dataSetType == VTK_IMAGE_DATA)
    {
    int* ext = this->GetPVSource()->GetDataInformation()->GetExtent();
    if (ext[0] == ext[1] || ext[2] == ext[3] || ext[4] == ext[5])
      {
      this->SetRepresentation(VTK_PV_SURFACE_LABEL);
      }
    else
      {
      this->SetRepresentation(VTK_PV_OUTLINE_LABEL);
      }
    }
  else if (dataSetType == VTK_UNSTRUCTURED_GRID)
    {
    if (this->GetPVSource()->GetDataInformation()->GetNumberOfCells() 
        < this->GetPVRenderView()->GetRenderModuleUI()->GetOutlineThreshold())
      {
      this->SetRepresentation(VTK_PV_SURFACE_LABEL);
      }
    else
      {
      this->GetPVApplication()->GetMainWindow()->SetStatusText("Using outline for large unstructured grid.");
      this->SetRepresentation(VTK_PV_OUTLINE_LABEL);
      }
    }
  else
    {
    this->SetRepresentation(VTK_PV_OUTLINE_LABEL);
    this->ShouldReinitialize = 1;
    return;
    }

  this->ShouldReinitialize = 0;
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::CenterCamera()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkRenderer* ren = pvApp->GetProcessModule()->GetRenderModule()->GetRenderer();

  double bounds[6];
  this->GetPVSource()->GetDataInformation()->GetBounds(bounds);
  if (bounds[0]<=bounds[1] && bounds[2]<=bounds[3] && bounds[4]<=bounds[5])
    {
    vtkPVWindow* window = this->GetPVSource()->GetPVWindow();
    window->SetCenterOfRotation(0.5*(bounds[0]+bounds[1]), 
                                0.5*(bounds[2]+bounds[3]),
                                0.5*(bounds[4]+bounds[5]));
    window->ResetCenterCallback();

    ren->ResetCamera(bounds[0], bounds[1], bounds[2], 
                     bounds[3], bounds[4], bounds[5]);
    ren->ResetCameraClippingRange();
        
    if ( this->GetPVRenderView() )
      {
      this->GetPVRenderView()->EventuallyRender();
      }
    }
  
  this->AddTraceEntry("$kw(%s) CenterCamera", this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::VisibilityCheckCallback()
{
  this->GetPVSource()->SetVisibility(this->VisibilityCheck->GetState());
}

//----------------------------------------------------------------------------
vtkPVRenderView* vtkPVDisplayGUI::GetPVRenderView()
{
  if ( !this->GetPVSource() )
    {
    return 0;
    }
  return this->GetPVSource()->GetPVRenderView();
}

//----------------------------------------------------------------------------
vtkPVApplication* vtkPVDisplayGUI::GetPVApplication()
{
  if (this->GetApplication() == NULL)
    {
    return NULL;
    }
  
  if (this->GetApplication()->IsA("vtkPVApplication"))
    {  
    return (vtkPVApplication*)(this->GetApplication());
    }
  else
    {
    vtkErrorMacro("Bad typecast");
    return NULL;
    } 
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ScalarBarCheckCallback()
{
  if (this->PVSource == 0 || this->PVSource->GetPVColorMap() == 0)
    {
    vtkErrorMacro("Cannot find the color map.");
    return;
    }
  this->PVSource->GetPVColorMap()->SetScalarBarVisibility(
          this->ScalarBarCheck->GetState());
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::CubeAxesCheckCallback()
{
  //law int fixme;  // Loading the trace will not trace the visibility.
  // Move the tracing into vtkPVSource.
  this->AddTraceEntry("$kw(%s) SetCubeAxesVisibility %d", 
                      this->PVSource->GetTclName(),
                      this->CubeAxesCheck->GetState());
  this->PVSource->SetCubeAxesVisibility(this->CubeAxesCheck->GetState());
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::PointLabelCheckCallback()
{
  //law int fixme;  // Loading the trace will not trace the visibility.
  // Move the tracing into vtkPVSource.
  this->AddTraceEntry("$kw(%s) SetPointLabelVisibility %d", 
                      this->PVSource->GetTclName(),
                      this->PointLabelCheck->GetState());
  this->PVSource->SetPointLabelVisibility(this->PointLabelCheck->GetState());
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::MapScalarsCheckCallback()
{
  this->SetMapScalarsFlag(this->MapScalarsCheck->GetState());
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetMapScalarsFlag(int val)
{
  this->AddTraceEntry("$kw(%s) SetMapScalarsFlag %d", this->GetTclName(), val);
  if (this->MapScalarsCheck->GetState() != val)
    {
    this->MapScalarsCheck->SetState(val);
    }

  this->UpdateEnableState();

  this->PVSource->GetPartDisplay()->SetDirectColorFlag(!val);
  this->UpdateColorGUI();
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::InterpolateColorsCheckCallback()
{
  this->SetInterpolateColorsFlag(this->InterpolateColorsCheck->GetState());
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetInterpolateColorsFlag(int val)
{
  this->AddTraceEntry("$kw(%s) SetInterpolateColorsFlag %d", this->GetTclName(), val);
  if (this->InterpolateColorsCheck->GetState() != val)
    {
    this->InterpolateColorsCheck->SetState(val);
    }

  this->PVSource->GetPartDisplay()->SetInterpolateColorsFlag(val);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetPointSize(int size)
{
  if ( this->PointSizeThumbWheel->GetValue() == size )
    {
    return;
    }
  // The following call with trigger the ChangePointSize callback (below)
  // but won't add a trace entry. Let's do it. A trace entry is also
  // added by the ChangePointSizeEndCallback but this callback is only
  // called when the interaction on the scale is stopped.
  this->PointSizeThumbWheel->SetValue(size);
  this->AddTraceEntry("$kw(%s) SetPointSize %d", this->GetTclName(),
                      (int)(this->PointSizeThumbWheel->GetValue()));
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ChangePointSize()
{
  this->PVSource->GetPartDisplay()->SetPointSize(this->PointSizeThumbWheel->GetValue());
 
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
} 

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ChangePointSizeEndCallback()
{
  this->ChangePointSize();
  this->AddTraceEntry("$kw(%s) SetPointSize %d", this->GetTclName(),
                      (int)(this->PointSizeThumbWheel->GetValue()));
} 

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetLineWidth(int width)
{
  if ( this->LineWidthThumbWheel->GetValue() == width )
    {
    return;
    }
  // The following call with trigger the ChangeLineWidth callback (below)
  // but won't add a trace entry. Let's do it. A trace entry is also
  // added by the ChangeLineWidthEndCallback but this callback is only
  // called when the interaction on the scale is stopped.
  this->LineWidthThumbWheel->SetValue(width);
  this->AddTraceEntry("$kw(%s) SetLineWidth %d", this->GetTclName(),
                      (int)(this->LineWidthThumbWheel->GetValue()));
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ChangeLineWidth()
{
  this->PVSource->GetPartDisplay()->SetLineWidth(this->LineWidthThumbWheel->GetValue());

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ChangeLineWidthEndCallback()
{
  this->ChangeLineWidth();
  this->AddTraceEntry("$kw(%s) SetLineWidth %d", this->GetTclName(),
                      (int)(this->LineWidthThumbWheel->GetValue()));
}





//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetVolumeOpacityUnitDistance( double d )
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  vtkClientServerStream stream;

  vtkSMProxy *volume = 
    this->PVSource->GetPartDisplay()->GetVolumePropertyProxy();
  stream 
    << vtkClientServerStream::Invoke 
    << volume->GetID(0) << "SetScalarOpacityUnitDistance" << d 
    << vtkClientServerStream::End;
  pm->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ClearVolumeOpacity()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  vtkClientServerStream stream;

  vtkSMProxy *volume = this->PVSource->GetPartDisplay()->GetVolumeOpacityProxy();
  stream 
    << vtkClientServerStream::Invoke 
    << volume->GetID(0) << "RemoveAllPoints" 
    << vtkClientServerStream::End;
  pm->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::AddVolumeOpacity( double scalar, double opacity )
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  vtkClientServerStream stream;

  vtkSMProxy *volume = this->PVSource->GetPartDisplay()->GetVolumeOpacityProxy();
  stream 
    << vtkClientServerStream::Invoke 
    << volume->GetID(0) << "AddPoint" << scalar << opacity 
    << vtkClientServerStream::End;
  pm->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ClearVolumeColor()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  vtkClientServerStream stream;

  vtkSMProxy *volume = this->PVSource->GetPartDisplay()->GetVolumeColorProxy();
  stream 
    << vtkClientServerStream::Invoke 
    << volume->GetID(0) << "RemoveAllPoints" 
    << vtkClientServerStream::End;
  pm->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::AddVolumeColor( double scalar, double r, double g, double b )
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  vtkClientServerStream stream;

  vtkSMProxy *volume = this->PVSource->GetPartDisplay()->GetVolumeColorProxy();
  stream 
    << vtkClientServerStream::Invoke 
    << volume->GetID(0) << "AddRGBPoint" << scalar << r << g << b 
    << vtkClientServerStream::End;
  pm->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ColorMenu: " << this->ColorMenu << endl;
  os << indent << "VolumeScalarsMenu: " << this->VolumeScalarsMenu << endl;
  os << indent << "ResetCameraButton: " << this->ResetCameraButton << endl;
  os << indent << "EditColorMapButton: " << this->EditColorMapButton << endl;
  os << indent << "PVSource: " << this->GetPVSource() << endl;
  os << indent << "CubeAxesCheck: " << this->CubeAxesCheck << endl;
  os << indent << "PointLabelCheck: " << this->PointLabelCheck << endl;
  os << indent << "ScalarBarCheck: " << this->ScalarBarCheck << endl;
  os << indent << "RepresentationMenu: " << this->RepresentationMenu << endl;
  os << indent << "InterpolationMenu: " << this->InterpolationMenu << endl;
  os << indent << "ActorControlFrame: " << this->ActorControlFrame << endl;
  os << indent << "ArraySetByUser: " << this->ArraySetByUser << endl;
  os << indent << "ActorColor: " << this->ActorColor[0] << ", " << this->ActorColor[1]
               << ", " << this->ActorColor[2] << endl;
  os << indent << "ColorSetByUser: " << this->ColorSetByUser << endl;
  os << indent << "ShouldReinitialize: " << this->ShouldReinitialize << endl;
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetOpacity(float val)
{ 
  this->OpacityScale->SetValue(val);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::OpacityChangedCallback()
{
  this->PVSource->GetPartDisplay()->SetOpacity(this->OpacityScale->GetValue());

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::OpacityChangedEndCallback()
{
  this->OpacityChangedCallback();
  this->AddTraceEntry("$kw(%s) SetOpacity %f", 
                      this->GetTclName(), this->OpacityScale->GetValue());
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::GetActorTranslate(double* point)
{
  vtkSMPartDisplay* pDisp = this->PVSource->GetPartDisplay();
  if (pDisp)
    {
    pDisp->GetTranslate(point);
    }
  else
    {
    point[0] = this->TranslateThumbWheel[0]->GetValue();
    point[1] = this->TranslateThumbWheel[1]->GetValue();
    point[2] = this->TranslateThumbWheel[2]->GetValue();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetActorTranslateNoTrace(double x, double y, double z)
{
  this->TranslateThumbWheel[0]->SetValue(x);
  this->TranslateThumbWheel[1]->SetValue(y);
  this->TranslateThumbWheel[2]->SetValue(z);
  this->PVSource->GetPartDisplay()->SetTranslate(x, y, z);
  // Do not render here (do it in the callback, since it could be either
  // Render or EventuallyRender depending on the interaction)
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetActorTranslate(double x, double y, double z)
{
  this->SetActorTranslateNoTrace(x, y, z);
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }

  this->AddTraceEntry("$kw(%s) SetActorTranslate %f %f %f",
                      this->GetTclName(), x, y, z);  
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetActorTranslate(double* point)
{
  this->SetActorTranslate(point[0], point[1], point[2]);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ActorTranslateCallback()
{
  double point[3];
  point[0] = this->TranslateThumbWheel[0]->GetValue();
  point[1] = this->TranslateThumbWheel[1]->GetValue();
  point[2] = this->TranslateThumbWheel[2]->GetValue();
  this->SetActorTranslateNoTrace(point[0], point[1], point[2]);
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->Render();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ActorTranslateEndCallback()
{
  double point[3];
  point[0] = this->TranslateThumbWheel[0]->GetValue();
  point[1] = this->TranslateThumbWheel[1]->GetValue();
  point[2] = this->TranslateThumbWheel[2]->GetValue();
  this->SetActorTranslate(point);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::GetActorScale(double* point)
{
  vtkSMPartDisplay* pDisp = this->PVSource->GetPartDisplay();
  if (pDisp)
    {
    pDisp->GetScale(point);
    }
  else
    {
    point[0] = this->ScaleThumbWheel[0]->GetValue();
    point[1] = this->ScaleThumbWheel[1]->GetValue();
    point[2] = this->ScaleThumbWheel[2]->GetValue();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetActorScaleNoTrace(double x, double y, double z)
{
  this->ScaleThumbWheel[0]->SetValue(x);
  this->ScaleThumbWheel[1]->SetValue(y);
  this->ScaleThumbWheel[2]->SetValue(z);

  this->PVSource->GetPartDisplay()->SetScale(x, y, z);

  // Do not render here (do it in the callback, since it could be either
  // Render or EventuallyRender depending on the interaction)
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetActorScale(double x, double y, double z)
{
  this->SetActorScaleNoTrace(x, y, z);
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }

  this->AddTraceEntry("$kw(%s) SetActorScale %f %f %f",
                      this->GetTclName(), x, y, z);  
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetActorScale(double* point)
{
  this->SetActorScale(point[0], point[1], point[2]);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ActorScaleCallback()
{
  double point[3];
  point[0] = this->ScaleThumbWheel[0]->GetValue();
  point[1] = this->ScaleThumbWheel[1]->GetValue();
  point[2] = this->ScaleThumbWheel[2]->GetValue();
  this->SetActorScaleNoTrace(point[0], point[1], point[2]);
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->Render();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ActorScaleEndCallback()
{
  double point[3];
  point[0] = this->ScaleThumbWheel[0]->GetValue();
  point[1] = this->ScaleThumbWheel[1]->GetValue();
  point[2] = this->ScaleThumbWheel[2]->GetValue();
  this->SetActorScale(point);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::GetActorOrientation(double* point)
{
  vtkSMPartDisplay* pDisp = this->PVSource->GetPartDisplay();
  if (pDisp)
    {
    pDisp->GetOrientation(point);
    }
  else
    {
    point[0] = this->OrientationScale[0]->GetValue();
    point[1] = this->OrientationScale[1]->GetValue();
    point[2] = this->OrientationScale[2]->GetValue();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetActorOrientationNoTrace(double x, double y, double z)
{
  this->OrientationScale[0]->SetValue(x);
  this->OrientationScale[1]->SetValue(y);
  this->OrientationScale[2]->SetValue(z);
  this->PVSource->GetPartDisplay()->SetOrientation(x, y, z);

  // Do not render here (do it in the callback, since it could be either
  // Render or EventuallyRender depending on the interaction)
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetActorOrientation(double x, double y, double z)
{
  this->SetActorOrientationNoTrace(x, y, z);
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }

  this->AddTraceEntry("$kw(%s) SetActorOrientation %f %f %f",
                      this->GetTclName(), x, y, z);  
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetActorOrientation(double* point)
{
  this->SetActorOrientation(point[0], point[1], point[2]);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ActorOrientationCallback()
{
  double point[3];
  point[0] = this->OrientationScale[0]->GetValue();
  point[1] = this->OrientationScale[1]->GetValue();
  point[2] = this->OrientationScale[2]->GetValue();
  this->SetActorOrientationNoTrace(point[0], point[1], point[2]);
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->Render();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ActorOrientationEndCallback()
{
  double point[3];
  point[0] = this->OrientationScale[0]->GetValue();
  point[1] = this->OrientationScale[1]->GetValue();
  point[2] = this->OrientationScale[2]->GetValue();
  this->SetActorOrientation(point);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::GetActorOrigin(double* point)
{
  if (this->PVSource->GetPartDisplay())
    {
    this->PVSource->GetPartDisplay()->GetOrigin(point);
    }
  else
    {
    point[0] = this->OriginThumbWheel[0]->GetValue();
    point[1] = this->OriginThumbWheel[1]->GetValue();
    point[2] = this->OriginThumbWheel[2]->GetValue();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetActorOriginNoTrace(double x, double y, double z)
{
  this->OriginThumbWheel[0]->SetValue(x);
  this->OriginThumbWheel[1]->SetValue(y);
  this->OriginThumbWheel[2]->SetValue(z);
  this->PVSource->GetPartDisplay()->SetOrigin(x, y, z);

  // Do not render here (do it in the callback, since it could be either
  // Render or EventuallyRender depending on the interaction)
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetActorOrigin(double x, double y, double z)
{
  this->SetActorOriginNoTrace(x, y, z);
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }

  this->AddTraceEntry("$kw(%s) SetActorOrigin %f %f %f",
                      this->GetTclName(), x, y, z);  
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SetActorOrigin(double* point)
{
  this->SetActorOrigin(point[0], point[1], point[2]);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ActorOriginCallback()
{
  double point[3];
  point[0] = this->OriginThumbWheel[0]->GetValue();
  point[1] = this->OriginThumbWheel[1]->GetValue();
  point[2] = this->OriginThumbWheel[2]->GetValue();
  this->SetActorOriginNoTrace(point[0], point[1], point[2]);
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->Render();
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::ActorOriginEndCallback()
{
  double point[3];
  point[0] = this->OriginThumbWheel[0]->GetValue();
  point[1] = this->OriginThumbWheel[1]->GetValue();
  point[2] = this->OriginThumbWheel[2]->GetValue();
  this->SetActorOrigin(point);
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdateActorControl()
{
  int i;
  double translate[3];
  double scale[3];
  double origin[3];
  double orientation[3];
  
  this->PVSource->GetPartDisplay()->GetTranslate(translate);
  this->PVSource->GetPartDisplay()->GetScale(scale);
  this->PVSource->GetPartDisplay()->GetOrientation(orientation);
  this->PVSource->GetPartDisplay()->GetOrigin(origin);
  for (i = 0; i < 3; i++)
    {    
    this->TranslateThumbWheel[i]->SetValue(translate[i]);
    this->ScaleThumbWheel[i]->SetValue(scale[i]);
    this->OrientationScale[i]->SetValue(orientation[i]);
    this->OriginThumbWheel[i]->SetValue(origin[i]);
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::UpdateActorControlResolutions()
{
  double bounds[6];
  this->GetPVSource()->GetDataInformation()->GetBounds(bounds);

  double res, oneh, half;

  // Update the resolution according to the bounds
  // Set res to 1/20 of the range, rounding to nearest .1 or .5 form.

  int i;
  for (i = 0; i < 3; i++)
    {
    double delta = bounds[i * 2 + 1] - bounds[i * 2];
    if (delta <= 0)
      {
      res = 0.1;
      }
    else
      {
      oneh = log10(delta * 0.051234);
      half = 0.5 * pow(10.0, ceil(oneh));
      res = (oneh > log10(half) ? half : pow(10.0, floor(oneh)));
      // cout << "up i: " << i << ", delta: " << delta << ", oneh: " << oneh << ", half: " << half << ", res: " << res << endl;
      }
    this->TranslateThumbWheel[i]->SetResolution(res);
    this->OriginThumbWheel[i]->SetResolution(res);
    }
}

void vtkPVDisplayGUI::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->ColorFrame);
  this->PropagateEnableState(this->VolumeAppearanceFrame);
  this->PropagateEnableState(this->DisplayStyleFrame);
  this->PropagateEnableState(this->ViewFrame);
  this->PropagateEnableState(this->ColorMenuLabel);
  this->PropagateEnableState(this->ColorMenu);
  this->PropagateEnableState(this->VolumeScalarsMenuLabel);
  this->PropagateEnableState(this->VolumeScalarsMenu);
  if ( this->EditColorMapButtonVisible )
    {
    this->PropagateEnableState(this->EditColorMapButton);
    this->PropagateEnableState(this->DataColorRangeButton);
    }
  else
    {
    this->EditColorMapButton->SetEnabled(0);
    this->DataColorRangeButton->SetEnabled(0);
    }
  if ( this->MapScalarsCheckVisible)
    {
    this->PropagateEnableState(this->MapScalarsCheck);
    }
  else
    {
    this->MapScalarsCheck->SetEnabled(0);
    }
  if ( this->InterpolateColorsCheckVisible)
    {
    this->PropagateEnableState(this->InterpolateColorsCheck);
    }
  else
    {
    this->InterpolateColorsCheck->SetEnabled(0);
    }
  if (this->ColorButtonVisible)
    {
    this->PropagateEnableState(this->ColorButton);
    }
  else
    {
    this->ColorButton->SetEnabled(0);
    }
  this->PropagateEnableState(this->RepresentationMenuLabel);
  this->PropagateEnableState(this->RepresentationMenu);
  this->PropagateEnableState(this->VisibilityCheck);
  if ( this->ScalarBarCheckVisible )
    {
    this->PropagateEnableState(this->ScalarBarCheck);
    }
  else
    {
    this->ScalarBarCheck->SetEnabled(0);
    }
  this->PropagateEnableState(this->ActorControlFrame);
  this->PropagateEnableState(this->TranslateLabel);
  this->PropagateEnableState(this->ScaleLabel);
  this->PropagateEnableState(this->OrientationLabel);
  this->PropagateEnableState(this->OriginLabel);
  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->PropagateEnableState(this->TranslateThumbWheel[cc]);
    this->PropagateEnableState(this->ScaleThumbWheel[cc]);
    this->PropagateEnableState(this->OrientationScale[cc]);
    this->PropagateEnableState(this->OriginThumbWheel[cc]);
    }
  this->PropagateEnableState(this->CubeAxesCheck);
  this->PropagateEnableState(this->PointLabelCheck);
  this->PropagateEnableState(this->ResetCameraButton);
  
  if ( this->VolumeRenderMode )
    {
    this->InterpolationMenuLabel->SetEnabled(0);
    this->InterpolationMenu->SetEnabled(0);
    this->LineWidthLabel->SetEnabled(0);
    this->LineWidthThumbWheel->SetEnabled(0);
    this->PointSizeLabel->SetEnabled(0);
    this->PointSizeThumbWheel->SetEnabled(0);
    this->OpacityLabel->SetEnabled(0);
    this->OpacityScale->SetEnabled(0);
    }
  else
    {
    this->PropagateEnableState(this->InterpolationMenuLabel);
    this->PropagateEnableState(this->InterpolationMenu);
    this->PropagateEnableState(this->PointSizeLabel);
    this->PropagateEnableState(this->PointSizeThumbWheel);
    this->PropagateEnableState(this->LineWidthLabel);
    this->PropagateEnableState(this->LineWidthThumbWheel);
    this->PropagateEnableState(this->OpacityLabel);
    this->PropagateEnableState(this->OpacityScale);
    }
  
}

//----------------------------------------------------------------------------
void vtkPVDisplayGUI::SaveVolumeRenderStateDisplay(ofstream *file)
{

  if(this->VolumeRenderMode)
    {
    *file << "[$kw(" << this->GetPVSource()->GetTclName()
          << ") GetPVOutput] DrawVolume" << endl;
//    *file << "[$kw(" << this->GetPVSource()->GetTclName()
//          << ") GetPVOutput] ShowVolumeAppearanceEditor" << endl;

    vtkStdString command(this->VolumeScalarsMenu->GetValue());

    // The form of the command is of the form
    // vtkTemp??? ColorBy???Field {Field Name} NumComponents
    // The field name is between the first and last braces, and
    // the number of components is at the end of the string.
    vtkStdString::size_type firstspace = command.find_first_of(' ');
    vtkStdString::size_type lastspace = command.find_last_of(' ');
    vtkStdString name = command.substr(firstspace+1, lastspace-firstspace-1);
  //  vtkStdString numCompsStr = command.substr(command.find_last_of(' '));

    // have to visit this panel in order to initialize vars
//    this->ShowVolumeAppearanceEditor();

    if ( command.c_str() && strlen(command.c_str()) > 6 )
      {
      vtkPVDataInformation* dataInfo = this->PVSource->GetDataInformation();
      vtkPVArrayInformation *arrayInfo;
      int colorField = this->PVSource->GetPartDisplay()->GetColorField();
      if (colorField == vtkDataSet::POINT_DATA_FIELD)
        {
        vtkPVDataSetAttributesInformation *attrInfo
          = dataInfo->GetPointDataInformation();
        arrayInfo = attrInfo->GetArrayInformation(name.c_str());
        if(arrayInfo)
          *file << "[$kw(" << this->GetPVSource()->GetTclName() << ") GetPVOutput] VolumeRenderPointField {" << arrayInfo->GetName() << "} " << arrayInfo->GetNumberOfComponents() << endl;
        }
      else
        {
        vtkPVDataSetAttributesInformation *attrInfo
          = dataInfo->GetCellDataInformation();
        arrayInfo = attrInfo->GetArrayInformation(name.c_str());
        if(arrayInfo)
          *file << "[$kw(" << this->GetPVSource()->GetTclName() << ") GetPVOutput] VolumeRenderCellField {" << arrayInfo->GetName() << "} " << arrayInfo->GetNumberOfComponents() << endl;
        }
      }
    }
}
