/**
 * This is a visitor type that complies with the InpaintingVisitorConcept. It computes
 * and differences feature vectors (std::vector<float>) at each pixel.
 */
// template <typename TImage, typename TBoundaryNodeQueue,
//           typename TFillStatusMap, typename TDescriptorMap,
//           typename TPriorityMap, typename TBoundaryStatusMap>
// struct FeatureVectorInpaintingVisitor : public InpaintingVisitor<TImage, TBoundaryNodeQueue, TFillStatusMap, TDescriptorMap, TPriorityMap, TBoundaryStatusMap>
// {
// 
//   FeatureVectorInpaintingVisitor(TImage* const in_image, Mask* const in_mask,
//                     TBoundaryNodeQueue& in_boundaryNodeQueue, TFillStatusMap& in_fillStatusMap,
//                     TDescriptorMap& in_descriptorMap, TPriorityMap& in_priorityMap,
//                     Priority* const in_priorityFunction,
//                     const unsigned int in_half_width, TBoundaryStatusMap& in_boundaryStatusMap) :
//   InpaintingVisitor<TImage, TBoundaryNodeQueue, TFillStatusMap, TDescriptorMap, TPriorityMap, TBoundaryStatusMap>(in_image, in_mask, in_boundaryNodeQueue, in_priorityFunction, in_fillStatusMap, in_descriptorMap,
//                     in_priorityMap, in_boundaryStatusMap, in_half_width)
//   {
//   }

#ifndef FeatureVectorInpaintingVisitor_HPP
#define FeatureVectorInpaintingVisitor_HPP

#include "Priority/Priority.h"

#include "PixelDescriptors/FeatureVectorPixelDescriptor.h"

// Boost
#include <boost/graph/graph_traits.hpp>
#include <boost/property_map/property_map.hpp>

// Helpers
#include "Helpers/ITKHelpers.h"

/**
 * This is a visitor that complies with the InpaintingVisitorConcept. It creates
 * and differences FeatureVectorPixelDescriptor objects at each pixel.
 */
template <typename TImage, typename TBoundaryNodeQueue,
          typename TFillStatusMap, typename TDescriptorMap,
          typename TPriorityMap, typename TBoundaryStatusMap>
struct FeatureVectorInpaintingVisitor
{
  TImage* image;
  Mask* mask;
  TBoundaryNodeQueue& boundaryNodeQueue;
  Priority* priorityFunction;
  TFillStatusMap& fillStatusMap;
  TDescriptorMap& descriptorMap;
  typedef typename boost::property_traits<TDescriptorMap>::value_type DescriptorType;
  TPriorityMap& priorityMap;
  TBoundaryStatusMap& boundaryStatusMap;

  unsigned int half_width;
  unsigned int NumberOfFinishedVertices;

  FeatureVectorInpaintingVisitor(TImage* const in_image, Mask* const in_mask,
                                TBoundaryNodeQueue& in_boundaryNodeQueue, TFillStatusMap& in_fillStatusMap,
                                TDescriptorMap& in_descriptorMap, TPriorityMap& in_priorityMap,
                                Priority* const in_priorityFunction,
                                const unsigned int in_half_width, TBoundaryStatusMap& in_boundaryStatusMap) :
  image(in_image), mask(in_mask), boundaryNodeQueue(in_boundaryNodeQueue), priorityFunction(in_priorityFunction), fillStatusMap(in_fillStatusMap), descriptorMap(in_descriptorMap),
  priorityMap(in_priorityMap), boundaryStatusMap(in_boundaryStatusMap), half_width(in_half_width), NumberOfFinishedVertices(0)
  {
  }

  template <typename VertexType, typename Graph>
  void initialize_vertex(VertexType v, Graph& g) const
  {
    //std::cout << "Initializing " << v[0] << " " << v[1] << std::endl;
    // Create the patch object and associate with the node
//     itk::Index<2> index;
//     index[0] = v[0];
//     index[1] = v[1];

    std::vector<float> featureVector; // TODO: read this from a vtp file
    DescriptorType descriptor(featureVector);
    descriptor.SetVertex(v);
    put(descriptorMap, v, descriptor);

  };

  template <typename VertexType, typename Graph>
  void discover_vertex(VertexType v, Graph& g) const
  {
    std::cout << "Discovered " << v[0] << " " << v[1] << std::endl;
    std::cout << "Priority: " << get(priorityMap, v) << std::endl;
    DescriptorType& descriptor = get(descriptorMap, v);
    descriptor.SetStatus(DescriptorType::TARGET_PATCH);
  };

