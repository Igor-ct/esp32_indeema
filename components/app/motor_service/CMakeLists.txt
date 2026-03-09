idf_component_register(SRCS "motor_service.c"
                    REQUIRES stepper_28byj48 sg92r joystick_controller
                    INCLUDE_DIRS "include")
