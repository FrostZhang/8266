set(COMPONENT_SRCS "hello_world_main.c")

set(COMPONENT_ADD_INCLUDEDIRS
    "include/datacompent"
    "include/i2c_oled_fonts"
    "include/netcompent"
    "include/sh1106"
    "include/sntpcompent"
    "include/udpcompent"
    "include/start"
    "include/sb"
    "include/httpcompent"
    "include/htmlcompent"
    "include/navcompent"
    )

set(COMPONENT_SRCS 
    "source/datacompent"
    "source/start"
    "source/netcompent"
    "source/sh1106"
    "source/sntpcompent"
    "source/udpcompent"
    "source/sb"
    "source/httpcompent"
    "source/htmlcompent"
    "source/navcompent"
    )
#set(COMPONENT_EMBED_TXTFILES jQuery.js)
#set(COMPONENT_EMBED_TXTFILES style.css)
set(COMPONENT_EMBED_TXTFILES index.html)
register_component()

