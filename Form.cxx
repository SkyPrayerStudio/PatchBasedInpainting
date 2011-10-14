/*=========================================================================
 *
 *  Copyright David Doria 2011 daviddoria@gmail.com
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#include "ui_CriminisiInpainting.h"
#include "Form.h"

// ITK
#include "itkCastImageFilter.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkMaskImageFilter.h"
#include "itkRegionOfInterestImageFilter.h"
#include "itkVector.h"

// Qt
#include <QFileDialog>
#include <QIcon>
#include <QTextEdit>

// VTK
#include <vtkActor.h>
#include <vtkArrowSource.h>
#include <vtkCamera.h>
#include <vtkFloatArray.h>
#include <vtkGlyph2D.h>
#include <vtkImageData.h>
#include <vtkImageProperty.h>
#include <vtkImageSlice.h>
#include <vtkImageSliceMapper.h>
#include <vtkLookupTable.h>
#include <vtkMath.h>
#include <vtkPointData.h>
#include <vtkProperty2D.h>
#include <vtkPolyDataMapper.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>
#include <vtkImageSliceMapper.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkXMLImageDataWriter.h> // For debugging only

// Custom
#include "Helpers.h"
#include "InteractorStyleImageNoLevel.h"
#include "Mask.h"
#include "Types.h"

const unsigned char Form::Green[3] = {0,255,0};
const unsigned char Form::Red[3] = {255,0,0};


void Form::on_actionHelp_activated()
{
  QTextEdit* help=new QTextEdit();
  
  help->setReadOnly(true);
  help->append("<h1>Criminisi Inpainting</h1>\
  Load and image and a mask. <br/>\
  Set the settings such as patch size. <br/>\
  Click Inpaint.<br/> <p/>"
  );
  help->show();
}

// Constructor
Form::Form()
{
  this->setupUi(this);

  SetCheckboxVisibility(false);
  
  this->TargetPatchScene = new QGraphicsScene();
  this->gfxTarget->setScene(TargetPatchScene);
 
  this->SourcePatchScene = new QGraphicsScene();
  this->gfxSource->setScene(SourcePatchScene);
  
  this->CurrentUsedPatchDisplayed = 0;
  
  this->DebugImages = false;
  this->DebugMessages = false;

  // Setup icons
  QIcon openIcon = QIcon::fromTheme("document-open");
  QIcon saveIcon = QIcon::fromTheme("document-save");
  
  // Setup toolbar
  actionOpenImage->setIcon(openIcon);
  this->toolBar->addAction(actionOpenImage);

  actionOpenMask->setIcon(openIcon);
  this->toolBar->addAction(actionOpenMask);
  actionOpenMask->setEnabled(false);

  actionSaveResult->setIcon(saveIcon);
  this->toolBar->addAction(actionSaveResult);

  this->Flipped = false;
  
  this->InteractorStyle = vtkSmartPointer<InteractorStyleImageNoLevel>::New();
    
  // An image with potential target patches outlined
  this->PotentialTargetPatchesImage = UnsignedCharScalarImageType::New();
    
  // Add objects to the renderer
  this->Renderer = vtkSmartPointer<vtkRenderer>::New();
  this->qvtkWidget->GetRenderWindow()->AddRenderer(this->Renderer);
  
  this->Renderer->AddViewProp(this->ImageLayer.ImageSlice);
  this->Renderer->AddViewProp(this->ConfidenceLayer.ImageSlice);
  this->Renderer->AddViewProp(this->ConfidenceMapLayer.ImageSlice);
  this->Renderer->AddViewProp(this->BoundaryLayer.ImageSlice);
  this->Renderer->AddViewProp(this->PriorityLayer.ImageSlice);
  this->Renderer->AddViewProp(this->DataLayer.ImageSlice);
  this->Renderer->AddViewProp(this->IsophoteLayer.Actor);
  this->Renderer->AddViewProp(this->BoundaryNormalsLayer.Actor);
  this->Renderer->AddViewProp(this->MaskLayer.ImageSlice);
  this->Renderer->AddViewProp(this->PotentialPatchesLayer.ImageSlice);
  this->Renderer->AddViewProp(this->TargetPatchLayer.ImageSlice);
  this->Renderer->AddViewProp(this->SourcePatchLayer.ImageSlice);

  this->InteractorStyle->SetCurrentRenderer(this->Renderer);
  this->qvtkWidget->GetRenderWindow()->GetInteractor()->SetInteractorStyle(this->InteractorStyle);
  
  this->UserImage = FloatVectorImageType::New();
  this->UserMaskImage = Mask::New();
  
  connect(&ComputationThread, SIGNAL(StartProgressSignal()), this, SLOT(StartProgressSlot()), Qt::QueuedConnection);
  connect(&ComputationThread, SIGNAL(StopProgressSignal()), this, SLOT(StopProgressSlot()), Qt::QueuedConnection);
  connect(&ComputationThread, SIGNAL(IterationCompleteSignal()), this, SLOT(IterationCompleteSlot()), Qt::QueuedConnection);
  
  // Using a blocking connection allows everything (computation and drawing) to be performed sequentially which is helpful for debugging, but makes the interface very very choppy.
  // We are assuming that the computation takes longer than the drawing.
  //connect(&ComputationThread, SIGNAL(IterationCompleteSignal()), this, SLOT(IterationCompleteSlot()), Qt::BlockingQueuedConnection);
  
  connect(&ComputationThread, SIGNAL(RefreshSignal()), this, SLOT(RefreshSlot()), Qt::QueuedConnection);

  // Set the progress bar to marquee mode
  this->progressBar->setMinimum(0);
  this->progressBar->setMaximum(0);
  this->progressBar->hide();
  
  ComputationThread.SetObject(&(this->Inpainting));
};
  
void Form::on_chkDebugImages_clicked()
{
  QDir directoryMaker;
  directoryMaker.mkdir("Debug");
  
  this->Inpainting.SetDebugImages(this->chkDebugImages->isChecked());
  this->DebugImages = this->chkDebugImages->isChecked();
  std::cout << "this->DebugImages: " << this->DebugImages << std::endl;
}

void Form::on_chkDebugMessages_clicked()
{
  this->Inpainting.SetDebugMessages(this->chkDebugMessages->isChecked());
  this->DebugMessages = this->chkDebugMessages->isChecked();
}

void Form::on_actionQuit_activated()
{
  exit(0);
}

void Form::on_actionSaveResult_activated()
{
  // Get a filename to save
  QString fileName = QFileDialog::getSaveFileName(this, "Save File", ".", "Image Files (*.jpg *.jpeg *.bmp *.png *.mha)");

  DebugMessage<std::string>("Got filename: ", fileName.toStdString());
  if(fileName.toStdString().empty())
    {
    std::cout << "Filename was empty." << std::endl;
    return;
    }
    
  Helpers::WriteImage<FloatVectorImageType>(this->Inpainting.GetResult(), fileName.toStdString());
  
  this->statusBar()->showMessage("Saved result.");
}

void Form::StartProgressSlot()
{
  //std::cout << "Form::StartProgressSlot()" << std::endl;
  // Connected to the StartProgressSignal of the ProgressThread member
  this->progressBar->show();
}

void Form::StopProgressSlot()
{
  //std::cout << "Form::StopProgressSlot()" << std::endl;
  // When the ProgressThread emits the StopProgressSignal, we need to display the result of the segmentation

  this->progressBar->hide();
}

void Form::on_actionOpenImage_activated()
{
  // Get a filename to open
  QString fileName = QFileDialog::getOpenFileName(this, "Open File", ".", "Image Files (*.jpg *.jpeg *.bmp *.png *.mha);;PNG Files (*.png)");

  DebugMessage<std::string>("Got filename: ", fileName.toStdString());
  if(fileName.toStdString().empty())
    {
    std::cout << "Filename was empty." << std::endl;
    return;
    }

  // Set the working directory
  QFileInfo fileInfo(fileName);
  std::string workingDirectory = fileInfo.absoluteDir().absolutePath().toStdString() + "/";
  DebugMessage<std::string>("Working directory set to: ", workingDirectory);
  QDir::setCurrent(QString(workingDirectory.c_str()));
    
  typedef itk::ImageFileReader<FloatVectorImageType> ReaderType;
  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName(fileName.toStdString());
  reader->Update();

  //this->Image = reader->GetOutput();
  Helpers::DeepCopyVectorImage<FloatVectorImageType>(reader->GetOutput(), this->UserImage);
  
  Helpers::ITKVectorImagetoVTKImage(this->UserImage, this->ImageLayer.ImageData);
  
  this->Inpainting.SetImage(this->UserImage);
    
  this->Renderer->ResetCamera();
  this->qvtkWidget->GetRenderWindow()->Render();
  
  this->statusBar()->showMessage("Opened image.");
  actionOpenMask->setEnabled(true);
  
  if(this->UserImage && this->UserMaskImage && this->UserImage->GetLargestPossibleRegion() == this->UserMaskImage->GetLargestPossibleRegion())
    {
    SetupInitialIntermediateImages();
    SetCheckboxVisibility(true);
    }
  else
    {
    SetCheckboxVisibility(false);
    }
}


void Form::on_actionOpenMaskInverted_activated()
{
  std::cout << "on_actionOpenMaskInverted_activated()" << std::endl;
  on_actionOpenMask_activated();
  this->UserMaskImage->Invert();
  this->UserMaskImage->Cleanup();
  
  this->Inpainting.SetMask(this->UserMaskImage);
  Helpers::DebugWriteImageConditional<Mask>(this->UserMaskImage, "Debug/InvertedMask.png", this->DebugImages);
  
}


void Form::on_actionOpenMask_activated()
{
  // Get a filename to open
  QString fileName = QFileDialog::getOpenFileName(this, "Open File", ".", "Image Files (*.png *.bmp)");

  DebugMessage<std::string>("Got filename: ", fileName.toStdString());
  if(fileName.toStdString().empty())
    {
    std::cout << "Filename was empty." << std::endl;
    return;
    }

  typedef itk::ImageFileReader<Mask> ReaderType;
  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName(fileName.toStdString());
  reader->Update();
  
  if(this->UserImage->GetLargestPossibleRegion() != reader->GetOutput()->GetLargestPossibleRegion())
    {
    std::cerr << "Image and mask must be the same size!" << std::endl;
    return;
    }

  Helpers::DeepCopy<Mask>(reader->GetOutput(), this->UserMaskImage);
  
  // For this program, we ALWAYS assume the hole to be filled is white, and the valid/source region is black.
  // This is not simply reversible because of some subtle erosion operations that are performed.
  // For this reason, we provide an "load inverted mask" action in the file menu.
  this->UserMaskImage->SetValidValue(0);
  this->UserMaskImage->SetHoleValue(255);
  
  this->UserMaskImage->Cleanup();

  // This is only set here so we can visualize the mask right away
  this->Inpainting.SetMask(this->UserMaskImage);
  
  this->statusBar()->showMessage("Opened mask.");
  
  if(this->UserImage && this->UserMaskImage && this->UserImage->GetLargestPossibleRegion() == this->UserMaskImage->GetLargestPossibleRegion())
    {
    SetupInitialIntermediateImages();
    SetCheckboxVisibility(true);
    }
  else
    {
    SetCheckboxVisibility(false);
    }
}

void Form::ExtractIsophotesForDisplay()
{
  if(this->IntermediateImages[this->CurrentUsedPatchDisplayed].Isophotes->GetLargestPossibleRegion().GetSize()[0] != 0)
    {
    // Mask the isophotes image with the current boundary, because we only want to display the isophotes we are interested in.
    FloatVector2ImageType::Pointer normalizedIsophotes = FloatVector2ImageType::New();
    Helpers::DeepCopy<FloatVector2ImageType>(this->IntermediateImages[this->CurrentUsedPatchDisplayed].Isophotes, normalizedIsophotes);
    Helpers::NormalizeVectorImage(normalizedIsophotes);

    typedef itk::MaskImageFilter< FloatVector2ImageType, UnsignedCharScalarImageType, FloatVector2ImageType> MaskFilterType;
    typename MaskFilterType::Pointer maskFilter = MaskFilterType::New();
    maskFilter->SetInput(normalizedIsophotes);
    maskFilter->SetMaskImage(this->IntermediateImages[this->CurrentUsedPatchDisplayed].Boundary);
    FloatVector2ImageType::PixelType zero;
    zero.Fill(0);
    maskFilter->SetOutsideValue(zero);
    maskFilter->Update();
  
    Helpers::DebugWriteImageConditional<FloatVector2ImageType>(maskFilter->GetOutput(), "Debug/ShowIsophotes.BoundaryIsophotes.mha", this->DebugImages);
    Helpers::DebugWriteImageConditional<UnsignedCharScalarImageType>(this->IntermediateImages[this->CurrentUsedPatchDisplayed].Boundary, "Debug/ShowIsophotes.Boundary.mha", this->DebugImages);
    
    Helpers::ConvertNonZeroPixelsToVectors(maskFilter->GetOutput(), this->IsophoteLayer.Vectors);
    
    if(this->DebugImages)
      {
      vtkSmartPointer<vtkXMLImageDataWriter> writer = vtkSmartPointer<vtkXMLImageDataWriter>::New();
      writer->SetFileName("Debug/VTKIsophotes.vti");
      writer->SetInputConnection(this->IsophoteLayer.ImageData->GetProducerPort());
      writer->Write();
    
      vtkSmartPointer<vtkXMLPolyDataWriter> polyDataWriter = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
      polyDataWriter->SetFileName("Debug/VTKIsophotes.vtp");
      polyDataWriter->SetInputConnection(this->IsophoteLayer.Vectors->GetProducerPort());
      polyDataWriter->Write();
      }
    } 
}

void Form::DisplayMask()
{
  vtkSmartPointer<vtkImageData> temp = vtkSmartPointer<vtkImageData>::New();
  Helpers::ITKScalarImageToScaledVTKImage<Mask>(this->IntermediateImages[this->CurrentUsedPatchDisplayed].MaskImage, temp);  
  Helpers::MakePixelsTransparent(temp, this->MaskLayer.ImageData, 0); // Set the zero pixels of the mask to transparent
}

void Form::DisplayConfidence()
{
  vtkSmartPointer<vtkImageData> temp = vtkSmartPointer<vtkImageData>::New();
  Helpers::ITKScalarImageToScaledVTKImage<FloatScalarImageType>(this->IntermediateImages[this->CurrentUsedPatchDisplayed].Confidence, temp);  
  Helpers::MakePixelsTransparent(temp, this->ConfidenceLayer.ImageData, 0); // Set the zero pixels to transparent
}

void Form::DisplayConfidenceMap()
{
  Helpers::ITKScalarImageToScaledVTKImage<FloatScalarImageType>(this->IntermediateImages[this->CurrentUsedPatchDisplayed].ConfidenceMap, this->ConfidenceMapLayer.ImageData);
}

void Form::DisplayImage()
{
  Helpers::ITKVectorImagetoVTKImage(this->IntermediateImages[this->CurrentUsedPatchDisplayed].Image, this->ImageLayer.ImageData);
}

void Form::DisplayBoundary()
{
  Helpers::ITKScalarImageToScaledVTKImage<UnsignedCharScalarImageType>(this->IntermediateImages[this->CurrentUsedPatchDisplayed].Boundary, this->BoundaryLayer.ImageData);
}

void Form::DisplayPriority()
{
  vtkSmartPointer<vtkImageData> temp = vtkSmartPointer<vtkImageData>::New();
  Helpers::ITKScalarImageToScaledVTKImage<FloatScalarImageType>(this->IntermediateImages[this->CurrentUsedPatchDisplayed].Priority, temp);
  Helpers::MakePixelsTransparent(temp, this->PriorityLayer.ImageData, 0); // Set the zero pixels to transparent
}

void Form::DisplayData()
{
  vtkSmartPointer<vtkImageData> temp = vtkSmartPointer<vtkImageData>::New();
  Helpers::ITKScalarImageToScaledVTKImage<FloatScalarImageType>(this->IntermediateImages[this->CurrentUsedPatchDisplayed].Data, temp);
  Helpers::MakePixelsTransparent(temp, this->DataLayer.ImageData, 0); // Set the zero pixels to transparent
}

void Form::RefreshSlot()
{
  DebugMessage("RefreshSlot()");

  Refresh();
  
}

void Form::DisplayBoundaryNormals()
{
  if(this->Inpainting.GetBoundaryNormalsImage()->GetLargestPossibleRegion().GetSize()[0] != 0)
    {
    Helpers::ConvertNonZeroPixelsToVectors(this->IntermediateImages[this->CurrentUsedPatchDisplayed].BoundaryNormals, this->BoundaryNormalsLayer.Vectors);
  
    if(this->DebugImages)
      {
      std::cout << "Writing boundary normals..." << std::endl;
    
      Helpers::WriteImage<FloatVector2ImageType>(this->Inpainting.GetBoundaryNormalsImage(), "Debug/RefreshSlot.BoundaryNormals.mha");
    
      vtkSmartPointer<vtkXMLImageDataWriter> writer = vtkSmartPointer<vtkXMLImageDataWriter>::New();
      writer->SetFileName("Debug/RefreshSlot.VTKBoundaryNormals.vti");
      writer->SetInputConnection(this->BoundaryNormalsLayer.ImageData->GetProducerPort());
      writer->Write();
    
      vtkSmartPointer<vtkXMLPolyDataWriter> polyDataWriter = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
      polyDataWriter->SetFileName("Debug/RefreshSlot.VTKBoundaryNormals.vtp");
      polyDataWriter->SetInputConnection(this->BoundaryNormalsLayer.Vectors->GetProducerPort());
      polyDataWriter->Write();
      }
    }  
}

void Form::Refresh()
{
  DebugMessage("Refresh()");

  this->ImageLayer.ImageSlice->SetVisibility(this->chkImage->isChecked());
  DisplayImage();

  this->MaskLayer.ImageSlice->SetVisibility(this->chkMask->isChecked());
  DisplayMask();

  this->ConfidenceMapLayer.ImageSlice->SetVisibility(this->chkConfidenceMap->isChecked());
  DisplayConfidenceMap();

  this->ConfidenceLayer.ImageSlice->SetVisibility(this->chkConfidence->isChecked());
  DisplayConfidence();

  this->PriorityLayer.ImageSlice->SetVisibility(this->chkPriority->isChecked());
  DisplayPriority();

  this->BoundaryLayer.ImageSlice->SetVisibility(this->chkBoundary->isChecked());
  DisplayBoundary();

  this->IsophoteLayer.Actor->SetVisibility(this->chkIsophotes->isChecked());
  ExtractIsophotesForDisplay();

  this->DataLayer.ImageSlice->SetVisibility(this->chkData->isChecked());
  DisplayData();

  this->BoundaryNormalsLayer.Actor->SetVisibility(this->chkBoundaryNormals->isChecked());

  DisplayBoundaryNormals();

  this->PotentialPatchesLayer.ImageSlice->SetVisibility(this->chkPotentialPatches->isChecked());
  //Helpers::ITKScalarImageToScaledVTKImage<FloatScalarImageType>(this->Inpainting.GetDataImage(), this->VTKDataImage);
  
  this->SourcePatchLayer.ImageSlice->SetVisibility(this->chkHighlightUsedPatches->isChecked());
  this->TargetPatchLayer.ImageSlice->SetVisibility(this->chkHighlightUsedPatches->isChecked());
    
  this->qvtkWidget->GetRenderWindow()->Render();
  //this->Renderer->Render();
  
}

void Form::on_btnStop_clicked()
{
  this->ComputationThread.StopInpainting();
}

void Form::on_btnReset_clicked()
{
  this->Inpainting.SetImage(this->UserImage);
  this->Inpainting.SetMask(this->UserMaskImage);
  RefreshSlot();
}
  
void Form::on_btnStep_clicked()
{
  this->Inpainting.SetDebugImages(this->chkDebugImages->isChecked());
  this->Inpainting.SetDebugMessages(this->chkDebugMessages->isChecked());
  this->Inpainting.Iterate();
  
  IterationComplete();
}

void Form::on_btnInitialize_clicked()
{
  Initialize();
}

void Form::Initialize()
{
  // Reset some things (this is so that if we want to run another completion it will work normally)

  if(!this->UserImage || !this->UserMaskImage || this->UserImage->GetLargestPossibleRegion() != this->UserMaskImage->GetLargestPossibleRegion())
    {
    std::cerr << "Must have loaded both an image and a mask and they must be the same size!" << std::endl;
    return;
    }

  this->Inpainting.SetPatchRadius(this->txtPatchRadius->text().toUInt());
  this->Inpainting.SetDebugImages(this->chkDebugImages->isChecked());
  this->Inpainting.SetDebugMessages(this->chkDebugMessages->isChecked());
  this->Inpainting.SetImage(this->UserImage);
  this->Inpainting.SetMask(this->UserMaskImage);

  this->Inpainting.Initialize();
}

void Form::on_btnInpaint_clicked()
{
  DebugMessage("on_btnInpaint_clicked()");
  
  Initialize();
  
  Refresh();
  
  DebugMessage("Starting ComputationThread...");
  ComputationThread.start();
}


void Form::DebugMessage(const std::string& message)
{
  if(this->DebugMessages)
    {
    std::cout << message << std::endl;
    }
}

void Form::on_btnDisplayPreviousStep_clicked()
{
  if(this->CurrentUsedPatchDisplayed > 0)
    {
    this->CurrentUsedPatchDisplayed--;
    }
  ChangeDisplayedIteration();
}

void Form::on_btnDisplayNextStep_clicked()
{
  //std::cout << "CurrentUsedPatchDisplayed: " << this->CurrentUsedPatchDisplayed
    //        << " Inpainting iteration: " <<  static_cast<int>(this->Inpainting.GetIteration()) << std::endl;
  
  if(this->CurrentUsedPatchDisplayed < static_cast<int>(this->Inpainting.GetNumberOfCompletedIterations()) - 1)
    {
    this->CurrentUsedPatchDisplayed++;
    }
  ChangeDisplayedIteration();
}

void Form::DisplayUsedPatches()
{
  DebugMessage("DisplayUsedPatches()");
  PatchPair patchPair;
  this->Inpainting.GetUsedPatchPair(this->CurrentUsedPatchDisplayed, patchPair);

  FloatVectorImageType::Pointer currentImage = this->IntermediateImages[this->CurrentUsedPatchDisplayed].Image;
  
  // If we have chosen to display the masked target patch, we need to use the mask from the previous iteration (as the current mask has been cleared where the target patch was copied).
  Mask::Pointer currentMask;
  if(this->chkDisplayMaskedTargetPatch->isChecked())
    {
    currentMask = this->IntermediateImages[this->CurrentUsedPatchDisplayed - 1].MaskImage;
    }
  else
    {
    currentMask = this->IntermediateImages[this->CurrentUsedPatchDisplayed].MaskImage;
    }
  
  // Target
  Patch targetPatch = patchPair.TargetPatch;
  QImage targetImage = Helpers::GetQImage<FloatVectorImageType>(currentImage, currentMask, targetPatch.Region);
  targetImage = Helpers::FitToGraphicsView(targetImage, gfxTarget);
  this->TargetPatchScene->addPixmap(QPixmap::fromImage(targetImage));

  //Helpers::WritePatch<FloatVectorImageType>(this->Image, targetPatch, "targetPatch.mha");
  //Helpers::WriteMaskedPatch<FloatVectorImageType>(this->Inpainting.GetResult(), this->Inpainting.GetMaskImage(), targetPatch, "targetPatch.mha");
  
  // Source
  Patch sourcePatch = patchPair.SourcePatch;
  QImage sourceImage = Helpers::GetQImage<FloatVectorImageType>(currentImage, currentMask, sourcePatch.Region);
  sourceImage = Helpers::FitToGraphicsView(sourceImage, gfxTarget);
  this->SourcePatchScene->addPixmap(QPixmap::fromImage(sourceImage));

  Refresh();
}

void Form::HighlightUsedPatches()
{
  DebugMessage("HighlightUsedPatches()");
  
  unsigned int patchSize = Helpers::SideLengthFromRadius(this->txtPatchRadius->text().toUInt());
  
  DebugMessage<unsigned int>("Patch size: ", patchSize);

  PatchPair patchPair;
  this->Inpainting.GetUsedPatchPair(this->CurrentUsedPatchDisplayed, patchPair);

  // Target
  Patch targetPatch = patchPair.TargetPatch;

  //std::cout << "Displaying used target patch " << this->CurrentUsedPatchDisplayed << " : " << targetPatch.Region << std::endl;
  DebugMessage<itk::ImageRegion<2> >("Target patch region: ", targetPatch.Region);
  this->TargetPatchLayer.ImageData->SetDimensions(patchSize, patchSize, 1);
  Helpers::BlankAndOutlineImage(this->TargetPatchLayer.ImageData, this->Red);
  this->TargetPatchLayer.ImageSlice->SetPosition(targetPatch.Region.GetIndex()[0], targetPatch.Region.GetIndex()[1], 0);

  // Source
  Patch sourcePatch = patchPair.SourcePatch;

  //std::cout << "Displaying used source patch " << this->CurrentUsedPatchDisplayed << " : " << sourcePatch.Region << std::endl;
  DebugMessage<itk::ImageRegion<2> >("Source patch region: ", sourcePatch.Region);
  this->SourcePatchLayer.ImageData->SetDimensions(patchSize, patchSize, 1);
  Helpers::BlankAndOutlineImage(this->SourcePatchLayer.ImageData, this->Green);
  this->SourcePatchLayer.ImageSlice->SetPosition(sourcePatch.Region.GetIndex()[0], sourcePatch.Region.GetIndex()[1], 0);

  Refresh();

}

void Form::DisplayUsedPatchInformation()
{
  DebugMessage("DisplayUsedPatchInformation()");
  PatchPair patchPair;
  this->Inpainting.GetUsedPatchPair(this->CurrentUsedPatchDisplayed, patchPair);
  Patch targetPatch = patchPair.TargetPatch;
  Patch sourcePatch = patchPair.SourcePatch;

  // Patch pair information
  float ssd = patchPair.AverageSSD;
  
  std::stringstream ssSSD;
  ssSSD << ssd;
  this->lblAverageSSD->setText(ssSSD.str().c_str());
  
  std::stringstream ssHistogramDifference;
  ssHistogramDifference << patchPair.HistogramDifference;
  this->lblHistogramDistance->setText(ssHistogramDifference.str().c_str());

  // Target information
  std::stringstream ssTarget;
  ssTarget << "(" << targetPatch.Region.GetIndex()[0] << ", " << targetPatch.Region.GetIndex()[1] << ")";
  this->lblTargetCorner->setText(ssTarget.str().c_str());

  // Source information
  std::stringstream ssSource;
  ssSource << "(" << sourcePatch.Region.GetIndex()[0] << ", " << sourcePatch.Region.GetIndex()[1] << ")";
  this->lblSourceCorner->setText(ssSource.str().c_str());

  // Iteration information
  std::stringstream ss;
  ss << this->CurrentUsedPatchDisplayed;
  this->lblCurrentUsedPatches->setText(ss.str().c_str());

  Refresh();
}

void Form::CreatePotentialTargetPatchesImage()
{
  DebugMessage("CreatePotentialTargetPatchesImage()");
  // Draw potential patch pairs
  std::vector<PatchPair> potentialPatchPairs;
  this->Inpainting.GetPotentialPatchPairs(this->CurrentUsedPatchDisplayed, potentialPatchPairs);

//   std::stringstream ssPatchPairsFile;
//   ssPatchPairsFile << "Debug/PatchPairs_" << Helpers::ZeroPad(this->Inpainting.GetIteration(), 3) << ".txt";
//   OutputPairs(potentialPatchPairs, ssPatchPairsFile.str());

  this->PotentialTargetPatchesImage->SetRegions(this->Inpainting.GetFullRegion());
  this->PotentialTargetPatchesImage->Allocate();
  this->PotentialTargetPatchesImage->FillBuffer(0);

  for(unsigned int i = 0; i < potentialPatchPairs.size(); ++i)
    {
    Helpers::BlankAndOutlineRegion<UnsignedCharScalarImageType>(this->PotentialTargetPatchesImage, potentialPatchPairs[i].TargetPatch.Region, static_cast<unsigned char>(255));
    }

  vtkSmartPointer<vtkImageData> temp = vtkSmartPointer<vtkImageData>::New();
  Helpers::ITKScalarImageToScaledVTKImage<UnsignedCharScalarImageType>(this->PotentialTargetPatchesImage, temp);
  Helpers::MakePixelsTransparent(temp, this->PotentialPatchesLayer.ImageData, 0);

  Refresh();
}

void Form::OutputPairs(const std::vector<PatchPair>& patchPairs, const std::string& filename)
{
  std::ofstream fout(filename.c_str());
  
  for(unsigned int i = 0; i < patchPairs.size(); ++i)
    {
    fout << "Potential patch " << i << ": " << std::endl
	 << "target index: " << patchPairs[i].TargetPatch.Region.GetIndex() << std::endl
	 << "ssd score: " << patchPairs[i].AverageSSD << std::endl
	 << "histogram score: " << patchPairs[i].HistogramDifference << std::endl;
    }
    
  fout.close();
}

void Form::ChangeDisplayedIteration()
{
  DebugMessage("ChangeDisplayedIteration()");
  DisplayUsedPatches();
  CreatePotentialTargetPatchesImage();
  HighlightUsedPatches();
  DisplayUsedPatchInformation();
}

void Form::SetupInitialIntermediateImages()
{
  InpaintingVisualizationStack stack;
  
  Helpers::DeepCopyVectorImage<FloatVectorImageType>(this->UserImage, stack.Image);
  Helpers::DeepCopy<Mask>(this->UserMaskImage, stack.MaskImage);
  Helpers::DeepCopy<UnsignedCharScalarImageType>(this->Inpainting.GetBoundaryImage(), stack.Boundary);
  Helpers::DeepCopy<FloatScalarImageType>(this->Inpainting.GetPriorityImage(), stack.Priority);
  Helpers::DeepCopy<FloatScalarImageType>(this->Inpainting.GetDataImage(), stack.Data);
  Helpers::DeepCopy<FloatScalarImageType>(this->Inpainting.GetConfidenceImage(), stack.Confidence);
  Helpers::DeepCopy<FloatScalarImageType>(this->Inpainting.GetConfidenceMapImage(), stack.ConfidenceMap);
  Helpers::DeepCopy<FloatVector2ImageType>(this->Inpainting.GetBoundaryNormalsImage(), stack.BoundaryNormals);
  Helpers::DeepCopy<FloatVector2ImageType>(this->Inpainting.GetIsophoteImage(), stack.Isophotes);
  //Helpers::DeepCopy<UnsignedCharScalarImageType>(this->PotentialTargetPatchesImage, stack.PotentialTargetPatchesImage);

  this->IntermediateImages.clear();
  this->IntermediateImages.push_back(stack);
}

void Form::IterationComplete()
{
  DebugMessage("IterationComplete()");

  // Save the intermediate images
  
  InpaintingVisualizationStack stack;
  
  Helpers::DeepCopyVectorImage<FloatVectorImageType>(this->Inpainting.GetResult(), stack.Image);
  Helpers::DeepCopy<Mask>(this->Inpainting.GetMaskImage(), stack.MaskImage);
  Helpers::DeepCopy<UnsignedCharScalarImageType>(this->Inpainting.GetBoundaryImage(), stack.Boundary);
  Helpers::DeepCopy<FloatScalarImageType>(this->Inpainting.GetPriorityImage(), stack.Priority);
  Helpers::DeepCopy<FloatScalarImageType>(this->Inpainting.GetDataImage(), stack.Data);
  Helpers::DeepCopy<FloatScalarImageType>(this->Inpainting.GetConfidenceImage(), stack.Confidence);
  Helpers::DeepCopy<FloatScalarImageType>(this->Inpainting.GetConfidenceMapImage(), stack.ConfidenceMap);
  Helpers::DeepCopy<FloatVector2ImageType>(this->Inpainting.GetBoundaryNormalsImage(), stack.BoundaryNormals);
  Helpers::DeepCopy<FloatVector2ImageType>(this->Inpainting.GetIsophoteImage(), stack.Isophotes);
  Helpers::DeepCopy<UnsignedCharScalarImageType>(this->PotentialTargetPatchesImage, stack.PotentialTargetPatchesImage);

  this->IntermediateImages.push_back(stack);
  
  // After one iteration, GetNumberOfCompletedIterations will be 1. This is exactly the set of intermediate images we want to display, because the 0th intermediate images are the original inputs.
  this->CurrentUsedPatchDisplayed = this->Inpainting.GetNumberOfCompletedIterations();

  ChangeDisplayedIteration();

  Refresh();
}

void Form::IterationCompleteSlot()
{
  DebugMessage("IterationCompleteSlot()");
  IterationComplete();
}
