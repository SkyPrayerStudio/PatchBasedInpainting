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

#ifndef TopPatchListOrManual_HPP
#define TopPatchListOrManual_HPP

// Submodules
#include <Mask/Mask.h>

// Custom
#include "Node.h"
#include "Interactive/TopPatchesDialogHandler.h"

// STL
#include <iostream>

// Qt
#include <QGraphicsScene>

/**
  Display a list of patches and let the user select the one to use.
  This class will be called from a non-GUI thread, so we cannot exec() the
  TopPatchesDialog directly. To rememdy this, we create a wrapper class that
  sends signals to the TopPatchesDialog that it puts in the GUI thread.
 */
template <typename TImage>
class TopPatchListOrManual : public QObject
{
private:
  const TImage* Image;
  const Mask* MaskImage;
  unsigned int PatchHalfWidth;

  TopPatchesDialogHandler<TImage>* TopPatchesDialogHandlerInstance = nullptr;

  /** This variable tracks how many times the patch was selected by choosing it from the TopPatchesDialog. */
  unsigned int NumberOfUses = 0;

public:

  /** Contructor. */
  TopPatchListOrManual(const TImage* const image, const Mask* const mask,
                       const unsigned int patchHalfWidth, QWidget* parent = nullptr) :
  Image(image), MaskImage(mask), PatchHalfWidth(patchHalfWidth)
  {
    // Create a DialogHandler and move it to the main GUI thread.
    this->TopPatchesDialogHandlerInstance =
        new TopPatchesDialogHandler<TImage>(this->Image, this->MaskImage, this->PatchHalfWidth, parent);
    this->TopPatchesDialogHandlerInstance->moveToThread(QCoreApplication::instance()->thread());
  }

  /** Destructor. */
  ~TopPatchListOrManual()
  {
    std::cout << "TopPatchListOrManual was used " << this->NumberOfUses
              << " times." << std::endl;

    delete this->TopPatchesDialogHandlerInstance;
  }

  /** Display a report of the number of selections that were made. */
  void Report()
  {
    this->TopPatchesDialogHandlerInstance->GetTopPatchesChooser()->Report();
  }

  /** Get the dialog object. */
  TopPatchesDialog<TImage>* GetTopPatchesDialog()
  {
    return this->TopPatchesDialogHandlerInstance->GetTopPatchesChooser();
  }

  /** Return the best source node for a specified target node. */
  template <typename TVertexDescriptor, typename TForwardIterator>
  TVertexDescriptor operator()(TForwardIterator possibleNodesBegin,
                               TForwardIterator possibleNodesEnd,
                               const TVertexDescriptor& queryVertex)
  {
//    std::cout << "VisualSelectionBest::operator()" << std::endl;
//    std::cout << "There are " << possibleNodesEnd - possibleNodesBegin
//              << " nodes." << std::endl;

    std::vector<Node> sourceNodes;
    for(TForwardIterator iter = possibleNodesBegin; iter != possibleNodesEnd; ++iter)
    {
      Node node(*iter);
      // std::cout << "VisualSelectionBest::operator() node: "
      //           << node[0] << " " << node[1] << std::endl;
      sourceNodes.push_back(node);
    }

    this->TopPatchesDialogHandlerInstance->SetSourceNodes(sourceNodes);

    Node queryNode(queryVertex);
    this->TopPatchesDialogHandlerInstance->SetQueryNode(queryNode);

    // Signal the TopPatchesDialog and get the return value via this variable.
    Node selectedNode;
    this->TopPatchesDialogHandlerInstance->EmitSignal(&selectedNode);

    // Track how many times we used this class
    std::cout << "ManualPatchSelectionDialog has been used "
              << this->NumberOfUses << " times." << std::endl;
    this->NumberOfUses++;

//    std::cout << "Returning selected node..." << std::endl;
    return Helpers::ConvertFrom<TVertexDescriptor, Node>(selectedNode);
  }

  /** Get the number of times this class has been used. */
  void GetNumberOfUses()
  {
    return this->NumberOfUses;
  }

}; // TopPatchListOrManual

#endif
