function(core_link_library lib wraplib)
  message(AUTHOR_WARNING "core_link_library is not compatible with windows.")
endfunction()

function(find_soname lib)
  # Windows uses hardcoded dlls in xbmc/DllPaths_win32.h.
  # Therefore the output of this function is unused.
endfunction()

# Add precompiled header to target
# Arguments:
#   target existing target that will be set up to compile with a precompiled header
#   pch_header the precompiled header file
#   pch_source the precompiled header source file
# Optional Arguments:
#   PCH_TARGET build precompiled header as separate target with the given name
#              so that the same precompiled header can be used for multiple libraries
#   EXCLUDE_SOURCES if not all target sources shall use the precompiled header,
#                   the relevant files can be listed here
# On return:
#   Compiles the pch_source into a precompiled header and adds the header to
#   the given target
function(add_precompiled_header target pch_header pch_source)
  cmake_parse_arguments(PCH "" "PCH_TARGET" "EXCLUDE_SOURCES" ${ARGN})

  if(PCH_PCH_TARGET)
    set(pch_binary ${PRECOMPILEDHEADER_DIR}/${PCH_PCH_TARGET}.pch)
  else()
    set(pch_binary ${PRECOMPILEDHEADER_DIR}/${target}.pch)
  endif()

  # Set compile options and dependency for sources
  get_target_property(sources ${target} SOURCES)
  list(REMOVE_ITEM sources ${pch_source})
  foreach(exclude_source IN LISTS PCH_EXCLUDE_SOURCES)
    list(REMOVE_ITEM sources ${exclude_source})
  endforeach()
  set_source_files_properties(${sources}
                              PROPERTIES COMPILE_FLAGS "/Yu\"${pch_header}\" /Fp\"${pch_binary}\" /FI\"${pch_header}\""
                              OBJECT_DEPENDS "${pch_binary}")

  # Set compile options for precompiled header
  if(NOT PCH_PCH_TARGET OR NOT TARGET ${PCH_PCH_TARGET}_pch)
    set_source_files_properties(${pch_source}
                                PROPERTIES COMPILE_FLAGS "/Yc\"${pch_header}\" /Fp\"${pch_binary}\""
                                OBJECT_OUTPUTS "${pch_binary}")
  endif()

  # Compile precompiled header
  if(PCH_PCH_TARGET)
    # As own target for usage in multiple libraries
    if(NOT TARGET ${PCH_PCH_TARGET}_pch)
      add_library(${PCH_PCH_TARGET}_pch STATIC ${pch_source})
      set_target_properties(${PCH_PCH_TARGET}_pch PROPERTIES COMPILE_PDB_NAME vc140
                                                             COMPILE_PDB_OUTPUT_DIRECTORY ${PRECOMPILEDHEADER_DIR}
                                                             FOLDER "Build Utilities")
    endif()
    # From VS2012 onwards, precompiled headers have to be linked against (LNK2011).
    target_link_libraries(${target} PUBLIC ${PCH_PCH_TARGET}_pch)
    set_target_properties(${target} PROPERTIES COMPILE_PDB_NAME vc140
                                               COMPILE_PDB_OUTPUT_DIRECTORY ${PRECOMPILEDHEADER_DIR})
  else()
    # As part of the target
    target_sources(${target} PRIVATE ${pch_source})
  endif()
endfunction()

macro(winstore_set_assets target)
  file(GLOB ASSET_FILES "${CMAKE_SOURCE_DIR}/tools/windows/packaging/uwp/media/*.png")
  set_property(SOURCE ${ASSET_FILES} PROPERTY VS_DEPLOYMENT_CONTENT 1)
  set_property(SOURCE ${ASSET_FILES} PROPERTY VS_DEPLOYMENT_LOCATION "media")
  source_group("media" FILES ${ASSET_FILES})
  set(RESOURCES ${RESOURCES} ${ASSET_FILES} 
                            "${CMAKE_SOURCE_DIR}/tools/windows/packaging/uwp/kodi_temp_key.pfx")
endmacro()

macro(winstore_generate_manifest target)
  configure_file(
    ${CMAKE_SOURCE_DIR}/tools/windows/packaging/uwp/${APP_MANIFEST_NAME}.in
    ${CMAKE_CURRENT_BINARY_DIR}/${APP_MANIFEST_NAME}
    @ONLY)
  set(RESOURCES ${RESOURCES} ${CMAKE_CURRENT_BINARY_DIR}/${APP_MANIFEST_NAME})
endmacro()

