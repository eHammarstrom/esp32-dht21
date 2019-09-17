FILE(GLOB LibSources lib/*.c)

idf_component_register(
    SRCS "main.c" ${LibSources}
    INCLUDE_DIRS "." "lib"
)
