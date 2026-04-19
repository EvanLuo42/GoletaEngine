goleta_add_module(RenderGraph
    DEPENDENCIES Core RHI
)

# enkiTS drives optional parallel pass recording. The dependency is PRIVATE
# because enkiTS types never appear in the module's public headers -- callers
# pass an enki::TaskScheduler* through a type-erased setter.
if(TARGET enkiTS)
    target_link_libraries(RenderGraph PRIVATE enkiTS)
    target_include_directories(RenderGraph PRIVATE
        "${CMAKE_SOURCE_DIR}/Source/ThirdParty/enkiTS/src"
    )
    target_compile_definitions(RenderGraph PRIVATE GOLETA_RG_HAS_ENKITS=1)
else()
    target_compile_definitions(RenderGraph PRIVATE GOLETA_RG_HAS_ENKITS=0)
endif()
