# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

idf_component_register(
    SRCS					# list the source files of this component
    	main.c
    INCLUDE_DIRS			# optional, add here public include directories
    PRIV_INCLUDE_DIRS	# optional, add here private include directories
    REQUIRES				# optional, list the public requirements (component names)
    	driver esp_timer
    PRIV_REQUIRES			# optional, list the private requirements
)
