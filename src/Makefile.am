bin_PROGRAMS = openlitespeed
openlitespeed_SOURCES = modules/prelinkedmods.cpp httpdtest.cpp main.cpp \
   util/httpfetchdriver.cpp \
   util/httpfetch.cpp \
   util/linkobjpool.cpp \
   util/pidfile.cpp \
   util/hashdatacache.cpp \
   util/keydata.cpp \
   util/tlinklist.cpp \
   util/rlimits.cpp \
   util/duplicable.cpp \
   util/gfactory.cpp \
   util/fdpass.cpp \
   util/pcregex.cpp \
   util/autostr.cpp \
   util/staticobj.cpp \
   util/pool.cpp \
   util/hashstringmap.cpp \
   util/ghash.cpp \
   util/emailsender.cpp \
   util/guardedapp.cpp \
   util/crashguard.cpp \
   util/iconnection.cpp \
   util/dlinkqueue.cpp \
   util/connpool.cpp \
   util/gzipbuf.cpp \
   util/vmembuf.cpp \
   util/blockbuf.cpp \
   util/stringlist.cpp \
   util/semaphore.cpp \
   util/refcounter.cpp \
   util/gpointerlist.cpp \
   util/linkedobj.cpp \
   util/objpool.cpp \
   util/gpath.cpp \
   util/logtracker.cpp \
   util/poolalloc.cpp \
   util/env.cpp \
   util/xmlnode.cpp \
   util/iovec.cpp \
   util/accesscontrol.cpp \
   util/signalutil.cpp \
   util/loopbuf.cpp \
   util/stringtool.cpp \
   util/tsingleton.cpp \
   util/pcutil.cpp \
   util/daemonize.cpp \
   util/configentry.cpp \
   util/configctx.cpp \
   util/logidtracker.cpp \
   util/datetime.cpp \
   util/resourcepool.cpp \
   util/linkedqueue.cpp \
   util/httputil.cpp \
   util/misc/profiletime.cpp \
   util/sysinfo/partitioninfo.cpp \
   util/sysinfo/nicdetect.cpp \
   util/sysinfo/systeminfo.cpp \
   lsr/ls_aho.c \
   lsr/ls_base64.c \
   lsr/ls_buf.c \
   lsr/ls_confparser.c \
   lsr/ls_crc64.c \
   lsr/ls_dlinkq.c \
   lsr/ls_fileio.c \
   lsr/ls_hash.c \
   lsr/ls_lfqueue.c \
   lsr/ls_lfstack.c \
   lsr/ls_link.c \
   lsr/ls_lock.c \
   lsr/ls_loopbuf.c \
   lsr/ls_map.c \
   lsr/ls_md5.c \
   lsr/ls_mpscq.c \
   lsr/ls_objarray.c \
   lsr/ls_objpool.c \
   lsr/ls_pcreg.c \
   lsr/ls_pool.c \
   lsr/ls_ptrlist.c \
   lsr/ls_sha1.c \
   lsr/ls_str.c \
   lsr/ls_strlist.c \
   lsr/ls_strtool.c \
   lsr/ls_time.c \
   lsr/ls_xpool.c \
   lsr/xxhash.c \
   thread/pthreadcond.cpp \
   thread/pthreadmutex.cpp \
   thread/pthreadworkqueue.cpp \
   thread/thread.cpp \
   thread/workcrew.cpp \
   thread/worker.cpp


openlitespeed_LDADD =  ./main/libmain.a ./http/libhttp.a ./spdy/libspdy.a \
        ./extensions/libextensions.a ./log4cxx/liblog4cxx.a ./socket/libsocket.a \
        ./sslpp/libsslpp.a  ./ssi/libssi.a ./edio/libedio.a ./lsiapi/liblsiapi.a \
        ./modules/libmodules.a ./shm/liblsshm.a  \
        $(EXPAT_LIBS) $(OPENSSL_LIBS) $(PCRE_LIBS) -lGeoIP -lz -lexpat -lpthread $(DL_LIB_OPTION) $(RT_LIB_OPTION)


AM_CPPFLAGS = $(PCRE_CFLAGS) $(OPENSSL_INCLUDES) $(EXPAT_CFLAGS)
AM_LDFLAGS = -L/usr/lib $(OPENSSL_LDFLAGS)
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src
SUBDIRS =  socket http spdy sslpp ssi extensions log4cxx main edio lsiapi  modules shm


# the library search path.
openlitespeed_LDFLAGS = $(all_libraries)
