noinst_LIBRARIES = libchipmunk.a

#libchipmunk_a_CFLAGS = -std=gnu99 -O3 -DNDEBUG -ffast-math -I$(srcdir)/include
### WARNING: either -O3 or -ffast-math is causing memory corruption!
libchipmunk_a_CFLAGS = -std=gnu99 -O2 -DNDEBUG -I$(srcdir)/include

#libchipmunk_a_LDFLAGS =

libchipmunk_a_SOURCES = \
	include/chipmunk/chipmunk.h \
	include/chipmunk/chipmunk_ffi.h \
	include/chipmunk/chipmunk_private.h \
	include/chipmunk/chipmunk_structs.h \
	include/chipmunk/chipmunk_types.h \
	include/chipmunk/chipmunk_unsafe.h \
	include/chipmunk/cpArbiter.h \
	include/chipmunk/cpBB.h \
	include/chipmunk/cpBody.h \
	include/chipmunk/cpConstraint.h \
	include/chipmunk/cpDampedRotarySpring.h \
	include/chipmunk/cpDampedSpring.h \
	include/chipmunk/cpGearJoint.h \
	include/chipmunk/cpGrooveJoint.h \
	include/chipmunk/cpHastySpace.h \
	include/chipmunk/cpMarch.h \
	include/chipmunk/cpPinJoint.h \
	include/chipmunk/cpPivotJoint.h \
	include/chipmunk/cpPolyShape.h \
	include/chipmunk/cpPolyline.h \
	include/chipmunk/cpRatchetJoint.h \
	include/chipmunk/cpRobust.h \
	include/chipmunk/cpRotaryLimitJoint.h \
	include/chipmunk/cpShape.h \
	include/chipmunk/cpSimpleMotor.h \
	include/chipmunk/cpSlideJoint.h \
	include/chipmunk/cpSpace.h \
	include/chipmunk/cpSpatialIndex.h \
	include/chipmunk/cpTransform.h \
	include/chipmunk/cpVect.h \
        src/chipmunk.c \
        src/cpArbiter.c \
        src/cpArray.c \
        src/cpBBTree.c \
        src/cpBody.c \
        src/cpCollision.c \
        src/cpConstraint.c \
        src/cpDampedRotarySpring.c \
        src/cpDampedSpring.c \
        src/cpGearJoint.c \
        src/cpGrooveJoint.c \
        src/cpHashSet.c \
        src/cpHastySpace.c \
        src/cpMarch.c \
        src/cpPinJoint.c \
        src/cpPivotJoint.c \
        src/cpPolyShape.c \
        src/cpPolyline.c \
        src/cpRatchetJoint.c \
        src/cpRobust.c \
        src/cpRotaryLimitJoint.c \
        src/cpShape.c \
        src/cpSimpleMotor.c \
        src/cpSlideJoint.c \
        src/cpSpace.c \
        src/cpSpaceComponent.c \
        src/cpSpaceDebug.c \
        src/cpSpaceHash.c \
        src/cpSpaceQuery.c \
        src/cpSpaceStep.c \
        src/cpSpatialIndex.c \
        src/cpSweep1D.c \
        src/prime.h

EXTRA_DIST = \
	README.textile \
	LICENSE.txt \
	VERSION.txt


NO_ALLOC_SIZE_LARGER_THAN =
if CC_IS_GCC
NO_ALLOC_SIZE_LARGER_THAN += -Wno-alloc-size-larger-than
endif

src/libchipmunk_a-cpBBTree.$(OBJEXT): CFLAGS += $(NO_ALLOC_SIZE_LARGER_THAN)

