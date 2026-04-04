idf_component_register(SRCS "telemetry.c" "config.c"
                       INCLUDE_DIRS "."
                       REQUIRES driver nvs_flash)
