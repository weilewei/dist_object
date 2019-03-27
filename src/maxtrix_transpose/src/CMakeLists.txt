set(examples
    template_dist_object
   )

# TODO: Add non-interactive version that can be tested.

# for all targets specified above
foreach(example ${examples})
  set(client_sources ${example}_client.cpp)
  set(component_sources ${example}.cpp)
  set(component_headers ${example}.hpp server/${example}.hpp)

  source_group("Source Files" FILES ${client_sources} ${component_sources})

  # add example components
  add_hpx_component(${example}
    SOURCES ${component_sources}
    HEADERS ${component_headers}
    FOLDER "${example}")

  # add example executable
  add_hpx_executable(${example}_client
    SOURCES ${client_sources}
    DEPENDENCIES ${example}_component
    FOLDER "${example}"
    COMPONENT_DEPENDENCIES iostreams)

  # add a custom target for this example
  add_hpx_pseudo_target(examples.${example})

  # make pseudo-targets depend on master pseudo-target
   add_hpx_pseudo_dependencies(examples
                              examples.${example})

  # add dependencies to pseudo-target
   add_hpx_pseudo_dependencies(examples.${example}
                              ${example}_client)
endforeach()