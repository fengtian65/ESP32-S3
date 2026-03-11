idf_component_register(SRCS "app_main.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES esp-dl)  # 关键：改为依赖 tflite-micro
