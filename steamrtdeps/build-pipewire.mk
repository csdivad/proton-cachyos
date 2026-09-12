##
## pipewire
##

PIPEWIRE_MESON_ARGS := \
	-Davahi=disabled \
	-Ddocs=disabled \
	-Dexamples=disabled \
	-Djack=disabled \
	-Dman=disabled \
	-Dpw-cat=disabled \
	-Draop=disabled \
	-Dsession-managers=[] \
	-Dsnap=disabled \
	-Dsystemd-user-service=disabled \
	-Dtests=disabled \
	-Dudev=disabled

PIPEWIRE_i386_MESON_ARGS = \
	-Dudevrulesdir=$(PIPEWIRE_i386_DST)/lib/udev/rules.d

PIPEWIRE_DEPENDS = gstreamer gst_base

$(eval $(call rules-source,pipewire,$(SRCDIR)/steamrtdeps/pipewire))
$(eval $(call rules-meson,pipewire,i386,unix))

$(OBJ)/.pipewire-i386-post-build:
	rm -rf $(PIPEWIRE_i386_DST)/lib/udev
	touch $@

## Part of steamrt-libs:i386, use steamrt lib
## Only use as build dependency; don't ship it.
$(OBJ)/.pipewire-i386-dist:
	touch $@

# clang (used for the i686 build in the llvm SDK image) doesn't inline the
# 8-byte atomics in module-rtp-source on this field alignment and needs
# libatomic; meson's atomic probe doesn't catch it. Inert for the gcc image.
PIPEWIRE_i386_LIBFLAGS += -latomic

PIPEWIRE_DEPENDENCY := pipewire
