function(configure_ftdi)

message("configuring ftdi")

set(FTDI_INCLUDEDIR $ENV{FTDI_INCLUDEDIR} PARENT_SCOPE)
set(FTDI_LIBRARYDIR $ENV{FTDI_LIBRARYDIR})

set(FTDI_LIBRARIES_BASE ${FTDI_LIBRARYDIR}/ftd2xx.lib)

set(FTDI_LIBRARIES ${FTDI_LIBRARIES_BASE} PARENT_SCOPE)

endfunction(configure_ftdi)