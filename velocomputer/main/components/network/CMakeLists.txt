idf_component_register(SRCS "network.c"
                       INCLUDE_DIRS "." ".."
                       REQUIRES esp_wifi esp_http_client nvs_flash core)
