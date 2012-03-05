#ifndef DebugVisitor_HPP
#define DebugVisitor_HPP

#include <boost/graph/graph_traits.hpp>

// Custom
#include "Visitors/InpaintingVisitorParent.h"
#include "ImageProcessing/Mask.h"
#include "Helpers/OutputHelpers.h"
#include "Helpers/ITKHelpers.h"
#include "Helpers/BoostHelpers.h"

// ITK
#include "itkImage.h"
#include "itkImageRegion.h"

/**
  * This visitor writes out information and images at each iteration.
 */
template <typename TGraph, typename TImage, typename TBoundaryStatusMap, typename TBoundaryNodeQueue>
struct DebugVisitor : public InpaintingVisitorParent<TGraph>
{
  TImage* Image;
  Mask* MaskImage;

  const unsigned int HalfWidth;
  unsigned int NumberOfFinishedVertices;

  TBoundaryStatusMap& BoundaryStatusMap;
  TBoundaryNodeQueue& BoundaryNodeQueue;

  typedef typename boost::graph_traits<TGraph>::vertex_descriptor VertexDescriptorType;

  DebugVisitor(TImage* const image, Mask* const mask, const unsigned int halfWidth, TBoundaryStatusMap& boundaryStatusMap, TBoundaryNodeQueue& boundaryNodeQueue) :
  Image(image), MaskImage(mask), HalfWidth(halfWidth), NumberOfFinishedVertices(0), BoundaryStatusMap(boundaryStatusMap), BoundaryNodeQueue(boundaryNodeQueue)
  {

  }

  void InitializeVertex(VertexDescriptorType v) const
  {

  };

  void DiscoverVertex(VertexDescriptorType v) const
  {

  };

  void PotentialMatchMade(VertexDescriptorType target, VertexDescriptorType source)
  {
    std::cout << "Match made: target: " << target[0] << " " << target[1]
              << " with source: " << source[0] << " " << source[1] << std::endl;
  };

  void PaintVertex(VertexDescriptorType target, VertexDescriptorType source) const
  {

  };

  bool AcceptMatch(VertexDescriptorType target, VertexDescriptorType source) const
  {
    return true;
  };

  void FinishVertex(VertexDescriptorType target, VertexDescriptorType sourceNode)
  {
    {
    itk::Index<2> sourceIndex = ITKHelpers::CreateIndex(sourceNode);

    itk::ImageRegion<2> sourceRegion = ITKHelpers::GetRegionInRadiusAroundPixel(sourceIndex, HalfWidth);

//     OutputHelpers::WriteVectorImageRegionAsRGB(Image, sourceRegion,
//                                                Helpers::GetSequentialFileName("sourcePatch",
//                                                                               this->NumberOfFinishedVertices, "png"));
    }

    {
    // Construct the region around the vertex
    itk::Index<2> indexToFinish = ITKHelpers::CreateIndex(target);

    itk::ImageRegion<2> region = ITKHelpers::GetRegionInRadiusAroundPixel(indexToFinish, HalfWidth);

//     OutputHelpers::WriteVectorImageRegionAsRGB(Image, region,
//                                                Helpers::GetSequentialFileName("targetPatch",
//                                                                               this->NumberOfFinishedVertices, "png"));
//     OutputHelpers::WriteRegion(MaskImage, region,
//                                Helpers::GetSequentialFileName("maskPatch", this->NumberOfFinishedVertices, "png"));
    }

    //OutputHelpers::WriteImage(MaskImage, Helpers::GetSequentialFileName("mask", this->NumberOfFinishedVertices, "png"));
    OutputHelpers::WriteImage(MaskImage, Helpers::GetSequentialFileName("mask", this->NumberOfFinishedVertices, "mha"));
    //OutputHelpers::WriteVectorImageAsRGB(Image, Helpers::GetSequentialFileName("output", this->NumberOfFinishedVertices, "png"));
    OutputHelpers::WriteImage(Image, Helpers::GetSequentialFileName("output", this->NumberOfFinishedVertices, "mha"));

    typedef itk::Image<unsigned char, 2> IndicatorImageType;

    IndicatorImageType::Pointer boundaryStatusMapImage = IndicatorImageType::New();
    boundaryStatusMapImage->SetRegions(Image->GetLargestPossibleRegion());
    boundaryStatusMapImage->Allocate();
//     BoostHelpers::WritePropertyMapAsImage(BoundaryStatusMap, boundaryStatusMapImage.GetPointer(),
//                                           Helpers::GetSequentialFileName("boundaryStatusMap", this->NumberOfFinishedVertices, "png"));

    IndicatorImageType::Pointer validBoundaryNodeImage = IndicatorImageType::New();
    validBoundaryNodeImage->SetRegions(Image->GetLargestPossibleRegion());
    validBoundaryNodeImage->Allocate();
//     BoostHelpers::WriteValidQueueNodesAsImage(BoundaryNodeQueue, BoundaryStatusMap, validBoundaryNodeImage.GetPointer(),
//                                           Helpers::GetSequentialFileName("boundaryQueueValidNodes", this->NumberOfFinishedVertices, "png"));

    IndicatorImageType::Pointer allBoundaryNodeImage = IndicatorImageType::New();
    allBoundaryNodeImage->SetRegions(Image->GetLargestPossibleRegion());
    allBoundaryNodeImage->Allocate();
//     BoostHelpers::WriteAllQueueNodesAsImage(BoundaryNodeQueue, allBoundaryNodeImage.GetPointer(),
//                                           Helpers::GetSequentialFileName("boundaryQueueAllNodes", this->NumberOfFinishedVertices, "png"));

    this->NumberOfFinishedVertices++;

    std::cout << "Finished node " << this->NumberOfFinishedVertices << std::endl;

  };

  void InpaintingComplete() const
  {
  }
};

#endif