  template <typename VertexType, typename Graph>
  void vertex_match_made(VertexType target, VertexType source, Graph& g) const
  {
    std::cout << "Match made: target: " << target[0] << " " << target[1]
              << " with source: " << source[0] << " " << source[1] << std::endl;
    assert(get(fillStatusMap, source));
    assert(get(descriptorMap, source).IsFullyValid());
  };

  template <typename VertexType, typename Graph>
  void paint_vertex(VertexType target, VertexType source, Graph& g) const
  {
    itk::Index<2> target_index;
    target_index[0] = target[0];
    target_index[1] = target[1];

    itk::Index<2> source_index;
    source_index[0] = source[0];
    source_index[1] = source[1];

    assert(image->GetLargestPossibleRegion().IsInside(source_index));
    assert(image->GetLargestPossibleRegion().IsInside(target_index));

    image->SetPixel(target_index, image->GetPixel(source_index));
  };

  template <typename VertexType, typename Graph>
  bool accept_painted_vertex(VertexType v, Graph& g) const
  {
    return true;
  };

  template <typename VertexType, typename Graph>
  void finish_vertex(VertexType v, Graph& g)
  {
    // Construct the region around the vertex
    itk::Index<2> indexToFinish;
    indexToFinish[0] = v[0];
    indexToFinish[1] = v[1];

    itk::ImageRegion<2> region = ITKHelpers::GetRegionInRadiusAroundPixel(indexToFinish, half_width);

    region.Crop(image->GetLargestPossibleRegion()); // Make sure the region is entirely inside the image

    // Mark all the pixels in this region as filled.
    // It does not matter which image we iterate over, we just want the indices.
    itk::ImageRegionConstIteratorWithIndex<TImage> gridIterator(image, region);

    while(!gridIterator.IsAtEnd())
      {
      VertexType v;
      v[0] = gridIterator.GetIndex()[0];
      v[1] = gridIterator.GetIndex()[1];
      put(fillStatusMap, v, true);
      mask->SetPixel(gridIterator.GetIndex(), mask->GetValidValue());
      ++gridIterator;
      }

    // Additionally, initialize the filled vertices because they may now be valid.
    // This must be done in a separate loop like this because the mask image used to check for boundary pixels is incorrect until the above loop updates it.
    gridIterator.GoToBegin();
    while(!gridIterator.IsAtEnd())
      {
      VertexType v;
      v[0] = gridIterator.GetIndex()[0];
      v[1] = gridIterator.GetIndex()[1];
      initialize_vertex(v, g);
      ++gridIterator;
      }

    // Update the priority function.
    this->priorityFunction->Update(indexToFinish);

    // Add pixels that are on the new boundary to the queue, and mark other pixels as not in the queue.
    itk::ImageRegionConstIteratorWithIndex<Mask> imageIterator(mask, region);

    while(!imageIterator.IsAtEnd())
      {
      VertexType v;
      v[0] = imageIterator.GetIndex()[0];
      v[1] = imageIterator.GetIndex()[1];

      // Mark all nodes in the patch around this node as filled (in the FillStatusMap).
      // This makes them ignored if they are still in the boundaryNodeQueue.
      if(ITKHelpers::HasNeighborWithValue(imageIterator.GetIndex(), mask, mask->GetHoleValue()))
        {
        put(boundaryStatusMap, v, true);
        this->boundaryNodeQueue.push(v);
        float priority = this->priorityFunction->ComputePriority(imageIterator.GetIndex());
        //std::cout << "updated priority: " << priority << std::endl;
        put(priorityMap, v, priority);
        }
      else
        {
        put(boundaryStatusMap, v, false);
        }

      ++imageIterator;
      }

    {
    // Debug only - write the mask to a file
    HelpersOutput::WriteImage(mask, Helpers::GetSequentialFileName("debugMask", this->NumberOfFinishedVertices, "png"));
    HelpersOutput::WriteVectorImageAsRGB(image, Helpers::GetSequentialFileName("output", this->NumberOfFinishedVertices, "png"));
    this->NumberOfFinishedVertices++;
    }
  }; // finish_vertex

}; // FeatureVectorInpaintingVisitor

#endif