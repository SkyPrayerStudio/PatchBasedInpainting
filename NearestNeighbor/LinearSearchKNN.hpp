#ifndef LinearSearchKNN_HPP
#define LinearSearchKNN_HPP

// STL
#include <limits> // for infinity()
#include <algorithm> // for lower_bound()

/**
  * This function template is similar to std::min_element but can be used when the comparison
  * involves computing a derived quantity (a.k.a. distance). This algorithm will search for the
  * the elements in the range [first,last) with the "smallest" distances (of course, both the
  * distance metric and comparison can be overriden to perform something other than the canonical
  * Euclidean distance and less-than comparison, which would yield the element with minimum distance).
  * This function will fill the output container with a number of nearest-neighbors.
  * \tparam DistanceValue The value-type for the distance measures.
  * \tparam ForwardIterator The forward-iterator type.
  * \tparam OutputContainer The container type which can contain the list of nearest-neighbors (STL like container, with iterators, insert, size, and pop_back).
  * \tparam GetDistanceFunction The functor type to compute the distance measure.
  * \tparam CompareFunction The functor type that can compare two distance measures (strict weak-ordering).
  * \param first Start of the range in which to search.
  * \param last One element past the last element in the range in which to search.
  * \param output The container that will have the sorted list of elements with the smallest distance.
  * \param distance A callable object that returns a DistanceValue for a given element from the ForwardIterator dereferencing.
  * \param compare A callable object that returns true if the first element is the preferred one (less-than) of the two.
  * \param max_neighbors The maximum number of elements of smallest distance to output in the sorted list.
  */
template <typename ForwardIterator,
          typename OutputContainer,
          typename DistanceFunctionType,
          typename DistanceValueType = float,
          typename CompareFunctionType = std::less<DistanceValueType> >
struct LinearSearchKNN
{
  DistanceFunctionType DistanceFunction;
  CompareFunctionType CompareFunction;
  
  LinearSearchKNN(DistanceFunctionType distanceFunction = DistanceFunctionType(), const unsigned int k = 1000) : DistanceFunction(distanceFunction), K(k)
  {
  }

  void SetK(const unsigned int k)
  {
    this->K = k;
  }
  
  unsigned int GetK() const
  {
    return this->K;
  }

  void operator()(ForwardIterator first, ForwardIterator last, OutputContainer& output)
  {
    output.clear();
    if(first == last)
    {
      return;
    }

    std::vector<DistanceValueType> output_dist;
    for(; first != last; ++first)
    {
      DistanceValueType d = DistanceFunction(*first);
      if(!CompareFunction(d, std::numeric_limits<DistanceValueType>::infinity()))
      {
        continue;
      }
      typename std::vector<DistanceValueType>::iterator it_lo = std::lower_bound(output_dist.begin(),output_dist.end(), d, CompareFunction);
      if((it_lo != output_dist.end()) || (output_dist.size() < this->K))
      {
        output_dist.insert(it_lo, d);
        typename OutputContainer::iterator itv = output.begin();
        for(typename std::vector<DistanceValueType>::iterator it = output_dist.begin();
            (itv != output.end()) && (it != it_lo); ++itv,++it) ;
        output.insert(itv, *first);
        if(output.size() > this->K)
        {
          output.pop_back();
          output_dist.pop_back();
        }
      }
    }
  }

private:
  unsigned int K;
};

#endif
