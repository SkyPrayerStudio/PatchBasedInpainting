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

#include "Derivatives.h"
#include "Types.h"
#include "Mask.h"
#include "TestHelpers.h"

static void CreateMask(Mask* mask);

static void CreateImage(UnsignedCharScalarImageType* image);

int main(int argc, char*argv[])
{
  Mask::Pointer mask = Mask::New();
  CreateMask(mask);

  UnsignedCharScalarImageType::Pointer image = UnsignedCharScalarImageType::New();
  CreateImage(image);

  unsigned int direction = 0; // X-direction
  FloatScalarImageType::Pointer output = FloatScalarImageType::New();
  Derivatives::MaskedDerivative<UnsignedCharScalarImageType>(image, mask, direction, output);

  FloatScalarImageType::Pointer correctDerivative = FloatScalarImageType::New();
  if(!TestHelpers::ImagesEqual<FloatScalarImageType>(output, correctDerivative))
    {
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}


void CreateMask(Mask* mask)
{
  itk::Index<2> maskCorner;
  maskCorner.Fill(0);

  itk::Size<2> maskSize;
  maskSize.Fill(100);

  itk::ImageRegion<2> maskRegion(maskCorner, maskSize);

  mask->SetRegions(maskRegion);
  mask->Allocate();

  unsigned int indeterminateValue = (static_cast<unsigned int>(mask->GetHoleValue()) + static_cast<unsigned int>(mask->GetValidValue()))/2;

  // Make the left half of the mask the hole, the center indeterminate, and the right half valid.
  for(unsigned int column = 0; column < maskSize[0]; ++column)
    {
    for(unsigned int row = 0; row < maskSize[1]; ++row)
      {
      itk::Index<2> currentIndex;
      currentIndex[0] = column;
      currentIndex[1] = row;
      if(column < maskSize[0] / 3)
        {
        mask->SetPixel(currentIndex, mask->GetHoleValue());
        }
      else if(column < 2*maskSize[0]/3)
        {
        mask->SetPixel(currentIndex, indeterminateValue);
        }
      else
        {
        mask->SetPixel(currentIndex, mask->GetValidValue());
        }
      }
    }
}

void CreateImage(UnsignedCharScalarImageType* image)
{
  itk::Index<2> corner;
  corner.Fill(0);

  itk::Size<2> size;
  size.Fill(100);

  itk::ImageRegion<2> region(corner, size);

  image->SetRegions(region);
  image->Allocate();
  image->FillBuffer(0);

  // Make a white square on a black background
  for(unsigned int column = 25; column < 75; ++column)
    {
    for(unsigned int row = 25; row < 75; ++row)
      {
      itk::Index<2> currentIndex;
      currentIndex[0] = column;
      currentIndex[1] = row;
      image->SetPixel(currentIndex, 255);
      }
    }
}