add_subdirectory(Interactive)
include_directories(${Interactive_includes})

QT4_WRAP_CPP(ManualSelection_MOCSrcs
            Visitors/InformationVisitors/DisplayVisitor.hpp
            Visitors/NearestNeighborsDisplayVisitor.hpp)

ADD_EXECUTABLE(InpaintingWithManualSelection InpaintingWithManualSelection.cpp
              ${InpaintingGUI_UISrcs} ${ManualSelection_MOCSrcs} ${InpaintingGUI_MOCSrcs})

TARGET_LINK_LIBRARIES(InpaintingWithManualSelection ${PatchBasedInpainting_libraries})
INSTALL( TARGETS InpaintingWithManualSelection RUNTIME DESTINATION ${INSTALL_DIR} )
