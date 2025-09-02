idf_component_register(
    SRCS "main.c" "ST7789/st7789.c" "ST7789/fonts.c"
    INCLUDE_DIRS "."
    REQUIRES driver
    )

spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)