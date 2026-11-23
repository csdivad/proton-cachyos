##
## wayland-protocols
##

WAYLAND_PROTOCOLS_MESON_ARGS := \
	-Dtests=false

$(eval $(call rules-source,wayland-protocols,$(SRCDIR)/steamrtdeps/wayland-protocols))
$(eval $(call rules-meson,wayland-protocols,i386,unix))
$(eval $(call rules-meson,wayland-protocols,x86_64,unix))
$(eval $(call rules-meson,wayland-protocols,aarch64,unix))

$(OBJ)/.wayland-protocols-post-source: patches-source
	$(foreach p,$(shell find $(PATCHES_SRC)/wayland-protocols/ -name "*.patch" | sort),patch -d $(WAYLAND_PROTOCOLS_SRC) -Np1 -i $(p) &&) true
	touch $@

WAYLAND_PROTOCOLS_DEPENDENCY := wayland-protocols