macro(add_deployment_content_group group path link match exclude)
  string(REPLACE "/" "\\" _path ${path})
  string(REPLACE "/" "\\" _link ${link})
  string(REPLACE "/" "\\" _match ${match})
  if(NOT "${exclude}" STREQUAL "")
    string(REPLACE "/" "\\" _exclude ${exclude})
  else()
    set(_exclude "")
  endif()
  string(CONCAT UWP_DEPLOYMENT_CONTENT_STR "${UWP_DEPLOYMENT_CONTENT_STR}"
    "  <ItemGroup>\n"
    "    <_${group} Include=\"${_path}\\${_match}\" Exclude=\"${_exclude}\">\n"
    "      <Link>${_link}\\%(RecursiveDir)%(FileName)%(Extension)</Link>\n"
    "      <DeploymentContent>true</DeploymentContent>\n"
    "    </_${group}>\n"
    "  </ItemGroup>\n")
  string(CONCAT UWP_DEPLOYMENT_CONTENT_STR_ "${UWP_DEPLOYMENT_CONTENT_STR_}"
    "    <ItemGroup>\n"
    "      <None Include=\"@(_${group})\" />\n"
    "    </ItemGroup>\n")
endmacro() 

macro(winstore_append_props target)
  set(PACKAGE_ADDONS_EXCLUDES "$(RootPath)/addons/skin.*/**" 
                              "\;$(RootPath)/addons/script.module.pil/**"
                              "\;$(RootPath)/addons/script.module.pycryptodome/**")

  add_deployment_content_group(system $(RootPath)/system system *.xml "")
  add_deployment_content_group(keyboardlayouts $(RootPath)/system/keyboardlayouts system/keyboardlayouts **/* "")
  add_deployment_content_group(keymaps $(RootPath)/system/keymaps system/keymaps **/* "")
  add_deployment_content_group(library $(RootPath)/system/library system/library **/* "")
  add_deployment_content_group(settings $(RootPath)/system/settings system/settings **/* "")
  add_deployment_content_group(shaders $(RootPath)/system/shaders system/shaders **/* "")
  add_deployment_content_group(media $(RootPath)/media/Fonts media/Fonts **/* "")
  add_deployment_content_group(userdata $(RootPath)/userdata userdata **/* "")
  add_deployment_content_group(addons $(RootPath)/addons addons **/* "${PACKAGE_ADDONS_EXCLUDES}")
  add_deployment_content_group(addons2 ${CMAKE_CURRENT_BINARY_DIR}/addons addons **/* "")
  add_deployment_content_group(python $(BuildDependenciesPath)/system/python system/python **/* "")

  set(VCPROJECT_PROPS_FILE "${CMAKE_CURRENT_BINARY_DIR}/${target}.props")
  file(WRITE ${VCPROJECT_PROPS_FILE}
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<Project ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n"
    "  <ImportGroup Label=\"PropertySheets\" />\n"
    "  <PropertyGroup Label=\"APP_DLLS\">\n"
    "    <BuildDependenciesPath>${BUILDDEPENDENCIES_ROOT_DIR}</BuildDependenciesPath>\n"
    "    <BinPath>${VCPKG_ROOT_DIR}\\bin</BinPath>\n"
    "    <BinDebugPath>${VCPKG_ROOT_DIR}\\debug\\bin</BinDebugPath>\n"
    "    <RootPath>${CMAKE_SOURCE_DIR}</RootPath>\n"
    "  </PropertyGroup>\n"
    "  <ItemGroup Label=\"Binaries\">\n"
    "    <None Include=\"$(BuildDependenciesPath)\\bin\\python27.dll\">\n"
    "      <DeploymentContent>true</DeploymentContent>\n"
    "    </None>\n"
    "    <None Include=\"$(BinDebugPath)\\zlibd1.dll\" Condition=\"'$(Configuration)'=='Debug'\">\n"
    "      <DeploymentContent>true</DeploymentContent>\n"
    "    </None>\n")
  foreach(_lib ${KODI_UWP_DEPENDS_DLLS})
    file(APPEND ${VCPROJECT_PROPS_FILE}
    "    <None Include=\"$(BinPath)\\${_lib}\">\n"
    "      <DeploymentContent>true</DeploymentContent>\n"
    "    </None>\n")
  endforeach()
  file(APPEND ${VCPROJECT_PROPS_FILE}
    "  </ItemGroup>\n"
    "${UWP_DEPLOYMENT_CONTENT_STR}"
    "  <Target Name=\"_CollectCustomResources\" BeforeTargets=\"AssignTargetPaths\">\n"
    "    <Message Text=\"Collecting package resources...\"/>\n"
    "${UWP_DEPLOYMENT_CONTENT_STR_}"
    "  </Target>\n"
    "</Project>")
endmacro()

macro(winstore_add_target_properties target)
  winstore_set_assets(${target})
  winstore_generate_manifest(${target})
  winstore_append_props(${target})
endmacro()