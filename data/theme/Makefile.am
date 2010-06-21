themedir = $(pkgdatadir)/theme
dist_theme_DATA = \
	panel.css \
	pasteboard-delete-bg.png \
	pasteboard-delete-hover.png \
	pasteboard-delete.png \
	pasteboard-item-delete-hover.png \
	pasteboard-item-delete.png \
	pasteboard-item-hover.png \
	pasteboard-items-list-bg.png \
	pasteboard-search-bg.png \
	placeholder-bg.png \
	search-entry-bg.png \
	search-entry-clear-hover.png \
	search-entry-clear.png \
	search-entry-separator.png

# Manage mutter-meego's texture cache.

if ENABLE_CACHE
texture_cache = $(DESTDIR)$(pkgdatadir)/mx.cache
CACHE_GEN = $(Q:@=@echo '  GEN   '$(texture_cache);)

install-data-hook:
	$(CACHE_GEN)$(MX_CREATE_IMAGE_CACHE) $(DESTDIR)$(pkgdatadir)

uninstall-hook:
	rm -f $(texture_cache)
	$(CACHE_GEN)$(MX_CREATE_IMAGE_CACHE) $(DESTDIR)$(pkgdatadir)
endif
