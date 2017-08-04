TEMPLATE        = app
CONFIG          = warn_on release m64
#LIBS           += -lnsl -lrt -lsqlite3 -L../lib 
LIBS           += -lsqlite3 -lrt
DEPENDPATH	= ../inc
SOURCES         = livermore.cpp 
