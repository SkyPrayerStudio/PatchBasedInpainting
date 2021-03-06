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

// ITK
#include "itkImageFileReader.h"

// Submodules
#include <ITKHelpers/ITKHelpers.h>
#include <Mask/Mask.h>

// Driver
#include "Drivers/InpaintingGMH.hpp"

// Run with: image.png image.mask 15 filled.png
int main(int argc, char *argv[])
{
  // Verify arguments
  if(argc != 6)
  {
    std::cerr << "Required arguments: image.png image.mask patchHalfWidth knn output.png" << std::endl;
    std::cerr << "Input arguments: ";
    for(int i = 1; i < argc; ++i)
    {
      std::cerr << argv[i] << " ";
    }
    return EXIT_FAILURE;
  }

  // Parse arguments
  std::string imageFilename = argv[1];
  std::string maskFilename = argv[2];

  std::stringstream ssPatchRadius;
  ssPatchRadius << argv[3];
  unsigned int patchHalfWidth = 0;
  ssPatchRadius >> patchHalfWidth;

  std::stringstream ssKNN;
  ssKNN << argv[4];
  unsigned int knn = 0;
  ssKNN >> knn;

  std::string outputFileName = argv[5];

  // Output arguments
  std::cout << "Reading image: " << imageFilename << std::endl;
  std::cout << "Reading mask: " << maskFilename << std::endl;
  std::cout << "Patch half width: " << patchHalfWidth << std::endl;
  std::cout << "KNN: " << knn << std::endl;
  std::cout << "Output: " << outputFileName << std::endl;

  typedef itk::Image<itk::CovariantVector<float, 3>, 2> ImageType;

  typedef  itk::ImageFileReader<ImageType> ImageReaderType;
  ImageReaderType::Pointer imageReader = ImageReaderType::New();
  imageReader->SetFileName(imageFilename);
  imageReader->Update();

  ImageType::Pointer image = ImageType::New();
  ITKHelpers::DeepCopy(imageReader->GetOutput(), image.GetPointer());

  Mask::Pointer mask = Mask::New();
  mask->Read(maskFilename);

  std::cout << "Mask size: " << mask->GetLargestPossibleRegion().GetSize() << std::endl;
  std::cout << "hole pixels: " << mask->CountHolePixels() << std::endl;
  std::cout << "valid pixels: " << mask->CountValidPixels() << std::endl;

  InpaintingGMH(image, mask, patchHalfWidth, knn);

  // If the output filename is a png file, then use the RGBImage writer so that it is first
  // casted to unsigned char. Otherwise, write the file directly.
  if(Helpers::GetFileExtension(outputFileName) == "png")
  {
    ITKHelpers::WriteRGBImage(image.GetPointer(), outputFileName);
  }
  else
  {
    ITKHelpers::WriteImage(image.GetPointer(), outputFileName);
  }

  return EXIT_SUCCESS;
}
