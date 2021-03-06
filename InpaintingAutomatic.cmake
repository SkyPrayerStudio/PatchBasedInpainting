  QT4_WRAP_UI(InpaintingAutomatic_UISrcs Interactive/BasicViewerWidget.ui
              Interactive/TopPatchesDialog.ui
              Interactive/PriorityViewerWidget.ui
              Interactive/ManualPatchSelectionDialog.ui)
  QT4_WRAP_CPP(InpaintingAutomatic_MOCSrcs Interactive/BasicViewerWidget.h Visitors/InformationVisitors/DisplayVisitor.hpp
                       Visitors/NearestNeighborsDisplayVisitor.hpp
                       #NearestNeighbor/VisualSelectionBest.hpp
                       Interactive/TopPatchesDialog.h
                       Interactive/Delegates/PixmapDelegate.h
                       Interactive/PriorityViewerWidget.h
                       Interactive/ManualPatchSelectionDialog.h
                       Interactive/MovablePatch.h)

  ADD_EXECUTABLE(InpaintingAutomatic InpaintingAutomatic.cpp
                Interactive/Layer.cpp
                Interactive/Delegates/PixmapDelegate.cpp
                Interactive/InteractorStyleImageWithDrag.cpp
                ${InpaintingAutomatic_UISrcs} ${InpaintingAutomatic_MOCSrcs})

  TARGET_LINK_LIBRARIES(InpaintingAutomatic ${PatchBasedInpainting_libraries})
  INSTALL( TARGETS InpaintingAutomatic RUNTIME DESTINATION ${INSTALL_DIR} )
