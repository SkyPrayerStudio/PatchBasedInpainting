/*=========================================================================
 *
 *  Copyright David Doria 2012 daviddoria@gmail.com
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

#ifndef PatchHelpers_HPP
#define PatchHelpers_HPP

#include "PatchHelpers.h"

// Submodules
#include <ITKHelpers/ITKHelpers.h>
#include <ITKQtHelpers/ITKQtHelpers.h>

// STL
#include <cassert>
#include <stdexcept>

// Qt
#include <QApplication>
#include <QColor>
#include <QPainter>

namespace PatchHelpers
{

template <typename TNodeQueue, typename TPriorityMap>
void WriteValidQueueNodesLocationsImage(TNodeQueue nodeQueue, const TPriorityMap propertyMap,
                                       const itk::ImageRegion<2>& fullRegion, const std::string& fileName)
{
  typedef itk::Image<unsigned char, 2> ImageType;
  ImageType::Pointer image = ImageType::New();
  image->SetRegions(fullRegion);
  image->Allocate();
  image->FillBuffer(0);

  while(!nodeQueue.empty())
  {
    typename TNodeQueue::value_type queuedNode = nodeQueue.top();

    bool valid = get(propertyMap, queuedNode);
    if(valid)
    {
      itk::Index<2> index = Helpers::ConvertFrom<itk::Index<2>, typename TNodeQueue::value_type>(queuedNode);
      image->SetPixel(index, 255);
    }

    nodeQueue.pop();
  }

  ITKHelpers::WriteImage(image.GetPointer(), fileName);
}

template <typename TNodeQueue, typename TBoundaryStatusMap, typename TPriorityMap>
void WriteValidQueueNodesPrioritiesImage(TNodeQueue nodeQueue, const TBoundaryStatusMap boundaryStatusMap, const TPriorityMap priorityMap,
                                         const itk::ImageRegion<2>& fullRegion, const std::string& fileName)
{
  typedef itk::Image<float, 2> ImageType;
  ImageType::Pointer image = ImageType::New();
  image->SetRegions(fullRegion);
  image->Allocate();
  image->FillBuffer(0);

  while(!nodeQueue.empty())
  {
    typename TNodeQueue::value_type queuedNode = nodeQueue.top();

    bool valid = get(boundaryStatusMap, queuedNode);
    if(valid)
    {
      itk::Index<2> index = Helpers::ConvertFrom<itk::Index<2>, typename TNodeQueue::value_type>(queuedNode);
//      image->SetPixel(index, get(nodeQueue.keys(), queuedNode));
      image->SetPixel(index, get(priorityMap, queuedNode));
    }

    nodeQueue.pop();
  }

  ITKHelpers::WriteImage(image.GetPointer(), fileName);
}

template <typename TNodeQueue, typename TBoundaryStatusMap, typename TPriorityMap>
void DumpQueue(TNodeQueue nodeQueue, const TBoundaryStatusMap boundaryStatusMap, const TPriorityMap priorityMap)
{
  while(!nodeQueue.empty())
  {
    typename TNodeQueue::value_type queuedNode = nodeQueue.top();

    bool valid = get(boundaryStatusMap, queuedNode);
    if(valid)
    {
      float priority = get(priorityMap, queuedNode);
      std::cout << "(" << queuedNode[0] << ", " << queuedNode[1] << ") : " << priority << std::endl;
    }

    nodeQueue.pop();
  }
}

template <class TPriorityQueue>
void WritePriorityQueue(TPriorityQueue q, const std::string& fileName)
{
  std::ofstream fout(fileName.c_str());

  while(!q.empty())
  {
    float priority = get(q.keys(), q.top());
    fout << priority << std::endl;
    q.pop();
  }
}

template <typename TImage>
QImage GetQImageCombinedPatch(const TImage* const image, const itk::ImageRegion<2>& sourceRegion,
                              const itk::ImageRegion<2>& targetRegion, const Mask* const mask)
{
  assert(sourceRegion.GetSize() == targetRegion.GetSize());

  QImage qimage(sourceRegion.GetSize()[0], sourceRegion.GetSize()[1], QImage::Format_RGB888);

  typedef itk::RegionOfInterestImageFilter<TImage, TImage> RegionOfInterestImageFilterType;

  typename RegionOfInterestImageFilterType::Pointer sourcePatchExtractor =
      RegionOfInterestImageFilterType::New();
  sourcePatchExtractor->SetRegionOfInterest(sourceRegion);
  sourcePatchExtractor->SetInput(image);
  sourcePatchExtractor->Update();

  typename RegionOfInterestImageFilterType::Pointer targetPatchExtractor =
      RegionOfInterestImageFilterType::New();
  targetPatchExtractor->SetRegionOfInterest(targetRegion);
  targetPatchExtractor->SetInput(image);
  targetPatchExtractor->Update();

  typedef itk::RegionOfInterestImageFilter<Mask, Mask> RegionOfInterestMaskFilterType;
  typename RegionOfInterestMaskFilterType::Pointer regionOfInterestMaskFilter =
      RegionOfInterestMaskFilterType::New();
  regionOfInterestMaskFilter->SetRegionOfInterest(targetRegion);
  regionOfInterestMaskFilter->SetInput(mask);
  regionOfInterestMaskFilter->Update();

  itk::ImageRegionIterator<TImage> sourcePatchIterator(sourcePatchExtractor->GetOutput(),
                                                       sourcePatchExtractor->GetOutput()->GetLargestPossibleRegion());

  itk::ImageRegionIterator<TImage> targetPatchIterator(targetPatchExtractor->GetOutput(),
                                                       targetPatchExtractor->GetOutput()->GetLargestPossibleRegion());

  while(!sourcePatchIterator.IsAtEnd())
  {
    itk::Index<2> index = targetPatchIterator.GetIndex();

    typename TImage::PixelType pixel;
    if(regionOfInterestMaskFilter->GetOutput()->IsHole(index))
    {
      pixel = sourcePatchIterator.Get();
    }
    else
    {
      pixel = targetPatchIterator.Get();
    }

    QColor pixelColor(static_cast<int>(pixel[0]), static_cast<int>(pixel[1]), static_cast<int>(pixel[2]));
    qimage.setPixel(index[0], index[1], pixelColor.rgb());

    ++targetPatchIterator;
    ++sourcePatchIterator;
  }

  // std::cout << "There were " << numberOfHolePixels << " hole pixels." << std::endl;

  return qimage;
}

template <class TImage>
void CopyRegion(const TImage* sourceImage, TImage* targetImage,
               const itk::Index<2>& sourcePosition, const itk::Index<2>& targetPosition, const unsigned int radius)
{
  // Copy a patch of radius 'radius' centered at 'sourcePosition' from 'sourceImage' to 'targetImage' centered at 'targetPosition'
  typedef itk::RegionOfInterestImageFilter<TImage, TImage> ExtractFilterType;

  typename ExtractFilterType::Pointer extractFilter = ExtractFilterType::New();
  extractFilter->SetRegionOfInterest(ITKHelpers::GetRegionInRadiusAroundPixel(sourcePosition, radius));
  extractFilter->SetInput(sourceImage);
  extractFilter->Update();

  CopyPatchIntoImage<TImage>(extractFilter->GetOutput(), targetImage, targetPosition);
}


template <class TImage>
void CopyPatchIntoImage(const TImage* const patch, TImage* const image, const Mask* const mask,
                        const itk::Index<2>& position)
{
  // This function copies 'patch' into 'image' centered at 'position' only where the 'mask' is non-zero

  // 'Mask' must be the same size as 'image'
  if(mask->GetLargestPossibleRegion().GetSize() != image->GetLargestPossibleRegion().GetSize())
    {
    throw std::runtime_error("mask and image must be the same size!");
    }

  // The PasteFilter expects the lower left corner of the destination position, but we have passed the center pixel.
  position[0] -= patch->GetLargestPossibleRegion().GetSize()[0]/2;
  position[1] -= patch->GetLargestPossibleRegion().GetSize()[1]/2;

  itk::ImageRegion<2> region = GetRegionInRadiusAroundPixel(position,
                                                            patch->GetLargestPossibleRegion().GetSize()[0]/2);

  itk::ImageRegionConstIterator<TImage> patchIterator(patch,patch->GetLargestPossibleRegion());
  itk::ImageRegionConstIterator<Mask> maskIterator(mask,region);
  itk::ImageRegionIterator<TImage> imageIterator(image, region);

  while(!patchIterator.IsAtEnd())
    {
    if(mask->IsHole(maskIterator.GetIndex())) // we are in the target region
      {
      imageIterator.Set(patchIterator.Get());
      }
    ++imageIterator;
    ++maskIterator;
    ++patchIterator;
    }
}


template <class TImage>
void CopyPatchIntoImage(const TImage* patch, TImage* const image, const itk::Index<2>& centerPixel)
{
  // This function copies 'patch' into 'image' centered at 'position'.

  // The PasteFilter expects the lower left corner of the destination position, but we have passed the center pixel.
  itk::Index<2> cornerPixel;
  cornerPixel[0] = centerPixel[0] - patch->GetLargestPossibleRegion().GetSize()[0]/2;
  cornerPixel[1] = centerPixel[1] - patch->GetLargestPossibleRegion().GetSize()[1]/2;

  typedef itk::PasteImageFilter <TImage, TImage> PasteImageFilterType;

  typename PasteImageFilterType::Pointer pasteFilter = PasteImageFilterType::New();
  pasteFilter->SetInput(0, image);
  pasteFilter->SetInput(1, patch);
  pasteFilter->SetSourceRegion(patch->GetLargestPossibleRegion());
  pasteFilter->SetDestinationIndex(cornerPixel);
  pasteFilter->InPlaceOn();
  pasteFilter->Update();

  image->Graft(pasteFilter->GetOutput());

}

template <typename TIterator, typename TImage, typename TPropertyMap>
void WriteTopPatches(TImage* const image, TPropertyMap propertyMap, const TIterator first, const TIterator last,
                     const std::string& prefix, const unsigned int iteration)
{
  itk::Size<2> patchSize = get(propertyMap, *first).GetRegion().GetSize();

  unsigned int patchSideLength = patchSize[0]; // Assumes square patches

  unsigned int numberOfTopPatches = last - first;
//  std::cout << "WriteTopPatches:numberOfTopPatches = " << numberOfTopPatches << std::endl;

  itk::Index<2> topPatchesImageCorner = {{0,0}};

  // Make space for all the patches and a colored line dividing them (and the -1 is so there is no dividing line at the bottom)
  itk::Size<2> topPatchesImageSize = {{patchSideLength * 2,
                                       patchSideLength * numberOfTopPatches + numberOfTopPatches - 1}};
  itk::ImageRegion<2> topPatchesImageRegion(topPatchesImageCorner, topPatchesImageSize);

//  std::cout << "topPatchesImageRegion: " << topPatchesImageRegion << std::endl;

  typename TImage::Pointer topPatchesImage = TImage::New();
  topPatchesImage->SetRegions(topPatchesImageRegion);
  topPatchesImage->Allocate();
  topPatchesImage->FillBuffer(itk::NumericTraits<typename TImage::PixelType>::Zero);

  typename TImage::PixelType green;
  green.Fill(0);
  green[1] = 255;

  QApplication* app = 0;
  if(!QApplication::instance())
  {
    int fakeargc = 1;
    const char* fakeArgv[1];
    fakeArgv[0] = "FakeQApplication";
    app = new QApplication(fakeargc, const_cast<char**>(fakeArgv));
  }

  QPixmap pixmap(patchSideLength, patchSideLength);

  QPainter painter(&pixmap);

  QFont font("", 6); // arbitrary (default) font, size 6
  painter.setFont(font);

//  painter.drawText(QPointF(10,10), "test"); // bottom left corner of the text seems to start at this point

  typename TImage::Pointer numberImage = TImage::New();

  QColor qtwhite(255,255,255);

  for(TIterator currentPatch = first; currentPatch != last; ++currentPatch)
  {
    unsigned int currentPatchId = currentPatch - first;

    pixmap.fill(qtwhite);
    std::stringstream ssNumber;
    ssNumber << currentPatchId;
    painter.drawText(QPointF(1,10), ssNumber.str().c_str()); // bottom left corner of the text seems to start at this point

    ITKQtHelpers::QImageToITKImage(pixmap.toImage(), numberImage.GetPointer());

    // The extra + currentPatchId is to skip the extra dividing lines that have been drawn
    itk::Index<2> topPatchesImageNumberCorner = {{static_cast<itk::Index<2>::IndexValueType>(patchSideLength),
                                                  static_cast<itk::Index<2>::IndexValueType>(patchSideLength * currentPatchId + currentPatchId)}};

    itk::ImageRegion<2> topPatchesImageNumberRegion(topPatchesImageNumberCorner, patchSize);
    ITKHelpers::CopyRegion(numberImage.GetPointer(), topPatchesImage.GetPointer(),
                           numberImage->GetLargestPossibleRegion(), topPatchesImageNumberRegion);

    // The extra + currentPatchId is to skip the extra dividing lines that have been drawn
    itk::Index<2> topPatchesImageCorner = {{0, static_cast<itk::Index<2>::IndexValueType>(patchSideLength * currentPatchId + currentPatchId)}};
    itk::ImageRegion<2> currentTopPatchesImageRegion(topPatchesImageCorner, patchSize);
    itk::ImageRegion<2> currentRegion = get(propertyMap, *currentPatch).GetRegion();
    // The patches passed to this function aren't necessarily inside the image. If they are not, we cannot write them into the list of top patches,
    // and we simply leave the patch blank.
    if(image->GetLargestPossibleRegion().IsInside(currentRegion))
    {
      ITKHelpers::CopyRegion(image, topPatchesImage.GetPointer(), currentRegion, currentTopPatchesImageRegion);
    }

    if(currentPatchId != numberOfTopPatches - 1)
    {
//      std::cout << "CurrentPatchId " << currentPatchId << " numberOfPatches " << numberOfPatches << std::endl;
      itk::Index<2> dividingLineCorner = {{0, static_cast<itk::Index<2>::IndexValueType>(topPatchesImageCorner[1] + patchSideLength)}};
      itk::Size<2> dividingLineSize = {{patchSideLength, 1}};
      itk::ImageRegion<2> dividingLine(dividingLineCorner, dividingLineSize);
//      std::cout << "Dividing line: " << dividingLine << std::endl;

      ITKHelpers::SetRegionToConstant(topPatchesImage.GetPointer(), dividingLine, green);
    }
  }

  ITKHelpers::WriteRGBImage(topPatchesImage.GetPointer(), Helpers::GetSequentialFileName(prefix, iteration,"png",3));

  delete app; // c++ allows null pointers to be deleted
}

template <typename TPatchContainer>
void WriteTopPatchesGrid(const TPatchContainer& patches,
                         unsigned int gridWidth, unsigned int gridHeight,
                         const std::string& outputFileName)
{
  // 'iterations' is only used for the file name to write
  // Note that not all patches may be written (only the first ones that fit in the grid)

  unsigned int numberOfTopPatches = patches.size();

  if(numberOfTopPatches < gridWidth * gridHeight)
  {
    std::stringstream ss;
    ss << "WriteTopPatchesGrid(): Not enough patches (" << numberOfTopPatches << ") to create a "
       << gridWidth << " x " << gridHeight << " grid (need " << gridWidth * gridHeight << ").";
    throw std::runtime_error(ss.str());
  }
  itk::Size<2> patchSize = patches[0]->GetLargestPossibleRegion().GetSize();

  unsigned int patchSideLength = patchSize[0]; // Assumes square patches

//  std::cout << "WriteTopPatches:numberOfTopPatches = " << numberOfTopPatches << std::endl;

  itk::Index<2> topPatchesImageCorner = {{0,0}};

  // Make space for all the patches and a colored line dividing them (and the -1 is so there is no dividing line at the bottom)
  unsigned int padding = 4;

  unsigned int topPatchesImageWidth = (patchSideLength + padding) * gridWidth - padding; // the (- padding) is because we don't want padding after the last column
  unsigned int topPatchesImageHeight = (patchSideLength + padding) * gridHeight - padding; // the (- padding) is because we don't want padding after the last row
  itk::Size<2> topPatchesImageSize = {{topPatchesImageWidth,
                                       topPatchesImageHeight}};
  itk::ImageRegion<2> topPatchesImageRegion(topPatchesImageCorner, topPatchesImageSize);

//  std::cout << "topPatchesImageRegion: " << topPatchesImageRegion << std::endl;

  typedef itk::Image<itk::CovariantVector<unsigned char, 3>, 2> ImageType;

  ImageType::PixelType white;
  white.Fill(255);

  ImageType::Pointer topPatchesImage = ImageType::New();
  topPatchesImage->SetRegions(topPatchesImageRegion);
  topPatchesImage->Allocate();
  topPatchesImage->FillBuffer(white);

  unsigned int currentPatchId = 0;
  for(unsigned int gridRow = 0; gridRow < gridHeight; ++gridRow)
  {
    for(unsigned int gridColumn = 0; gridColumn < gridWidth; ++gridColumn)
    {
      // The extra + currentPatchId * padding is to skip the padding
      int xPos = static_cast<itk::Index<2>::IndexValueType>((patchSideLength + padding) * gridColumn);
      int yPos = static_cast<itk::Index<2>::IndexValueType>((patchSideLength + padding) * gridRow);
      itk::Index<2> topPatchesImageCorner = {{xPos, yPos}};
      itk::ImageRegion<2> currentTopPatchesImageRegion(topPatchesImageCorner, patchSize);

      // The patches passed to this function aren't necessarily inside the image.
      // If they are not, we cannot write them into the list of top patches,
      // and we simply leave the patch blank.
      ITKHelpers::CopyRegion(patches[currentPatchId].GetPointer(), topPatchesImage.GetPointer(),
                             patches[currentPatchId]->GetLargestPossibleRegion(), currentTopPatchesImageRegion);

      currentPatchId++;
    }
  }

  ITKHelpers::WriteRGBImage(topPatchesImage.GetPointer(), outputFileName);
}

template <typename TIterator, typename TImage, typename TPropertyMap>
void WriteTopPatchesGrid(const TImage* const image, TPropertyMap propertyMap,
                         const TIterator first, const TIterator last,
                         const std::string& prefix, const unsigned int iteration,
                         unsigned int gridWidth, unsigned int gridHeight)
{
  // 'iterations' is only used for the file name to write
  // Note that not all patches may be written (only the first ones that fit in the grid)

  unsigned int numberOfTopPatches = last - first;

  if(numberOfTopPatches < gridWidth * gridHeight)
  {
    std::stringstream ss;
    ss << "WriteTopPatchesGrid(): Not enough patches (" << numberOfTopPatches << ") to create a "
       << gridWidth << " x " << gridHeight << " grid (need " << gridWidth * gridHeight << ").";
    throw std::runtime_error(ss.str());
  }
  itk::Size<2> patchSize = get(propertyMap, *first).GetRegion().GetSize();

  unsigned int patchSideLength = patchSize[0]; // Assumes square patches

//  std::cout << "WriteTopPatches:numberOfTopPatches = " << numberOfTopPatches << std::endl;

  itk::Index<2> topPatchesImageCorner = {{0,0}};

  // Make space for all the patches and a colored line dividing them (and the -1 is so there is no dividing line at the bottom)
  unsigned int padding = 4;

  unsigned int topPatchesImageWidth = (patchSideLength + padding) * gridWidth - padding; // the (- padding) is because we don't want padding after the last column
  unsigned int topPatchesImageHeight = (patchSideLength + padding) * gridHeight - padding; // the (- padding) is because we don't want padding after the last row
  itk::Size<2> topPatchesImageSize = {{topPatchesImageWidth,
                                       topPatchesImageHeight}};
  itk::ImageRegion<2> topPatchesImageRegion(topPatchesImageCorner, topPatchesImageSize);

//  std::cout << "topPatchesImageRegion: " << topPatchesImageRegion << std::endl;

  typename TImage::PixelType white;
  white.Fill(255);

  typename TImage::Pointer topPatchesImage = TImage::New();
  topPatchesImage->SetRegions(topPatchesImageRegion);
  topPatchesImage->Allocate();
  topPatchesImage->FillBuffer(white);

  TIterator currentPatch = first;
  for(unsigned int gridRow = 0; gridRow < gridHeight; ++gridRow)
  {
    for(unsigned int gridColumn = 0; gridColumn < gridWidth; ++gridColumn)
    {
      // The extra + currentPatchId * padding is to skip the padding
      int xPos = static_cast<itk::Index<2>::IndexValueType>((patchSideLength + padding) * gridColumn);
      int yPos = static_cast<itk::Index<2>::IndexValueType>((patchSideLength + padding) * gridRow);
      itk::Index<2> topPatchesImageCorner = {{xPos, yPos}};
      itk::ImageRegion<2> currentTopPatchesImageRegion(topPatchesImageCorner, patchSize);

      itk::ImageRegion<2> currentRegion = get(propertyMap, *currentPatch).GetRegion();
      // The patches passed to this function aren't necessarily inside the image. If they are not, we cannot write them into the list of top patches,
      // and we simply leave the patch blank.
      if(image->GetLargestPossibleRegion().IsInside(currentRegion))
      {
        ITKHelpers::CopyRegion(image, topPatchesImage.GetPointer(), currentRegion, currentTopPatchesImageRegion);
      }

      ++currentPatch;
    }
  }

  ITKHelpers::WriteRGBImage(topPatchesImage.GetPointer(), Helpers::GetSequentialFileName(prefix, iteration,"png",3));
}

} // end namespace

#endif
